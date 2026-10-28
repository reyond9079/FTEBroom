// SPDX-License-Identifier: GPL-2.0-or-later
// A retained base snapshot plus independently owned geometry for edited brushes.
// Native brush/entity data remains authoritative for saving and history.
typedef struct fb_incremental_brush_s {
    int node,gridCount,faceCount;
    struct fb_incremental_brush_s *next;
    fb_grid_face_t *grid;
    fb_render_face_t *faces;
    vector *gridPoints;
    vector *points;
    vector *uv;
    trisoup_simple_vert_t *gridVertices;
    trisoup_simple_vert_t *vertices;
    int *gridTriangles;
    int *triangles;
    int *lines;
} fb_incremental_brush_t;
fb_incremental_brush_t *fb_incremental_brushes;
int *fb_incremental_free_nodes;int fb_incremental_free_count;
fb_grid_face_t *fb_incremental_grid;int fb_incremental_grid_count,fb_incremental_grid_capacity;
fb_render_face_t *fb_incremental_faces;int fb_incremental_face_count,fb_incremental_face_capacity;
int fb_incremental_brush_count;
void fb_incremental_reset()
{
    while(fb_incremental_brushes){fb_incremental_brush_t *entry=fb_incremental_brushes;fb_incremental_brushes=entry->next;memfree(entry);}
    memfree(fb_incremental_free_nodes);fb_incremental_free_nodes=__NULL__;fb_incremental_free_count=0;
    memfree(fb_incremental_grid);fb_incremental_grid=__NULL__;fb_incremental_grid_count=fb_incremental_grid_capacity=0;
    memfree(fb_incremental_faces);fb_incremental_faces=__NULL__;fb_incremental_face_count=fb_incremental_face_capacity=0;
    fb_incremental_brush_count=0;fb_incremental_revision=-1;fb_incremental_epoch++;
}
void fb_incremental_settings()
{
    static string previous;
    string key=strcat(cvar_string("ftebroom_incremental_edits")," ",cvar_string("ftebroom_cached_meshes")," ",cvar_string("ftebroom_render_faces")," ",cvar_string("ftebroom_shade_faces"));
    key=strcat(key," ",cvar_string("ftebroom_material_tag_epoch")," ",cvar_string("ftebroom_native_faces"));
    if(previous!="" && key!=previous){if(fb_incremental_brush_count)fb_document_rebuild();else fb_grid_geometry_epoch++;}
    if(previous!=key){strunzone(previous);previous=strzone(key);}
}
// The single allocation keeps ownership simple and avoids per-face heap churn.
// Every packed field is word-aligned. A variant is vector-sized in QC;
// dividing byte offsets by its size truncates headers into adjacent arrays.
int *fb_incremental_at(int *memory,int bytes){return memory+bytes/sizeof(int);}
fb_incremental_brush_t *fb_incremental_build(int node)
{
    int model=fb_nodes[node].model,id=fb_nodes[node].id,count=brush_get(model,id,__NULL__,0,__NULL__),vertices=0;
    static vector temporary[256];
    for(int face=1;face<=count;face++)vertices+=brush_getfacepoints(model,id,face,temporary,256);
    int bytes=sizeof(fb_incremental_brush_t)+count*(sizeof(fb_grid_face_t)+sizeof(fb_render_face_t))+vertices*(sizeof(vector)*3+sizeof(trisoup_simple_vert_t)*3+sizeof(int)*8);
    int *memory=memalloc(bytes);fb_incremental_brush_t *entry=(fb_incremental_brush_t*)memory;entry->node=node;
    int at=sizeof(fb_incremental_brush_t);
    entry->grid=(fb_grid_face_t*)fb_incremental_at(memory,at);at+=count*sizeof(fb_grid_face_t);
    entry->faces=(fb_render_face_t*)fb_incremental_at(memory,at);at+=count*sizeof(fb_render_face_t);
    entry->gridPoints=(vector*)fb_incremental_at(memory,at);at+=vertices*sizeof(vector);
    entry->points=(vector*)fb_incremental_at(memory,at);at+=vertices*sizeof(vector);
    entry->uv=(vector*)fb_incremental_at(memory,at);at+=vertices*sizeof(vector);
    entry->gridVertices=(trisoup_simple_vert_t*)fb_incremental_at(memory,at);at+=vertices*2*sizeof(trisoup_simple_vert_t);
    entry->vertices=(trisoup_simple_vert_t*)fb_incremental_at(memory,at);at+=vertices*sizeof(trisoup_simple_vert_t);
    entry->gridTriangles=(int*)fb_incremental_at(memory,at);at+=vertices*3*sizeof(int);
    entry->triangles=(int*)fb_incremental_at(memory,at);at+=vertices*3*sizeof(int);
    entry->lines=(int*)fb_incremental_at(memory,at);
    fb_grid_face_t *oldGrid=fb_grid_faces;int oldGridCount=fb_grid_face_count,oldGridCapacity=fb_grid_face_capacity;
    vector *oldGridPoints=fb_grid_packed_points;trisoup_simple_vert_t *oldGridVertices=fb_grid_packed_vertices;
    int *oldGridTriangles=fb_grid_packed_triangles;int *oldLines=fb_grid_packed_lines;
    fb_grid_faces=entry->grid;fb_grid_face_count=0;fb_grid_face_capacity=count;
    fb_grid_packed_points=entry->gridPoints;fb_grid_packed_vertices=entry->gridVertices;fb_grid_packed_triangles=entry->gridTriangles;fb_grid_packed_lines=entry->lines;
    fb_grid_append_brush(node,0);entry->gridCount=fb_grid_face_count;
    fb_grid_faces=oldGrid;fb_grid_face_count=oldGridCount;fb_grid_face_capacity=oldGridCapacity;
    fb_grid_packed_points=oldGridPoints;fb_grid_packed_vertices=oldGridVertices;fb_grid_packed_triangles=oldGridTriangles;fb_grid_packed_lines=oldLines;
    fb_render_face_t *oldFaces=fb_render_faces;int oldFaceCount=fb_render_face_count,oldFaceCapacity=fb_render_face_capacity;
    vector *oldPoints=fb_render_points;vector *oldUV=fb_render_uv;
    trisoup_simple_vert_t *oldVertices=fb_render_vertices;int *oldTriangles=fb_render_triangles;
    fb_render_faces=entry->faces;fb_render_face_count=0;fb_render_face_capacity=count;
    fb_render_points=entry->points;fb_render_uv=entry->uv;fb_render_vertices=entry->vertices;fb_render_triangles=entry->triangles;
    fb_render_append_brush(node,0);entry->faceCount=fb_render_face_count;
    fb_render_faces=oldFaces;fb_render_face_count=oldFaceCount;fb_render_face_capacity=oldFaceCapacity;
    fb_render_points=oldPoints;fb_render_uv=oldUV;fb_render_vertices=oldVertices;fb_render_triangles=oldTriangles;
    return entry;
}
void fb_incremental_flatten()
{
    int grids=0,faces=0;
    for(fb_incremental_brush_t *entry=fb_incremental_brushes;entry;entry=entry->next){grids+=entry->gridCount;faces+=entry->faceCount;}
    if(grids>fb_incremental_grid_capacity){fb_incremental_grid_capacity=max(grids,max(512,fb_incremental_grid_capacity*2));memfree(fb_incremental_grid);fb_incremental_grid=memalloc(fb_incremental_grid_capacity*sizeof(fb_grid_face_t));}
    if(faces>fb_incremental_face_capacity){fb_incremental_face_capacity=max(faces,max(512,fb_incremental_face_capacity*2));memfree(fb_incremental_faces);fb_incremental_faces=memalloc(fb_incremental_face_capacity*sizeof(fb_render_face_t));}
    fb_incremental_grid_count=fb_incremental_face_count=0;
    for(fb_incremental_brush_t *entry=fb_incremental_brushes;entry;entry=entry->next){
        int start=fb_incremental_grid_count;memcpy(&fb_incremental_grid[start],entry->grid,entry->gridCount*sizeof(fb_grid_face_t));fb_incremental_grid_count+=entry->gridCount;
        for(int f=start;f<fb_incremental_grid_count;f++)fb_incremental_grid[f].brushEnd=fb_incremental_grid_count;
        start=fb_incremental_face_count;memcpy(&fb_incremental_faces[start],entry->faces,entry->faceCount*sizeof(fb_render_face_t));fb_incremental_face_count+=entry->faceCount;
        for(int f=start;f<fb_incremental_face_count;f++)fb_incremental_faces[f].brushEnd=fb_incremental_face_count;
    }
}
void fb_incremental_hide_base(int node)
{
    int first=fb_grid_first[node]-1;
    if(first>=0){int end=fb_grid_faces[first].brushEnd;for(int f=first;f<end;f++)fb_grid_faces[f].occluder|=8;}
    first=fb_render_first[node]-1;
    if(first>=0){int faceEnd=fb_render_faces[first].brushEnd;for(int f=first;f<faceEnd;f++)fb_render_faces[f].enabled=0;}
}
void fb_incremental_remove(int node)
{
    fb_incremental_brush_t *previous=__NULL__;
    for(fb_incremental_brush_t *entry=fb_incremental_brushes;entry;entry=entry->next){
        if(entry->node!=node){previous=entry;continue;}
        if(previous)previous->next=entry->next;else fb_incremental_brushes=entry->next;
        fb_incremental_brush_count--;memfree(entry);break;
    }
}
void fb_incremental_insert(fb_incremental_brush_t *entry)
{
    fb_incremental_brush_t *previous=__NULL__;fb_incremental_brush_t *next=fb_incremental_brushes;
    while(next && next->node<entry->node){previous=next;next=next->next;}
    entry->next=next;if(previous)previous->next=entry;else fb_incremental_brushes=entry;
    fb_incremental_brush_count++;
}
int fb_entity_selection_key()
{
    // Brush selection does not change point models, entity links or their colors.
    static int last=-1,key;static string previous;
    if(last==fb_selection_epoch)return key;last=fb_selection_epoch;
    strbuf pieces=buf_create();
    for(int i=0;i<fb_selected_entity_count;i++)bufstr_add(pieces,strcat(" e",itos(fb_selected_entities[i])),TRUE);
    for(int i=0;i<selectedbrushcount;i++)if(selectedbrushes[i].model!=1)bufstr_add(pieces,sprintf(" b%i:%i",selectedbrushes[i].model,selectedbrushes[i].id),TRUE);
    string selected=buf_implode(pieces,"");buf_del(pieces);
    if(previous!=selected){strunzone(previous);previous=strzone(selected);key++;}
    return key;
}
int(void *bounds,int count,int stride) ftebroom_incremental_bounds=#0:ftebroom_incremental_bounds;
void fb_incremental_materials(int first,int last,int redo)
{
    if(!fb_material_counts)return;
    for(int r=first;r<last;r++){
        history_t *h=&historyring[r&(historyring_length-1)];if(h->kind!=HIST_GEOMETRY)continue;
        int sign=h->wasdelete?-1:1;if(!redo)sign=-sign;
        for(int f=0;f<h->brushinfo.numfaces;f++){
            string name=fb_material_key(h->brushdata[f].shadername);
            fb_material_counts[name]=(int)fb_material_counts[name]+sign;
        }
    }
}
int fb_incremental_apply(int first,int last,int redo)
{
    if(first<0 || first<history_min || first>=last || last>history_max || !historyring_length)return FALSE;
    // Validate the entire transaction before changing any retained ownership.
    if(!fb_container_cache || !fb_grid_first || !fb_render_first || !fb_filter_tags || !fb_render_access || !fb_grid_brush_bounds || fb_grid_built_epoch!=fb_grid_geometry_epoch || fb_incremental_brush_count>2048)return FALSE;
    static int ids[4096],models[4096];static int *entities;static int entityCapacity;
    int records=last-first;if(records<=0 || records>1000000)return FALSE;
    if(entityCapacity<records){memfree(entities);entityCapacity=records;entities=memalloc(entityCapacity*sizeof(int));}
    int count=0,entityCount=0,valid=TRUE;
    hashtable seen=hash_createtab(max(64,(last-first)*2),EV_INTEGER);
    for(int r=first;r<last;r++){
        history_t *h=&historyring[r&(historyring_length-1)];
        if(h->kind==HIST_SELECTION || h->kind==HIST_VISIBILITY || h->kind==HIST_PIVOT)continue;
        if(h->kind==HIST_ENTITY || h->kind==HIST_TREE || h->kind==HIST_NODEFLAGS){
            string entityKey=strcat("e",itos(h->entityindex));
            if(!(int)seen[entityKey]){seen[entityKey]=1i;entities[entityCount++]=h->entityindex;}continue;
        }
        if(h->kind!=HIST_GEOMETRY || h->patchdata || !h->brushdata){valid=FALSE;break;}
        string brushKey=sprintf("b%i:%i",h->brushmodel,h->id);
        if(!(int)seen[brushKey]){if(count>=4096){valid=FALSE;break;}seen[brushKey]=1i;ids[count]=h->id;models[count++]=h->brushmodel;}
    }
    hash_destroytab(seen);if(!valid)return FALSE;
    int existingOnly=FALSE;
    if(records>4096){
        // Large point/group property transactions need no brush rebuild or new
        // node slots. Keep large creates/deletes and brush-owner changes on the
        // established fallback, validated before touching retained caches.
        if(count || !entityCount)return FALSE;
        hashtable existing=hash_createtab(max(64,fb_node_count),EV_INTEGER);
        for(int node=0;node<fb_node_count;node++)if(node!=1 && fb_nodes[node].kind>=0 && fb_nodes[node].kind<FB_NODE_BRUSH){
            if(substring(fb_nodes[node].fields["model"],0,1)=="*")continue;
            existing[itos(fb_nodes[node].id)]=1i;
        }
        existingOnly=TRUE;
        for(int e=0;e<entityCount;e++)if(!(int)existing[itos(entities[e])] || terrain_edit(TEREDIT_ENT_GET,entities[e])==""){existingOnly=FALSE;break;}
        hash_destroytab(existing);if(!existingOnly)return FALSE;
    }
    if((!count && !entityCount) || (!existingOnly && fb_node_count+count+entityCount>=fb_node_capacity) || fb_incremental_brush_count+count>2048)return FALSE;
    for(int i=0;i<count;i++){
        int faces=brush_get(models[i],ids[i],__NULL__,0,__NULL__);
        if(faces>64 || patch_getcp(models[i],ids[i],__NULL__,0,__NULL__)>0)return FALSE;
    }
    // Prime the index before adding/removing stable node slots.
    fb_document_brush_node(1,0);
    if(entityCount)if(!fb_document_update_entities(entities,entityCount))return FALSE;
    if(!fb_incremental_free_nodes)fb_incremental_free_nodes=memalloc(fb_node_capacity*sizeof(int));
    for(int i=0;i<count;i++){
        int id=ids[i],model=models[i],node=fb_document_brush_node(model,id),faces=brush_get(model,id,__NULL__,0,__NULL__);
        if(node>=2){
            fb_incremental_hide_base(node);fb_incremental_remove(node);fb_wire_cache_forget(node);
            fb_grid_first[node]=fb_render_first[node]=0;
            hash_delete(fb_document_brush_index,sprintf("%i:%i",model,id));
        }
        if(faces<=0){
            if(node>=2){strunzone(fb_nodes[node].name);fb_nodes[node].name="";fb_nodes[node].kind=-1;fb_nodes[node].flags=FB_NODE_HIDDEN;fb_nodes[node].model=0;fb_nodes[node].id=0;fb_render_access[node]=fb_static_access[node]=0;fb_filter_tags[node]=0;fb_container_cache[node].valid=0;fb_incremental_free_nodes[fb_incremental_free_count++]=node;}
            continue;
        }
        if(node<2){
            if(fb_incremental_free_count){node=fb_incremental_free_nodes[--fb_incremental_free_count];fb_nodes[node].kind=FB_NODE_BRUSH;fb_nodes[node].model=model;fb_nodes[node].id=id;fb_nodes[node].flags=0;fb_nodes[node].parent=fb_nodes[node].layer=1;fb_nodes[node].name=strzone("Brush");}
            else node=fb_document_add(FB_NODE_BRUSH,model,id,1,"Brush");
            fb_nodes[node].parent=fb_document_model_parent(model);fb_nodes[node].layer=fb_nodes[fb_nodes[node].parent].layer;
        }
        fb_document_brush_index[sprintf("%i:%i",model,id)]=node+1i;
        static brushface_t attributes[64];int contents;brush_get(model,id,attributes,64,&contents);int tags=0;
        for(int f=0;f<faces;f++)tags|=fb_filter_face_tags(&attributes[f],contents);
        for(int parent=fb_nodes[node].parent;parent>=2;parent=fb_nodes[parent].parent)if(fb_nodes[parent].kind==FB_NODE_ENTITY)
            if(substring(fb_nodes[parent].fields["classname"],0,8)=="trigger_")tags|=FB_TAG_TRIGGER;
        fb_filter_tag_count=fb_render_access_count=fb_node_count;fb_filter_tags[node]=tags;
        fb_render_access[node]=((tags&FB_TAG_TRIGGER)?0:4)|(fb_pick_allowed(node,FB_PICK_LOCKED)?1:0)|(fb_pick_allowed(node,0)?2:0)|((fb_filter_custom(node)||brush_isselected(model,id))?16:0);
        fb_incremental_insert(fb_incremental_build(node));fb_visibility_node(node);
        static vector bounds[2];fb_container_cache[node].valid=brush_getfacepoints(model,id,0,bounds,2)==2 && (fb_render_access[node]&1);
        fb_container_cache[node].mins=bounds[0];fb_container_cache[node].maxs=bounds[1];
    }
    if(entityCount){fb_visibility_epoch++;fb_document_structure_epoch++;}
    if(fb_wire_cache_epoch==fb_grid_geometry_epoch)fb_wire_cache_count=fb_node_count;
    int containersChanged=entityCount!=0;for(int i=0;i<count;i++)if(models[i]!=1)containersChanged=TRUE;
    if(containersChanged){fb_container_edit_epoch++;fb_container_cache_prepare();}
    ftebroom_incremental_bounds(fb_container_cache,fb_node_count,sizeof(fb_container_bounds_t));
    fb_incremental_materials(first,last,redo);fb_incremental_flatten();fb_incremental_epoch++;fb_occlusion_revision++;
    cvar_set("ftebroom_node_count",itos(fb_node_count));
    return TRUE;
}
int *fb_incremental_config()
{
    static int config[5];config[0]=(int)fb_incremental_grid;config[1]=fb_incremental_grid_count;config[2]=(int)fb_incremental_faces;config[3]=fb_incremental_face_count;config[4]=fb_grid_geometry_epoch;
    return fb_incremental_enabled()?config:__NULL__;
}
void fb_incremental_draw_grid()
{
    if(!fb_incremental_grid_count)return;
    static vector params[6];params[0]=autocvar_ftebroom_grid_origin;params[1]=autocvar_ftebroom_grid_basis_x;params[2]=autocvar_ftebroom_grid_basis_y;params[3]=autocvar_ftebroom_grid_basis_z;params[4]=[fb_grid(),bound(0,cvar("ftebroom_grid_alpha"),1),0];params[5]=[1,0,0];
    if(autocvar_ftebroom_grid_rotation){float radians=-autocvar_ftebroom_grid_rotation*M_PI/180;params[5]=[cos(radians),sin(radians),0];}
    ftebroom_build_grid(fb_incremental_grid,fb_incremental_grid_count,params);

}

void fb_incremental_access()
{
    if(!fb_incremental_enabled())return;
    static int selection=-1,geometry=-1,count;static int *nodes;
    if(selection==fb_selection_epoch && geometry==fb_grid_geometry_epoch)return;
    if(geometry!=fb_grid_geometry_epoch){memfree(nodes);nodes=memalloc(fb_node_capacity*sizeof(int));count=0;geometry=fb_grid_geometry_epoch;}
    for(int i=0;i<count;i++){
        int n=nodes[i];if(n>=fb_node_count||fb_nodes[n].kind!=FB_NODE_BRUSH)continue;
        if(fb_filter_custom(n)||brush_isselected(fb_nodes[n].model,fb_nodes[n].id))fb_render_access[n]|=16;
        else fb_render_access[n]&=~16i;
    }
    count=0;
    for(int i=0;i<selectedbrushcount;i++){
        int n=fb_document_brush_node(selectedbrushes[i].model,selectedbrushes[i].id);if(n<2)continue;
        fb_render_access[n]|=16;nodes[count++]=n;
    }
    selection=fb_selection_epoch;fb_occlusion_revision++;
}

int(void *bounds,int count,int stride,vector *ray,int *nodes,int capacity) ftebroom_pick_candidates=#0:ftebroom_pick_candidates;
int fb_incremental_pick(vector start,vector end,int *nodes,int capacity)
{
    if(!fb_incremental_enabled() || !checkbuiltin(ftebroom_pick_candidates))return -1;
    fb_container_cache_prepare();static vector ray[2];ray[0]=start;ray[1]=end;
    return ftebroom_pick_candidates(fb_container_cache,fb_node_count,sizeof(fb_container_bounds_t),ray,nodes,capacity);
}
