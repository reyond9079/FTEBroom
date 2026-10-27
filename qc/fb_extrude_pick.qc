// SPDX-License-Identifier: GPL-2.0-or-later
// Cached convex brush silhouettes for Shift-drag face picking.
int fb_visibility_epoch,fb_filter_epoch;
typedef struct {int node,model,id,a,b;vector first,last,na,nb;} fb_extrude_edge_t;
fb_extrude_edge_t *fb_extrude_edges;int fb_extrude_edge_count,fb_extrude_edge_capacity;
hashtable fb_extrude_candidates;
int fb_extrude_plane_support;
vector fb_extrude_support_normal;
var float autocvar_ftebroom_extrude_handle_radius=3;
void fb_extrude_cache()
{
    static string previous="";
    string key=sprintf("%i %i %i %i %i",fb_grid_geometry_epoch,fb_visibility_epoch,fb_filter_epoch,fb_selection_epoch,fb_open_group);
    key=strcat(key," ",itos(fb_incremental_epoch));
    if(key==previous)return;strunzone(previous);previous=strzone(key);fb_extrude_edge_count=0;
    if(fb_extrude_candidates)hash_destroytab(fb_extrude_candidates);fb_extrude_candidates=hash_createtab(512,EV_INTEGER);
    static brushface_t faces[64];static vector points[MAX_FACEPOINTS];
    for(int node=2;node<fb_node_count;node++) {
        fb_node_t item=fb_nodes[node];
        if(item.kind!=FB_NODE_BRUSH || !brush_isselected(item.model,item.id) || !fb_render_allowed(node,0))continue;
        int count=brush_get(item.model,item.id,faces,64,__NULL__);if(count<4)continue;count=min(count,64);
        fb_extrude_candidates[sprintf("%i:%i",item.model,item.id)]=1i;
        int first=fb_extrude_edge_count;
        for(int f=0;f<count;f++) {
            int vertices=brush_getfacepoints(item.model,item.id,f+1,points,MAX_FACEPOINTS);
            if(vertices<3 || vertices>MAX_FACEPOINTS)continue;
            for(int p=0;p<vertices;p++) {
                vector a=points[p],b=points[p+1==vertices?0:p+1];int matched=FALSE;
                for(int e=first;e<fb_extrude_edge_count;e++) {
                    fb_extrude_edge_t *edge=&fb_extrude_edges[e];
                    if((vlen(a-edge.first)<0.03125 && vlen(b-edge.last)<0.03125) || (vlen(b-edge.first)<0.03125 && vlen(a-edge.last)<0.03125)) {
                        edge.b=f+1;edge.nb=faces[f].planenormal;matched=TRUE;break;
                    }
                }
                if(matched)continue;
                if(fb_extrude_edge_count==fb_extrude_edge_capacity) {
                    int capacity=max(256,fb_extrude_edge_capacity*2);
                    fb_extrude_edge_t *edges=memalloc(capacity*sizeof(fb_extrude_edge_t));
                    memcpy(edges,fb_extrude_edges,fb_extrude_edge_count*sizeof(fb_extrude_edge_t));memfree(fb_extrude_edges);
                    fb_extrude_edges=edges;fb_extrude_edge_capacity=capacity;
                }
                fb_extrude_edge_t *added=&fb_extrude_edges[fb_extrude_edge_count++];
                added.node=node;added.model=item.model;added.id=item.id;added.a=f+1;added.b=0;
                added.first=a;added.last=b;added.na=faces[f].planenormal;
            }
        }
    }
}
float fb_extrude_edge_distance(vector start,vector ray,vector a,vector b,vector *point)
{
    vector segment=b-a,offset=start-a;float length2=segment*segment;
    if(length2<0.000001){*point=a;return vlen(start+ray*max(0,(a-start)*ray)-a);}
    float alignment=ray*segment,denominator=length2-alignment*alignment;
    float t=0;if(denominator>0.000001)t=max(0,(alignment*(segment*offset)-length2*(ray*offset))/denominator);
    float u=bound(0,(segment*offset+t*alignment)/length2,1);
    t=max(0,(a+segment*u-start)*ray);
    u=bound(0,(segment*offset+t*alignment)/length2,1);
    *point=a+segment*u;return vlen(start+ray*t-*point);
}
int fb_extrude_seam(int index)
{
    fb_extrude_edge_t edge=fb_extrude_edges[index];vector direction=normalize(edge.last-edge.first);float length=vlen(edge.last-edge.first);
    for(int i=0;i<fb_extrude_edge_count;i++) {
        fb_extrude_edge_t other=fb_extrude_edges[i];if(other.node==edge.node)continue;
        if(vlen(crossproduct(other.first-edge.first,direction))>0.03125 || vlen(crossproduct(other.last-edge.first,direction))>0.03125)continue;
        float a=(other.first-edge.first)*direction,b=(other.last-edge.first)*direction;
        if(min(length,max(a,b))-max(0,min(a,b))>0.001)return TRUE;
    }
    return FALSE;
}
int fb_extrude_silhouette_pick(vector pixel,int view)
{
    if(fb_seen_revision!=fb_document_revision())fb_document_tick();
    fb_extrude_cache();fb_hit_count=0;fb_extrude_plane_support=FALSE;
    vector start=unproject(pixel),ray=normalize(unproject(pixel+'0 0 8192')-start);
    if(view){fb_view_axes(view);ray=-fb_view_normal;}
    // Trace selected brushes only. An exiting face means the ray starts inside.
    int previous=-1;
    for(int i=0;i<fb_extrude_edge_count;i++) {
        int node=fb_extrude_edges[i].node;if(node==previous)continue;previous=node;
        fb_pick_brush(node,start,ray,10000000);
    }
    fb_hit_t direct={0};int hasDirect=FALSE;
    for(int h=0;h<fb_hit_count;h++)if(fb_hits[h].normal*ray<0 && (!hasDirect || fb_hits[h].distance<direct.distance)) {direct=fb_hits[h];hasDirect=TRUE;}
    int best=-1,seam=-1;float nearest=100000000,seamDistance=100000000;vector bestPoint='0 0 0',seamPoint='0 0 0';
    for(int e=0;e<fb_extrude_edge_count;e++) {
        fb_extrude_edge_t edge=fb_extrude_edges[e];if(!edge.b)continue;
        float da=edge.na*ray,db=edge.nb*ray;
        if((da<0)==(db<0) || (fabs(da)<0.000001 && fabs(db)<0.000001))continue;
        vector point;float distance=fb_extrude_edge_distance(start,ray,edge.first,edge.last,&point);
        if(distance<nearest){nearest=distance;best=e;bestPoint=point;}
        if(distance>=seamDistance || fb_pick_segment_distance(pixel,edge.first,edge.last)>autocvar_ftebroom_extrude_handle_radius)continue;
        if(hasDirect && !(direct.node==edge.node && (direct.face==edge.a || direct.face==edge.b)) && (point-start)*ray>direct.distance+0.001)continue;
        if(fb_extrude_seam(e)){seam=e;seamDistance=distance;seamPoint=point;}
    }
    fb_hit_count=0;
    if(seam>=0){best=seam;bestPoint=seamPoint;}
    else if(hasDirect){if(view)return FALSE;fb_pick_add(direct);return TRUE;}
    if(best<0)return FALSE;
    fb_extrude_edge_t chosen=fb_extrude_edges[best];float da=chosen.na*ray,db=chosen.nb*ray;
    int useA=da>db;if(view){if(fabs(da)<0.000001)useA=TRUE;else if(fabs(db)<0.000001)useA=FALSE;}
    fb_hit_t hit={0};hit.kind=FB_HIT_BRUSH;hit.node=chosen.node;hit.model=chosen.model;hit.id=chosen.id;
    hit.face=useA?chosen.a:chosen.b;hit.normal=useA?chosen.na:chosen.nb;hit.point=bestPoint;
    fb_extrude_plane_support=TRUE;fb_extrude_support_normal=view?ray:(useA?chosen.nb:chosen.na);
    fb_pick_add(hit);return TRUE;
}
// A positive overlap on every separating axis excludes edge-only contact.
int fb_extrude_overlap(vector *a,int na,vector *b,int nb,vector normal)
{
    for(int pass=0;pass<2;pass++) {
        vector *polygon=a;int count=na;if(pass){polygon=b;count=nb;}
        for(int i=0;i<count;i++) {
            vector axis=normalize(crossproduct(normal,polygon[i+1==count?0:i+1]-polygon[i]));if(vlen(axis)<0.5)continue;
            float amin=a[0]*axis,amax=amin,bmin=b[0]*axis,bmax=bmin;
            for(int p=1;p<na;p++){amin=min(amin,a[p]*axis);amax=max(amax,a[p]*axis);}
            for(int p=1;p<nb;p++){bmin=min(bmin,b[p]*axis);bmax=max(bmax,b[p]*axis);}
            if(min(amax,bmax)-max(amin,bmin)<=0.001)return FALSE;
        }
    }
    return TRUE;
}
