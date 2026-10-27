// SPDX-License-Identifier: GPL-2.0-or-later
// Resize previews retain immutable originals and commit a single transaction.
vector fb_downpixel,fb_lastpixel;
int fb_model,fb_brush,fb_face;
int fb_extrude_active,fb_extrude_vertex_count;
int fb_extrude_mode;
int *fb_extrude_faces;
vector fb_extrude_normal,fb_extrude_anchor;
vector fb_extrude_vertices[MAX_FACEPOINTS];
float fb_extrude_distance,fb_extrude_start_distance;
int fb_extrude_ray_distance(vector pixel,float *distance)
{
    vector start=unproject(pixel),ray=normalize(unproject(pixel+'0 0 8192')-start);
    if(fb_extrude_plane_support) {
        float planeDenominator=ray*fb_extrude_support_normal;if(fabs(planeDenominator)<0.000001)return FALSE;
        vector point=start+ray*((fb_extrude_anchor-start)*fb_extrude_support_normal/planeDenominator);
        *distance=(point-fb_extrude_anchor)*fb_extrude_normal;return TRUE;
    }
    float alignment=ray*fb_extrude_normal,denominator=1-alignment*alignment;
    if(denominator<0.0001) return FALSE;
    vector offset=start-fb_extrude_anchor;
    *distance=(offset*fb_extrude_normal-alignment*(offset*ray))/denominator;return TRUE;
}
void fb_extrude_clear()
{
    memfree(fb_extrude_faces);fb_extrude_faces=__NULL__;
    fb_extrude_active=FALSE;fb_extrude_distance=0;
    fb_extrude_mode=0;
}
int fb_extrude_part(int index,float distance,brushface_t *outfaces)
{
    fb_transform_item_t *item=&fb_transform_items[index];int face=fb_extrude_faces[index];
    if(face<0 || fabs(distance)<0.0001) return 0;
    if(fb_extrude_mode==2) {
        static vector vertices[MAX_FACEPOINTS*2];
        for(int p=0;p<fb_extrude_vertex_count;p++) {
            vertices[p]=fb_extrude_vertices[p];
            vertices[p+fb_extrude_vertex_count]=vertices[p]+fb_extrude_normal*distance;
        }
        fb_poly_t prism={0};int count=0;
        if(fb_hull(vertices,fb_extrude_vertex_count*2,&prism))
            count=fb_poly_to_faces(&prism,item->faces,item->count,outfaces,MAX_BRUSHFACES);
        fb_poly_free(&prism);
        // New sides inherit the source face material and its world projection.
        for(int f=0;f<count;f++) {
            vector normal=outfaces[f].planenormal;float dist=outfaces[f].planedist;
            outfaces[f]=item->faces[face];outfaces[f].planenormal=normal;outfaces[f].planedist=dist;
        }
        return count;
    }
    if(item->count>MAX_BRUSHFACES) return 0;
    static brushface_t candidate[MAX_BRUSHFACES+1];
    memcpy(candidate,item->faces,sizeof(brushface_t)*item->count);
    brushface_t cap=item->faces[face];
    if(distance>0) candidate[face].planedist+=distance;
    else cap.planedist+=distance;
    cap.planenormal=-cap.planenormal;cap.planedist=-cap.planedist;
    candidate[item->count]=cap;
    static vector points[MAX_FACEPOINTS];int compactCount=0;
    for(int f=0;f<=item->count;f++) {
        if(brush_calcfacepoints(f+1,candidate,item->count+1,points,MAX_FACEPOINTS)<3) continue;
        if(compactCount==MAX_BRUSHFACES) return 0;
        outfaces[compactCount++]=candidate[f];
    }
    return compactCount;
}
int fb_extrude_prepare(int model,int id,int face)
{
    if(!fb_transform_capture()) return FALSE;
    fb_extrude_cache();
    fb_extrude_faces=memalloc(sizeof(int)*fb_transform_count);
    for(int i=0;i<fb_transform_count;i++) fb_extrude_faces[i]=-1;
    fb_extrude_vertex_count=brush_getfacepoints(model,id,face,fb_extrude_vertices,MAX_FACEPOINTS);
    if(fb_extrude_vertex_count<3 || fb_extrude_vertex_count>MAX_FACEPOINTS) {fb_extrude_clear();fb_transform_clear();return FALSE;}
    static vector points[MAX_FACEPOINTS];
    int found=FALSE;float plane=0;
    for(int i=0;i<fb_transform_count;i++)if(fb_transform_items[i].model==model && fb_transform_items[i].id==id) {
        fb_extrude_normal=fb_transform_items[i].faces[face-1].planenormal;
        plane=fb_transform_items[i].faces[face-1].planedist;break;
    }
    for(int i=0;i<fb_transform_count;i++) {
        fb_transform_item_t *item=&fb_transform_items[i];if(item->kind!=FB_NODE_BRUSH)continue;
        if(!(int)fb_extrude_candidates[sprintf("%i:%i",item->model,item->id)])continue;
        for(int f=0;f<min(item->count,64);f++) {
            float alignment=item->faces[f].planenormal*fb_extrude_normal;
            if(fabs(alignment)<1-0.000001)continue;
            float sign=alignment<0?-1:1;
            if(fabs(item->faces[f].planedist*sign-plane)>=0.001)continue;
            int count=brush_calcfacepoints(f+1,item->faces,item->count,points,MAX_FACEPOINTS);
            if(count<3 || count>MAX_FACEPOINTS)continue;
            if(alignment<0 && !fb_extrude_overlap(points,count,fb_extrude_vertices,fb_extrude_vertex_count,fb_extrude_normal))continue;
            fb_extrude_faces[i]=f;found=TRUE;break;
        }
    }
    if(!found) {fb_extrude_clear();fb_transform_clear();return FALSE;}
    fb_extrude_distance=0;fb_transform_delta='0 0 0';fb_extrude_active=TRUE;return TRUE;
}
void fb_extrude_preview_faces(int index,brushface_t *faces)
{
    int face=fb_extrude_faces[index];if(face<0) return;
    faces[face].planedist+=fb_extrude_distance*(faces[face].planenormal*fb_extrude_normal);
}
int fb_extrude_valid(float distance)
{
    static vector points[MAX_FACEPOINTS];
    if(fabs(distance)<0.0001) return TRUE;
    for(int i=0;i<fb_transform_count;i++) {
        int face=fb_extrude_faces[i];if(face<0) continue;
        fb_transform_item_t *item=&fb_transform_items[i];
        if(fb_extrude_mode) {
            static brushface_t part[MAX_BRUSHFACES];
            int partCount=fb_extrude_part(i,distance,part);
            if(partCount<4) return FALSE;
            if(!fb_brush_valid(part,partCount)) return FALSE;
            if(fb_extrude_mode==2) continue;
        }
        brushface_t *faces=memalloc(sizeof(brushface_t)*item->count);
        memcpy(faces,item->faces,sizeof(brushface_t)*item->count);
        faces[face].planedist+=distance*(faces[face].planenormal*fb_extrude_normal);
        int valid=fb_brush_valid(faces,item->count);
        if(valid) for(int f=0;f<item->count;f++) if(brush_calcfacepoints(f+1,faces,item->count,points,MAX_FACEPOINTS)<3) valid=FALSE;
        memfree(faces);if(!valid) return FALSE;
    }
    return TRUE;
}
void fb_extrude_set_distance(float distance)
{
    float raw=distance,best=fb_snap_move_distance(raw,fb_extrude_vertices,fb_extrude_vertex_count,fb_extrude_normal);
    static vector snapPoints[MAX_FACEPOINTS];
    for(int i=0;i<fb_transform_count;i++)if(fb_extrude_faces[i]>=0) {
        fb_transform_item_t *item=&fb_transform_items[i];
        int count=brush_calcfacepoints(fb_extrude_faces[i]+1,item->faces,item->count,snapPoints,MAX_FACEPOINTS);
        if(count<3 || count>MAX_FACEPOINTS)continue;
        float candidate=fb_snap_move_distance(raw,snapPoints,count,fb_extrude_normal);
        if(fabs(candidate-raw)<fabs(best-raw))best=candidate;
    }
    distance=best;
    if(!fb_extrude_valid(distance)) {
        float low=0,high=distance;
        for(int step=0;step<24 && fabs(high-low)>0.03125;step++) {
            float middle=(low+high)*0.5;
            if(fb_extrude_valid(middle)) low=middle;else high=middle;
        }
        float unit=cvar("ftebroom_grid_snap")?fb_grid():0.0625;
        distance=floor(fabs(low)/unit)*unit;if(low<0) distance=-distance;
    }
    if(fb_extrude_valid(distance)) {
        fb_extrude_distance=distance;fb_transform_preview=fabs(distance)>0.0001;
        cvar_set("ftebroom_selection_status",sprintf("Extrude: face %i / brush %i, distance %.3g",fb_face,fb_brush,distance));
    }
}
int fb_extrude_pick(vector pixel,int view)
{
    return fb_extrude_silhouette_pick(pixel,view);
}
int fb_extrude_begin(vector pixel,int view)
{
    if(cvar("ftebroom_alt") && !cvar("ftebroom_ctrl")) return FALSE;
    if(!fb_extrude_pick(pixel,view)) return FALSE;
    fb_hit_t hit=fb_hits[0];fb_extrude_anchor=hit.point;
    if(!fb_extrude_prepare(hit.model,hit.id,hit.face)) return FALSE;
    if(cvar("ftebroom_ctrl")) {
        fb_extrude_mode=cvar("ftebroom_alt")?2:1;
        for(int i=0;i<fb_transform_count;i++) if(fb_extrude_faces[i]>=0) {
            if(fb_transform_items[i].faces[fb_extrude_faces[i]].planenormal*fb_extrude_normal<0) {
                fb_extrude_clear();fb_transform_clear();
                cvar_set("ftebroom_selection_status","Cannot split or stamp an opposing shared wall");return FALSE;
            }
        }
    }
    if(!fb_extrude_ray_distance(pixel,&fb_extrude_start_distance)) {fb_extrude_clear();fb_transform_clear();return FALSE;}
    fb_owner=FB_OWNER_EXTRUDE;fb_active=TRUE;fb_downpixel=fb_lastpixel=pixel;
    fb_model=hit.model;fb_brush=hit.id;fb_face=hit.face;return TRUE;
}
int fb_extrude_draw_parts(int index)
{
    if(!fb_extrude_mode || fb_extrude_faces[index]<0) return FALSE;
    fb_transform_item_t *item=&fb_transform_items[index];
    brushface_t *base=memalloc(sizeof(brushface_t)*item->count);
    memcpy(base,item->faces,sizeof(brushface_t)*item->count);
    if(fb_extrude_mode==1 && fb_extrude_distance<0) fb_extrude_preview_faces(index,base);
    if(!cvar("ca_brush_view")) DrawQCBrushTextured(base,item->count,'1 1 1',0.65);
    DrawQCBrushWireframe(base,item->count,"chop",'1 0.5 0',1);memfree(base);
    static brushface_t part[MAX_BRUSHFACES];int count=fb_extrude_part(index,fb_extrude_distance,part);
    if(count>=4) {
        if(!cvar("ca_brush_view")) DrawQCBrushTextured(part,count,'1 1 1',0.65);
        DrawQCBrushWireframe(part,count,"chop",'1 1 0',1);
    }
    return TRUE;
}
vector *fb_extrude_outline;
int fb_extrude_outline_count,fb_extrude_outline_capacity;
void fb_extrude_draw()
{
    if(fb_active || cvar("ftebroom_tool") || !cvar("ftebroom_shift") || cvar("ftebroom_camera_captured")) return;
    vector pixel=[cvar("ftebroom_mouse_x"),cvar("ftebroom_mouse_y"),0];
    vector low=getproperty(VF_MIN),size=getproperty(VF_SIZE);
    if(pixel_x<low_x || pixel_y<low_y || pixel_x>=low_x+size_x || pixel_y>=low_y+size_y) return;
    if(!fb_extrude_pick(pixel,cvar("ca_brush_view"))) return;
    fb_hit_t hit=fb_hits[0];
    // Geometry changes invalidate the outline; pointer motion within one face does not.
    static string previous="";
    string key=strcat(sprintf("%i %i %i %i %i",fb_grid_geometry_epoch,fb_selection_epoch,fb_visibility_epoch,fb_filter_epoch,fb_open_group),sprintf(" %i %i %i",hit.model,hit.id,hit.face));
    key=strcat(key," ",itos(fb_incremental_epoch));
    if(key!=previous) {
        strunzone(previous);previous=strzone(key);fb_extrude_outline_count=0;
    static vector points[MAX_FACEPOINTS],source[MAX_FACEPOINTS];
    int sourceCount=brush_getfacepoints(hit.model,hit.id,hit.face,source,MAX_FACEPOINTS);
    if(sourceCount<3 || sourceCount>MAX_FACEPOINTS)return;
    float plane=source[0]*hit.normal;
    static brushface_t faces[64];
    for(int b=0;b<selectedbrushcount;b++) {
        selbrush_t item=selectedbrushes[b];
        if(!(int)fb_extrude_candidates[sprintf("%i:%i",item.model,item.id)])continue;
        int total=brush_get(item.model,item.id,faces,64,__NULL__);total=min(total,64);
        for(int f=0;f<total;f++) {
            float dot=faces[f].planenormal*hit.normal,sign=dot<0?-1:1;
            if(fabs(dot)<1-0.000001 || fabs(faces[f].planedist*sign-plane)>=0.001)continue;
            int count=brush_getfacepoints(item.model,item.id,f+1,points,MAX_FACEPOINTS);
            if(count<3 || count>MAX_FACEPOINTS)continue;
            if(dot<0 && !fb_extrude_overlap(points,count,source,sourceCount,hit.normal))continue;
            for(int i=0;i<count;i++) {
                int next=i+1;if(next==count)next=0;
                if(fb_extrude_outline_count+2>fb_extrude_outline_capacity) {
                    int capacity=max(256,fb_extrude_outline_capacity*2);
                    vector *grown=memalloc(capacity*sizeof(vector));
                    memcpy(grown,fb_extrude_outline,fb_extrude_outline_count*sizeof(vector));
                    memfree(fb_extrude_outline);fb_extrude_outline=grown;fb_extrude_outline_capacity=capacity;
                }
                fb_extrude_outline[fb_extrude_outline_count++]=points[i];
                fb_extrude_outline[fb_extrude_outline_count++]=points[next];
            }
        }
    }
    }
    shaderforname("ftebroom/extrude","{\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc blend\nnodepthtest\n}\n}");
    for(int edge=0;edge<fb_extrude_outline_count;edge+=2) {
        R_BeginPolygon("ftebroom/extrude");
        R_PolygonVertex(fb_extrude_outline[edge],'0 0','1 1 0',1);
        R_PolygonVertex(fb_extrude_outline[edge+1],'0 0','1 1 0',1);R_EndPolygon();
    }
    cvar_set("ftebroom_selection_status",sprintf("Extrude: face %i / brush %i, distance 0",hit.face,hit.id));
}
void fb_extrude_move(vector pixel)
{
    float distance=0;
    if(fb_extrude_ray_distance(pixel,&distance)) fb_extrude_set_distance(distance-fb_extrude_start_distance);
}
void fb_extrude_commit()
{
    if(fabs(fb_extrude_distance)<0.0001 || !fb_extrude_valid(fb_extrude_distance)) return;
    string label="Resize Brushes";if(fb_extrude_mode==1) label="Split Brushes";if(fb_extrude_mode==2) label="Stamp Brushes";
    fb_tx_begin(label);
    if(fb_extrude_mode) {brush_deselectall();fb_restore_entity_selection(-1);}
    for(int i=0;i<fb_transform_count;i++) {
        if(fb_extrude_faces[i]<0) continue;
        fb_transform_item_t *item=&fb_transform_items[i];
        if(fb_extrude_mode) {
            if(fb_extrude_mode==1 && fb_extrude_distance<0) {
                brushface_t *shortened=memalloc(sizeof(brushface_t)*item->count);
                memcpy(shortened,item->faces,sizeof(brushface_t)*item->count);fb_extrude_preview_faces(i,shortened);
                brush_history_edit(item->model,item->id,shortened,item->count,item->contents);memfree(shortened);
            }
            static brushface_t added[MAX_BRUSHFACES];int count=fb_extrude_part(i,fb_extrude_distance,added);
            brush_history_create(item->model,added,count,item->contents,TRUE);continue;
        }
        brushface_t *faces=memalloc(sizeof(brushface_t)*item->count);
        memcpy(faces,item->faces,sizeof(brushface_t)*item->count);fb_extrude_preview_faces(i,faces);
        brush_history_edit(item->model,item->id,faces,item->count,item->contents);memfree(faces);
    }
    fb_tx_commit();
}
