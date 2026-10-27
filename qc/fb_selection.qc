// SPDX-License-Identifier: GPL-2.0-or-later
// Selection stores geometry references and point entity IDs, never transient hit pointers.
int fb_face_ctrl,fb_face_paint_ready;
int fb_select_ctrl,fb_select_node,fb_select_paint,fb_select_blocked;
int fb_selection_descendant(int node,int parent)
{
    for(int steps=0;node>=0 && node<fb_node_count && steps<fb_node_count;steps++) {
        if(node==parent) return TRUE;
        node=fb_nodes[node].parent;
    }
    return FALSE;
}
int fb_selection_open_node()
{
    if(!fb_open_group)return -1;
    for(int i=2;i<fb_node_count;i++) if(fb_nodes[i].kind==FB_NODE_GROUP && fb_nodes[i].id==fb_open_group) return i;
    return -1;
}
int fb_selection_target(int node)
{
    int target=node,opened=fb_selection_open_node();
    for(int n=node,steps=0;n>=0 && n<fb_node_count && steps<fb_node_count;steps++) {
        if(fb_nodes[n].kind==FB_NODE_GROUP && !fb_selection_descendant(opened,n)) target=n;
        n=fb_nodes[n].parent;
    }
    return target;
}
int fb_selection_leaf(int node)
{
    if(fb_nodes[node].kind>=FB_NODE_BRUSH) return TRUE;
    return fb_nodes[node].kind==FB_NODE_ENTITY && substring(fb_nodes[node].fields["model"],0,1)!="*";
}
int fb_selection_selected_uncached(int node)
{
    if(!selectedbrushcount && !fb_selected_entity_count)return FALSE;
    if(node<0 || node>=fb_node_count)return FALSE;
    if(fb_selection_leaf(node)) {
        if(!fb_pick_allowed(node,0))return FALSE;
        if(fb_nodes[node].kind==FB_NODE_ENTITY)return fb_entity_selected(fb_nodes[node].id);
        int leafSelected=brush_isselected(fb_nodes[node].model,fb_nodes[node].id);
        if(!leafSelected)return FALSE;
        return selbrush_whole(&selectedbrushes[leafSelected-1]);
    }
    int found=FALSE;
    for(int i=0;i<fb_node_count;i++) {
        if(!fb_selection_leaf(i))continue;
        if(!fb_selection_descendant(i,node))continue;
        if(!fb_pick_allowed(i,0))continue;
        found=TRUE;
        if(fb_nodes[i].kind==FB_NODE_ENTITY) {if(!fb_entity_selected(fb_nodes[i].id)) return FALSE;}
        else {
            int selected=brush_isselected(fb_nodes[i].model,fb_nodes[i].id);
            if(!selected) return FALSE;
            if(!selbrush_whole(&selectedbrushes[selected-1])) return FALSE;
        }
    }
    return found;
}
int fb_filter_epoch,fb_visibility_epoch;
int fb_selection_selected(int node)
{
    if(node<0 || node>=fb_node_count || (!selectedbrushcount && !fb_selected_entity_count))return FALSE;
    if(fb_selection_leaf(node))return fb_selection_selected_uncached(node);
    static int selection=-1,geometry=-1,filters=-1,visibility=-1,opened=-1;
    static int *cached;
    if(selection!=fb_selection_epoch || geometry!=fb_grid_geometry_epoch || filters!=fb_filter_epoch || visibility!=fb_visibility_epoch || opened!=fb_open_group) {
        selection=fb_selection_epoch;geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;visibility=fb_visibility_epoch;opened=fb_open_group;
        memfree(cached);cached=memalloc(fb_node_count*sizeof(int));
        // Accumulate leaf selection into ancestors once, rather than scanning
        // the whole document separately for every group after an edit.
        for(int leaf=0;leaf<fb_node_count;leaf++) {
            if(!fb_selection_leaf(leaf))continue;
            if(!fb_pick_allowed(leaf,0))continue;
            int whole=fb_selection_selected_uncached(leaf);
            for(int parent=fb_nodes[leaf].parent,steps=0;parent>=0 && parent<fb_node_count && steps<fb_node_count;steps++) {
                cached[parent]|=whole?1:3;
                parent=fb_nodes[parent].parent;
            }
        }
        for(int container=0;container<fb_node_count;container++)cached[container]=cached[container]==1?2:1;
    }
    return cached[node]==2;
}
void fb_selection_set(int node,int selected)
{
    if(node<0 || node>=fb_node_count) return;
    if(fb_selection_leaf(node)) {
        if(!fb_pick_allowed(node,0))return;
        if(fb_nodes[node].kind==FB_NODE_ENTITY)fb_entity_select(fb_nodes[node].id,selected);
        else if(selected)brush_select(fb_nodes[node].model,fb_nodes[node].id);
        else brush_deselect(fb_nodes[node].model,fb_nodes[node].id);
        return;
    }
    for(int i=0;i<fb_node_count;i++) {
        if(!fb_selection_leaf(i))continue;
        if(!fb_selection_descendant(i,node))continue;
        if(!fb_pick_allowed(i,0))continue;
        if(fb_nodes[i].kind==FB_NODE_ENTITY) fb_entity_select(fb_nodes[i].id,selected);
        else if(selected) brush_select(fb_nodes[i].model,fb_nodes[i].id);
        else brush_deselect(fb_nodes[i].model,fb_nodes[i].id);
    }
}
void fb_selection_clear() {brush_deselectall();fb_restore_entity_selection(-1);}
void fb_selection_clear_faces()
{
    for(int i=0;i<selectedbrushcount;i++) if(!selbrush_whole(&selectedbrushes[i])) {fb_selection_clear();return;}
}
void fb_selection_click(int node,int ctrl)
{
    fb_tx_begin("Select");
    if(!ctrl) fb_selection_clear();else fb_selection_clear_faces();
    if(node>=0) fb_selection_set(node,!ctrl || !fb_selection_selected(node));
    fb_tx_commit();
}
void fb_selection_paint(vector pixel)
{
    if(!fb_select_paint) {
        fb_tx_begin("Paint Selection");fb_select_paint=TRUE;
        fb_selection_clear_faces();fb_selection_set(fb_select_node,TRUE);
    }
    if(fb_pick(pixel)>0 && fb_hits[0].kind!=FB_HIT_HANDLE) fb_selection_set(fb_selection_target(fb_hits[0].node),TRUE);
}
void fb_selection_drill_hits(int direction)
{
    int *nodes=memalloc(sizeof(int)*fb_hit_count);int count=0;
    // Drill always follows depth, including orthographic views (normal clicks use area).
    for(int i=1;i<fb_hit_count;i++) {
        fb_hit_t hit=fb_hits[i];int j=i-1;
        while(j>=0 && fb_hits[j].distance>hit.distance) {fb_hits[j+1]=fb_hits[j];j--;}
        fb_hits[j+1]=hit;
    }
    for(int h=0;h<fb_hit_count;h++) {
        if(fb_hits[h].kind==FB_HIT_HANDLE) continue;
        int node=fb_selection_target(fb_hits[h].node),seen=FALSE;
        for(int n=0;n<count;n++) if(nodes[n]==node) seen=TRUE;
        if(!seen) nodes[count++]=node;
    }
    if(count) {
        int index=-1;
        for(int k=0;k<count;k++) if(fb_selection_selected(nodes[k])) {index=k;break;}
        if(index<0) index=direction>0?0:count-1;
        else index=bound(0,index+direction,count-1);
        fb_tx_begin("Drill Selection");fb_selection_clear();fb_selection_set(nodes[index],TRUE);fb_tx_commit();
    }
    memfree(nodes);
}
void fb_selection_double(vector pixel,int ctrl)
{
    fb_cancel();
    int node=-1;
    if(fb_pick(pixel)>0 && fb_hits[0].kind!=FB_HIT_HANDLE) node=fb_hits[0].node;
    int opened=fb_selection_open_node();
    fb_tx_begin("Select Siblings");
    if(opened>=0 && (node<0 || !fb_selection_descendant(node,opened))) {
        int parent=fb_nodes[opened].parent;
        fb_open_group=0;
        for(int p=parent,steps=0;p>=0 && steps<fb_node_count;steps++) {
            if(fb_nodes[p].kind==FB_NODE_GROUP) {fb_open_group=fb_nodes[p].id;break;}
            p=fb_nodes[p].parent;
        }
        fb_selection_clear();fb_selection_set(opened,TRUE);
    } else if(node>=0) {
        int target=fb_selection_target(node);
        if(fb_nodes[target].kind==FB_NODE_GROUP) {
            fb_open_group=fb_nodes[target].id;fb_selection_clear();
        } else {
            if(!ctrl) fb_selection_clear();else fb_selection_clear_faces();
            int parentNode=fb_nodes[target].parent;
            for(int sibling=0;sibling<fb_node_count;sibling++) if(fb_nodes[sibling].parent==parentNode)
                fb_selection_set(fb_selection_target(sibling),TRUE);
        }
    }
    fb_tx_commit();
}

// Cache exactly the existing face loops, without simplifying or dropping edges.
typedef struct {
    vector *points;
    int *counts,*lines;
    trisoup_simple_vert_t *vertices;
    vector color;
    int faces,total,ready;
    vector low,high;
} fb_wire_cache_t;
fb_wire_cache_t *fb_wire_cache;
int fb_wire_cache_count;var int fb_wire_cache_epoch=-1;
void fb_wire_cache_prepare()
{
    if(fb_wire_cache_epoch==fb_grid_geometry_epoch && fb_wire_cache_count==fb_node_count)return;
    for(int i=0;i<fb_wire_cache_count;i++){memfree(fb_wire_cache[i].points);memfree(fb_wire_cache[i].counts);memfree(fb_wire_cache[i].lines);memfree(fb_wire_cache[i].vertices);}
    memfree(fb_wire_cache);fb_wire_cache_count=fb_node_count;
    fb_wire_cache=memalloc(max(1,fb_node_count)*sizeof(fb_wire_cache_t));
    fb_wire_cache_epoch=fb_grid_geometry_epoch;
}
void fb_wire_cache_draw(int node)
{
    fb_wire_cache_t *cache=&fb_wire_cache[node];
    if(!cache.ready) {
        cache.ready=TRUE;
        cache.counts=memalloc(MAX_BRUSHFACES*sizeof(int));
        for(int f=0;f<MAX_BRUSHFACES;f++) {
            int n=brush_getfacepoints(fb_nodes[node].model,fb_nodes[node].id,f+1,facepoints,MAX_FACEPOINTS);
            if(n<=0)break;
            cache.counts[cache.faces]=n;cache.faces=cache.faces+1;cache.total=cache.total+n;
        }
        cache.points=memalloc(max(1,cache.total)*sizeof(vector));
        cache.lines=memalloc(max(1,cache.total)*2*sizeof(int));
        cache.vertices=memalloc(max(1,cache.total)*sizeof(trisoup_simple_vert_t));
        cache.color='-1 -1 -1';
        int storeOffset=0;
        for(int f=0;f<cache.faces;f++) {
            brush_getfacepoints(fb_nodes[node].model,fb_nodes[node].id,f+1,facepoints,MAX_FACEPOINTS);
            memcpy(cache.points+storeOffset,facepoints,cache.counts[f]*sizeof(vector));
            int count=cache.counts[f];
            for(int p=0;p<count;p++) {
                int index=storeOffset+p;cache.lines[index*2]=index;
                int next=p+1;if(next>=count)next=0;cache.lines[index*2+1]=storeOffset+next;
                for(int a=0;a<3;a++)cache.vertices[index].xyz[a]=facepoints[p][a];
                cache.vertices[index].rgba[3]=1;
            }
            storeOffset+=cache.counts[f];
        }
    }
    if(cache.color!=fb_visual_current_wire) {
        cache.color=fb_visual_current_wire;
        for(int p=0;p<cache.total;p++)for(int a=0;a<3;a++)cache.vertices[p].rgba[a]=cache.color[a];
    }
    if(cache.total)addtrisoup_simple("ftebroom/orthowire",3|DRAWFLAG_LINES,cache.vertices,cache.lines,cache.total*2);
}
// Draw every visible model: selecting a group must not hide the world in 2D.
void fb_selection_wireframe(vector sizes)
{
    static vector bounds[2],axes[3];
    fb_wire_cache_prepare();
    shaderforname("ftebroom/orthowire","{\ncull none\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\nnodepthtest\n}\n}");
    axes[0]=v_right;axes[1]=v_up;axes[2]=v_forward;
    vector origin=getproperty(VF_ORIGIN);
    for(int i=0;i<fb_node_count;i++) {
        fb_node_t node=fb_nodes[i];
        if(node.kind<FB_NODE_BRUSH || !fb_pick_allowed(i,FB_PICK_LOCKED)) continue;
        if(fb_preview_replaces(node.model,node.id)) continue;
        if(brush_getfacepoints(node.model,node.id,0,bounds,2)!=2) continue;
        vector center=(bounds[0]+bounds[1])*0.5-origin,half=(bounds[1]-bounds[0])*0.5;
        int visible=TRUE;
        for(int a=0;a<3;a++) {
            vector axis=axes[a];
            float radius=fabs(axis_x)*half_x+fabs(axis_y)*half_y+fabs(axis_z)*half_z;
            if(fabs(center*axis)>sizes[a]+radius) visible=FALSE;
        }
        if(visible) {
            fb_visual_current_wire='0.65 0.65 0.7';
            if(brush_isselected(node.model,node.id)) fb_visual_current_wire=stov(cvar_string("ftebroom_color_selected"));
            if(!fb_pick_allowed(i,0)) fb_visual_current_wire=stov(cvar_string("ftebroom_color_locked"));
            fb_wire_cache_draw(i);
        }
    }
}
