// SPDX-License-Identifier: GPL-2.0-or-later
// One transaction owns the immutable batch. Native brush IDs and topology stay stable.
int(int operation,int *ids,int count,int *packet,int bytes,vector delta,int lock) ftebroom_brush_move=#0:ftebroom_brush_move;
void fb_batch_release(fb_batch_move_t *batch)
{
    if(!batch)return;
    for(int i=0;i<batch->pointCount;i++){strunzone(batch->points[i].beforefields);strunzone(batch->points[i].afterfields);strunzone(batch->points[i].beforeOrigin);}
    memfree(batch->points);memfree(batch->ids);memfree(batch->packet);memfree(batch);
}
int fb_batch_validate(fb_batch_move_t *batch)
{
    if(!batch)return FALSE;
    for(int i=0;i<batch->pointCount;i++)if(!fb_entity_move_ready(batch->points[i].id))return FALSE;
    if(!batch->brushCount)return TRUE;
    return ftebroom_brush_move(3,batch->ids,batch->brushCount,batch->packet,batch->bytes,'0 0 0',batch->textureLock);
}
int fb_batch_apply(fb_batch_move_t *batch,int redo)
{
    if(!batch)return FALSE;
    for(int i=0;i<batch->pointCount;i++)if(!fb_entity_move_ready(batch->points[i].id))return FALSE;
    if(batch->brushCount)if(!ftebroom_brush_move(2,batch->ids,batch->brushCount,batch->packet,batch->bytes,redo?batch->delta:'0 0 0',batch->textureLock))return FALSE;
    for(int i=0;i<batch->pointCount;i++){
        fb_batch_point_t *point=&batch->points[i];int id=point->id;
        terrain_edit(TEREDIT_ENT_SET,id,redo?point->afterfields:point->beforefields);
        vector position=redo?point->after:point->before;
        string origin=redo?sprintf("%.9g %.9g %.9g",position_x,position_y,position_z):point->beforeOrigin;
        fb_entity_move_cached(id,position,origin,redo || point->hadOrigin);
    }
    return TRUE;
}
int fb_batch_move(vector delta)
{
    // Keep topology edits, auto-hull snapping and linked-group propagation on the
    // established transaction path. A failed capture does not change the map.
    if(!fb_transform_count || fb_open_group || (fb_transform_geometry_count && cvar("ftebroom_auto_integer_snap")))return FALSE;
    if(fb_transform_geometry_count && (!cvar("ftebroom_texture_lock") || !checkbuiltin(ftebroom_brush_move)))return FALSE;
    for(int i=0;i<fb_transform_count;i++)if(fb_transform_items[i].kind==FB_NODE_PATCH)return FALSE;
    float started91=gettime(1);
    fb_batch_move_t *batch=memalloc(sizeof(fb_batch_move_t));batch->delta=delta;batch->textureLock=cvar("ftebroom_texture_lock");
    batch->brushCount=fb_transform_geometry_count;batch->pointCount=fb_transform_count-batch->brushCount;
    batch->ids=memalloc(batch->brushCount*2*sizeof(int));batch->points=memalloc(batch->pointCount*sizeof(fb_batch_point_t));
    for(int i=0;i<batch->brushCount;i++){batch->ids[i*2]=fb_transform_items[i].model;batch->ids[i*2+1]=fb_transform_items[i].id;}
    if(batch->brushCount){
        batch->bytes=ftebroom_brush_move(0,batch->ids,batch->brushCount,__NULL__,0,delta,batch->textureLock);
        if(batch->bytes<=0){fb_batch_release(batch);return FALSE;}
        batch->packet=memalloc(batch->bytes);
        if(!ftebroom_brush_move(1,batch->ids,batch->brushCount,batch->packet,batch->bytes,delta,batch->textureLock)){fb_batch_release(batch);return FALSE;}
    }
    for(int i=0;i<batch->pointCount;i++){
        fb_transform_item_t *item=&fb_transform_items[batch->brushCount+i];fb_batch_point_t *point=&batch->points[i];point->id=item->id;point->before=item->origin;point->after=item->origin+delta;
        string data=strzone(terrain_edit(TEREDIT_ENT_GET,point->id));point->beforefields=data;
        if(data=="" || strstrofs(data,"\\",0)>=0 || !fb_entity_move_ready(point->id)){fb_batch_release(batch);return FALSE;}
        int tokens=tokenize_console(data),start=-1,end=-1;
        for(int k=0;k+1<tokens;k+=2)if(argv(k)=="origin"){point->hadOrigin=TRUE;point->beforeOrigin=strzone(argv(k+1));start=argv_start_index(k+1);end=argv_end_index(k+1);break;}
        string origin=sprintf("\"%.9g %.9g %.9g\"",point->after[0],point->after[1],point->after[2]);
        if(start>=0)point->afterfields=strzone(strcat(substring(data,0,start),origin,substring(data,end,-1)));
        else point->afterfields=strzone(strcat(data,"\n\"origin\" ",origin,"\n"));
    }
    fb_commit_profile("batch91_capture",started91);started91=gettime(1);
    fb_container_cache_prepare();
    fb_tx_begin("Move");fb_linked_translate(delta);history_t *h=history_allocate();h->kind=HIST_MOVE_BATCH;h->batch=batch;
    if(!fb_batch_apply(batch,TRUE)){history_deallocate(h);fb_tx_cancel();return FALSE;}
    fb_tx_commit();fb_commit_profile("batch91_apply",started91);return TRUE;
}
void fb_batch_grid_face(fb_grid_face_t *face)
{
    face->center='0 0 0';
    for(int p=0;p<face->count;p++)face->center+=face->points[p];face->center/=(float)face->count;
    for(int p=0;p<face->count;p++){
        for(int a=0;a<3;a++)face->vertices[p].xyz[a]=face->vertices[p+face->count].xyz[a]=face->points[p][a]+face->normal[a]*.02;
        vector gridPoint=fb_grid_local(face->points[p]);face->vertices[p].st[0]=gridPoint[face->u]/fb_grid();face->vertices[p].st[1]=gridPoint[face->v]/fb_grid();
    }
}
int fb_batch_document(int first,int last,int redo)
{
    float started91=gettime(1);
    if(first<0 || first>=last || last>history_max)return FALSE;
    int batches=0;
    for(int r=first;r<last;r++){int kind=historyring[r&(historyring_length-1)].kind;if(kind==HIST_MOVE_BATCH)batches++;else if(kind!=HIST_SELECTION && kind!=HIST_TREE)return FALSE;}
    if(!batches)return FALSE;
    if(!fb_container_cache || !fb_grid_first || !fb_render_first || fb_grid_built_epoch!=fb_grid_geometry_epoch)return FALSE;
    int *groups=memalloc((last-first)*sizeof(int));int groupCount=0;
    for(int r=first;r<last;r++){history_t *record=&historyring[r&(historyring_length-1)];if(record->kind==HIST_TREE)groups[groupCount++]=record->entityindex;}
    if(groupCount)if(!fb_document_update_entities(groups,groupCount)){memfree(groups);return FALSE;}memfree(groups);
    hashtable nodes=hash_createtab(max(64,fb_node_count),EV_INTEGER),overrides=hash_createtab(max(64,fb_incremental_brush_count*2),EV_POINTER);
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_nodes[n].model==0)nodes[itos(fb_nodes[n].id)]=n+1i;
    for(fb_incremental_brush_t *entry=fb_incremental_brushes;entry;entry=entry->next)overrides[itos(entry->node)]=entry;
    int brushesChanged=FALSE,pointsChanged=FALSE;
    for(int r=first;r<last;r++){
        history_t *h=&historyring[r&(historyring_length-1)];if(h->kind!=HIST_MOVE_BATCH)continue;fb_batch_move_t *batch=h->batch;
        for(int i=0;i<batch->pointCount;i++){
            fb_batch_point_t *point=&batch->points[i];int node=(int)nodes[itos(point->id)]-1i;if(node<2)continue;
            fb_entity_move_copy_origin(point->id,fb_nodes[node].fields);
            static vector pointBounds[2];fb_entity_bounds(point->id,pointBounds);fb_container_cache[node].mins=pointBounds[0];fb_container_cache[node].maxs=pointBounds[1];pointsChanged=TRUE;
        }
        for(int i=0;i<batch->brushCount;i++){
            int model=batch->ids[i*2],id=batch->ids[i*2+1],node=fb_document_brush_node(model,id);if(node<2)continue;
            fb_incremental_brush_t *entry=overrides[itos(node)];
            int gridStart=fb_grid_first[node]-1,gridEnd=0;if(gridStart>=0)gridEnd=fb_grid_faces[gridStart].brushEnd;if(entry){gridStart=0;gridEnd=entry->gridCount;}
            fb_grid_face_t *grid=fb_grid_faces;if(entry)grid=entry->grid;
            int faceStart=fb_render_first[node]-1,faceEnd=0;if(faceStart>=0)faceEnd=fb_render_faces[faceStart].brushEnd;if(entry){faceStart=0;faceEnd=entry->faceCount;}
            fb_render_face_t *faces=fb_render_faces;if(entry)faces=entry->faces;
            static vector bounds[2];brush_getfacepoints(model,id,0,bounds,2);vector center=(bounds[0]+bounds[1])*.5;
            fb_container_cache[node].mins=bounds[0];fb_container_cache[node].maxs=bounds[1];fb_grid_brush_bounds[node*2]=center;
            for(int f=max(0,gridStart);f<gridEnd;f++){fb_grid_face_t *face=&grid[f];brush_getfacepoints(model,id,f-gridStart+1,face->points,face->count);fb_batch_grid_face(face);}
            for(int f=max(0,faceStart);f<faceEnd;f++){
                fb_render_face_t *face=&faces[f];brush_getfacepoints(model,id,f-faceStart+1,face->points,face->count);face->center='0 0 0';face->brushCenter=center;
                for(int p=0;p<face->count;p++){face->center+=face->points[p];for(int a=0;a<3;a++)face->vertices[p].xyz[a]=face->points[p][a];}face->center/=(float)face->count;face->distance=face->normal*face->points[0];
            }
            fb_wire_cache_forget(node);brushesChanged=TRUE;
        }
    }
    hash_destroytab(nodes);hash_destroytab(overrides);
    // Re-aggregate retained leaf bounds without querying/reparsing the whole map.
    for(int n=0;n<fb_node_count;n++)if(!fb_selection_leaf(n))fb_container_cache[n].valid=FALSE;
    for(int n=2;n<fb_node_count;n++)if(fb_selection_leaf(n)&&fb_container_cache[n].valid){
        vector low=fb_container_cache[n].mins,high=fb_container_cache[n].maxs;
        for(int parent=fb_nodes[n].parent,steps=0;parent>=0&&parent<fb_node_count&&steps++<fb_node_count;parent=fb_nodes[parent].parent){
            fb_container_bounds_t *item=&fb_container_cache[parent];if(!item->valid){item->mins=low;item->maxs=high;item->valid=TRUE;}else for(int a=0;a<3;a++){item->mins[a]=min(item->mins[a],low[a]);item->maxs[a]=max(item->maxs[a],high[a]);}
        }
    }
    if(pointsChanged && fb_point_group_bounds){
        static vector groupBounds[2];
        for(int i=0;i<fb_point_node_count;i++){
            int group=i/64;fb_entity_bounds(fb_nodes[fb_point_nodes[i]].id,groupBounds);
            if(!(i%64)){fb_point_group_bounds[group*2]=groupBounds[0];fb_point_group_bounds[group*2+1]=groupBounds[1];}
            else for(int a=0;a<3;a++){fb_point_group_bounds[group*2][a]=min(fb_point_group_bounds[group*2][a],groupBounds[0][a]);fb_point_group_bounds[group*2+1][a]=max(fb_point_group_bounds[group*2+1][a],groupBounds[1][a]);}
        }
    }
    if(pointsChanged){if(fb_links_entities==fb_entity_render_epoch)fb_links_entities++;fb_entity_render_epoch++;fb_point_move_refresh=TRUE;fb_point_move_visibility=fb_visibility_epoch;}
    fb_container_entityRevision=fb_entity_render_epoch;fb_container_edits=fb_container_edit_epoch;fb_container_revision=fb_container_geometry=fb_grid_geometry_epoch;fb_container_filters=fb_filter_epoch;
    ftebroom_incremental_bounds(fb_container_cache,fb_node_count,sizeof(fb_container_bounds_t));
    if(brushesChanged){fb_face_upload_epoch++;fb_grid_upload_epoch++;fb_incremental_flatten();fb_occlusion_revision++;}
    fb_entity_links_moved();fb_visibility_epoch++;fb_incremental_epoch++;fb_commit_profile("batch91_document",started91);return TRUE;
}
