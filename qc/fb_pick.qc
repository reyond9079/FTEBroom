int fb_render_allowed(int node,int flags);
int fb_transform_replaces(int model,int id);
int fb_visibility_get(int model,int id);
int fb_filter_allowed(int node);
void fb_filter_prepare();
// SPDX-License-Identifier: GPL-2.0-or-later
enum : int { FB_HIT_BRUSH=1, FB_HIT_PATCH=2, FB_HIT_ENTITY=4, FB_HIT_HANDLE=8 };
enum : int { FB_PICK_HIDDEN=1, FB_PICK_LOCKED=2 };
typedef struct {
    int kind,model,id,face,node;
    float distance,area;
    vector point,normal;
} fb_hit_t;
fb_hit_t *fb_hits;
int fb_hit_count,fb_hit_capacity;
typedef struct {int id,node;vector point;float radius;} fb_handle_t;
fb_handle_t fb_handles[256];
int fb_pick_handle_count;
void fb_pick_clear_handles() {fb_pick_handle_count=0;}
void fb_pick_handle(int id,vector point,float radius,int node)
{
    if(fb_pick_handle_count>=fb_handles.length) return;
    fb_handle_t *h=&fb_handles[fb_pick_handle_count++];
    h->id=id;h->node=node;h->point=point;h->radius=radius>0?radius:6;
}
int fb_pick_allowed(int node,int flags)
{
    if(!(flags&FB_PICK_LOCKED) && fb_open_group) {
        int inside=FALSE;
        for(int p=node,depth=0;p>=0 && p<fb_node_count && depth++<fb_node_count;p=fb_nodes[p].parent)
            if(fb_nodes[p].kind==FB_NODE_GROUP && fb_nodes[p].id==fb_open_group) inside=TRUE;
        if(!inside)return FALSE;
    }
    for(int steps=0;node>=0 && node<fb_node_count && steps<fb_node_count;steps++) {
        int state=fb_nodes[node].flags;
        if(!(flags&FB_PICK_HIDDEN) && !fb_filter_allowed(node))return FALSE;
        if(!(flags&FB_PICK_HIDDEN) && fb_visibility_get(fb_nodes[node].model,fb_nodes[node].id)) return FALSE;
        if(!(flags&FB_PICK_HIDDEN) && (state&FB_NODE_HIDDEN)) return FALSE;
        if(!(flags&FB_PICK_LOCKED) && (state&FB_NODE_LOCKED)) return FALSE;
        node=fb_nodes[node].parent;
    }
    return TRUE;
}
void fb_pick_add(fb_hit_t hit)
{
    if(fb_hit_count==fb_hit_capacity) {
        int capacity=max(64,fb_hit_capacity*2);
        fb_hit_t *items=memalloc(sizeof(fb_hit_t)*capacity);
        memcpy(items,fb_hits,sizeof(fb_hit_t)*fb_hit_count);
        memfree(fb_hits);fb_hits=items;fb_hit_capacity=capacity;
    }
    fb_hits[fb_hit_count++]=hit;
}
float fb_pick_box(vector start,vector direction,vector low,vector high,float length)
{
    float enter=0,leave=length;
    for(int axis=0;axis<3;axis++) {
        if(fabs(direction[axis])<0.000001) {
            if(start[axis]<low[axis] || start[axis]>high[axis]) return -1;
        } else {
            float a=(low[axis]-start[axis])/direction[axis],b=(high[axis]-start[axis])/direction[axis];
            enter=max(enter,min(a,b));leave=min(leave,max(a,b));
            if(enter>leave) return -1;
        }
    }
    return enter;
}
float fb_pick_area(vector *points,int count)
{
    float area=0;
    for(int i=0;i<count;i++) {
        int next=i+1;if(next>=count) next=0;
        vector a=project(points[i]),b=project(points[next]);
        area+=a_x*b_y-b_x*a_y;
    }
    return fabs(area)*0.5;
}
void fb_pick_brush(int node,vector start,vector direction,float length)
{
    fb_node_t item=fb_nodes[node];
    static vector bounds[2],points[256];
    if(brush_getfacepoints(item.model,item.id,0,bounds,2)!=2) return;
    if(fb_pick_box(start,direction,bounds[0],bounds[1],length)<0) return;
    int count=brush_get(item.model,item.id,__NULL__,0,__NULL__);
    if(count<4) return;
    brushface_t *faces=memalloc(sizeof(brushface_t)*count);
    brush_get(item.model,item.id,faces,count,__NULL__);
    float enter=-1,leave=length;
    int front=-1,back=-1,valid=TRUE;
    for(int i=0;i<count;i++) {
        float distance=faces[i].planedist-start*faces[i].planenormal;
        float denominator=direction*faces[i].planenormal;
        if(fabs(denominator)<0.000001) {
            if(distance<0) {valid=FALSE;break;}
        } else {
            float t=distance/denominator;
            if(denominator<0) {if(t>enter) {enter=t;front=i;}}
            else if(t<leave) {leave=t;back=i;}
            if(enter>leave) {valid=FALSE;break;}
        }
    }
    if(enter<0) {enter=leave;front=back;}
    if(valid && front>=0 && enter>=0 && enter<=length) {
        fb_hit_t hit={0};
        hit.kind=FB_HIT_BRUSH;hit.model=item.model;hit.id=item.id;hit.node=node;
        hit.face=front+1;hit.distance=enter;hit.point=start+direction*enter;
        hit.normal=faces[front].planenormal;
        int n=brush_getfacepoints(item.model,item.id,hit.face,points,points.length);
        hit.area=n>=3?fb_pick_area(points,n):0;
        fb_pick_add(hit);
    }
    memfree(faces);
}
float fb_pick_triangle(vector start,vector direction,vector a,vector b,vector c)
{
    vector edge=b-a,other=c-a,p=crossproduct(direction,other);
    float determinant=edge*p;
    if(fabs(determinant)<0.000001) return -1;
    vector relative=start-a;
    float u=relative*p/determinant;
    if(u<-0.0001 || u>1.0001) return -1;
    vector q=crossproduct(relative,edge);
    float v=direction*q/determinant;
    if(v<-0.0001 || u+v>1.0001) return -1;
    return other*q/determinant;
}
void fb_pick_patch(int node,vector start,vector direction,float length)
{
    fb_node_t item=fb_nodes[node];
    static vector patchBounds[2];
    if(brush_getfacepoints(item.model,item.id,0,patchBounds,2)!=2) return;
    if(fb_pick_box(start,direction,patchBounds[0],patchBounds[1],length)<0) return;
    patchinfo_t info;
    int count=patch_getmesh(item.model,item.id,__NULL__,0,__NULL__);
    if(count<4) return;
    patchvert_t *mesh=memalloc(sizeof(patchvert_t)*count);
    patch_getmesh(item.model,item.id,mesh,count,&info);
    fb_hit_t hit={0};hit.kind=FB_HIT_PATCH;hit.model=item.model;hit.id=item.id;
    hit.node=node;hit.face=0;hit.distance=length+1;hit.area=0;
    for(int y=0;y<info.cpheight-1;y++) for(int x=0;x<info.cpwidth-1;x++) {
        int index=y*info.cpwidth+x;
        for(int triangle=0;triangle<2;triangle++) {
            vector a=mesh[index].xyz,b=mesh[index+1].xyz,c=mesh[index+1+info.cpwidth].xyz;
            if(triangle) {b=c;c=mesh[index+info.cpwidth].xyz;}
            static vector vertices[3];vertices[0]=a;vertices[1]=b;vertices[2]=c;
            hit.area+=fb_pick_area(vertices,3);
            float distance=fb_pick_triangle(start,direction,a,b,c);
            if(distance>=0 && distance<hit.distance) {
                hit.distance=distance;hit.point=start+direction*distance;
                hit.normal=normalize(crossproduct(b-a,c-a));
            }
        }
    }
    if(hit.distance<=length) fb_pick_add(hit);
    memfree(mesh);
}
void fb_pick_entity(int node,vector start,vector direction,float length)
{
    fb_node_t item=fb_nodes[node];
    if(!item.fields || substring(item.fields["model"],0,1)=="*") return;
    static vector bounds[2];
    if(!fb_entity_bounds(item.id,bounds)) return;
    float distance=fb_pick_box(start,direction,bounds[0],bounds[1],length);
    if(distance<0) return;
    fb_hit_t hit={0};hit.kind=FB_HIT_ENTITY;hit.node=node;hit.id=item.id;
    hit.model=0;hit.face=0;hit.distance=distance;hit.point=start+direction*distance;
    hit.normal='0 0 0';
    int axis=0;
    for(int a=1;a<3;a++) if(fabs(direction[a])>fabs(direction[axis])) axis=a;
    hit.normal[axis]=direction[axis]>0?-1:1;
    int u=(axis+1)%3,v=(axis+2)%3;
    static vector corners[4];
    for(int i=0;i<4;i++) {
        corners[i]=bounds[0];
        corners[i][u]=(i==1 || i==2)?bounds[1][u]:bounds[0][u];
        corners[i][v]=i>=2?bounds[1][v]:bounds[0][v];
    }
    hit.area=fb_pick_area(corners,4);fb_pick_add(hit);
}
int fb_pick_query(vector start,vector end,vector pixel,int flags,int kinds,int orthographic)
{
    fb_document_tick();fb_filter_prepare();fb_hit_count=0;
    vector delta=end-start;
    float length=vlen(delta);
    if(length<0.000001) return 0;
    vector direction=delta/length;
    for(int node=0;node<fb_node_count;node++) {
        if(!fb_pick_allowed(node,flags)) continue;
        int kind=fb_nodes[node].kind;
        if(kind==FB_NODE_BRUSH && (kinds&FB_HIT_BRUSH)) fb_pick_brush(node,start,direction,length);
        else if(kind==FB_NODE_PATCH && (kinds&FB_HIT_PATCH)) fb_pick_patch(node,start,direction,length);
        else if(kind==FB_NODE_ENTITY && (kinds&FB_HIT_ENTITY)) fb_pick_entity(node,start,direction,length);
    }
    if(kinds&FB_HIT_HANDLE) for(int h=0;h<fb_pick_handle_count;h++) {
        fb_handle_t handle=fb_handles[h];
        if(!fb_pick_allowed(handle.node,flags)) continue;
        float depth=(handle.point-start)*direction;
        if(depth<0 || depth>length) continue;
        vector screen=project(handle.point)-pixel;screen_z=0;
        if(vlen(screen)>handle.radius) continue;
        fb_hit_t hit={0};hit.kind=FB_HIT_HANDLE;hit.model=0;hit.id=handle.id;
        hit.face=0;hit.node=handle.node;hit.distance=depth;
        hit.point=handle.point;hit.normal='0 0 0';hit.area=0;fb_pick_add(hit);
    }
    // Stable insertion sort preserves document order for exact ties.
    for(int i=1;i<fb_hit_count;i++) {
        fb_hit_t value=fb_hits[i];int j=i-1;
        while(j>=0) {
            fb_hit_t previous=fb_hits[j];
            int before=FALSE;
            if((value.kind==FB_HIT_HANDLE)!=(previous.kind==FB_HIT_HANDLE)) before=value.kind==FB_HIT_HANDLE;
            else if(orthographic && value.kind!=FB_HIT_HANDLE && value.area!=previous.area) before=value.area<previous.area;
            else before=value.distance<previous.distance;
            if(!before) break;
            fb_hits[j+1]=previous;j--;
        }
        fb_hits[j+1]=value;
    }
    return fb_hit_count;
}
int fb_pick(vector pixel)
{
    vector start=unproject(pixel),end=unproject(pixel+'0 0 8192');
    if(cvar("ca_brush_view")) {end=unproject(pixel+'0 0 1');}
    return fb_pick_query(start,end,pixel,0,
        FB_HIT_BRUSH|FB_HIT_PATCH|FB_HIT_ENTITY|FB_HIT_HANDLE,cvar("ca_brush_view")!=0);
}
void fb_pick_draw_entities(int labelsOnly)
{
    static vector bounds[2],corners[8];
    cvar_set("ftebroom_selected_entity","");
    string entityWire="ftebroom/entitybounds";
    if(cvar("ca_brush_view")) entityWire="ftebroom/entitybounds2d";
    shaderforname(entityWire,strcat("{\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\n",cvar("ca_brush_view")?"nodepthtest\n":"","}\n}"));
    shaderforname("ftebroom/entitysolid","{\ncull none\nsort opaque\n{\nmap $whiteimage\nrgbgen vertex\ndepthwrite\n}\n}");
    fb_document_lists_prepare();
    for(int entry=0;entry<fb_point_node_count;entry++) {
        int node=fb_point_nodes[entry];fb_node_t item=fb_nodes[node];
        if(!fb_render_allowed(node,FB_PICK_LOCKED)) continue;
        if(substring(item.fields["model"],0,1)=="*") continue;
        if(!fb_entity_bounds(item.id,bounds)) continue;
        if(fb_transform_replaces(0,item.id)) continue;
        vector colour=fb_entity_color(item.id);
        if(!fb_render_allowed(node,0))colour=stov(cvar_string("ftebroom_color_locked"));
        if(fb_entity_selected(item.id)) {
            colour=stov(cvar_string("ftebroom_color_selected"));cvar_set("ftebroom_selected_entity",item.name);
        }
        for(int i=0;i<8;i++) for(int axis=0;axis<3;axis++)
            corners[i][axis]=(i&(1i<<axis))?bounds[1][axis]:bounds[0][axis];
        if(!labelsOnly && !cvar("ca_brush_view") && cvar("ftebroom_show_models"))fb_entity_draw_model(item.id);
        // Point entities remain visible even when their game model is unavailable.
        if(!labelsOnly && !cvar("ca_brush_view"))if(!cvar("ftebroom_show_models") || fb_entity_model_name(item.id)=="") {
            for(int axis=0;axis<3;axis++)for(int side=0;side<2;side++) {
                int u=(axis+1)%3,v=(axis+2)%3,base=side<<axis;
                int faceCorners[4];faceCorners[0]=base;faceCorners[1]=base|(1i<<u);faceCorners[2]=base|(1i<<u)|(1i<<v);faceCorners[3]=base|(1i<<v);
                float shade=0.65+0.15*(float)axis;
                R_BeginPolygon("ftebroom/entitysolid",3);
                for(int c=0;c<4;c++)R_PolygonVertex(corners[faceCorners[c]],'0 0',colour*shade,1);
                R_EndPolygon();
            }
        }
        if(!labelsOnly && (cvar("ftebroom_show_entity_bounds") || cvar("ca_brush_view"))) for(int vertex=0;vertex<8;vertex++) for(int a=0;a<3;a++) {
            int other=vertex^(1i<<a);
            if(other<vertex) continue;
            R_BeginPolygon(entityWire);
            R_PolygonVertex(corners[vertex],'0 0',colour,1);
            R_PolygonVertex(corners[other],'0 0',colour,1);R_EndPolygon();
        }
        if(labelsOnly && cvar("ftebroom_show_entity_names") && !fb_view_render_cache) {
            makevectors(getproperty(VF_ANGLES));
            vector label=(bounds[0]+bounds[1])*.5;label_z=bounds[1][2]+12;
            if(cvar("ca_brush_view") || (label-getproperty(VF_ORIGIN))*v_forward>0) {
                int visible=TRUE;
                if(!cvar("ca_brush_view")){traceline(getproperty(VF_ORIGIN),label,TRUE,world);visible=trace_fraction>=.999;}
                if(visible)drawstring(project(label),item.name,'8 8 0',colour,1,0);
            }
        }
    }
}
float fb_pick_segment_distance(vector pixel,vector a,vector b)
{
    a=project(a);b=project(b);a_z=b_z=pixel_z=0;
    vector edge=b-a;
    float length=edge*edge;
    float t=length>0?bound(0,(pixel-a)*edge/length,1):0;
    return vlen(pixel-a-edge*t);
}
// Return a selected back-facing face close to an edge, even when the ray misses.
int fb_pick_near_face(vector pixel)
{
    fb_document_tick();
    vector start=unproject(pixel),direction=normalize(unproject(pixel+'0 0 8192')-start);
    float nearest=8;
    fb_hit_t best={0};int found=FALSE;
    static vector points[256];
    for(int node=0;node<fb_node_count;node++) {
        fb_node_t item=fb_nodes[node];
        if(item.kind!=FB_NODE_BRUSH || !fb_pick_allowed(node,0)) continue;
        if(!brush_isselected(item.model,item.id)) continue;
        int count=brush_get(item.model,item.id,__NULL__,0,__NULL__);
        brushface_t *faces=memalloc(sizeof(brushface_t)*count);
        brush_get(item.model,item.id,faces,count,__NULL__);
        for(int f=0;f<count;f++) {
            if(faces[f].planenormal*direction<=0) continue;
            int n=brush_getfacepoints(item.model,item.id,f+1,points,points.length);
            for(int e=0;e<n;e++) {
                int nextEdge=e+1;if(nextEdge>=n) nextEdge=0;
                if(((points[e]+points[nextEdge])*0.5-start)*direction<=0) continue;
                float distance=fb_pick_segment_distance(pixel,points[e],points[nextEdge]);
                if(distance>nearest) continue;
                nearest=distance;found=TRUE;
                best.kind=FB_HIT_BRUSH;best.model=item.model;best.id=item.id;best.node=node;
                best.face=f+1;best.normal=faces[f].planenormal;
                best.point=points[e];best.distance=vlen(best.point-start);best.area=fb_pick_area(points,n);
            }
        }
        memfree(faces);
    }
    fb_hit_count=0;if(found) fb_pick_add(best);
    return found;
}
