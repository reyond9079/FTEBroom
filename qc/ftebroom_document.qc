int fb_delete_document(int first,int last,int redo);
int fb_batch_document(int first,int last,int redo);
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
int fb_grid_geometry_epoch,fb_document_structure_epoch;
int fb_incremental_epoch;var int fb_incremental_revision=-1;
int fb_incremental_apply(int first,int last,int redo);
int *fb_incremental_config();
void fb_incremental_access();
int fb_entity_selection_key();
void fb_incremental_reset();
void fb_incremental_settings();
void fb_incremental_draw_grid();
var float autocvar_ftebroom_incremental_edits=1;
int() ftebroom_incremental_support=#0:ftebroom_incremental_support;
int fb_incremental_enabled(){return autocvar_ftebroom_incremental_edits && cvar("ftebroom_cached_meshes") && cvar("ftebroom_native_faces") && cvar("ftebroom_render_faces")!=2 && fb_node_count>=10000 && checkbuiltin(ftebroom_incremental_support);}
hashtable fb_document_brush_index;var int fb_document_brush_index_epoch=-1;
int fb_document_brush_node(int model,int id)
{
    if(fb_document_brush_index_epoch!=fb_grid_geometry_epoch){
        fb_document_brush_index_epoch=fb_grid_geometry_epoch;
        if(fb_document_brush_index)hash_destroytab(fb_document_brush_index);
        fb_document_brush_index=hash_createtab(max(64,fb_node_capacity*2),EV_INTEGER);
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind>=FB_NODE_BRUSH)fb_document_brush_index[sprintf("%i:%i",fb_nodes[n].model,fb_nodes[n].id)]=n+1i;
    }
    return (int)fb_document_brush_index[sprintf("%i:%i",model,id)]-1i;
}
int fb_document_revision();
void fb_commit_profile(string stage,float started)
{
    if(cvar("ftebroom_profile_commit") && gettime(1)-started>.005)
        print(sprintf("RB_COMMIT81 %s ms=%g revision=%i\n",stage,(gettime(1)-started)*1000,fb_document_revision()));
}
int *fb_point_nodes;
vector *fb_point_group_bounds;int *fb_point_group_flags;
int *fb_container_nodes;
int *fb_patch_nodes;
int fb_point_node_count,fb_container_node_count,fb_patch_node_count;
void fb_document_lists_prepare()
{
    static int geometry=-1,count=-1,structure=-1;
    if(geometry==fb_grid_geometry_epoch && structure==fb_document_structure_epoch)return;
    geometry=fb_grid_geometry_epoch;structure=fb_document_structure_epoch;count=fb_node_count;
    memfree(fb_point_nodes);memfree(fb_container_nodes);memfree(fb_patch_nodes);
    fb_point_nodes=memalloc(max(1,count)*sizeof(int));fb_container_nodes=memalloc(max(1,count)*sizeof(int));fb_patch_nodes=memalloc(max(1,count)*sizeof(int));
    fb_point_node_count=fb_container_node_count=fb_patch_node_count=0;
    memfree(fb_point_group_bounds);memfree(fb_point_group_flags);
    int groups=(count+63)/64;
    fb_point_group_bounds=memalloc(max(1,groups)*2*sizeof(vector));fb_point_group_flags=memalloc(max(1,groups)*sizeof(int));
    static vector pointBounds[2];
    for(int n=2;n<count;n++){
        int kind=fb_nodes[n].kind;
        if(kind==FB_NODE_ENTITY){if(substring(fb_nodes[n].fields["model"],0,1)!="*"){
            int group=fb_point_node_count/64;fb_point_nodes[fb_point_node_count++]=n;
            if(fb_entity_bounds(fb_nodes[n].id,pointBounds)){
                if(!(fb_point_group_flags[group]&1)){fb_point_group_bounds[group*2]=pointBounds[0];fb_point_group_bounds[group*2+1]=pointBounds[1];fb_point_group_flags[group]|=1;}
                else for(int a=0;a<3;a++){fb_point_group_bounds[group*2][a]=min(fb_point_group_bounds[group*2][a],pointBounds[0][a]);fb_point_group_bounds[group*2+1][a]=max(fb_point_group_bounds[group*2+1][a],pointBounds[1][a]);}
            }
            if(fb_entity_has_model(fb_nodes[n].id))fb_point_group_flags[group]|=2;
        }}
        if(kind==FB_NODE_GROUP || (kind==FB_NODE_ENTITY && substring(fb_nodes[n].fields["model"],0,1)=="*"))fb_container_nodes[fb_container_node_count++]=n;
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
    int found=fb_document_brush_node(model,oldid);
    if(found>=2)fb_nodes[found].id=newid;
    if(fb_document_brush_index_epoch==fb_grid_geometry_epoch){
        int node=(int)fb_document_brush_index[sprintf("%i:%i",model,oldid)];
        hash_delete(fb_document_brush_index,sprintf("%i:%i",model,oldid));
        if(node)fb_document_brush_index[sprintf("%i:%i",model,newid)]=node;
    }
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
    float profile81Time=gettime(1);
    fb_incremental_reset();
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
    fb_commit_profile("fb_document_rebuild",profile81Time);
}
// World-brush transactions do not change entity text or hierarchy. Keep their
// parsed fields and rebuild only brush membership; complex transactions retain
// the full loader. Node-indexed consumers are invalidated by the geometry epoch.
void fb_document_rebuild_world_geometry()
{
    float profile81Time=gettime(1);
    fb_incremental_reset();
    int oldCount=fb_node_count;fb_node_t *old=fb_nodes;
    int *remap=memalloc(max(1,oldCount)*sizeof(int));
    fb_nodes=memalloc(max(64,fb_node_capacity)*sizeof(fb_node_t));fb_node_count=0;
    int worldAdded=FALSE;
    for(int n=0;n<oldCount;n++){
        if(n==2 && !worldAdded){fb_document_brushes(1,1);worldAdded=TRUE;}
        if(old[n].kind<0 || (old[n].kind>=FB_NODE_BRUSH && old[n].model==1)){
            if(!worldAdded){fb_document_brushes(1,1);worldAdded=TRUE;}
            strunzone(old[n].name);strunzone(old[n].tbid);strunzone(old[n].parentref);strunzone(old[n].layerref);
            if(old[n].fields)hash_destroytab(old[n].fields);
        }else{int next=fb_document_add(old[n].kind,old[n].model,old[n].id,old[n].parent,old[n].name);strunzone(fb_nodes[next].name);remap[n]=next;fb_nodes[next]=old[n];}
    }
    if(!worldAdded)fb_document_brushes(1,1);
    for(int n=0;n<fb_node_count;n++)if(fb_nodes[n].parent>=0)fb_nodes[n].parent=remap[fb_nodes[n].parent];
    memfree(old);memfree(remap);fb_grid_geometry_epoch++;
    fb_document_resolve();
    cvar_set("ftebroom_node_count",itos(fb_node_count));fb_organize_publish();
    fb_commit_profile("fb_document_rebuild_world_geometry",profile81Time);
}
// Refresh only entity records touched by a transaction; brush slots and their
// retained GPU surfaces keep their identities across group duplication and moves.
int fb_document_update_entities(int *entities,int count)
{
    hashtable nodes=hash_createtab(max(64,fb_node_count),EV_INTEGER);
    for(int n=0;n<fb_node_count;n++)if(n!=1 && fb_nodes[n].kind<FB_NODE_BRUSH && (n==0 || (fb_nodes[n].model==0 && fb_nodes[n].id>0)))nodes[itos(fb_nodes[n].id)]=n+1i;
    // Changing an existing owner type/model can alter untouched child geometry.
    // Keep those uncommon operations on the full document loader.
    for(int e=0;e<count;e++){
        int node=(int)nodes[itos(entities[e])]-1i;
        if(node<0 || fb_nodes[node].kind<0)continue;
        string data=terrain_edit(TEREDIT_ENT_GET,entities[e]);if(data=="")continue;
        string classname="",model="",type="";int tokens=fb_entity_tokenize(data);
        for(int k=0;k<tokens;k+=2){if(argv(k)=="classname")classname=argv(k+1);if(argv(k)=="model")model=argv(k+1);if(argv(k)=="_tb_type")type=argv(k+1);}
        if(classname!=fb_nodes[node].fields["classname"] || model!=fb_nodes[node].fields["model"] || type!=fb_nodes[node].fields["_tb_type"]){hash_destroytab(nodes);return FALSE;}
    }
    for(int e=0;e<count;e++){
        int id=entities[e],node=(int)nodes[itos(id)]-1i;
        string data=terrain_edit(TEREDIT_ENT_GET,id);
        if(data==""){
            if(node>=2){fb_nodes[node].kind=-1;fb_nodes[node].flags=FB_NODE_HIDDEN;}
            continue;
        }
        hashtable fields=hash_createtab(32,EV_STRING);int tokens=fb_entity_tokenize(data);
        for(int k=0;k<tokens;k+=2)fields[argv(k)]=argv(k+1);
        int kind=FB_NODE_ENTITY;string classname=fields["classname"];
        if(classname=="func_group"){
            if(fields["_tb_type"]=="_tb_layer")kind=FB_NODE_LAYER;
            else if(fields["_tb_type"]=="_tb_group")kind=FB_NODE_GROUP;
        }
        string name=fields["_tb_name"];if(name=="")name=classname;
        if(node<0){node=fb_document_add(kind,0,id,1,name);nodes[itos(id)]=node+1i;}
        else {strunzone(fb_nodes[node].name);fb_nodes[node].name=strzone(name);}
        if(fb_nodes[node].fields)hash_destroytab(fb_nodes[node].fields);
        strunzone(fb_nodes[node].tbid);strunzone(fb_nodes[node].parentref);strunzone(fb_nodes[node].layerref);
        fb_nodes[node].kind=id?kind:FB_NODE_WORLD;fb_nodes[node].fields=fields;
        fb_nodes[node].tbid=strzone(fields["_tb_id"]);fb_nodes[node].parentref=strzone(fields["_tb_group"]);fb_nodes[node].layerref=strzone(fields["_tb_layer"]);
        if(id)fb_nodes[node].parent=1;
        int flagNode=id?node:1;fb_nodes[flagNode].flags=0;
        if(fields["_tb_layer_hidden"]=="1")fb_nodes[flagNode].flags|=FB_NODE_HIDDEN;
        if(fields["_tb_layer_locked"]=="1")fb_nodes[flagNode].flags|=FB_NODE_LOCKED;
    }
    hash_destroytab(nodes);fb_document_resolve();return TRUE;
}
int fb_document_model_parent(int model)
{
    if(model==1)return 1;
    static hashtable parents;static int geometry=-1,structure=-1,entities=-1;
    if(geometry!=fb_grid_geometry_epoch || structure!=fb_document_structure_epoch || entities!=fb_entity_render_epoch){
        geometry=fb_grid_geometry_epoch;structure=fb_document_structure_epoch;entities=fb_entity_render_epoch;
        if(parents)hash_destroytab(parents);parents=hash_createtab(max(64,fb_node_count),EV_INTEGER);
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind>=0 && fb_nodes[n].kind<FB_NODE_BRUSH){
            string name=fb_nodes[n].fields["model"];if(substring(name,0,1)!="*")continue;
            setmodel(fb_document_probe,name);parents[itos((int)fb_document_probe.modelindex)]=n;
        }
    }
    int parent=(int)parents[itos(model)];return parent>=2?parent:1;
}
var float fb_document_assets_epoch=-1;
void fb_document_tick()
{
    float currentAssets=cvar("ftebroom_entity_assets_epoch");
    if(fb_document_assets_epoch!=currentAssets){fb_document_assets_epoch=currentAssets;fb_entity_definitions_load();editor_ents_reload();fb_grid_geometry_epoch++;}
    int revision=fb_document_revision();
    cvar_set("ftebroom_revision",itos(revision));
    if(revision!=fb_saved_revision || cvar("ftebroom_terrain_dirty")) cvar_set("ftebroom_dirty","1");
    else cvar_set("ftebroom_dirty","0");
    if(revision!=fb_seen_revision) {
        static int structureRevision=-1,previousHistory=-1;
        int structure=history_base_revision;
        for(int record=history-1;record>=history_min;record--) {
            history_t *entry=&historyring[record&(historyring_length-1)];
            if(entry->kind==HIST_SELECTION || entry->kind==HIST_VISIBILITY || entry->kind==HIST_PIVOT)continue;
            structure=entry->revision;break;
        }
        int reload=fb_seen_revision<0 || structureRevision!=structure;
        int first=min(previousHistory,history),last=max(previousHistory,history);
        int incremental=fb_seen_revision>=0 && fb_node_count>=10000 && first>=history_min && first<last && last<=history_max;
        int worldOnly=incremental;
        if(incremental)for(int record=first;record<last;record++){
            history_t *changed=&historyring[record&(historyring_length-1)];
            if(changed->kind==HIST_SELECTION || changed->kind==HIST_VISIBILITY || changed->kind==HIST_PIVOT)continue;
            if(changed->kind!=HIST_GEOMETRY || changed->brushmodel!=1)worldOnly=FALSE;
            if(changed->kind!=HIST_GEOMETRY && changed->kind!=HIST_ENTITY && changed->kind!=HIST_TREE && changed->kind!=HIST_NODEFLAGS){incremental=FALSE;break;}
        }
        int redo=history>previousHistory;previousHistory=history;
        fb_seen_revision=revision;structureRevision=structure;
        // Selection history must not rebuild every brush surface and grid cache.
        if(reload){
            // QC boolean operators can evaluate both operands. Never invoke a
            // mutating fallback until its range and ownership are eligible.
            int updated=fb_batch_document(first,last,redo);
            if(!updated)updated=fb_delete_document(first,last,redo);
            if(!updated)if(incremental)if(fb_incremental_enabled())updated=fb_incremental_apply(first,last,redo);
            if(updated)fb_incremental_revision=revision;
            else if(worldOnly)fb_document_rebuild_world_geometry();else fb_document_rebuild();
        }
    }
    // Selection is queried through brush_selection_index; no duplicate per-node flags.

}
void fb_repeat_clear();
void fb_document_loaded()
{
    fb_repeat_clear();
    fb_visibility_reset();
    fb_open_group=0;fb_restore_entity_selection(-1);
    fb_entity_definitions_load();editor_ents_reload();fb_document_assets_epoch=cvar("ftebroom_entity_assets_epoch");
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
