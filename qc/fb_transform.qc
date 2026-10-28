int fb_batch_move(vector delta);
int fb_affine_preview_active;
int fb_extrude_active;
void fb_extrude_preview_faces(int index,brushface_t *faces);
int fb_extrude_draw_parts(int index);
void fb_affine_draw();
// SPDX-License-Identifier: GPL-2.0-or-later
// Gesture previews own immutable snapshots; only the final delta enters history.
typedef struct {
    int model,id,count,contents,kind;
    brushface_t *faces;
    patchvert_t *points;
    patchinfo_t patch;
    vector low,high,origin;
} fb_transform_item_t;
fb_transform_item_t *fb_transform_items;
int fb_transform_count,fb_transform_preview,fb_transform_duplicate;
vector fb_transform_delta;
int fb_transform_capture_epoch,fb_transform_bounds_valid;vector fb_transform_bounds_min,fb_transform_bounds_max;
// Immutable capture membership: point IDs are dense, brush IDs are per model.
int *fb_transform_entity_members;int fb_transform_entity_capacity,fb_transform_geometry_count;
hashtable fb_transform_brush_members;
vector *fb_transform_point_boxes;int fb_transform_point_count;
int *fb_transform_model_ids;int fb_transform_model_count;
int(vector *boxes,int count,vector delta) ftebroom_preview_boxes=#0:ftebroom_preview_boxes;
vector fb_transform_nudge_axis(int axis,int view,vector angles)
{
    if(view) {
        fb_view_axes(view);
        if(axis==0) return fb_view_right;
        if(axis==1) return fb_view_up;
        return fb_view_normal;
    }
    if(axis==2) return '0 0 1';
    makevectors(angles);
    vector forward=v_forward,right=v_right;
    if(fabs(forward_z)>0.999) {forward=v_up;if(v_forward_z>0) forward=-forward;}
    forward_z=right_z=0;
    int fa=0,ra=0;if(fabs(forward_y)>fabs(forward_x)) fa=1;
    if(fabs(right_y)>fabs(right_x)) ra=1;
    vector f='0 0 0',r='0 0 0';f[fa]=forward[fa]<0?-1:1;r[ra]=right[ra]<0?-1:1;
    if(fa==ra) r=crossproduct(f,'0 0 1');
    if(axis==0) return r;
    return f;
}
void fb_transform_clear()
{
    fb_transform_capture_epoch++;fb_transform_bounds_valid=FALSE;
    for(int i=0;i<fb_transform_count;i++) {
        fb_transform_item_t *item=&fb_transform_items[i];
        if(item->faces) for(int f=0;f<item->count;f++) strunzone(item->faces[f].shadername);
        if(item->points) strunzone(item->patch.shadername);
        memfree(item->faces);memfree(item->points);
    }
    memfree(fb_transform_entity_members);fb_transform_entity_members=__NULL__;fb_transform_entity_capacity=0;
    if(fb_transform_brush_members)hash_destroytab(fb_transform_brush_members);fb_transform_brush_members=(hashtable)0;
    memfree(fb_transform_point_boxes);fb_transform_point_boxes=__NULL__;fb_transform_point_count=0;
    memfree(fb_transform_model_ids);fb_transform_model_ids=__NULL__;fb_transform_model_count=0;fb_transform_geometry_count=0;
    memfree(fb_transform_items);fb_transform_items=__NULL__;fb_transform_count=0;fb_transform_preview=FALSE;fb_transform_duplicate=FALSE;
}
int fb_transform_capture()
{
    fb_transform_clear();fb_document_tick();
    if(fb_vertex_scope) return fb_vertex_selection_count()>0;
    fb_transform_items=memalloc(sizeof(fb_transform_item_t)*(selectedbrushcount+fb_selected_entity_count));
    static vector bounds[2];
    for(int b=0;b<selectedbrushcount;b++) {
        if(!selbrush_whole(&selectedbrushes[b])) {fb_transform_clear();return FALSE;}
        fb_transform_item_t *item=&fb_transform_items[fb_transform_count++];
        item->model=selectedbrushes[b].model;item->id=selectedbrushes[b].id;
        item->count=patch_getcp(item->model,item->id,__NULL__,0,__NULL__);
        if(item->count) {
            item->kind=FB_NODE_PATCH;
            item->points=memalloc(sizeof(patchvert_t)*item->count);
            patch_getcp(item->model,item->id,item->points,item->count,&item->patch);
            item->patch.shadername=strzone(item->patch.shadername);
        } else {
            item->kind=FB_NODE_BRUSH;
            item->count=brush_get(item->model,item->id,__NULL__,0,__NULL__);
            if(item->count<4) {fb_transform_clear();return FALSE;}
            item->faces=memalloc(sizeof(brushface_t)*item->count);
            brush_get(item->model,item->id,item->faces,item->count,&item->contents);
            for(int f=0;f<item->count;f++) item->faces[f].shadername=strzone(item->faces[f].shadername);
        }
        if(brush_getfacepoints(item->model,item->id,0,bounds,2)!=2) {fb_transform_clear();return FALSE;}
        item->low=bounds[0];item->high=bounds[1];
    }
    fb_transform_geometry_count=fb_transform_count;
    if(fb_transform_geometry_count>256){
        fb_transform_brush_members=hash_createtab(fb_transform_geometry_count,EV_INTEGER);
        for(int b=0;b<fb_transform_geometry_count;b++)fb_transform_brush_members[sprintf("%i:%i",fb_transform_items[b].model,fb_transform_items[b].id)]=1i;
    }
    if(fb_selected_entity_count){
        fb_transform_entity_capacity=terrain_edit(TEREDIT_ENT_COUNT);
        fb_transform_entity_members=memalloc(max(1,fb_transform_entity_capacity)*sizeof(int));
        fb_transform_point_boxes=memalloc(fb_selected_entity_count*3*sizeof(vector));
        fb_transform_model_ids=memalloc(fb_selected_entity_count*sizeof(int));
    }
    for(int e=0;e<fb_selected_entity_count;e++) {
        int id=fb_selected_entities[e];
        if(!fb_entity_bounds(id,bounds)) {fb_transform_clear();return FALSE;}
        fb_transform_item_t *item=&fb_transform_items[fb_transform_count++];
        item->id=id;item->kind=FB_NODE_ENTITY;item->origin=fb_entity_origin(id);
        item->low=bounds[0];item->high=bounds[1];
        if(id>=0 && id<fb_transform_entity_capacity)fb_transform_entity_members[id]=TRUE;
        fb_transform_point_boxes[fb_transform_point_count*3]=item->low;
        fb_transform_point_boxes[fb_transform_point_count*3+1]=item->high;
        fb_transform_point_boxes[fb_transform_point_count*3+2]='1 0.5 0';fb_transform_point_count++;
        if(fb_entity_has_model(id))fb_transform_model_ids[fb_transform_model_count++]=id;
    }
    for(int i=0;i<fb_transform_count;i++){
        if(!fb_transform_bounds_valid){fb_transform_bounds_min=fb_transform_items[i].low;fb_transform_bounds_max=fb_transform_items[i].high;fb_transform_bounds_valid=TRUE;}
        else for(int a=0;a<3;a++){fb_transform_bounds_min[a]=min(fb_transform_bounds_min[a],fb_transform_items[i].low[a]);fb_transform_bounds_max[a]=max(fb_transform_bounds_max[a],fb_transform_items[i].high[a]);}
    }
    fb_transform_delta='0 0 0';return fb_transform_count>0;
}
int fb_transform_valid(vector delta)
{
    for(int i=0;i<fb_transform_count;i++) {
        fb_transform_item_t *item=&fb_transform_items[i];
        if(!fb_geom_point(item->low+delta) || !fb_geom_point(item->high+delta)) return FALSE;
    }
    return TRUE;
}
void fb_linked_translate(vector delta);
void fb_repeat_move(vector delta,int duplicate);
void fb_transform_commit(vector delta)
{
    if(!fb_transform_valid(delta) || (!fb_transform_duplicate && vlen(delta)==0)) return;
    if(!fb_transform_duplicate)if(fb_batch_move(delta)){fb_repeat_move(delta,FALSE);fb_clone_clear();return;}
    fb_tx_begin(fb_transform_duplicate?"Duplicate and Move":"Move");
    if(fb_transform_duplicate) {
        if(!fb_clone_groups()) {fb_tx_cancel();fb_clone_clear();return;}
    }
    fb_linked_translate(delta);
    if(fb_transform_duplicate)fb_selection_clear();
    for(int i=0;i<fb_transform_count;i++) {
        fb_transform_item_t *item=&fb_transform_items[i];
        if(item->kind==FB_NODE_BRUSH) {
            brushface_t *faces=memalloc(sizeof(brushface_t)*item->count);
            memcpy(faces,item->faces,sizeof(brushface_t)*item->count);
            for(int f=0;f<item->count;f++) {
                faces[f].planedist+=faces[f].planenormal*delta;
                if(cvar("ftebroom_texture_lock")) {faces[f].sbias-=faces[f].sdir*delta;faces[f].tbias-=faces[f].tdir*delta;}
            }
            if(fb_transform_duplicate) brush_history_create(fb_clone_destination(item->model,item->id),faces,item->count,item->contents,TRUE);
            else brush_history_edit(item->model,item->id,faces,item->count,item->contents);
            memfree(faces);
        } else if(item->kind==FB_NODE_PATCH) {
            patchvert_t *points=memalloc(sizeof(patchvert_t)*item->count);
            memcpy(points,item->points,sizeof(patchvert_t)*item->count);
            for(int p=0;p<item->count;p++) points[p].xyz+=delta;
            if(fb_transform_duplicate) patch_history_create(fb_clone_destination(item->model,item->id),points,item->patch,TRUE);
            else patch_history_edit(item->model,item->id,points,item->patch);
            memfree(points);
        } else {
            int id=item->id;
            if(fb_transform_duplicate) {
                int cloned=fb_clone_entity(id);
                if(cloned>=0) id=cloned;
                else {
                    string data=strzone(terrain_edit(TEREDIT_ENT_GET,id));
                    id=fb_tx_empty_entity();
                    fb_tx_entity_set(id,data,HIST_ENTITY);strunzone(data);
                }
                fb_entity_select(id,TRUE);
            }
            vector movedOrigin=item->origin+delta;
            if(!fb_transform_duplicate || delta!='0 0 0')fb_tx_set_key(id,"origin",sprintf("%.9g %.9g %.9g",movedOrigin_x,movedOrigin_y,movedOrigin_z),HIST_ENTITY);
        }
    }
    fb_tx_commit();fb_repeat_move(delta,fb_transform_duplicate);fb_clone_clear();
}
int fb_transform_replaces(int model,int id)
{
    if(!fb_transform_preview || fb_transform_duplicate) return FALSE;
    if(!model){
        if(id<0 || id>=fb_transform_entity_capacity)return FALSE;
        return fb_transform_entity_members[id];
    }
    if(fb_transform_brush_members)return (int)fb_transform_brush_members[sprintf("%i:%i",model,id)]!=0;
    for(int i=0;i<fb_transform_geometry_count;i++)
        if(fb_transform_items[i].model==model && fb_transform_items[i].id==id)return TRUE;
    return FALSE;
}
void fb_transform_draw()
{
    if(!fb_transform_preview) return;
    if(fb_affine_preview_active) {fb_affine_draw();return;}
    shaderforname("ftebroom/movepatch","{\ncull none\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc blend\n}\n}");
    int batched=FALSE;
    if(fb_transform_point_count>256 && cvar("ftebroom_native_selection_box") && checkbuiltin(ftebroom_preview_boxes))
        batched=ftebroom_preview_boxes(fb_transform_point_boxes,fb_transform_point_count,fb_transform_delta)==fb_transform_point_count;
    if(batched && !cvar("ca_brush_view") && cvar("ftebroom_show_models"))
        for(int model=0;model<fb_transform_model_count;model++)fb_entity_draw_instance(fb_transform_model_ids[model],fb_transform_delta,'1 0 0','0 1 0','0 0 1');
    for(int i=0;i<(batched?fb_transform_geometry_count:fb_transform_count);i++) {
        fb_transform_item_t *item=&fb_transform_items[i];
        if(item->kind==FB_NODE_BRUSH) {
            if(fb_extrude_active) if(fb_extrude_draw_parts(i)) continue;
            brushface_t *faces=memalloc(sizeof(brushface_t)*item->count);
            memcpy(faces,item->faces,sizeof(brushface_t)*item->count);
            if(fb_extrude_active) fb_extrude_preview_faces(i,faces);
            for(int f=0;f<item->count;f++) {
                faces[f].planedist+=faces[f].planenormal*fb_transform_delta;
                if(cvar("ftebroom_texture_lock")) {faces[f].sbias-=faces[f].sdir*fb_transform_delta;faces[f].tbias-=faces[f].tdir*fb_transform_delta;}
            }
            if(!cvar("ca_brush_view")) DrawQCBrushTextured(faces,item->count,'1 1 1',0.65);
            DrawQCBrushWireframe(faces,item->count,"chop",'1 0.5 0',1);memfree(faces);
        } else if(item->kind==FB_NODE_PATCH) {
            patchinfo_t info;int n=patch_getmesh(item->model,item->id,__NULL__,0,__NULL__);
            patchvert_t *mesh=memalloc(sizeof(patchvert_t)*n);patch_getmesh(item->model,item->id,mesh,n,&info);
            for(int p=0;p<n;p++) mesh[p].xyz+=fb_transform_delta;
            if(!cvar("ca_brush_view")) DrawQCPatchTextured(mesh,info,'1 1 1',0.65);
            DrawQCPatchWireframe(mesh,info,"ftebroom/movepatch",'1 0.5 0',1);
            memfree(mesh);
        } else {
            if(!cvar("ca_brush_view") && cvar("ftebroom_show_models"))
                fb_entity_draw_instance(item->id,fb_transform_delta,'1 0 0','0 1 0','0 0 1');
            static vector corners[8];
            for(int c=0;c<8;c++) for(int axis=0;axis<3;axis++)
                corners[c][axis]=((c&(1i<<axis))?item->high[axis]:item->low[axis])+fb_transform_delta[axis];
            for(int c=0;c<8;c++) for(int axis=0;axis<3;axis++) if(!(c&(1i<<axis))) {
                R_BeginPolygon("ftebroom/movepatch");R_PolygonVertex(corners[c],'0 0 0','1 0.5 0',1);
                R_PolygonVertex(corners[c|(1i<<axis)],'0 0 0','1 0.5 0',1);R_EndPolygon();
            }
        }
    }
}
