// SPDX-License-Identifier: GPL-2.0-or-later
// Keep editable MAP data and remap history across a graphics device restart.
fb_transform_item_t *fb_restart_items;
int fb_restart_count,fb_restart_entities,fb_restart_selection_count;
string *fb_restart_fields;
selbrush_t *fb_restart_selection;
string fb_restart_selection_state;
int fb_restart_pending;
// Apply a whole ID map once; temporary IDs would require a document scan
// for every restored brush and exceed the VM instruction budget on large MAPs.
int fb_renderer_remapped_id(hashtable ids,int model,int id)
{
    string value=ids[sprintf("%i:%i",model,id)];
    return value==""?id:stoi(value);
}
void fb_renderer_remap_record(hashtable ids,history_t *record)
{
    record->id=fb_renderer_remapped_id(ids,record->brushmodel,record->id);
    for(int i=0;i<record->selectioncount;i++)record->selection[i].id=fb_renderer_remapped_id(ids,record->selection[i].model,record->selection[i].id);
    for(int i=0;i<record->aftercount;i++)record->afterselection[i].id=fb_renderer_remapped_id(ids,record->afterselection[i].model,record->afterselection[i].id);
}
void fb_renderer_remap_references(hashtable ids)
{
    for(int n=0;n<fb_node_count;n++)fb_nodes[n].id=fb_renderer_remapped_id(ids,fb_nodes[n].model,fb_nodes[n].id);
    for(int i=0;i<fb_line_count;i++){
        string value=ids[sprintf("%i:%i",fb_lines[i].model,fb_lines[i].id)];
        if(value!=""){fb_lines[i].id=stoi(value);fb_lines[i].mixed=FALSE;}
    }
    for(int i=0;i<fb_visibility_count;i++)fb_visibility_items[i].id=fb_renderer_remapped_id(ids,fb_visibility_items[i].model,fb_visibility_items[i].id);
    for(int i=history_min;i<history_max;i++)fb_renderer_remap_record(ids,&historyring[i&(historyring_length-1)]);
    for(int i=0;i<fb_tx_redocount;i++)fb_renderer_remap_record(ids,&fb_tx_redo[i]);
    for(int i=0;i<fb_restart_selection_count;i++)fb_restart_selection[i].id=fb_renderer_remapped_id(ids,fb_restart_selection[i].model,fb_restart_selection[i].id);
}
float fb_renderer_command()
{
    if(argv(0)=="ftebroom_renderer_restart") {
        if(fb_restart_pending)return TRUE;
        if(cvar("ftebroom_terrain_document") || cvar("ftebroom_mod_preview")) {
            print("ReyondBroom: restart the application to apply MSAA while using engine terrain or mod preview.\n");return TRUE;
        }
        if(fb_active)fb_cancel();
        fb_document_tick();
        fb_restart_items=memalloc(sizeof(fb_transform_item_t)*max(1,fb_node_count));fb_restart_count=0;
        for(int n=0;n<fb_node_count;n++) {
            fb_node_t node=fb_nodes[n];
            if(node.kind!=FB_NODE_BRUSH && node.kind!=FB_NODE_PATCH)continue;
            fb_transform_item_t *item=&fb_restart_items[fb_restart_count++];
            item->model=node.model;item->id=node.id;item->kind=node.kind;
            if(node.kind==FB_NODE_PATCH) {
                item->count=patch_getcp(node.model,node.id,__NULL__,0,__NULL__);
                item->points=memalloc(sizeof(patchvert_t)*item->count);
                patch_getcp(node.model,node.id,item->points,item->count,&item->patch);
                item->patch.shadername=strzone(item->patch.shadername);
            } else {
                item->count=brush_get(node.model,node.id,__NULL__,0,__NULL__);
                item->faces=memalloc(sizeof(brushface_t)*item->count);
                brush_get(node.model,node.id,item->faces,item->count,&item->contents);
                for(int f=0;f<item->count;f++)item->faces[f].shadername=strzone(item->faces[f].shadername);
            }
        }
        fb_restart_entities=terrain_edit(TEREDIT_ENT_COUNT);
        fb_restart_fields=memalloc(sizeof(string)*max(1,fb_restart_entities));
        for(int e=0;e<fb_restart_entities;e++)fb_restart_fields[e]=strzone(terrain_edit(TEREDIT_ENT_GET,e));
        fb_restart_selection_count=selectedbrushcount;
        fb_restart_selection=memalloc(sizeof(selbrush_t)*max(1,selectedbrushcount));
        memcpy(fb_restart_selection,selectedbrushes,sizeof(selbrush_t)*selectedbrushcount);
        fb_restart_selection_state=strzone(fb_selection_state());fb_restart_pending=TRUE;
        localcmd("vid_restart\nftebroom_renderer_restore\n");return TRUE;
    }
    if(argv(0)!="ftebroom_renderer_restore")return FALSE;
    if(!fb_restart_pending)return TRUE;
    // The engine reloaded the saved map: discard those objects before restoring the live document.
    fb_document_rebuild();
    for(int n=0;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH || fb_nodes[n].kind==FB_NODE_PATCH)
        brush_delete(fb_nodes[n].model,fb_nodes[n].id);
    int entities=terrain_edit(TEREDIT_ENT_COUNT);
    for(int e=0;e<max(entities,fb_restart_entities);e++) {
        if(e<fb_restart_entities && fb_restart_fields[e]!="")terrain_edit(TEREDIT_ENT_SET,e,fb_restart_fields[e]);
        else terrain_edit(TEREDIT_ENT_SET,e,__NULL__);
    }
    editor_ents_reload();fb_document_rebuild();
    hashtable remapped=hash_createtab(max(64,fb_restart_count*2),EV_STRING);
    for(int i=0;i<fb_restart_count;i++) {
        fb_transform_item_t *item=&fb_restart_items[i];
        int newid;
        if(item->kind==FB_NODE_PATCH)newid=patch_create(item->model,0,item->points,item->patch);
        else newid=brush_create(item->model,item->faces,item->count,item->contents);
        remapped[sprintf("%i:%i",item->model,item->id)]=itos(newid);
        item->id=newid;
        if(item->faces)for(int f=0;f<item->count;f++)strunzone(item->faces[f].shadername);
        if(item->points)strunzone(item->patch.shadername);
        memfree(item->faces);memfree(item->points);
    }
    fb_renderer_remap_references(remapped);hash_destroytab(remapped);
    fb_document_rebuild();history_restore_list(fb_restart_selection,fb_restart_selection_count,-1);
    fb_selection_restore_state(fb_restart_selection_state);strunzone(fb_restart_selection_state);
    for(int e=0;e<fb_restart_entities;e++)strunzone(fb_restart_fields[e]);
    memfree(fb_restart_fields);memfree(fb_restart_items);memfree(fb_restart_selection);
    fb_restart_fields=__NULL__;fb_restart_items=__NULL__;fb_restart_selection=__NULL__;fb_restart_pending=FALSE;
    fb_grid_geometry_epoch++;brush_selection_invalidate();
    print("ReyondBroom: restored editable MAP after renderer restart.\n");return TRUE;
}
