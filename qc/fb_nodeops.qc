// SPDX-License-Identifier: GPL-2.0-or-later
void fb_organize_cleanup();
void fb_delete_geometry()
{
    int count=selectedbrushcount;if(!count)return;
    int *pairs=memalloc(count*2*sizeof(int));for(int i=0;i<count;i++){pairs[i*2]=selectedbrushes[i].model;pairs[i*2+1]=selectedbrushes[i].id;}
    int batch=FALSE;if(checkbuiltin(ftebroom_brush_delete_batch))if(count>=32)batch=ftebroom_brush_delete_batch(FALSE,pairs,count)==count;
    if(batch){
        fb_history_defer_delete=TRUE;for(int i=0;i<count;i++)brush_history_delete(pairs[i*2],pairs[i*2+1]);fb_history_defer_delete=FALSE;
        brush_deselectall();if(ftebroom_brush_delete_batch(TRUE,pairs,count)!=count)for(int i=0;i<count;i++)brush_delete(pairs[i*2],pairs[i*2+1]);
    }else while(selectedbrushcount)brush_history_delete(selectedbrushes[0].model,selectedbrushes[0].id);
    memfree(pairs);
}
void fb_delete_selection()
{
    fb_tool_refresh();fb_document_tick();
    if(!selectedbrushcount && !fb_selected_entity_count)return;
    float started92=gettime(1);int *containers=memalloc(fb_node_count*sizeof(int));int containerCount=0;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP||fb_nodes[n].kind==FB_NODE_ENTITY)if(!fb_selection_leaf(n)&&fb_selection_selected(n)){containers[n]=TRUE;containerCount++;}
    fb_tx_begin("Delete");fb_delete_geometry();
    for(int e=0;e<fb_selected_entity_count;e++)fb_tx_entity_set(fb_selected_entities[e],"",HIST_ENTITY);
    if(containerCount){
    int *alive=memalloc(fb_node_count*sizeof(int));
    for(int n=2;n<fb_node_count;n++){
        if(!fb_selection_leaf(n))continue;int present=FALSE;
        if(fb_nodes[n].kind==FB_NODE_BRUSH)present=brush_get(fb_nodes[n].model,fb_nodes[n].id,__NULL__,0,__NULL__)>0;
        else if(fb_nodes[n].kind==FB_NODE_PATCH)present=patch_getcp(fb_nodes[n].model,fb_nodes[n].id,__NULL__,0,__NULL__)>0;
        else present=terrain_edit(TEREDIT_ENT_GET,fb_nodes[n].id)!="";
        if(present)for(int parent=fb_nodes[n].parent,steps=0;parent>=2&&parent<fb_node_count&&steps++<fb_node_count;parent=fb_nodes[parent].parent)alive[parent]=TRUE;
    }
    for(int n=fb_node_count-1;n>=2;n--)if(containers[n]){
        if(fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_descendant(fb_selection_open_node(),n))continue;
        if(!fb_selection_leaf(n)&&!alive[n])if(terrain_edit(TEREDIT_ENT_GET,fb_nodes[n].id)!="")fb_tx_entity_set(fb_nodes[n].id,"",HIST_TREE);
    }
    memfree(alive);}
    memfree(containers);fb_selection_clear();fb_tx_commit();fb_commit_profile("delete92_commit",started92);
}
