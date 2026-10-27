// SPDX-License-Identifier: GPL-2.0-or-later
typedef struct {int model,id,hidden;} fb_visibility_t;
fb_visibility_t *fb_visibility_items;int fb_visibility_count,fb_visibility_epoch;
int fb_active_layer;
int fb_filter_epoch;
int fb_filter_custom(int node);
string fb_structural_material(string material);
string fb_organize_escape(string value);
void fb_organize_publish()
{
    filestream stream=fopen("ftebroom-ui-tree.txt",FILE_WRITE);if((float)stream<0) return;
    for(int n=1;n<fb_node_count;n++) if(fb_nodes[n].kind==FB_NODE_LAYER || fb_nodes[n].kind==FB_NODE_GROUP) {
        int id=0;hashtable fields=(hashtable)fb_nodes[0].fields;
        if(n>1) {id=fb_nodes[n].id;fields=(hashtable)fb_nodes[n].fields;}
        fputs(stream,sprintf("%i %i %i %i %i \"%s\"\n",fb_nodes[n].kind,id,fb_nodes[n].flags,stoi(fields["_tb_layer_omit_from_export"]),stoi(fields["_tb_layer_sort_index"]),fb_organize_escape(fb_nodes[n].name)));
    }
    fclose(stream);
    stream=fopen("ftebroom-entity-list.txt",FILE_WRITE);
    if((float)stream>=0) {
        for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind==FB_NODE_GROUP || fb_nodes[n].kind==FB_NODE_ENTITY) {
            string label=fb_nodes[n].name,classname=fb_nodes[n].fields["classname"],targetname=fb_nodes[n].fields["targetname"];
            if(fb_nodes[n].kind==FB_NODE_ENTITY && targetname!="")label=targetname;
            fputs(stream,sprintf("%i %i %i \"%s\" \"%s\"\n",fb_nodes[n].id,fb_nodes[n].kind,fb_nodes[n].flags,fb_organize_escape(label),fb_organize_escape(classname)));
        }
        fclose(stream);
    }
    cvar_set("ftebroom_tree_epoch",itos(fb_grid_geometry_epoch));
}
int fb_visibility_get(int model,int id)
{
    for(int i=0;i<fb_visibility_count;i++) if(fb_visibility_items[i].model==model && fb_visibility_items[i].id==id) return fb_visibility_items[i].hidden;
    return FALSE;
}
void fb_visibility_set(int model,int id,int hidden)
{
    for(int i=0;i<fb_visibility_count;i++) if(fb_visibility_items[i].model==model && fb_visibility_items[i].id==id) {
        fb_visibility_items[i].hidden=hidden;fb_visibility_epoch++;return;
    }
    fb_visibility_t *items=memalloc((fb_visibility_count+1)*sizeof(fb_visibility_t));
    memcpy(items,fb_visibility_items,fb_visibility_count*sizeof(fb_visibility_t));memfree(fb_visibility_items);fb_visibility_items=items;
    items[fb_visibility_count].model=model;items[fb_visibility_count].id=id;items[fb_visibility_count++].hidden=hidden;fb_visibility_epoch++;
}
void fb_visibility_remap(int model,int oldid,int newid)
{
    for(int i=0;i<fb_visibility_count;i++) if(fb_visibility_items[i].model==model && fb_visibility_items[i].id==oldid) fb_visibility_items[i].id=newid;
}
void fb_visibility_reset() {memfree(fb_visibility_items);fb_visibility_items=__NULL__;fb_visibility_count=0;fb_visibility_epoch++;fb_active_layer=0;cvar_set("ftebroom_active_layer","0");}
void fb_visibility_change(int model,int id,int hidden)
{
    int before=fb_visibility_get(model,id);if(before==hidden) return;
    history_t *h=history_allocate();h->kind=HIST_VISIBILITY;h->brushmodel=model;h->id=id;
    h->beforefields=strzone(itos(before));h->afterfields=strzone(itos(hidden));fb_visibility_set(model,id,hidden);
}
void fb_visibility_command(string operation)
{
    fb_tool_refresh();fb_document_tick();fb_tx_begin(operation=="hide"?"Hide":(operation=="isolate"?"Isolate":"Show All"));
    if(operation=="showall") {
        for(int i=0;i<fb_visibility_count;i++) if(fb_visibility_items[i].hidden) fb_visibility_change(fb_visibility_items[i].model,fb_visibility_items[i].id,FALSE);
        for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind==FB_NODE_LAYER) fb_tx_set_key(fb_nodes[n].id,"_tb_layer_hidden","0",HIST_NODEFLAGS);
        fb_tx_set_key(0,"_tb_layer_hidden","0",HIST_NODEFLAGS);
    } else {
        int any=selectedbrushcount || fb_selected_entity_count;
        if(operation=="isolate" && !any) {fb_tx_cancel();return;}
        for(int n=2;n<fb_node_count;n++) if(fb_selection_leaf(n)) {
            int selected=FALSE;
            if(fb_nodes[n].kind==FB_NODE_ENTITY) selected=fb_entity_selected(fb_nodes[n].id);
            else selected=brush_isselected(fb_nodes[n].model,fb_nodes[n].id)!=0;
            if((operation=="hide" && selected) || (operation=="isolate" && !selected)) fb_visibility_change(fb_nodes[n].model,fb_nodes[n].id,TRUE);
        }
        if(operation=="hide") fb_selection_clear();
    }
    fb_tx_commit();
}
void fb_visibility_node(int n)
{
    if(n<2 || n>=fb_node_count || fb_nodes[n].kind<FB_NODE_BRUSH)return;
    int hideNative=!fb_pick_allowed(n,FB_PICK_LOCKED) || brush_isselected(fb_nodes[n].model,fb_nodes[n].id) || fb_filter_custom(n) || fb_preview_replaces(fb_nodes[n].model,fb_nodes[n].id);
    brush_selected(fb_nodes[n].model,fb_nodes[n].id,-1,hideNative);
}
void fb_visibility_sync()
{
    static int revision=-1,epoch=-1,filter=-1,geometry=-1,selectionEpoch=-1,previewState=-1;
    static int *previous;static int previousCount;
    int preview=(fb_transform_preview && !fb_transform_duplicate)?fb_transform_count:0;
    int current=fb_document_revision();if(current==revision && epoch==fb_visibility_epoch && filter==fb_filter_epoch && geometry==fb_grid_geometry_epoch && selectionEpoch==fb_selection_epoch && previewState==preview)return;
    int full=geometry!=fb_grid_geometry_epoch || epoch!=fb_visibility_epoch || filter!=fb_filter_epoch || !fb_incremental_enabled();
    if(geometry!=fb_grid_geometry_epoch){memfree(previous);previous=memalloc(fb_node_capacity*sizeof(int));previousCount=0;}
    if(full)for(int n=2;n<fb_node_count;n++)fb_visibility_node(n);
    else for(int i=0;i<previousCount;i++)fb_visibility_node(previous[i]);
    previousCount=0;
    for(int i=0;i<selectedbrushcount;i++){
        int n=fb_document_brush_node(selectedbrushes[i].model,selectedbrushes[i].id);if(n<2)continue;
        if(!full)fb_visibility_node(n);previous[previousCount++]=n;
    }
    selectionEpoch=fb_selection_epoch;previewState=preview;revision=current;epoch=fb_visibility_epoch;filter=fb_filter_epoch;geometry=fb_grid_geometry_epoch;
}
int fb_organize_node(int entityIndex)
{
    if(!entityIndex) return 1;
    for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind<FB_NODE_BRUSH && fb_nodes[n].id==entityIndex) return n;
    return -1;
}
int fb_organize_model(int entityIndex)
{
    if(!entityIndex) return 1;
    string data=strzone(terrain_edit(TEREDIT_ENT_GET,entityIndex)),name="";int count=fb_entity_tokenize(data);
    for(int k=0;k<count;k+=2) if(argv(k)=="model") name=argv(k+1);
    name=strzone(name);
    if(substring(name,0,1)!="*") {strunzone(name);name=strzone(brush_model_create());if(name!="") fb_tx_set_key(entityIndex,"model",name,HIST_ENTITY);}
    int model=0;if(name!="") {precache_model(name);if(!fb_document_probe) fb_document_probe=spawn();setmodel(fb_document_probe,name);model=fb_document_probe.modelindex;}
    strunzone(name);strunzone(data);return model;
}
string fb_organize_escape(string value) {return strreplace("\n","\\n",strreplace("\"","\\\"",strreplace("\\","\\\\",value)));}
int fb_organize_new(string kind,string name,int parent)
{
    int next=1;for(int n=2;n<fb_node_count;n++) next=max(next,stoi(fb_nodes[n].tbid)+1);
    string type="_tb_group";if(kind=="layer") type="_tb_layer";
    string fields=strcat("\"classname\" \"func_group\"\n\"_tb_type\" \"",type,"\"\n\"_tb_name\" \"",fb_organize_escape(name),"\"\n\"_tb_id\" \"",itos(next),"\"\n");
    if(parent>=2 && kind!="layer") {
        string key="_tb_group";if(fb_nodes[parent].kind==FB_NODE_LAYER) key="_tb_layer";
        fields=strcat(fields,"\"",key,"\" \"",fb_nodes[parent].tbid,"\"\n");
    }
    if(kind=="layer") fields=strcat(fields,"\"_tb_layer_sort_index\" \"",itos(next),"\"\n");
    fields=strzone(fields);int id=fb_tx_empty_entity();fb_tx_entity_set(id,fields,HIST_TREE);strunzone(fields);return id;
}
void fb_organize_parent(int entityIndex,int parent)
{
    string group="",layer="";
    if(parent>=2) {if(fb_nodes[parent].kind==FB_NODE_GROUP) group=fb_nodes[parent].tbid;else if(fb_nodes[parent].kind==FB_NODE_LAYER) layer=fb_nodes[parent].tbid;}
    fb_tx_set_key(entityIndex,"_tb_group",group,HIST_TREE);fb_tx_set_key(entityIndex,"_tb_layer",layer,HIST_TREE);
}
int fb_organize_move_brush(int model,int id,int destination,int structural)
{
    int count=patch_getcp(model,id,__NULL__,0,__NULL__);int result=0;
    if(count) {
        patchinfo_t info;patchvert_t *points=memalloc(count*sizeof(patchvert_t));patch_getcp(model,id,points,count,&info);
        result=patch_history_create(destination,points,info,TRUE);memfree(points);
    } else {
        count=brush_get(model,id,__NULL__,0,__NULL__);if(count<4 || count>64) return FALSE;
        brushface_t *faces=memalloc(count*sizeof(brushface_t));int contents;brush_get(model,id,faces,count,&contents);
        if(structural) {
            contents=CONTENTBIT_SOLID;
            for(int f=0;f<count;f++)faces[f].shadername=fb_structural_material(faces[f].shadername);
        }
        result=brush_history_create(destination,faces,count,contents,TRUE);memfree(faces);
    }
    if(!result) return FALSE;brush_history_delete(model,id);return TRUE;
}
void fb_organize_cleanup()
{
    fb_document_rebuild();
    int *alive=memalloc(fb_node_count*sizeof(int));
    // Mark surviving ancestors from each leaf once, even on documents with
    // thousands of groups. Empty containers do not keep ancestors alive.
    for(int b=2;b<fb_node_count;b++) {
        if(!fb_selection_leaf(b))continue;
        int present=FALSE;
        if(fb_nodes[b].kind>=FB_NODE_BRUSH)present=brush_get(fb_nodes[b].model,fb_nodes[b].id,__NULL__,0,__NULL__) || patch_getcp(fb_nodes[b].model,fb_nodes[b].id,__NULL__,0,__NULL__);
        else present=terrain_edit(TEREDIT_ENT_GET,fb_nodes[b].id)!="";
        if(!present)continue;
        for(int parent=fb_nodes[b].parent,steps=0;parent>=2 && parent<fb_node_count && steps<fb_node_count;steps++) {
            alive[parent]=TRUE;parent=fb_nodes[parent].parent;
        }
    }
    for(int n=fb_node_count-1;n>=2;n--) if(fb_nodes[n].kind==FB_NODE_GROUP || fb_nodes[n].kind==FB_NODE_ENTITY) {
        if(fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_descendant(fb_selection_open_node(),n))continue;
        if(fb_selection_leaf(n))continue;
        if(!alive[n])fb_tx_entity_set(fb_nodes[n].id,"",HIST_TREE);
    }
    memfree(alive);
}
int fb_organize_move(int entityIndex,int parent,int structural)
{
    if(parent>=2) for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_selected(n)) {
        if(fb_selection_descendant(fb_selection_open_node(),n))continue;
        if(fb_selection_descendant(parent,n)) {cvar_set("ftebroom_selection_status","Cannot move a group into itself or its descendants");return FALSE;}
    }
    int destination=fb_organize_model(entityIndex);if(!destination) return FALSE;
    int *roots=memalloc(fb_node_count*sizeof(int));memfill8((__variant*)roots,0,fb_node_count*sizeof(int));
    if(!structural) for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind==FB_NODE_GROUP || fb_nodes[n].kind==FB_NODE_ENTITY) {
        if(fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_descendant(fb_selection_open_node(),n))continue;
        if(fb_selection_selected(n)) roots[n]=TRUE;
    }
    for(int n=2;n<fb_node_count;n++) if(roots[n]) for(int a=2;a<fb_node_count;a++) if(a!=n && roots[a] && fb_selection_descendant(n,a)) {roots[n]=FALSE;break;}
    int count=selectedbrushcount;selbrush_t *selected=memalloc(count*sizeof(selbrush_t));memcpy(selected,selectedbrushes,count*sizeof(selbrush_t));
    int valid=TRUE;
    for(int b=0;b<count;b++) {
        int belongs=FALSE;
        for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind>=FB_NODE_BRUSH && fb_nodes[n].model==selected[b].model && fb_nodes[n].id==selected[b].id)
            for(int r=2;r<fb_node_count;r++) if(roots[r] && fb_selection_descendant(n,r)) belongs=TRUE;
        if(!belongs) if(!fb_organize_move_brush(selected[b].model,selected[b].id,destination,structural)) {valid=FALSE;break;}
    }
    if(valid) for(int n=2;n<fb_node_count;n++) if(roots[n]) fb_organize_parent(fb_nodes[n].id,parent);
    memfree(roots);memfree(selected);return valid;
}
void fb_group_create(string name)
{
    if(cvar("ftebroom_tool")!=FB_TOOL_NONE){cvar_set("ftebroom_selection_status","Exit the modal tool before grouping objects");return;}
    string owned=strzone(name);fb_tool_refresh();fb_document_tick();
    if(!selectedbrushcount && !fb_selected_entity_count) {strunzone(owned);return;}
    int parent=fb_selection_open_node();if(parent<0) parent=fb_organize_node(fb_active_layer);
    fb_tx_begin("Group");int group=fb_organize_new("group",owned,parent);strunzone(owned);
    // Publish the new container before reparenting, while retaining selection references.
    fb_document_rebuild();int node=fb_organize_node(group);
    if(fb_organize_move(group,node,FALSE)) {fb_organize_cleanup();fb_tx_commit();}else fb_tx_cancel();
}
int fb_active_layer_model()
{
    int node=fb_organize_node(fb_active_layer);if(node<1) {fb_active_layer=0;node=1;cvar_set("ftebroom_active_layer","0");}
    if(!fb_pick_allowed(node,0)) {cvar_set("ftebroom_selection_status","The active layer is hidden or locked");return 0;}
    return fb_organize_model(fb_active_layer);
}
int fb_group_release(int node,int parent)
{
    int parentEntity=0;if(parent>=2) parentEntity=fb_nodes[parent].id;
    int model=fb_organize_model(parentEntity);if(!model) return FALSE;
    for(int c=2;c<fb_node_count;c++) if(fb_nodes[c].parent==node) {
        if(fb_nodes[c].kind>=FB_NODE_BRUSH) {if(!fb_organize_move_brush(fb_nodes[c].model,fb_nodes[c].id,model,FALSE)) return FALSE;}
        else fb_organize_parent(fb_nodes[c].id,parent);
    }
    if(fb_open_group==fb_nodes[node].id) fb_open_group=0;
    fb_tx_entity_set(fb_nodes[node].id,"",HIST_TREE);return TRUE;
}
void fb_organize_action(string operation,int target,string value)
{
    string op=strzone(operation),name=strzone(value);fb_tool_refresh();fb_document_tick();
    if(op=="focus") {
        int focusNode=fb_organize_node(target);
        if(focusNode>=2 && (fb_nodes[focusNode].kind==FB_NODE_GROUP || fb_nodes[focusNode].kind==FB_NODE_ENTITY)) {
            int previousOpen=fb_open_group;fb_open_group=0;int allowed=fb_pick_allowed(focusNode,0);fb_open_group=previousOpen;
            if(allowed) {
                fb_tx_begin("Select from Entity List");fb_open_group=0;
                for(int parent=fb_nodes[focusNode].parent,steps=0;parent>=2 && steps<fb_node_count;steps++) {
                    if(fb_nodes[parent].kind==FB_NODE_GROUP){fb_open_group=fb_nodes[parent].id;break;}
                    parent=fb_nodes[parent].parent;
                }
                fb_selection_clear();fb_selection_set(focusNode,TRUE);fb_tx_commit();fb_camera_focus();
            } else cvar_set("ftebroom_selection_status","Object is hidden, filtered or locked; enable it first");
        }
        strunzone(op);strunzone(name);return;
    }
    if(op=="layeractive") {
        int activeNode=fb_organize_node(target);
        if(activeNode>=1 && fb_nodes[activeNode].kind==FB_NODE_LAYER) {fb_active_layer=target;cvar_set("ftebroom_active_layer",itos(target));}
        strunzone(op);strunzone(name);return;
    }
    fb_tx_begin(strcat("Organize: ",op));int valid=TRUE,node=fb_organize_node(target);
    if(op=="layernew") {fb_active_layer=fb_organize_new("layer",name,0);cvar_set("ftebroom_active_layer",itos(fb_active_layer));}
    else if(op=="rename") {
        for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_selected(n)) fb_tx_set_key(fb_nodes[n].id,"_tb_name",name,HIST_TREE);
    } else if(op=="ungroup") {
        int *groups=memalloc(fb_node_count*sizeof(int));
        for(int n=2;n<fb_node_count;n++) groups[n]=fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_selected(n) && fb_selection_target(n)==n;
        for(int n=2;n<fb_node_count && valid;n++) if(groups[n]) valid=fb_group_release(n,fb_nodes[n].parent);
        memfree(groups);
    } else if(op=="mergegroups") {
        int destination=-1,total=0;
        int *merged=memalloc(fb_node_count*sizeof(int));
        for(int n=2;n<fb_node_count;n++) if(fb_nodes[n].kind==FB_NODE_GROUP && fb_selection_target(n)==n && fb_selection_selected(n)) {
            if(destination<0)destination=n;else merged[total++]=n;
        }
        if(total<1)valid=FALSE;
        for(int n=0;n<total && valid;n++)valid=fb_group_release(merged[n],destination);
        memfree(merged);
    } else if(op=="move") {
        if(node<1) valid=FALSE;else valid=fb_organize_move(target,node,FALSE);
    } else if(op=="removefromgroup") {
        int opened=fb_selection_open_node(),layer=1;if(opened>=0) layer=fb_nodes[opened].layer;
        int entityIndex=0;if(layer>=2) entityIndex=fb_nodes[layer].id;
        valid=fb_organize_move(entityIndex,layer,FALSE);
    } else if(op=="structural") {
        int count=selectedbrushcount;selbrush_t *selection=memalloc(count*sizeof(selbrush_t));memcpy(selection,selectedbrushes,count*sizeof(selbrush_t));
        for(int b=0;b<count && valid;b++) {
            int parent=1;
            for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind>=FB_NODE_BRUSH && fb_nodes[n].model==selection[b].model && fb_nodes[n].id==selection[b].id){parent=fb_nodes[n].parent;break;}
            if(parent>=2 && fb_nodes[parent].kind==FB_NODE_ENTITY)parent=fb_nodes[parent].parent;
            int owner=0;if(parent>=2)owner=fb_nodes[parent].id;
            int structuralModel=fb_organize_model(owner);
            if(!structuralModel)valid=FALSE;else valid=fb_organize_move_brush(selection[b].model,selection[b].id,structuralModel,TRUE);
        }
        memfree(selection);
    } else if(op=="layerremove") {
        if(node<=1 || fb_nodes[node].kind!=FB_NODE_LAYER) valid=FALSE;
        else {valid=fb_group_release(node,1);if(fb_active_layer==target) fb_active_layer=0;}
    } else if(op=="layerselect") {
        if(node<1 || fb_nodes[node].kind!=FB_NODE_LAYER) valid=FALSE;
        else {fb_selection_clear();fb_selection_set(node,TRUE);}
    } else if(op=="layerrename" || op=="layerhidden" || op=="layerlocked" || op=="layeromit" || op=="layersort") {
        if(node<1 || fb_nodes[node].kind!=FB_NODE_LAYER || (node==1 && (op=="layerrename" || op=="layersort"))) valid=FALSE;
        else {
            string key="_tb_name";if(op=="layerhidden") key="_tb_layer_hidden";if(op=="layerlocked") key="_tb_layer_locked";
            if(op=="layeromit") key="_tb_layer_omit_from_export";if(op=="layersort") key="_tb_layer_sort_index";
            if((op=="layerhidden" || op=="layerlocked") && name=="1") fb_selection_set(node,FALSE);
            fb_tx_set_key(target,key,name,HIST_TREE);
        }
    } else valid=FALSE;
    if(valid) {fb_organize_cleanup();fb_tx_commit();}else fb_tx_cancel();
    strunzone(op);strunzone(name);
}
