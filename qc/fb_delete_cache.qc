// SPDX-License-Identifier: GPL-2.0-or-later
// Deleted surfaces keep their retained slots hidden; unaffected geometry stays resident.
int fb_delete_document(int first,int last,int redo)
{
    if(!redo || first<0 || first>=last || last>history_max)return FALSE;
    if(!fb_container_cache||!fb_grid_first||!fb_render_first||!fb_render_access||!fb_filter_tags||fb_grid_built_epoch!=fb_grid_geometry_epoch)return FALSE;
    int brushes=0,entities=0;
    for(int r=first;r<last;r++){
        history_t *h=&historyring[r&(historyring_length-1)];
        if(h->kind==HIST_SELECTION)continue;
        if(h->kind==HIST_GEOMETRY){if(!h->wasdelete||h->patchdata||!h->brushdata)return FALSE;brushes++;}
        else if(h->kind==HIST_ENTITY||h->kind==HIST_TREE){if(terrain_edit(TEREDIT_ENT_GET,h->entityindex)!="")return FALSE;entities++;}
        else return FALSE;
    }
    if(!brushes)return FALSE;
    float started=gettime(1);fb_document_brush_node(1,0);
    if(!fb_incremental_free_nodes)fb_incremental_free_nodes=memalloc(fb_node_capacity*sizeof(int));
    int *entityIds=memalloc(entities*sizeof(int));int entityCount=0;
    for(int r=first;r<last;r++){
        history_t *h=&historyring[r&(historyring_length-1)];
        if(h->kind==HIST_ENTITY||h->kind==HIST_TREE){entityIds[entityCount++]=h->entityindex;continue;}
        if(h->kind!=HIST_GEOMETRY)continue;
        int node=fb_document_brush_node(h->brushmodel,h->id);if(node<2)continue;
        fb_incremental_hide_base(node);fb_incremental_remove(node);fb_wire_cache_forget(node);
        hash_delete(fb_document_brush_index,sprintf("%i:%i",h->brushmodel,h->id));fb_grid_first[node]=fb_render_first[node]=0;
        strunzone(fb_nodes[node].name);fb_nodes[node].name="";fb_nodes[node].kind=-1;fb_nodes[node].flags=FB_NODE_HIDDEN;
        fb_render_access[node]=fb_filter_tags[node]=0;if(fb_static_access)fb_static_access[node]=0;fb_container_cache[node].valid=FALSE;
        fb_incremental_free_nodes[fb_incremental_free_count++]=node;
    }
    if(entityCount)fb_document_update_entities(entityIds,entityCount);memfree(entityIds);
    for(int n=0;n<fb_node_count;n++)if(!fb_selection_leaf(n))fb_container_cache[n].valid=FALSE;
    for(int n=2;n<fb_node_count;n++)if(fb_selection_leaf(n)&&fb_container_cache[n].valid){
        vector low=fb_container_cache[n].mins,high=fb_container_cache[n].maxs;
        for(int parent=fb_nodes[n].parent,steps=0;parent>=0&&parent<fb_node_count&&steps++<fb_node_count;parent=fb_nodes[parent].parent){
            fb_container_bounds_t *box=&fb_container_cache[parent];if(!box->valid){box->mins=low;box->maxs=high;box->valid=TRUE;}
            else for(int a=0;a<3;a++){box->mins[a]=min(box->mins[a],low[a]);box->maxs[a]=max(box->maxs[a],high[a]);}
        }
    }
    fb_container_entityRevision=fb_entity_render_epoch;fb_container_edits=fb_container_edit_epoch;fb_container_revision=fb_container_geometry=fb_grid_geometry_epoch;fb_container_filters=fb_filter_epoch;
    fb_document_structure_epoch++;fb_visibility_epoch++;fb_incremental_epoch++;fb_occlusion_revision++;fb_face_upload_epoch++;fb_grid_upload_epoch++;
    ftebroom_incremental_bounds(fb_container_cache,fb_node_count,sizeof(fb_container_bounds_t));fb_incremental_materials(first,last,TRUE);fb_incremental_flatten();fb_commit_profile("delete92_document",started);return TRUE;
}
