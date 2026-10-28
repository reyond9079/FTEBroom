// SPDX-License-Identifier: GPL-2.0-or-later
int fb_faces_count()
{
    int count=0;
    for(int b=0;b<selectedbrushcount;b++) if(!selbrush_whole(&selectedbrushes[b])) {
        int faces=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,__NULL__,0,__NULL__);
        for(int f=0;f<min(faces,64);f++) if(selbrush_face(&selectedbrushes[b],f)) count++;
    }
    return count;
}
void fb_faces_convert()
{
    fb_restore_entity_selection(-1);
    for(int b=selectedbrushcount-1;b>=0;b--) {
        int n=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,__NULL__,0,__NULL__);
        if(n<4) {brush_deselect(selectedbrushes[b].model,selectedbrushes[b].id);continue;}
        if(selbrush_whole(&selectedbrushes[b])) {
            selbrush_reset_mask(&selectedbrushes[b],FALSE);
            for(int f=0;f<min(n,64);f++) selbrush_set_face(&selectedbrushes[b],f,TRUE);
            if(n>64) fb_face_limit();
        }
    }
}
int fb_faces_valid(int model,int id,int face)
{
    if(face<1) return FALSE;
    if(face>64) {fb_face_limit();return FALSE;}
    return face<=brush_get(model,id,__NULL__,0,__NULL__);
}
void fb_faces_click(int model,int id,int face,int ctrl)
{
    if(cvar("ftebroom_shift") && selectedbrushcount)if(!brush_isselected(model,id))return;
    if(face>64) {fb_face_limit();return;}
    fb_tx_begin("Select Faces");
    if(!ctrl || !fb_faces_valid(model,id,face)) fb_selection_clear();
    else if(!fb_faces_count() && !brush_isselected(model,id)) fb_selection_clear();
    else fb_faces_convert();
    if(fb_faces_valid(model,id,face)) {
        if(ctrl && brushface_isselected(model,id,face)) brushface_deselect(model,id,face);
        else brushface_select(model,id,face);
    }
    fb_tx_commit();
}
void fb_faces_add_at(vector pixel)
{
    if(!fb_pick(pixel)) return;
    fb_hit_t hit=fb_hits[0];
    if(hit.kind!=FB_HIT_BRUSH) return;
    if(cvar("ftebroom_shift") && selectedbrushcount)if(!brush_isselected(hit.model,hit.id))return;
    if(fb_faces_valid(hit.model,hit.id,hit.face)) brushface_select(hit.model,hit.id,hit.face);
}
void fb_faces_paint(vector pixel)
{
    if(!fb_select_paint) {
        fb_tx_begin("Paint Faces");fb_select_paint=TRUE;
        if(fb_faces_valid(fb_model,fb_brush,fb_face)) brushface_select(fb_model,fb_brush,fb_face);
    }
    fb_faces_add_at(pixel);
}
void fb_faces_all(int model,int id,int ctrl)
{
    int count=brush_get(model,id,__NULL__,0,__NULL__);
    if(count<4) return;
    fb_tx_begin("Select Brush Faces");
    if(!ctrl) fb_selection_clear();else fb_faces_convert();
    for(int f=1;f<=min(count,64);f++) brushface_select(model,id,f);
    if(count>64) fb_face_limit();
    fb_tx_commit();
}
typedef struct {
    int model,id,face,count;
    vector points[256],low,high;
} fb_flood_face_t;
int fb_faces_connected(fb_flood_face_t *a,fb_flood_face_t *b,vector normal)
{
    for(int axis=0;axis<3;axis++) if(a->low[axis]>b->high[axis]+0.01 || b->low[axis]>a->high[axis]+0.01) return FALSE;
    // Convex polygon SAT. Strict overlap on every edge normal implies positive area.
    int strict=TRUE;
    for(int side=0;side<2;side++) {
        fb_flood_face_t *poly=a;if(side) poly=b;
        for(int edge=0;edge<poly->count;edge++) {
            int next=edge+1;if(next==poly->count) next=0;
            vector direction=poly->points[next]-poly->points[edge];
            if(vlen(direction)<0.00001) continue;
            vector axisNormal=normalize(crossproduct(normal,direction));
            float amin=1000000000,amax=-1000000000,bmin=1000000000,bmax=-1000000000;
            for(int p=0;p<a->count;p++) {float av=a->points[p]*axisNormal;amin=min(amin,av);amax=max(amax,av);}
            for(int q=0;q<b->count;q++) {float bv=b->points[q]*axisNormal;bmin=min(bmin,bv);bmax=max(bmax,bv);}
            float overlap=min(amax,bmax)-max(amin,bmin);
            if(overlap < -0.001) return FALSE;
            if(overlap<=0.001) strict=FALSE;
        }
    }
    if(strict) return TRUE;
    // A shared segment connects tiles; a shared vertex alone does not.
    for(int i=0;i<a->count;i++) {
        int nextA=i+1;if(nextA==a->count) nextA=0;
        vector start=a->points[i],delta=a->points[nextA]-start;
        float length=vlen(delta);if(length<0.01) continue;
        vector unit=delta/length;
        for(int j=0;j<b->count;j++) {
            int nextB=j+1;if(nextB==b->count) nextB=0;
            vector b0=b->points[j]-start,b1=b->points[nextB]-start;
            if(vlen(crossproduct(b0,unit))>0.001 || vlen(crossproduct(b1,unit))>0.001) continue;
            float t0=b0*unit,t1=b1*unit;
            if(min(length,max(t0,t1))-max(0,min(t0,t1))>0.001) return TRUE;
        }
    }
    return FALSE;
}
int fb_faces_polygon(fb_flood_face_t *face,int model,int id,int index)
{
    face->model=model;face->id=id;face->face=index;
    face->count=brush_getfacepoints(model,id,index,face->points,256);
    if(face->count<3 || face->count>=256) return FALSE;
    face->low=face->high=face->points[0];
    for(int i=1;i<face->count;i++) for(int axis=0;axis<3;axis++) {
        face->low[axis]=min(face->low[axis],face->points[i][axis]);
        face->high[axis]=max(face->high[axis],face->points[i][axis]);
    }
    return TRUE;
}
void fb_faces_flood(int model,int id,int face,int ctrl)
{
    if(cvar("ftebroom_shift") && selectedbrushcount)if(!brush_isselected(model,id))return;
    if(!fb_faces_valid(model,id,face)) return;
    int n=brush_get(model,id,__NULL__,0,__NULL__);
    brushface_t *source=memalloc(sizeof(brushface_t)*n);
    brush_get(model,id,source,n,__NULL__);
    vector normal=source[face-1].planenormal;float distance=source[face-1].planedist;
    memfree(source);
    int capacity=64,count=1,overflow=FALSE;
    fb_flood_face_t *faces=memalloc(sizeof(fb_flood_face_t)*capacity);
    if(!fb_faces_polygon(&faces[0],model,id,face)) {memfree(faces);return;}
    fb_document_tick();
    for(int node=0;node<fb_node_count && !overflow;node++) {
        fb_node_t item=fb_nodes[node];
        if(item.kind!=FB_NODE_BRUSH || !fb_pick_allowed(node,0)) continue;
        if(cvar("ftebroom_shift") && selectedbrushcount)if(!brush_isselected(item.model,item.id))continue;
        int total=brush_get(item.model,item.id,__NULL__,0,__NULL__);
        brushface_t *planes=memalloc(sizeof(brushface_t)*total);
        brush_get(item.model,item.id,planes,total,__NULL__);
        for(int f=0;f<min(total,64);f++) {
            if(item.model==model && item.id==id && f+1==face) continue;
            if(fabs(planes[f].planenormal*normal-1)>=0.0001 || fabs(planes[f].planedist-distance)>=0.01) continue;
            if(count==4096) {overflow=TRUE;break;}
            if(count==capacity) {
                capacity=min(4096,capacity*2);
                fb_flood_face_t *grown=memalloc(sizeof(fb_flood_face_t)*capacity);
                memcpy(grown,faces,sizeof(fb_flood_face_t)*count);memfree(faces);faces=grown;
            }
            if(fb_faces_polygon(&faces[count],item.model,item.id,f+1)) count++;
        }
        memfree(planes);
    }
    if(overflow) {
        memfree(faces);cvar_set("ftebroom_selection_status","Coplanar flood exceeds 4096 candidate faces; selection unchanged.");return;
    }
    int *visited=memalloc(sizeof(int)*count);memfill8((__variant*)visited,0,sizeof(int)*count);
    int *queue=memalloc(sizeof(int)*count);queue[0]=0;visited[0]=TRUE;
    int head=0,tail=1;
    while(head<tail) {
        int current=queue[head];head++;
        for(int candidate=1;candidate<count;candidate++) if(!visited[candidate])
            if(fb_faces_connected(&faces[current],&faces[candidate],normal)) {
                visited[candidate]=TRUE;queue[tail]=candidate;tail++;
            }
    }
    fb_tx_begin("Coplanar Face Selection");
    if(!ctrl) fb_selection_clear();else fb_faces_convert();
    for(int selected=0;selected<tail;selected++) {
        fb_flood_face_t itemFace=faces[queue[selected]];
        brushface_select(itemFace.model,itemFace.id,itemFace.face);
    }
    fb_tx_commit();memfree(queue);memfree(visited);memfree(faces);
}
void fb_faces_double(vector pixel,int ctrl,int alt)
{
    fb_cancel();if(cvar("ca_brush_view")) return;
    if(!fb_pick(pixel)) return;
    fb_hit_t hit=fb_hits[0];if(hit.kind!=FB_HIT_BRUSH) return;
    if(cvar("ftebroom_shift") && selectedbrushcount)if(!brush_isselected(hit.model,hit.id))return;
    if(alt) fb_faces_flood(hit.model,hit.id,hit.face,ctrl);
    else fb_faces_all(hit.model,hit.id,ctrl);
}
