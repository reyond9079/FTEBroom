// SPDX-License-Identifier: GPL-2.0-or-later
int fb_filter_custom(int node);
void fb_container_draw(int labels);
void fb_entity_links_draw();
void fb_entity_drop_preview();
// Rendering is read-only: no selection transactions or document mutations here.
var vector autocvar_ftebroom_color_selected = '1 0 0';
var vector autocvar_ftebroom_color_locked = '0.15 0.4 1';
var vector autocvar_ftebroom_color_hover = '1 0.8 0.25';
var float autocvar_ftebroom_hover_highlight = 0;
var float autocvar_ftebroom_selection_dimensions = 1;
var float autocvar_ftebroom_selection_spikes = 1;
int fb_visual_label_count,fb_visual_spike_count;
int fb_visual_hover,fb_visual_has_bounds,fb_visual_spikes;
vector fb_visual_min,fb_visual_max;
string fb_visual_pick_key;
float fb_visual_pick_time;
int fb_visual_cached_hover;
int fb_visual_locked(int node)
{
    if(!fb_render_allowed(node,FB_PICK_LOCKED)) return FALSE;
    return !fb_render_allowed(node,0);
}
void fb_visual_expand(vector point)
{
    if(!fb_visual_has_bounds) {fb_visual_min=fb_visual_max=point;fb_visual_has_bounds=TRUE;return;}
    for(int a=0;a<3;a++) {fb_visual_min[a]=min(fb_visual_min[a],point[a]);fb_visual_max[a]=max(fb_visual_max[a],point[a]);}
}
void fb_visual_prepare()
{
    fb_visual_hover=-1;fb_visual_has_bounds=FALSE;fb_visual_spikes=FALSE;
    static vector bounds[2],points[256];
    // Static selection bounds do not depend on camera motion. During a tool
    // preview the original replacement-aware path remains authoritative.
    static int selection=-1,geometry=-1,edits=-1,entities=-1,valid;
    static vector low,high;
    int cacheable=!fb_active && !fb_transform_preview && !fb_vertex_preview_valid && !fb_generator_count && !fb_clip_count;
    if(cacheable && selection==fb_selection_epoch && geometry==fb_grid_geometry_epoch && edits==fb_incremental_epoch && entities==fb_entity_render_epoch){
        fb_visual_has_bounds=valid;fb_visual_min=low;fb_visual_max=high;
    }else{
    for(int b=0;b<selectedbrushcount;b++) {
        selbrush_t item=selectedbrushes[b];
        if(fb_preview_replaces(item.model,item.id)) continue;
        if(selbrush_whole(&item)) {
            if(brush_getfacepoints(item.model,item.id,0,bounds,2)==2) {fb_visual_expand(bounds[0]);fb_visual_expand(bounds[1]);}
        } else for(int f=0;f<64;f++) if(selbrush_face(&item,f)) {
            int count=brush_getfacepoints(item.model,item.id,f+1,points,256);
            for(int p=0;p<count;p++) fb_visual_expand(points[p]);
        }
    }
    for(int e=0;e<fb_selected_entity_count;e++) {
        if(fb_transform_replaces(0,fb_selected_entities[e]))continue;
        if(fb_entity_bounds(fb_selected_entities[e],bounds)){fb_visual_expand(bounds[0]);fb_visual_expand(bounds[1]);}
    }
        if(cacheable){selection=fb_selection_epoch;geometry=fb_grid_geometry_epoch;edits=fb_incremental_epoch;entities=fb_entity_render_epoch;valid=fb_visual_has_bounds;low=fb_visual_min;high=fb_visual_max;}
        else selection=-1;
    }
    if(fb_active && fb_dragged && fb_owner==FB_OWNER_SHAPE) {
        fb_visual_has_bounds=FALSE;
        fb_visual_expand(fb_boxcenter-fb_boxsize*0.5);fb_visual_expand(fb_boxcenter+fb_boxsize*0.5);
    }
    if(fb_transform_preview && !fb_transform_duplicate) {
        if(!fb_affine_preview_active && !fb_extrude_active && fb_transform_bounds_valid){
            fb_visual_expand(fb_transform_bounds_min+fb_transform_delta);fb_visual_expand(fb_transform_bounds_max+fb_transform_delta);
        }else for(int t=0;t<fb_transform_count;t++) {
            if(fb_affine_preview_active) {
                fb_transform_item_t *item=&fb_transform_items[t];
                if(item->kind==FB_NODE_BRUSH) {
                    fb_poly_t poly={0};fb_poly_from_faces(item->faces,item->count,&poly);
                    for(int p=0;p<poly.nv;p++) fb_visual_expand(fb_affine_point(&fb_affine_preview_matrix,poly.vertices[p]));
                    fb_poly_free(&poly);
                } else if(item->kind==FB_NODE_PATCH) {
                    for(int p=0;p<item->count;p++) fb_visual_expand(fb_affine_point(&fb_affine_preview_matrix,item->points[p].xyz));
                } else for(int corner=0;corner<8;corner++) {
                    vector point='0 0 0';for(int a=0;a<3;a++) point[a]=(corner&(1i<<a))?item->high[a]:item->low[a];
                    fb_visual_expand(fb_affine_point(&fb_affine_preview_matrix,point));
                }
            } else if(fb_extrude_active && fb_transform_items[t].kind==FB_NODE_BRUSH) {
                fb_transform_item_t *resizeItem=&fb_transform_items[t];
                brushface_t *resizeFaces=memalloc(sizeof(brushface_t)*resizeItem->count);
                memcpy(resizeFaces,resizeItem->faces,sizeof(brushface_t)*resizeItem->count);
                if(!fb_extrude_mode || (fb_extrude_mode==1 && fb_extrude_distance<0)) fb_extrude_preview_faces(t,resizeFaces);
                fb_poly_t resized={0};fb_poly_from_faces(resizeFaces,resizeItem->count,&resized);
                for(int vertex=0;vertex<resized.nv;vertex++) fb_visual_expand(resized.vertices[vertex]);
                fb_poly_free(&resized);memfree(resizeFaces);
                if(fb_extrude_mode && fb_extrude_faces[t]>=0) {
                    static brushface_t extra[MAX_BRUSHFACES];
                    int extraCount=fb_extrude_part(t,fb_extrude_distance,extra);
                    if(extraCount>=4) {
                        fb_poly_from_faces(extra,extraCount,&resized);
                        for(int vertex=0;vertex<resized.nv;vertex++) fb_visual_expand(resized.vertices[vertex]);
                        fb_poly_free(&resized);
                    }
                }
            } else {
                fb_visual_expand(fb_transform_items[t].low+fb_transform_delta);
                fb_visual_expand(fb_transform_items[t].high+fb_transform_delta);
            }
        }
    }
    if(fb_active || cvar("ftebroom_camera_captured")) return;
    vector mn=getproperty(VF_MIN),sz=getproperty(VF_SIZE),pixel=curmousepos;
    if(pixel_x<mn_x || pixel_y<mn_y || pixel_x>=mn_x+sz_x || pixel_y>=mn_y+sz_y) return;
    if(!autocvar_ftebroom_hover_highlight && (!autocvar_ftebroom_selection_spikes || !fb_visual_has_bounds)) return;
    // Avoid retracing every brush while the pointer and camera remain stationary.
    string key=sprintf("%v %v %v %v %v",pixel,mn,sz,getproperty(VF_ORIGIN),getproperty(VF_ANGLES));
    key=strcat(key,sprintf(" %g %g %i %i %i %g",cvar("ca_brush_view"),cvar("ca_brush_viewsize"),fb_document_revision(),history,selectedbrushcount,fb_camera_zoom));
    if(key==fb_visual_pick_key && gettime(0)-fb_visual_pick_time<0.1) {
        fb_visual_hover=fb_visual_cached_hover;
        if(fb_visual_hover>=0) fb_visual_spikes=fb_selection_selected(fb_visual_hover);
        return;
    }
    strunzone(fb_visual_pick_key);fb_visual_pick_key=strzone(key);fb_visual_pick_time=gettime(0);
    if(fb_pick(pixel)>0 && fb_hits[0].kind!=FB_HIT_HANDLE) {
        fb_visual_hover=fb_selection_target(fb_hits[0].node);
        if(fb_selection_selected(fb_visual_hover)) fb_visual_spikes=TRUE;
    }
    fb_visual_cached_hover=fb_visual_hover;
}
void fb_visual_line(vector a,vector b,vector color,float alpha,float endAlpha,int depth)
{
    string shader="ftebroom/selectionoverlay";if(depth) shader="ftebroom/selectiondepth";
    R_BeginPolygon(shader);R_PolygonVertex(a,'0 0',color,alpha);R_PolygonVertex(b,'0 0',color,endAlpha);R_EndPolygon();
}
typedef struct {int first,count;vector color;float fill;vector center;float radius;} fb_visual_surface_t;
vector fb_visual_polygon_center;float fb_visual_polygon_radius;
fb_visual_surface_t *fb_visual_surfaces;int fb_visual_surface_count,fb_visual_surface_capacity;
vector *fb_visual_vertices;int fb_visual_vertex_count,fb_visual_vertex_capacity;
void fb_visual_polygon(vector *points,int count,vector color,float fill)
{
    if(count<3)return;
    if(fb_visual_surface_count==fb_visual_surface_capacity) {
        int capacity=max(256,fb_visual_surface_capacity*2);
        fb_visual_surface_t *items=memalloc(capacity*sizeof(fb_visual_surface_t));
        memcpy(items,fb_visual_surfaces,fb_visual_surface_count*sizeof(fb_visual_surface_t));
        memfree(fb_visual_surfaces);fb_visual_surfaces=items;fb_visual_surface_capacity=capacity;
    }
    if(fb_visual_vertex_count+count>fb_visual_vertex_capacity) {
        int vertexCapacity=max(fb_visual_vertex_count+count,max(1024,fb_visual_vertex_capacity*2));
        vector *vertices=memalloc(vertexCapacity*sizeof(vector));
        memcpy(vertices,fb_visual_vertices,fb_visual_vertex_count*sizeof(vector));
        memfree(fb_visual_vertices);fb_visual_vertices=vertices;fb_visual_vertex_capacity=vertexCapacity;
    }
    fb_visual_surface_t *surface=&fb_visual_surfaces[fb_visual_surface_count++];
    surface->center=fb_visual_polygon_center;surface->radius=fb_visual_polygon_radius;
    surface->first=fb_visual_vertex_count;surface->count=count;surface->color=color;surface->fill=fill;
    memcpy(&fb_visual_vertices[fb_visual_vertex_count],points,count*sizeof(vector));fb_visual_vertex_count+=count;
}
void fb_visual_flush()
{
    int ortho=cvar("ca_brush_view");
    // Group fills and the two depth modes, avoiding a draw-call flush per edge.
    for(int pass=0;pass<3;pass++) {
        if(ortho && pass!=1)continue;
        R_BeginPolygon(pass==0?"ftebroom/selectionfill":(pass==1?"ftebroom/selectionoverlay":"ftebroom/selectiondepth"),3);
        for(int f=0;f<fb_visual_surface_count;f++) {
            fb_visual_surface_t surface=fb_visual_surfaces[f];
            if(!pass) {
                for(int p=0;p<surface.count;p++)R_PolygonVertex(fb_visual_vertices[surface.first+p],'0 0',surface.color,surface.fill);
                R_EndPolygon();
            } else for(int p=0;p<surface.count;p++) {
                int next=p+1;if(next==surface.count)next=0;
                float alpha=1;if(!ortho && pass==1)alpha=0.25;
                R_PolygonVertex(fb_visual_vertices[surface.first+p],'0 0',surface.color,alpha);
                R_PolygonVertex(fb_visual_vertices[surface.first+next],'0 0',surface.color,alpha);R_EndPolygon();
            }
        }
    }
}
// Retain immutable selection meshes across camera frames. Each chunk stays
// below the engine's 16-bit polygon index limit, including expanded line pairs.
typedef struct {vector center;float radius;int triangleFirst,triangleCount,lineFirst,lineCount;} fb_visual_range_t;
typedef struct fb_visual_chunk_s {
    trisoup_simple_vert_t *fill;
    trisoup_simple_vert_t *faint;
    trisoup_simple_vert_t *solid;
    int *triangles;
    int *lines;
    int vertices,triangleCount,lineCount;
    struct fb_visual_chunk_s *next;
    fb_visual_range_t *ranges;int rangeCount;
} fb_visual_chunk_t;
fb_visual_chunk_t *fb_visual_chunks;
int fb_visual_test_reference;
void fb_visual_cache_clear()
{
    while(fb_visual_chunks){
        fb_visual_chunk_t *chunk=fb_visual_chunks;fb_visual_chunks=chunk->next;
        memfree(chunk->fill);memfree(chunk->faint);memfree(chunk->solid);
        memfree(chunk->triangles);memfree(chunk->lines);memfree(chunk->ranges);memfree(chunk);
    }
}
void fb_visual_cache_build(int ortho)
{
    fb_visual_cache_clear();fb_visual_chunk_t *tail=__NULL__;
    for(int f=0;f<fb_visual_surface_count;f++){
        fb_visual_surface_t surface=fb_visual_surfaces[f];
        int newChunk=!tail;if(tail)if(tail->vertices+surface.count>4096)newChunk=TRUE;
        if(newChunk){
            fb_visual_chunk_t *chunk=memalloc(sizeof(fb_visual_chunk_t));fb_visual_chunk_t empty={0};*chunk=empty;
            chunk->fill=memalloc(4096*sizeof(trisoup_simple_vert_t));
            chunk->faint=memalloc(4096*sizeof(trisoup_simple_vert_t));
            chunk->solid=memalloc(4096*sizeof(trisoup_simple_vert_t));
            chunk->triangles=memalloc(12288*sizeof(int));chunk->lines=memalloc(8192*sizeof(int));chunk->ranges=memalloc(1366*sizeof(fb_visual_range_t));
            if(tail)tail->next=chunk;else fb_visual_chunks=chunk;tail=chunk;
        }
        fb_visual_range_t *range=&tail->ranges[tail->rangeCount++];range->center=surface.center;range->radius=surface.radius;range->triangleFirst=tail->triangleCount;range->lineFirst=tail->lineCount;
        int first=tail->vertices;
        for(int p=0;p<surface.count;p++){
            int v=first+p;
            for(int a=0;a<3;a++){tail->fill[v].xyz[a]=fb_visual_vertices[surface.first+p][a];tail->fill[v].rgba[a]=surface.color[a];}
            tail->fill[v].st[0]=tail->fill[v].st[1]=0;tail->fill[v].rgba[3]=surface.fill;
            tail->solid[v]=tail->faint[v]=tail->fill[v];tail->solid[v].rgba[3]=1;tail->faint[v].rgba[3]=ortho?1:.25;
            int next=p+1;if(next==surface.count)next=0;
            tail->lines[tail->lineCount++]=v;tail->lines[tail->lineCount++]=first+next;
            if(p>=2){tail->triangles[tail->triangleCount++]=first;tail->triangles[tail->triangleCount++]=v-1;tail->triangles[tail->triangleCount++]=v;}
        }
        range->triangleCount=tail->triangleCount-range->triangleFirst;range->lineCount=tail->lineCount-range->lineFirst;
        tail->vertices+=surface.count;
    }
}
void(string shader,int flags,trisoup_simple_vert_t *vertices,int vertexCount,int *indices,int indexCount,fb_visual_range_t *ranges,int rangeCount) ftebroom_indexed_overlay=#0:ftebroom_indexed_overlay;
void fb_visual_cache_draw(int ortho)
{
    for(int pass=0;pass<3;pass++){
        if(ortho && pass!=1)continue;
        for(fb_visual_chunk_t *chunk=fb_visual_chunks;chunk;chunk=chunk->next){
            if(checkbuiltin(ftebroom_indexed_overlay)){
                if(!pass)ftebroom_indexed_overlay("ftebroom/selectionfill",3,chunk->fill,chunk->vertices,chunk->triangles,chunk->triangleCount,chunk->ranges,chunk->rangeCount);
                else ftebroom_indexed_overlay(pass==1?"ftebroom/selectionoverlay":"ftebroom/selectiondepth",3|DRAWFLAG_LINES,pass==1?chunk->faint:chunk->solid,chunk->vertices,chunk->lines,chunk->lineCount,chunk->ranges,chunk->rangeCount);
            }else{
            if(!pass)addtrisoup_simple("ftebroom/selectionfill",3,chunk->fill,chunk->triangles,chunk->triangleCount);
            else addtrisoup_simple(pass==1?"ftebroom/selectionoverlay":"ftebroom/selectiondepth",3|DRAWFLAG_LINES,pass==1?chunk->faint:chunk->solid,chunk->lines,chunk->lineCount);
            }
        }
    }
}
// Trace editor brush planes, independent of game solidity and compiled BSP state.
int fb_visual_spike_hit;
int *fb_spike_models;int fb_spike_model_count;
int *fb_spike_ids;int *fb_spike_candidates;int fb_spike_id_capacity,fb_spike_candidate_capacity,fb_spike_candidate_count;
void fb_spike_candidates_prepare(vector start,vector end)
{
    // Reuse live document bounds instead of rebuilding FTE's whole trace BVH
    // after one edit. The exact convex-plane intersection below is unchanged.
    if(fb_incremental_enabled()){
        if(fb_spike_candidate_capacity<fb_node_count){memfree(fb_spike_candidates);fb_spike_candidate_capacity=fb_node_capacity;fb_spike_candidates=memalloc(fb_spike_candidate_capacity*sizeof(int));}
        int found=fb_incremental_pick(start,end,fb_spike_candidates,fb_spike_candidate_capacity);
        if(found>=0){fb_spike_candidate_count=found;return;}
    }

    static int geometry=-1;
    if(geometry!=fb_grid_geometry_epoch){
        geometry=fb_grid_geometry_epoch;
        memfree(fb_spike_models);fb_spike_models=memalloc(max(1,fb_node_count)*sizeof(int));fb_spike_model_count=0;
        hashtable models=hash_createtab(64,EV_INTEGER);
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH){
            int model=fb_nodes[n].model;
            if(!(int)models[itos(model)]){models[itos(model)]=1i;fb_spike_models[fb_spike_model_count++]=model;}
        }
        hash_destroytab(models);
    }
    static vector planes[6];static float distances[6];
    for(int axis=0;axis<3;axis++){
        planes[axis*2]='0 0 0';planes[axis*2+1]='0 0 0';planes[axis*2][axis]=1;planes[axis*2+1][axis]=-1;
        distances[axis*2]=max(start[axis],end[axis])+.01;distances[axis*2+1]=-min(start[axis],end[axis])+.01;
    }
    fb_spike_candidate_count=0;
    for(int m=0;m<fb_spike_model_count;m++){
        int total;
        do{
            if(!fb_spike_id_capacity){fb_spike_id_capacity=64;fb_spike_ids=memalloc(fb_spike_id_capacity*sizeof(int));}
            total=brush_findinvolume(fb_spike_models[m],planes,distances,6,fb_spike_ids,__NULL__,fb_spike_id_capacity);
            if(total<fb_spike_id_capacity)break;
            fb_spike_id_capacity*=2;memfree(fb_spike_ids);fb_spike_ids=memalloc(fb_spike_id_capacity*sizeof(int));
        }while(TRUE);
        if(fb_spike_candidate_count+total>fb_spike_candidate_capacity){
            fb_spike_candidate_capacity=max(fb_spike_candidate_count+total,max(64,fb_spike_candidate_capacity*2));
            int *larger=memalloc(fb_spike_candidate_capacity*sizeof(int));memcpy(larger,fb_spike_candidates,fb_spike_candidate_count*sizeof(int));memfree(fb_spike_candidates);fb_spike_candidates=larger;
        }
        for(int i=0;i<total;i++){int n=fb_document_brush_node(fb_spike_models[m],fb_spike_ids[i]);if(n>=2)fb_spike_candidates[fb_spike_candidate_count++]=n;}
    }
}
vector fb_visual_spike_trace(vector start,vector end,optional int reference)
{
    float nearest=vlen(end-start);vector direction=normalize(end-start);fb_visual_spike_hit=FALSE;
    static vector bounds[2];
    int indexed=!reference && fb_node_count>=10000 && checkbuiltin(brush_findinvolume);
    if(indexed)fb_spike_candidates_prepare(start,end);
    for(int candidate=0;candidate<(indexed?fb_spike_candidate_count:fb_node_count);candidate++) {
        int node=indexed?fb_spike_candidates[candidate]:candidate;
        fb_node_t item=fb_nodes[node];
        if(item.kind!=FB_NODE_BRUSH || brush_isselected(item.model,item.id) || !fb_render_allowed(node,FB_PICK_LOCKED))continue;
        if(brush_getfacepoints(item.model,item.id,0,bounds,2)!=2 || fb_pick_box(start,direction,bounds[0],bounds[1],nearest)<0)continue;
        int count=brush_get(item.model,item.id,__NULL__,0,__NULL__);if(count<4)continue;
        brushface_t *faces=memalloc(count*sizeof(brushface_t));brush_get(item.model,item.id,faces,count,__NULL__);
        float enter=0,leave=nearest;int valid=TRUE;
        for(int f=0;f<count;f++) {
            float distance=faces[f].planedist-start*faces[f].planenormal,denominator=direction*faces[f].planenormal;
            if(fabs(denominator)<0.000001){if(distance<-.001){valid=FALSE;break;}}
            else {float t=distance/denominator;if(denominator<0)enter=max(enter,t);else leave=min(leave,t);if(enter>leave+.001){valid=FALSE;break;}}
        }
        memfree(faces);if(valid&&enter<=nearest&&leave>=0){nearest=max(0,enter);fb_visual_spike_hit=TRUE;}
    }
    return start+direction*nearest;
}
vector fb_visual_spike_tips[24];
int fb_visual_spike_hits[24];
string fb_visual_spike_key;

typedef struct {int model,id;} fb_spike_brush_t;
int(fb_spike_brush_t *items,int count,vector *lines,int rays,int *hits,vector *tips) ftebroom_spike_batch=#0:ftebroom_spike_batch;
int fb_visual_spike_batch(vector low,vector high)
{
    if(!checkbuiltin(ftebroom_spike_batch))return FALSE;
    static fb_spike_brush_t *items;static int capacity;
    if(capacity<fb_node_capacity){memfree(items);capacity=fb_node_capacity;items=memalloc(capacity*sizeof(fb_spike_brush_t));}
    fb_render_allowed(0,FB_PICK_LOCKED);int count=0;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH && (fb_render_access[n]&1) && !brush_isselected(fb_nodes[n].model,fb_nodes[n].id)){
        items[count].model=fb_nodes[n].model;items[count++].id=fb_nodes[n].id;
    }
    static vector rays[48];
    for(int vertex=0;vertex<8;vertex++)for(int axis=0;axis<3;axis++){
        int index=vertex*3+axis;
        for(int a=0;a<3;a++)rays[index*2][a]=(vertex&(1i<<a))?high[a]:low[a];
        rays[index*2+1]=rays[index*2];rays[index*2+1][axis]+=((vertex&(1i<<axis))?1:-1)*256;
    }
    return ftebroom_spike_batch(items,count,rays,24,fb_visual_spike_hits,fb_visual_spike_tips)==24;
}
void fb_visual_spike_dot(vector tip)
{
    makevectors(getproperty(VF_ANGLES));vector right=v_right,up=v_up;
    vector eye=getproperty(VF_ORIGIN),fov=getproperty(VF_FOV),size=getproperty(VF_SIZE);
    float radius=max(.05,vlen(tip-eye)*tan(fov_y*M_PI/360)*6/max(1,size_y));
    tip+=normalize(eye-tip)*.02;
    R_BeginPolygon("ftebroom/selectiondepth");
    for(int i=0;i<12;i++){float angle=i*M_PI/6;R_PolygonVertex(tip+(right*cos(angle)+up*sin(angle))*radius,'0 0','1 0.08 0.08',1);}
    R_EndPolygon();
}
var float autocvar_ftebroom_native_selection_box=1;
int(vector low,vector high,vector color,float alpha,int depth) ftebroom_selection_box=#0:ftebroom_selection_box;
void fb_visual_box(vector low,vector high,vector color,float alpha,int spikes,optional int depth)
{
    if(!spikes && autocvar_ftebroom_native_selection_box && checkbuiltin(ftebroom_selection_box))if(ftebroom_selection_box(low,high,color,alpha,depth))return;
    static vector corners[8];
    for(int c=0;c<8;c++) for(int a=0;a<3;a++) {
        corners[c][a]=low[a];if(c & (1i<<a)) corners[c][a]=high[a];
    }
    string spikeKey=sprintf("%s %v %v %i %i %i",cvar_string("mapname"),low,high,fb_document_revision(),history,selectedbrushcount);
    spikeKey=strcat(spikeKey,sprintf(" %i %i %i %i",fb_grid_geometry_epoch,fb_visibility_epoch,fb_filter_epoch,fb_selection_epoch));
    int refresh=spikes&&(spikeKey!=fb_visual_spike_key);
    int batched=FALSE;
    if(refresh){strunzone(fb_visual_spike_key);fb_visual_spike_key=strzone(spikeKey);batched=fb_visual_spike_batch(low,high);}
    for(int vertex=0;vertex<8;vertex++) for(int axis=0;axis<3;axis++) {
        int other=vertex^(1i<<axis);
        if(other>vertex) fb_visual_line(corners[vertex],corners[other],color,alpha,alpha,depth);
        if(spikes) {
            fb_visual_spike_count++;
            vector tip=corners[vertex];float direction=-1;if(vertex & (1i<<axis)) direction=1;
            tip[axis]+=direction*256;
            int index=vertex*3+axis;
            if(refresh && !batched){fb_visual_spike_tips[index]=fb_visual_spike_trace(corners[vertex],tip);fb_visual_spike_hits[index]=fb_visual_spike_hit;}
            tip=fb_visual_spike_tips[index];
            fb_visual_line(corners[vertex],tip,color,0.65,fb_visual_spike_hits[index]?.65:0,FALSE);
            if(fb_visual_spike_hits[index])fb_visual_spike_dot(tip);
        }
    }
}
float fb_visual_profile[4];
int *fb_visual_candidates;int fb_visual_candidate_count,fb_visual_candidate_capacity;
int(int *marks,int count,int serial,int *nodes) ftebroom_collect_marks=#0:ftebroom_collect_marks;
int fb_visual_candidates_incremental()
{
    if(!fb_incremental_enabled() || !checkbuiltin(ftebroom_collect_marks) || (autocvar_ftebroom_hover_highlight && fb_visual_hover>=0))return FALSE;
    static int geometry=-1,filters=-1,visibility=-1,known,lockedCount,serial,capacity;
    static int *locked;static int *marks;static hashtable points;
    if(capacity!=fb_node_capacity){memfree(locked);memfree(marks);capacity=fb_node_capacity;locked=memalloc(capacity*sizeof(int));marks=memalloc(capacity*sizeof(int));geometry=-1;}
    if(geometry!=fb_grid_geometry_epoch || filters!=fb_filter_epoch || visibility!=fb_visibility_epoch){
        geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;visibility=fb_visibility_epoch;known=2;lockedCount=0;
        if(points)hash_destroytab(points);points=hash_createtab(max(64,fb_node_capacity),EV_INTEGER);
    }
    for(int n=known;n<fb_node_count;n++){
        if(!fb_selection_leaf(n))continue;
        if(fb_nodes[n].kind==FB_NODE_ENTITY)points[itos(fb_nodes[n].id)]=n+1i;
        if(fb_visual_locked(n))locked[lockedCount++]=n;
    }
    known=fb_node_count;
    serial++;if(!serial){memfill8((__variant*)marks,0,capacity*sizeof(int));serial=1;}
    for(int i=0;i<lockedCount;i++){int n=locked[i];if(fb_selection_leaf(n)&&fb_visual_locked(n))marks[n]=serial;}
    for(int i=0;i<selectedbrushcount;i++){
        int n=fb_document_brush_node(selectedbrushes[i].model,selectedbrushes[i].id);
        if(n>=2 && fb_selection_leaf(n) && fb_render_allowed(n,FB_PICK_LOCKED))marks[n]=serial;
    }
    for(int i=0;i<fb_selected_entity_count;i++){
        int n=(int)points[itos(fb_selected_entities[i])]-1i;
        if(n>=2 && fb_render_allowed(n,FB_PICK_LOCKED))marks[n]=serial;
    }
    // A native linear compaction preserves the original overlay order without
    // a QC walk through every node on each selection or brush transaction.
    int count=ftebroom_collect_marks(marks,fb_node_count,serial,fb_visual_candidates);
    if(count<0)return FALSE;fb_visual_candidate_count=count;return TRUE;
}
int fb_visual_candidate_epoch;
void fb_visual_candidates_prepare()
{
    static int geometry=-1,filters=-1,visibility=-1,selection=-1,revision=-1,hover=-2,enabled=-1;
    int current=fb_document_revision(),highlight=autocvar_ftebroom_hover_highlight!=0;
    int hoverNode=highlight?fb_visual_hover:-1;
    if(geometry==fb_grid_geometry_epoch&&filters==fb_filter_epoch&&visibility==fb_visibility_epoch&&selection==fb_selection_epoch&&revision==current&&hover==hoverNode&&enabled==highlight)return;
    geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;visibility=fb_visibility_epoch;selection=fb_selection_epoch;revision=current;hover=hoverNode;enabled=highlight;fb_visual_candidate_epoch++;
    if(fb_node_count>fb_visual_candidate_capacity){fb_visual_candidate_capacity=fb_node_capacity;memfree(fb_visual_candidates);fb_visual_candidates=memalloc(max(1,fb_node_capacity)*sizeof(int));}
    fb_visual_candidate_count=0;
    if(fb_visual_candidates_incremental())return;
    for(int node=2;node<fb_node_count;node++){
        if(!fb_selection_leaf(node)||!fb_render_allowed(node,FB_PICK_LOCKED))continue;
        int selected=fb_nodes[node].kind==FB_NODE_ENTITY?fb_entity_selected(fb_nodes[node].id):brush_isselected(fb_nodes[node].model,fb_nodes[node].id);
        if(selected||fb_visual_locked(node)||(enabled&&hover>=0&&fb_selection_descendant(node,hover)))fb_visual_candidates[fb_visual_candidate_count++]=node;
    }
}
int(fb_point_render_t *points,fb_point_geometry_t *geometry,int count,int *access,int accessCount,int *selected,int selectedCount,vector *colors) ftebroom_selection_points=#0:ftebroom_selection_points;
int(vector *boxes,int count) ftebroom_selection_boxes=#0:ftebroom_selection_boxes;
void fb_visual_draw()
{
    float started=gettime(1),profile81Time=started;
    fb_visual_spike_count=0;fb_visual_surface_count=fb_visual_vertex_count=0;
    shaderforname("ftebroom/selectionoverlay","{\ncull none\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\nnodepthtest\n}\n}");
    shaderforname("ftebroom/selectiondepth","{\ncull none\npolygonoffset\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\n}\n}");
    shaderforname("ftebroom/selectionfill","{\ncull none\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\nnodepthtest\n}\n}");
    static vector points[256],bounds[2];
    vector eye=getproperty(VF_ORIGIN),fov=getproperty(VF_FOV);makevectors(getproperty(VF_ANGLES));
    vector forward=v_forward,right=v_right,up=v_up;
    float tx=tan(fov_x*M_PI/360),ty=tan(fov_y*M_PI/360);
    fb_visual_candidates_prepare();
    int nativePointBoxes=FALSE;
    if(fb_visual_candidate_count>256 && autocvar_ftebroom_point_cache && autocvar_ftebroom_native_selection_box && !fb_transform_preview && !autocvar_ftebroom_hover_highlight && checkbuiltin(ftebroom_selection_points)){
        fb_point_render_prepare();static vector colors[2];colors[0]=autocvar_ftebroom_color_selected;colors[1]=autocvar_ftebroom_color_locked;
        nativePointBoxes=ftebroom_selection_points(fb_point_render,fb_point_geometry,fb_point_render_count,fb_render_access,fb_render_access_count,fb_selected_entities,fb_selected_entity_count,colors)>=0;
    }
    static int reportedSelection=-1;
    if(cvar("ftebroom_profile_commit") && fb_visual_candidate_count>256 && reportedSelection!=fb_selection_epoch){reportedSelection=fb_selection_epoch;print(sprintf("RB_SELECTION87 native=%i candidates=%i pointCache=%g hover=%g preview=%i builtin=%i\n",nativePointBoxes,fb_visual_candidate_count,autocvar_ftebroom_point_cache,autocvar_ftebroom_hover_highlight,fb_transform_preview,(int)checkbuiltin(ftebroom_selection_points)));}
    static vector *boxes;static int boxCapacity;int boxCount=0;
    int batchBoxes=autocvar_ftebroom_native_selection_box && checkbuiltin(ftebroom_selection_boxes);
    if(batchBoxes && boxCapacity<fb_visual_candidate_count){memfree(boxes);boxCapacity=max(64,fb_visual_candidate_count);boxes=memalloc(boxCapacity*3*sizeof(vector));}
    // Native point overlays already handled these entities. Cache the remaining
    // brush/patch list instead of revisiting every selected point every frame.
    static int *geometryCandidates;static int geometryCapacity,geometryCount,geometryEpoch=-1;
    if(nativePointBoxes && geometryEpoch!=fb_visual_candidate_epoch){
        geometryEpoch=fb_visual_candidate_epoch;geometryCount=0;
        if(geometryCapacity<fb_visual_candidate_count){memfree(geometryCandidates);geometryCapacity=max(64,fb_visual_candidate_count);geometryCandidates=memalloc(geometryCapacity*sizeof(int));}
        for(int entry=0;entry<fb_visual_candidate_count;entry++){
            int node=fb_visual_candidates[entry];if(fb_nodes[node].kind!=FB_NODE_ENTITY)geometryCandidates[geometryCount++]=node;
        }
    }
    // Moving point originals are excluded once per capture, including locked
    // point candidates that are not part of the gesture and must stay visible.
    static int *originalCandidates;static int originalCapacity,originalCount,originalEpoch=-1,originalCapture=-1;
    int movingPoints=fb_transform_preview && !fb_transform_duplicate;
    if(movingPoints && (originalEpoch!=fb_visual_candidate_epoch || originalCapture!=fb_transform_capture_epoch)){
        originalEpoch=fb_visual_candidate_epoch;originalCapture=fb_transform_capture_epoch;originalCount=0;
        if(originalCapacity<fb_visual_candidate_count){memfree(originalCandidates);originalCapacity=max(64,fb_visual_candidate_count);originalCandidates=memalloc(originalCapacity*sizeof(int));}
        for(int entry=0;entry<fb_visual_candidate_count;entry++){
            int node=fb_visual_candidates[entry];
            if(fb_nodes[node].kind==FB_NODE_ENTITY && fb_transform_replaces(0,fb_nodes[node].id))continue;
            originalCandidates[originalCount++]=node;
        }
    }
    int ortho=cvar("ca_brush_view");
    int cacheable=!fb_visual_test_reference && checkbuiltin(ftebroom_indexed_overlay) && fb_visual_candidate_count>256 && !fb_active && !fb_transform_preview && !fb_vertex_preview_valid && !fb_generator_count && !fb_clip_count && !autocvar_ftebroom_hover_highlight;
    static string cacheKey;
    string currentKey=sprintf("%i %i %i %i %g %v %v",fb_visual_candidate_epoch,fb_grid_geometry_epoch,fb_incremental_epoch,ortho,autocvar_ftebroom_sky_overlays,autocvar_ftebroom_color_selected,autocvar_ftebroom_color_locked);
    int reuse=cacheable && currentKey==cacheKey;
    if(!cacheable){strunzone(cacheKey);cacheKey="";fb_visual_cache_clear();}
    int candidates=nativePointBoxes?geometryCount:(movingPoints?originalCount:fb_visual_candidate_count);
    if(reuse && nativePointBoxes)candidates=0;
    for(int candidate=0;candidate<candidates;candidate++) {
        int node=nativePointBoxes?geometryCandidates[candidate]:(movingPoints?originalCandidates[candidate]:fb_visual_candidates[candidate]);
        fb_node_t item=fb_nodes[node];
        if(reuse && item.kind!=FB_NODE_ENTITY)continue;
        if(!fb_selection_leaf(node) || !fb_render_allowed(node,FB_PICK_LOCKED)) continue;
        int selected=0,locked=fb_visual_locked(node),hover=FALSE;
        if(item.kind==FB_NODE_ENTITY) selected=fb_entity_selected(item.id);
        else selected=brush_isselected(item.model,item.id);
        if(fb_visual_hover>=0 && autocvar_ftebroom_hover_highlight) hover=fb_selection_descendant(node,fb_visual_hover);
        if(!selected && !locked && !hover) continue;
        if(item.kind>=FB_NODE_BRUSH) {
            fb_visual_polygon_center='0 0 0';fb_visual_polygon_radius=1000000000;
            if(brush_getfacepoints(item.model,item.id,0,bounds,2)==2) {
                vector center=(bounds[0]+bounds[1])*0.5,delta=center-eye;
                float radius=vlen(bounds[1]-center),z=delta*forward;fb_visual_polygon_center=center;fb_visual_polygon_radius=radius;
                if(!cacheable && !ortho){
                    if(z+radius<0)continue;
                    if(fabs(delta*right)>z*tx+radius*(1+tx) || fabs(delta*up)>z*ty+radius*(1+ty))continue;
                }
            }
        }
        if(item.kind>=FB_NODE_BRUSH) if(fb_preview_replaces(item.model,item.id)) continue;
        // Selected surfaces use the shared face cache; overlays are batched below.
        vector color=autocvar_ftebroom_color_hover;float fill=0.06;
        if(selected) {color=autocvar_ftebroom_color_selected;fill=0.08;}
        if(locked) {color=autocvar_ftebroom_color_locked;fill=0.08;}
        if(item.kind==FB_NODE_ENTITY) {
            if(fb_transform_replaces(0,item.id)) continue;
            if(fb_entity_bounds(item.id,bounds)){
                if(batchBoxes){boxes[boxCount*3]=bounds[0];boxes[boxCount*3+1]=bounds[1];boxes[boxCount*3+2]=color;boxCount++;}
                else fb_visual_box(bounds[0],bounds[1],color,1,FALSE);
            }
        } else if(item.kind==FB_NODE_BRUSH) {
            static brushface_t attributes[128];
            int faces=brush_get(item.model,item.id,attributes,128,__NULL__);
            for(int f=0;f<faces;f++) {
                if(f<128 && !cvar("ca_brush_view") && !autocvar_ftebroom_sky_overlays)
                    if((attributes[f].surfaceflags&4) || fb_sky_material(attributes[f].shadername))continue;
                if(selected && !locked) if(!selbrush_whole(&selectedbrushes[selected-1])) {
                    if(!selbrush_face(&selectedbrushes[selected-1],f)) continue;fill=0.3;
                }
                int count=brush_getfacepoints(item.model,item.id,f+1,points,256);
                fb_visual_polygon(points,count,color,fill);
            }
        } else if(item.kind==FB_NODE_PATCH) {
            patchinfo_t info;int total=patch_getmesh(item.model,item.id,__NULL__,0,&info);
            if(total<1 || total>65536) continue;
            patchvert_t *mesh=memalloc(sizeof(patchvert_t)*total);patch_getmesh(item.model,item.id,mesh,total,&info);
            for(int y=0;y<info.cpheight-1;y++) for(int x=0;x<info.cpwidth-1;x++) {
                int at=y*info.cpwidth+x;
                points[0]=mesh[at].xyz;points[1]=mesh[at+1].xyz;points[2]=mesh[at+info.cpwidth+1].xyz;points[3]=mesh[at+info.cpwidth].xyz;
                fb_visual_polygon(points,4,color,fill);
            }
            memfree(mesh);
        }
    }
    if(boxCount)ftebroom_selection_boxes(boxes,boxCount);
    fb_visual_profile[0]+=gettime(1)-started;started=gettime(1);
    if(cacheable){
        if(!reuse){fb_visual_cache_build(ortho);strunzone(cacheKey);cacheKey=strzone(currentKey);}
        fb_visual_cache_draw(ortho);
    }else fb_visual_flush();
    fb_visual_profile[1]+=gettime(1)-started;started=gettime(1);
    fb_container_draw(FALSE);fb_entity_links_draw();
    fb_visual_profile[2]+=gettime(1)-started;started=gettime(1);
    fb_entity_drop_preview();
    fb_visual_profile[3]+=gettime(1)-started;
    if(fb_visual_has_bounds) fb_visual_box(fb_visual_min,fb_visual_max,autocvar_ftebroom_color_selected,0.6,
        !cvar("ca_brush_view") && fb_visual_spikes && autocvar_ftebroom_selection_spikes);
    fb_commit_profile("fb_visual_draw",profile81Time);
}
// Draw only the brush face boundary, after the 3D pass and outside cached views.
void fb_visual_face_hover()
{
    int testHover=cvar("ftebroom_skip_setup") && cvar("ftebroom_test_face_hover");
    if((!cvar("ftebroom_shift") && !testHover) || fb_active || cvar("ftebroom_camera_captured"))return;
    vector pointer=curmousepos;
    if(testHover){pointer=project('0 -64 64');pointer_z=0;}
    vector mn=getproperty(VF_MIN),sz=getproperty(VF_SIZE);
    if(pointer_x<mn_x || pointer_y<mn_y || pointer_x>=mn_x+sz_x || pointer_y>=mn_y+sz_y)return;
    int ortho=cvar("ca_brush_view");
    static string last="";static vector points[256];static int count;
    string key=strcat(sprintf("%v %v %v %v %v %i %g",pointer,getproperty(VF_ORIGIN),getproperty(VF_ANGLES),mn,sz,ortho,cvar("ca_brush_viewsize")),sprintf(" %i %i %i %i",fb_grid_geometry_epoch,fb_visibility_epoch,fb_filter_epoch,fb_incremental_epoch)," ",itos(fb_selection_epoch));
    if(key!=last){strunzone(last);last=strzone(key);count=0;
        vector end=pointer;if(ortho)end_z+=1;else end_z+=8192;
        if(fb_pick_query(unproject(pointer),unproject(end),pointer,0,FB_HIT_BRUSH,ortho))
            if(!selectedbrushcount || brush_isselected(fb_hits[0].model,fb_hits[0].id))count=brush_getfacepoints(fb_hits[0].model,fb_hits[0].id,fb_hits[0].face,points,256);
    }
    if(count<3 || count>256)return;
    vector eye=getproperty(VF_ORIGIN);makevectors(getproperty(VF_ANGLES));vector forward=v_forward;
    drawsetcliparea(mn_x,mn_y,sz_x,sz_y);
    for(int p=0;p<count;p++) {
        int next=p+1;if(next==count)next=0;
        vector a=points[p],b=points[next];
        if(!ortho) {
            float da=(a-eye)*forward,db=(b-eye)*forward;
            if(da<4 && db<4)continue;
            if(da<4)a=a+(b-a)*((4-da)/(db-da));
            else if(db<4)b=b+(a-b)*((4-db)/(da-db));
        }
        a=project(a);b=project(b);a_z=b_z=0;
        drawline(2,a,b,'1 1 0',1,0);
    }
    drawresetcliparea();
}

void fb_visual_labels()
{
    fb_visual_label_count=0;fb_container_draw(TRUE);
    if(!fb_visual_has_bounds || !autocvar_ftebroom_selection_dimensions) return;
    vector mn=getproperty(VF_MIN),sz=getproperty(VF_SIZE),origin=getproperty(VF_ORIGIN);
    int view=cvar("ca_brush_view"),depth=-1;if(view) depth=fb_view_depth_axis(view);
    vector middle=mn+sz*0.5;middle_z=0;
    vector forward=normalize(unproject(middle+'0 0 1')-unproject(middle));
    for(int axis=0;axis<3;axis++) {
        if(axis==depth) continue;
        vector position=fb_visual_min;position[axis]=(fb_visual_min[axis]+fb_visual_max[axis])*0.5;
        if(!view && (position-origin)*forward<=0) continue;
        vector pixel=project(position);pixel_z=0;
        string axisName="X";if(axis==1) axisName="Y";if(axis==2) axisName="Z";
        string label=sprintf("%s: %g",axisName,fb_visual_max[axis]-fb_visual_min[axis]);
        float width=strlen(label)*8;
        if(pixel_x<mn_x || pixel_y<mn_y || pixel_x>=mn_x+sz_x || pixel_y>=mn_y+sz_y) continue;
        pixel_x=bound(mn_x+3,pixel_x+6,mn_x+sz_x-width-3);pixel_y=bound(mn_y+3,pixel_y+6,mn_y+sz_y-15);
        fb_visual_label_count++;
        drawfill(pixel-'2 1 0',[width+4,13,0],'0.05 0.05 0.05',0.85,0);
        drawstring(pixel,label,'8 8 0',autocvar_ftebroom_color_selected,1,0);
    }
}
