// SPDX-License-Identifier: GPL-2.0-or-later
void fb_organize_cleanup();
void fb_delete_selection()
{
    fb_tool_refresh();fb_document_tick();
    if(!selectedbrushcount && !fb_selected_entity_count) return;
    int worldOnly=fb_incremental_enabled() && !fb_selected_entity_count;
    for(int b=0;b<selectedbrushcount;b++)if(selectedbrushes[b].model!=1)worldOnly=FALSE;
    if(worldOnly){
        fb_tx_begin("Delete");while(selectedbrushcount)brush_history_delete(selectedbrushes[0].model,selectedbrushes[0].id);
        fb_selection_clear();fb_tx_commit();return;
    }
    int *selected=memalloc(sizeof(int)*fb_node_count);
    for(int n=0;n<fb_node_count;n++) selected[n]=fb_selection_selected(n);
    fb_tx_begin("Delete");
    while(selectedbrushcount) brush_history_delete(selectedbrushes[0].model,selectedbrushes[0].id);
    for(int e=0;e<fb_selected_entity_count;e++) fb_tx_entity_set(fb_selected_entities[e],"",HIST_ENTITY);
    for(int n=2;n<fb_node_count;n++) if(selected[n] && (fb_nodes[n].kind==FB_NODE_GROUP || fb_nodes[n].kind==FB_NODE_ENTITY)) {
        if(fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_descendant(fb_selection_open_node(),n))continue;
        if(fb_selection_leaf(n)) continue;
        int alive=FALSE;
        for(int c=0;c<fb_node_count;c++) if(fb_selection_descendant(c,n) && fb_selection_leaf(c)) {
            if(fb_nodes[c].kind>=FB_NODE_BRUSH) {
                if(brush_get(fb_nodes[c].model,fb_nodes[c].id,__NULL__,0,__NULL__) ||
                    patch_getcp(fb_nodes[c].model,fb_nodes[c].id,__NULL__,0,__NULL__)) alive=TRUE;
            } else {
                string data=terrain_edit(TEREDIT_ENT_GET,fb_nodes[c].id);
                if(data!="") alive=TRUE;
            }
        }
        if(!alive) fb_tx_entity_set(fb_nodes[n].id,"",HIST_ENTITY);
    }
    fb_selection_clear();fb_organize_cleanup();fb_tx_commit();memfree(selected);
}
