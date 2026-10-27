// SPDX-License-Identifier: GPL-2.0-or-later
int fb_active_layer_model();
int *fb_clone_entities;
int *fb_clone_models;
int *fb_clone_has_brush;
int *fb_clone_group_ids;
hashtable fb_clone_entity_ids;
int fb_clone_count;
void fb_clone_clear()
{
    memfree(fb_clone_entities);memfree(fb_clone_models);memfree(fb_clone_has_brush);memfree(fb_clone_group_ids);
    fb_clone_has_brush=fb_clone_group_ids=__NULL__;if(fb_clone_entity_ids)hash_destroytab(fb_clone_entity_ids);fb_clone_entity_ids=0i;
    fb_clone_entities=fb_clone_models=__NULL__;fb_clone_count=0;
}
int fb_clone_groups()
{
    fb_clone_clear();fb_clone_count=fb_node_count;
    fb_clone_entities=memalloc(sizeof(int)*fb_clone_count);
    fb_clone_models=memalloc(sizeof(int)*fb_clone_count);
    fb_clone_group_ids=memalloc(sizeof(int)*fb_clone_count);
    fb_clone_has_brush=memalloc(sizeof(int)*fb_clone_count);
    fb_clone_entity_ids=hash_createtab(max(64,fb_clone_count),EV_INTEGER);
    for(int child=2;child<fb_clone_count;child++)if(fb_nodes[child].kind>=FB_NODE_BRUSH) {
        int parent=fb_nodes[child].parent;if(parent>=0 && parent<fb_clone_count)fb_clone_has_brush[parent]=1;
    }
    int nextId=1;
    for(int n=0;n<fb_clone_count;n++) {
        fb_clone_entities[n]=-1;
        nextId=max(nextId,stoi(fb_nodes[n].tbid)+1);
    }
    for(int n=2;n<fb_clone_count;n++) if(fb_nodes[n].kind<FB_NODE_BRUSH && fb_nodes[n].kind!=FB_NODE_LAYER) {
        int target=fb_selection_target(n);
        int grouped=fb_nodes[target].kind==FB_NODE_GROUP && fb_selection_selected(target);
        int wholeEntity=fb_nodes[n].kind==FB_NODE_ENTITY && fb_selection_selected(n);
        if(!grouped && !wholeEntity) continue;
        int model=0;
        string modelName="";
        if(fb_clone_has_brush[n]) {
            modelName=strzone(brush_model_create());
            if(modelName=="") {strunzone(modelName);return FALSE;}
            precache_model(modelName);
            if(!fb_document_probe) fb_document_probe=spawn();
            setmodel(fb_document_probe,modelName);model=fb_document_probe.modelindex;
            if(!model) {strunzone(modelName);return FALSE;}
        }
        string data=strzone(terrain_edit(TEREDIT_ENT_GET,fb_nodes[n].id));
        int entityIndex=fb_tx_empty_entity();
        fb_tx_entity_set(entityIndex,data,HIST_ENTITY);strunzone(data);
        if(modelName!="") {fb_tx_set_key(entityIndex,"model",modelName,HIST_ENTITY);strunzone(modelName);}
        if(fb_nodes[n].kind==FB_NODE_GROUP){fb_clone_group_ids[n]=nextId++;fb_tx_set_key(entityIndex,"_tb_id",itos(fb_clone_group_ids[n]),HIST_TREE);}
        fb_clone_entities[n]=entityIndex;fb_clone_models[n]=model;
        if(fb_nodes[n].kind==FB_NODE_ENTITY)fb_clone_entity_ids[itos(fb_nodes[n].id)]=entityIndex+1i;
    }
    // Resolve parent IDs after every container has received its independent identity.
    for(int n=2;n<fb_clone_count;n++) if(fb_clone_entities[n]>=0) {
        int parent=fb_nodes[n].parent;
        if(parent>=0 && parent<fb_clone_count && fb_clone_entities[parent]>=0) {
            string parentId="";if(fb_clone_group_ids[parent])parentId=itos(fb_clone_group_ids[parent]);
            fb_tx_set_key(fb_clone_entities[n],"_tb_group",parentId,HIST_TREE);
        }
    }
    return TRUE;
}
int fb_clone_destination(int model,int id)
{
    int n=fb_document_brush_node(model,id);
    if(n>=0 && n<fb_clone_count) {
        int parent=fb_nodes[n].parent;
        if(parent>=0 && parent<fb_clone_count && fb_clone_models[parent])return fb_clone_models[parent];
    }
    return model;
}
int fb_clone_entity(int id)
{
    if(!fb_clone_entity_ids)return -1;
    int found=(int)fb_clone_entity_ids[itos(id)];return found-1i;

}
// Return the editable model of the open container, allocating it transactionally.
int fb_creation_model()
{
    int opened=fb_selection_open_node();
    if(opened<0) return fb_active_layer_model();
    int entityIndex=fb_nodes[opened].id;
    string name="";int count=fb_entity_tokenize(terrain_edit(TEREDIT_ENT_GET,entityIndex));
    for(int k=0;k<count;k+=2) if(argv(k)=="model") name=argv(k+1);
    name=strzone(name);
    if(substring(name,0,1)!="*") {
        strunzone(name);name=strzone(brush_model_create());
        if(name=="") {strunzone(name);return 0;}
        fb_tx_set_key(entityIndex,"model",name,HIST_ENTITY);
    }
    precache_model(name);if(!fb_document_probe) fb_document_probe=spawn();setmodel(fb_document_probe,name);
    int model=fb_document_probe.modelindex;strunzone(name);return model;
}
