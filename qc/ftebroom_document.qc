void fb_lines_remap(int model,int oldid,int newid);
void fb_visibility_remap(int model,int oldid,int newid);
void fb_visibility_reset();
void fb_organize_publish();
// SPDX-License-Identifier: GPL-2.0-or-later
void fb_pick_clear_handles();
// The engine remains the MAP serializer; this tree references its entity/submodel data.
enum : int { FB_NODE_WORLD, FB_NODE_LAYER, FB_NODE_GROUP, FB_NODE_ENTITY, FB_NODE_BRUSH, FB_NODE_PATCH };
enum : int { FB_NODE_HIDDEN=1, FB_NODE_LOCKED=2, FB_NODE_SELECTED=4 };
typedef struct {
    int kind, model, id, parent, layer, flags;
    string name, tbid, parentref, layerref;
    hashtable fields;
} fb_node_t;
fb_node_t *fb_nodes;
int fb_node_count,fb_node_capacity,fb_saved_revision,fb_seen_revision;
entity fb_document_probe;
int fb_grid_geometry_epoch;
int *fb_point_nodes;
int *fb_container_nodes;
int *fb_patch_nodes;
int fb_point_node_count,fb_container_node_count,fb_patch_node_count;
void fb_document_lists_prepare()
{
    static int geometry=-1,count=-1;
    if(geometry==fb_grid_geometry_epoch&&count==fb_node_count)return;
    geometry=fb_grid_geometry_epoch;count=fb_node_count;
    memfree(fb_point_nodes);memfree(fb_container_nodes);memfree(fb_patch_nodes);
    fb_point_nodes=memalloc(max(1,count)*sizeof(int));fb_container_nodes=memalloc(max(1,count)*sizeof(int));fb_patch_nodes=memalloc(max(1,count)*sizeof(int));
    fb_point_node_count=fb_container_node_count=fb_patch_node_count=0;
    for(int n=2;n<count;n++){
        int kind=fb_nodes[n].kind;
        if(kind==FB_NODE_ENTITY){if(substring(fb_nodes[n].fields["model"],0,1)!="*")fb_point_nodes[fb_point_node_count++]=n;}
        if(kind==FB_NODE_ENTITY||kind==FB_NODE_GROUP)fb_container_nodes[fb_container_node_count++]=n;
        if(kind==FB_NODE_PATCH)fb_patch_nodes[fb_patch_node_count++]=n;
    }
}
int fb_document_revision()
{
    if(history>history_min) return historyring[(history-1)&(historyring_length-1)].revision;
    return history_base_revision;
}
void fb_document_clear()
{
    fb_grid_geometry_epoch++;
    for(int i=0;i<fb_node_count;i++) {
        strunzone(fb_nodes[i].name);strunzone(fb_nodes[i].tbid);
        strunzone(fb_nodes[i].parentref);strunzone(fb_nodes[i].layerref);
        if(fb_nodes[i].fields) hash_destroytab(fb_nodes[i].fields);
    }
    fb_node_count=0;
}
int fb_document_add(int kind,int model,int id,int parent,string name)
{
    if(fb_node_count==fb_node_capacity) {
        int capacity=max(64,fb_node_capacity*2);
        fb_node_t *nodes=memalloc(sizeof(fb_node_t)*capacity);
        memcpy(nodes,fb_nodes,sizeof(fb_node_t)*fb_node_count);memfree(fb_nodes);
        fb_nodes=nodes;fb_node_capacity=capacity;
    }
    int index=fb_node_count++;
    fb_nodes[index].flags=0;fb_nodes[index].fields=0i;
    fb_nodes[index].tbid=fb_nodes[index].parentref=fb_nodes[index].layerref="";
    fb_nodes[index].kind=kind;fb_nodes[index].model=model;fb_nodes[index].id=id;
    fb_nodes[index].parent=parent;fb_nodes[index].layer=1;
    fb_nodes[index].name=strzone(name);return index;
}
void fb_document_remap(int model,int oldid,int newid)
{
    fb_lines_remap(model,oldid,newid);
    fb_visibility_remap(model,oldid,newid);
    for(int i=0;i<fb_node_count;i++) if(fb_nodes[i].model==model) if(fb_nodes[i].id==oldid) fb_nodes[i].id=newid;
}
void fb_document_brushes(int model,int parent)
{
    int capacity=256,count;
    int *ids;
    static vector normals[1];static float distances[1];
    do {
        ids=memalloc(sizeof(int)*capacity);
        count=brush_findinvolume(model,normals,distances,0,ids,__NULL__,capacity);
        if(count<capacity) break;
        memfree(ids);capacity*=2;
        if(capacity>1048576) {print("ReyondBroom: document node limit exceeded.\n");return;}
    } while(TRUE);
    for(int b=0;b<count;b++) {
        int kind=patch_getcp(model,ids[b],__NULL__,0,__NULL__)>0?FB_NODE_PATCH:FB_NODE_BRUSH;
        fb_document_add(kind,model,ids[b],parent,kind==FB_NODE_PATCH?"Patch":"Brush");
    }
    memfree(ids);
}
// Resolve parent IDs once, then memoize layer ancestry without recursive walks.
void fb_document_resolve()
{
    hashtable ids=hash_createtab(max(64,fb_node_count*2),EV_STRING);
    for(int i=2;i<fb_node_count;i++) if(fb_nodes[i].tbid!="")
        if(ids[fb_nodes[i].tbid]=="")ids[fb_nodes[i].tbid]=itos(i);
    for(int i=2;i<fb_node_count;i++) {
        if(fb_nodes[i].kind>=FB_NODE_BRUSH)continue;
        if(fb_nodes[i].kind==FB_NODE_LAYER){fb_nodes[i].parent=0;continue;}
        string ref=fb_nodes[i].parentref;if(ref=="")ref=fb_nodes[i].layerref;
        if(ref!="") {
            int target=stoi(ids[ref]);
            if(target>=2 && target!=i)fb_nodes[i].parent=target;
        }
    }
    hash_destroytab(ids);
    int *done=memalloc(fb_node_count*sizeof(int));
    int *seen=memalloc(fb_node_count*sizeof(int));
    int *path=memalloc(fb_node_count*sizeof(int));
    done[0]=done[1]=TRUE;fb_nodes[0].layer=fb_nodes[1].layer=1;
    for(int i=2;i<fb_node_count;i++)if(fb_nodes[i].kind==FB_NODE_LAYER){done[i]=TRUE;fb_nodes[i].layer=i;}
    for(int i=2;i<fb_node_count;i++) {
        if(done[i])continue;
        int parent=i,length=0;
        while(!done[parent]) {
            if(seen[parent]==i) {
                fb_nodes[parent].parent=1;parent=1;
                print("ReyondBroom: cyclic group parent; using Default Layer.\n");break;
            }
            seen[parent]=i;path[length++]=parent;
            int next=fb_nodes[parent].parent;
            if(next<0 || next>=fb_node_count){fb_nodes[parent].parent=1;next=1;}
            parent=next;
        }
        int layer=fb_nodes[parent].layer;
        for(int j=0;j<length;j++){fb_nodes[path[j]].layer=layer;done[path[j]]=TRUE;}
    }
    memfree(path);memfree(seen);memfree(done);
}
void fb_document_rebuild()
{
    fb_document_clear();
    fb_document_add(FB_NODE_WORLD,1,0,-1,"World");
    fb_document_add(FB_NODE_LAYER,0,-1,0,"Default Layer");
    if(!fb_document_probe) fb_document_probe=spawn();
    int entities=terrain_edit(TEREDIT_ENT_COUNT);
    for(int e=0;e<entities;e++) {
        string data=terrain_edit(TEREDIT_ENT_GET,e);
        if not(data) continue;
        hashtable fields=hash_createtab(32,EV_STRING);
        int count=fb_entity_tokenize(data);
        for(int k=0;k<count;k+=2) fields[argv(k)]=argv(k+1);
        string classname=fields["classname"],type=fields["_tb_type"];
        int kind=FB_NODE_ENTITY;
        if(classname=="func_group") {
            if(type=="_tb_layer") kind=FB_NODE_LAYER;
            else if(type=="_tb_group") kind=FB_NODE_GROUP;
        }
        string name=fields["_tb_name"];
        if(name=="") name=classname;
        int node=e?fb_document_add(kind,0,e,1,name):0;
        fb_nodes[node].fields=fields;
        fb_nodes[node].tbid=strzone(fields["_tb_id"]);
        fb_nodes[node].parentref=strzone(fields["_tb_group"]);
        fb_nodes[node].layerref=strzone(fields["_tb_layer"]);
        int flagNode=node;if(!e) flagNode=1;
        if(fields["_tb_layer_hidden"]=="1") fb_nodes[flagNode].flags|=FB_NODE_HIDDEN;
        if(fields["_tb_layer_locked"]=="1") fb_nodes[flagNode].flags|=FB_NODE_LOCKED;
        string modelname=fields["model"];
        int model=0;
        if(!e) model=1;
        else if(substring(modelname,0,1)=="*") {
            precache_model(modelname);setmodel(fb_document_probe,modelname);model=fb_document_probe.modelindex;
        }
        if(model) fb_document_brushes(model,e?node:1);
    }
    fb_document_resolve();
    cvar_set("ftebroom_node_count",itos(fb_node_count));
    fb_organize_publish();
}
void fb_document_tick()
{
    static float assetsEpoch=-1;
    float currentAssets=cvar("ftebroom_entity_assets_epoch");
    if(assetsEpoch!=currentAssets){assetsEpoch=currentAssets;fb_entity_definitions_load();editor_ents_reload();fb_grid_geometry_epoch++;}
    int revision=fb_document_revision();
    cvar_set("ftebroom_revision",itos(revision));
    if(revision!=fb_saved_revision || cvar("ftebroom_terrain_dirty")) cvar_set("ftebroom_dirty","1");
    else cvar_set("ftebroom_dirty","0");
    if(revision!=fb_seen_revision) {
        static int structureRevision=-1;
        int structure=history_base_revision;
        for(int record=history-1;record>=history_min;record--) {
            history_t *entry=&historyring[record&(historyring_length-1)];
            if(entry->kind==HIST_SELECTION || entry->kind==HIST_VISIBILITY || entry->kind==HIST_PIVOT)continue;
            structure=entry->revision;break;
        }
        int reload=fb_seen_revision<0 || structureRevision!=structure;
        fb_seen_revision=revision;structureRevision=structure;
        // Selection history must not rebuild every brush surface and grid cache.
        if(reload)fb_document_rebuild();
    }
    // Selection is queried through brush_selection_index; no duplicate per-node flags.

}
void fb_repeat_clear();
void fb_document_loaded()
{
    fb_repeat_clear();
    fb_visibility_reset();
    fb_open_group=0;fb_restore_entity_selection(-1);
    fb_entity_definitions_load();editor_ents_reload();
    fb_pick_clear_handles();
    fb_saved_revision=fb_document_revision();fb_seen_revision=-1;
    fb_history_labels();
    fb_document_tick();
}
float fb_document_command()
{
    if(argv(0)=="ftebroom_document_saved") {cvar_set("ftebroom_terrain_dirty","0");fb_saved_revision=stoi(argv(1));fb_document_tick();}
    else if(argv(0)=="ftebroom_document_reload") fb_document_loaded();
    else if(argv(0)=="ftebroom_document_dump") {
        fb_document_rebuild();fb_document_tick();
        print(sprintf("FTEBROOM_DOCUMENT revision=%i saved=%i dirty=%g nodes=%i\n",fb_document_revision(),fb_saved_revision,cvar("ftebroom_dirty"),fb_node_count));
        for(int i=0;i<fb_node_count;i++) print(sprintf("FTEBROOM_NODE %i kind=%i model=%i id=%i parent=%i layer=%i name=%s\n",i,fb_nodes[i].kind,fb_nodes[i].model,fb_nodes[i].id,fb_nodes[i].parent,fb_nodes[i].layer,fb_nodes[i].name));
    } else return FALSE;
    return TRUE;
}
