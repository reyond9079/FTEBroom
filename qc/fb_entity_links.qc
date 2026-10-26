// SPDX-License-Identifier: GPL-2.0-or-later
typedef struct {vector mins,maxs;int valid;vector color;trisoup_simple_vert_t vertices[8];} fb_container_bounds_t;
fb_container_bounds_t *fb_container_cache;
void fb_container_cache_prepare()
{
    static int revision=-1,geometry=-1,filters=-1,entityRevision=-1;
    int current=fb_document_revision();
    if(revision==current && geometry==fb_grid_geometry_epoch && filters==fb_filter_epoch && entityRevision==fb_entity_render_epoch)return;
    revision=current;geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;entityRevision=fb_entity_render_epoch;
    memfree(fb_container_cache);fb_container_cache=memalloc(fb_node_count*sizeof(fb_container_bounds_t));
    static vector bounds[2];
    // Aggregate each leaf once through its ancestors. This replaces one full
    // document traversal per container and per viewport.
    for(int n=2;n<fb_node_count;n++) {
        if(!fb_selection_leaf(n))continue;
        if(!fb_pick_allowed(n,FB_PICK_LOCKED))continue;
        int valid=FALSE;
        if(fb_nodes[n].kind==FB_NODE_ENTITY)valid=fb_entity_bounds(fb_nodes[n].id,bounds);
        else valid=brush_getfacepoints(fb_nodes[n].model,fb_nodes[n].id,0,bounds,2)==2;
        if(!valid)continue;
        for(int parent=n,steps=0;parent>=0 && parent<fb_node_count && steps++<fb_node_count;parent=fb_nodes[parent].parent) {
            fb_container_bounds_t *item=&fb_container_cache[parent];
            if(!item->valid){item->mins=bounds[0];item->maxs=bounds[1];item->valid=TRUE;}
            else for(int axis=0;axis<3;axis++){item->mins[axis]=min(item->mins[axis],bounds[0][axis]);item->maxs[axis]=max(item->maxs[axis],bounds[1][axis]);}
        }
    }
}
int fb_container_bounds(int node,vector *bounds)
{
    if(node<0 || node>=fb_node_count)return FALSE;
    fb_container_cache_prepare();
    if(!fb_container_cache[node].valid)return FALSE;
    bounds[0]=fb_container_cache[node].mins;bounds[1]=fb_container_cache[node].maxs;return TRUE;
}
fb_point_render_t *fb_container_render;int fb_container_render_count;
int *fb_container_visible;
void fb_container_render_prepare()
{
    static int geometry=-1,filters=-1,visibility=-1,revision=-1,entityRevision=-1;
    int current=fb_document_revision();
    if(geometry==fb_grid_geometry_epoch && filters==fb_filter_epoch && visibility==fb_visibility_epoch && revision==current && entityRevision==fb_entity_render_epoch)return;
    geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;visibility=fb_visibility_epoch;revision=current;entityRevision=fb_entity_render_epoch;
    fb_document_lists_prepare();fb_container_cache_prepare();memfree(fb_container_render);memfree(fb_container_visible);
    fb_container_render=memalloc(max(1,fb_container_node_count)*sizeof(fb_point_render_t));fb_container_visible=memalloc(max(1,fb_container_node_count)*sizeof(int));fb_container_render_count=0;
    for(int entry=0;entry<fb_container_node_count;entry++){
        int node=fb_container_nodes[entry];if(!fb_render_allowed(node,FB_PICK_LOCKED))continue;
        fb_container_bounds_t *box=&fb_container_cache[node];if(!box->valid)continue;
        fb_point_render_t *item=&fb_container_render[fb_container_render_count++];item->node=node;item->model=0;item->id=fb_nodes[node].kind==FB_NODE_GROUP;
        item->center=(box->mins+box->maxs)*.5;item->radius=vlen(box->maxs-box->mins)*.5+12;item->label=item->center;item->label[2]=box->maxs[2]+12;item->name=fb_nodes[node].name;
    }
}
void fb_container_draw(int labels)
{
    static vector bounds[2];vector camera=getproperty(VF_ORIGIN),angles=getproperty(VF_ANGLES);makevectors(angles);vector forward=v_forward,right=v_right,up=v_up;
    vector mn=getproperty(VF_MIN),size=getproperty(VF_SIZE);
    vector fov=getproperty(VF_FOV);float tx=tan(fov_x*M_PI/360),ty=tan(fov_y*M_PI/360);
    int ortho=cvar("ca_brush_view"),groups=cvar("ftebroom_show_group_bounds"),entities=cvar("ftebroom_show_brush_entity_bounds");
    if(!labels && !cvar("ca_brush_view") && cvar("ftebroom_bounds_visible")) {
        if(tokenize(cvar_string("ftebroom_map_bounds"))==6)fb_visual_box([stof(argv(0)),stof(argv(1)),stof(argv(2))],[stof(argv(3)),stof(argv(4)),stof(argv(5))],'1 0.5 0',0.7,FALSE);
    }
    fb_container_render_prepare();
    int visibleCount=-1;
    if(!ortho && checkbuiltin(ftebroom_cull_points))visibleCount=ftebroom_cull_points(fb_container_render,fb_container_render_count,fb_container_visible,sizeof(fb_point_render_t));
    int entries=visibleCount<0?fb_container_render_count:visibleCount;
    for(int entry=0;entry<entries;entry++) {
        int index=visibleCount<0?entry:fb_container_visible[entry];fb_point_render_t *item=&fb_container_render[index];int n=item->node,group=item->id;
        if(group && !groups)continue;if(!group && !entities)continue;
        bounds[0]=fb_container_cache[n].mins;bounds[1]=fb_container_cache[n].maxs;
        if(visibleCount<0 && !ortho){vector delta=item->center-camera;float radius=item->radius,depth=delta*forward;if(depth+radius<0||fabs(delta*right)>depth*tx+radius*(1+tx)||fabs(delta*up)>depth*ty+radius*(1+ty))continue;}
        if(labels){vector top=(bounds[0]+bounds[1])*0.5;top_z=bounds[1][2]+12;if(!ortho && (top-camera)*forward<=0)continue;vector pixel=project(top);pixel_z=0;if(pixel_x<mn_x||pixel_y<mn_y||pixel_x>=mn_x+size_x||pixel_y>=mn_y+size_y)continue;if(!ortho && !fb_label_visible(camera,top,n))continue;drawstring(pixel,fb_nodes[n].name,'8 8 0','0.35 0.6 1',1,0);}
        else {
            vector color='0.2 0.45 1';if(group && fb_nodes[n].fields["_tb_linked_group_id"]!="")color='0.65 0.35 1';
            fb_container_bounds_t *box=&fb_container_cache[n];
            if(box->color!=color){box->color=color;for(int v=0;v<8;v++){
                for(int a=0;a<3;a++)box->vertices[v].xyz[a]=(v&(1i<<a))?bounds[1][a]:bounds[0][a];
                box->vertices[v].st=(vec2){0,0};box->vertices[v].rgba=(vec4){color_x,color_y,color_z,.75};
            }}
            addtrisoup_simple(ortho?"ftebroom/selectionoverlay":"ftebroom/selectiondepth",3|DRAWFLAG_LINES,box->vertices,bbox_line_idxs,bbox_line_idxs.length);
        }
    }
}
typedef struct {int from,to;} fb_entity_link_t;
fb_entity_link_t *fb_entity_connections;int fb_entity_connection_count;int *fb_entity_reachable;int *fb_entity_link_selected;vector *fb_entity_centers;
void fb_entity_links_prepare()
{
    static int revision=-1,geometry=-1,filters=-1,entityRevision=-1;int current=fb_document_revision();
    if(revision==current && geometry==fb_grid_geometry_epoch && filters==fb_filter_epoch)return;revision=current;geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;
    memfree(fb_entity_connections);fb_entity_connections=__NULL__;fb_entity_connection_count=0;
    memfree(fb_entity_reachable);fb_entity_reachable=memalloc(fb_node_count*sizeof(int));memfree(fb_entity_centers);fb_entity_centers=memalloc(fb_node_count*sizeof(vector));
    memfree(fb_entity_link_selected);fb_entity_link_selected=memalloc(fb_node_count*sizeof(int));
    static vector bounds[2];
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY) {
        fb_entity_link_selected[n]=fb_entity_reachable[n]=fb_selection_selected(n);if(fb_container_bounds(n,bounds))fb_entity_centers[n]=(bounds[0]+bounds[1])*0.5;
        string target=fb_nodes[n].fields["target"],killtarget=fb_nodes[n].fields["killtarget"];if(target=="" && killtarget=="")continue;
        for(int other=2;other<fb_node_count;other++)if(other!=n && fb_nodes[other].kind==FB_NODE_ENTITY && fb_nodes[other].fields["targetname"]!="" && (fb_nodes[other].fields["targetname"]==target || fb_nodes[other].fields["targetname"]==killtarget)) {
            fb_entity_link_t *links=memalloc((fb_entity_connection_count+1)*sizeof(fb_entity_link_t));memcpy(links,fb_entity_connections,fb_entity_connection_count*sizeof(fb_entity_link_t));memfree(fb_entity_connections);fb_entity_connections=links;
            links[fb_entity_connection_count].from=n;links[fb_entity_connection_count++].to=other;
        }
    }
    for(int step=0;step<fb_node_count;step++) {
        int changed=FALSE;
        for(int e=0;e<fb_entity_connection_count;e++) {fb_entity_link_t edge=fb_entity_connections[e];if(fb_entity_reachable[edge.from] || fb_entity_reachable[edge.to]) {
            if(!fb_entity_reachable[edge.from] || !fb_entity_reachable[edge.to])changed=TRUE;fb_entity_reachable[edge.from]=fb_entity_reachable[edge.to]=TRUE;
        }}
        if(!changed)break;
    }
}
void fb_entity_links_draw()
{
    static vector linkedBounds[2];int opened=fb_selection_open_node();
    fb_document_lists_prepare();
    for(int entry=0;entry<fb_container_node_count;entry++){int n=fb_container_nodes[entry];if(fb_nodes[n].kind==FB_NODE_GROUP && fb_nodes[n].fields["_tb_linked_group_id"]!="") {
        if(n!=opened && !fb_selection_selected(n))continue;
        if(!fb_container_bounds(n,linkedBounds))continue;
        vector from=(linkedBounds[0]+linkedBounds[1])*0.5;
        string link=fb_nodes[n].fields["_tb_linked_group_id"];
        for(int peer=2;peer<fb_node_count;peer++) {
            if(peer==n || fb_nodes[peer].kind!=FB_NODE_GROUP || fb_nodes[peer].fields["_tb_linked_group_id"]!=link)continue;
            if(!fb_render_allowed(peer,FB_PICK_LOCKED) || !fb_container_bounds(peer,linkedBounds))continue;
            vector to=(linkedBounds[0]+linkedBounds[1])*0.5,direction=normalize(to-from);
            if(vlen(to-from)<0.01)continue;
            vector side=normalize(crossproduct(direction,'0 0 1'));if(vlen(side)<0.1)side='1 0 0';
            vector tip=from+(to-from)*0.65;float arrow=min(12,vlen(to-from)*0.15);
            fb_visual_line(from,to,'0.65 0.35 1',0.8,0.8,TRUE);
            fb_visual_line(tip,tip-direction*arrow+side*arrow*0.5,'0.65 0.35 1',1,1,TRUE);
            fb_visual_line(tip,tip-direction*arrow-side*arrow*0.5,'0.65 0.35 1',1,1,TRUE);
        }
    }
    }
    int mode=cvar("ftebroom_entity_links");if(!mode)return;
    if(mode<=2 && !selectedbrushcount && !fb_selected_entity_count)return;
    fb_entity_links_prepare();
    for(int e=0;e<fb_entity_connection_count;e++) {
        fb_entity_link_t edge=fb_entity_connections[e];
        if(!fb_pick_allowed(edge.from,FB_PICK_LOCKED)||!fb_pick_allowed(edge.to,FB_PICK_LOCKED))continue;
        if(mode==1 && !fb_entity_link_selected[edge.from] && !fb_entity_link_selected[edge.to])continue;
        if(mode==2 && !fb_entity_reachable[edge.from])continue;
        vector from=fb_entity_centers[edge.from],to=fb_entity_centers[edge.to],direction=normalize(to-from);
        if(vlen(to-from)<0.01)continue;
        vector side=normalize(crossproduct(direction,'0 0 1'));if(vlen(side)<0.1)side='1 0 0';
        vector color='0.2 1 0.3';if(fb_entity_link_selected[edge.from]||fb_entity_link_selected[edge.to])color='1 0.2 0.2';
        fb_visual_line(from,to,color,0.8,0.8,TRUE);
        vector tip=from+(to-from)*0.65;float arrow=min(12,vlen(to-from)*0.15);
        fb_visual_line(tip,tip-direction*arrow+side*arrow*0.5,color,1,1,TRUE);
        fb_visual_line(tip,tip-direction*arrow-side*arrow*0.5,color,1,1,TRUE);
    }
}
