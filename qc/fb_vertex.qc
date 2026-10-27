// SPDX-License-Identifier: GPL-2.0-or-later
void fb_patch_preview(patchvert_t *points,patchinfo_t info);
// Handles are shared by position; geometry snapshots remain immutable during a drag.
typedef struct {
    fb_poly_t source,preview;int model,id,contents,changed;int *handles;int *removed;
    patchinfo_t patch;patchvert_t *control;patchvert_t *controlPreview;int controlCount;
} fb_vertex_brush_t;
typedef struct {vector point;int selected,collapsed;} fb_vertex_handle_t;
fb_vertex_brush_t *fb_vertex_brushes;
fb_vertex_handle_t fb_vertex_handles[2048];
int fb_vertex_brushcount,fb_vertex_count,fb_vertex_mode,fb_vertex_epoch;
int fb_vertex_drag,fb_vertex_hover,fb_vertex_view,fb_vertex_alt;
vector fb_vertex_pixel,fb_vertex_lastpixel,fb_vertex_anchor,fb_vertex_delta,fb_vertex_segment;
int fb_vertex_preview_valid,fb_vertex_preview_epoch;
int fb_vertex_adding,fb_vertex_add_hover;
vector fb_vertex_add_point;
fb_affine_t fb_vertex_matrix;
int fb_vertex_matrix_enabled;
int fb_vertex_tool() {int tool=cvar("ftebroom_tool");return tool>=FB_TOOL_VERTEX && tool<=FB_TOOL_CONTROLPOINT;}
void fb_vertex_clear()
{
    for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_poly_free(&fb_vertex_brushes[b].source);fb_poly_free(&fb_vertex_brushes[b].preview);memfree(fb_vertex_brushes[b].handles);memfree(fb_vertex_brushes[b].removed);
        if(fb_vertex_brushes[b].control)strunzone(fb_vertex_brushes[b].patch.shadername);
        memfree(fb_vertex_brushes[b].control);memfree(fb_vertex_brushes[b].controlPreview);
    }
    memfree(fb_vertex_brushes);fb_vertex_brushes=__NULL__;fb_vertex_brushcount=fb_vertex_count=0;
    fb_vertex_drag=fb_vertex_preview_valid=fb_vertex_adding=fb_vertex_add_hover=fb_vertex_matrix_enabled=0;fb_vertex_epoch=-1;
}
int fb_vertex_handle(vector point)
{
    for(int h=0;h<fb_vertex_count;h++) if(vlen(fb_vertex_handles[h].point-point)<FB_GEOM_MERGE) return h;
    if(fb_vertex_count==fb_vertex_handles.length) return -1;
    int result=fb_vertex_count++;fb_vertex_handles[result].point=point;fb_vertex_handles[result].selected=FALSE;fb_vertex_handles[result].collapsed=FALSE;return result;
}
void fb_vertex_rebuild()
{
    int mode=fb_vertex_mode;fb_vertex_clear();fb_vertex_mode=cvar("ftebroom_tool");if(fb_vertex_scope) fb_vertex_mode=mode;fb_vertex_epoch=history;
    // Apply/Undo callers also rebuild in ordinary object mode. No geometry
    // handles exist there; treating vertex counts as face counts reads past faces.
    if(fb_vertex_mode<FB_TOOL_VERTEX||fb_vertex_mode>FB_TOOL_CONTROLPOINT)return;
    fb_vertex_brushes=memalloc(sizeof(fb_vertex_brush_t)*selectedbrushcount);
    for(int b=0;b<selectedbrushcount;b++) {
        fb_vertex_brush_t *item=&fb_vertex_brushes[fb_vertex_brushcount];
        item->model=selectedbrushes[b].model;item->id=selectedbrushes[b].id;
        if(fb_vertex_mode==FB_TOOL_CONTROLPOINT) {
            int cp=patch_getcp(item->model,item->id,__NULL__,0,&item->patch);if(!cp)continue;
            item->controlCount=cp;item->control=memalloc(cp*sizeof(patchvert_t));item->controlPreview=memalloc(cp*sizeof(patchvert_t));
            patch_getcp(item->model,item->id,item->control,cp,&item->patch);item->patch.shadername=strzone(item->patch.shadername);
            item->handles=memalloc(cp*sizeof(int));fb_vertex_brushcount++;
            for(int h=0;h<cp;h++) {
                item->handles[h]=fb_vertex_handle(item->control[h].xyz);
                if(item->handles[h]<0){fb_vertex_clear();cvar_set("ftebroom_selection_status","Too many control point handles (limit 2048)");return;}
            }
            continue;
        }
        if(!fb_poly_from_brush(item->model,item->id,&item->source)) continue;
        fb_vertex_brushcount++;brush_get(item->model,item->id,__NULL__,0,&item->contents);
        fb_poly_t *poly=&item->source;int count=poly->nv;
        if(fb_vertex_mode==FB_TOOL_EDGE) count=poly->ne;
        if(fb_vertex_mode==FB_TOOL_FACE) count=poly->nf;
        item->handles=memalloc(count*sizeof(int));item->removed=memalloc(poly->nv*sizeof(int));
        for(int h=0;h<count;h++) {
            vector point='0 0 0';
            if(fb_vertex_mode==FB_TOOL_VERTEX) point=poly->vertices[h];
            else if(fb_vertex_mode==FB_TOOL_EDGE) point=(poly->vertices[poly->edges[h].a]+poly->vertices[poly->edges[h].b])*0.5;
            else {fb_poly_face_t face=poly->faces[h];for(int v=0;v<face.count;v++) point+=poly->vertices[poly->indices[face.start+v]];point/=(float)face.count;}
            item->handles[h]=fb_vertex_handle(point);
            if(item->handles[h]<0) {fb_vertex_clear();cvar_set("ftebroom_selection_status","Too many geometry handles (limit 2048)");return;}
        }
    }
}
int fb_vertex_selected(int h) {if(h<0 || h>=fb_vertex_count) return FALSE;return fb_vertex_handles[h].selected;}
int fb_vertex_membership(fb_vertex_brush_t *item,int v)
{
    fb_poly_t *poly=&item->source;
    if(fb_vertex_mode==FB_TOOL_VERTEX) return fb_vertex_selected(item->handles[v]);
    if(fb_vertex_mode==FB_TOOL_EDGE) {
        for(int e=0;e<poly->ne;e++) if(fb_vertex_selected(item->handles[e]) && (poly->edges[e].a==v || poly->edges[e].b==v)) return TRUE;
    } else for(int f=0;f<poly->nf;f++) if(fb_vertex_selected(item->handles[f])) {
        fb_poly_face_t face=poly->faces[f];for(int p=0;p<face.count;p++) if(poly->indices[face.start+p]==v) return TRUE;
    }
    return FALSE;
}
void fb_vertex_deselect() {for(int h=0;h<fb_vertex_count;h++) fb_vertex_handles[h].selected=FALSE;fb_handle_count=0;}
int fb_vertex_selection_count()
{
    int count=0;for(int h=0;h<fb_vertex_count;h++) if(fb_vertex_handles[h].selected) count++;
    fb_handle_count=count;cvar_set("ftebroom_handle_count",itos(count));return count;
}
// Match surviving surfaces by their original vertices, including newly triangulated faces.
void fb_vertex_materials(fb_vertex_brush_t *item,fb_poly_t *result,vector delta)
{
    fb_poly_t *source=&item->source;
    for(int f=0;f<result->nf;f++) {
        fb_poly_face_t face=result->faces[f];int best=0,bestMatches=-1;float bestNormal=-2;
        for(int s=0;s<source->nf;s++) {
            int matches=0;
            for(int p=0;p<face.count;p++) {
                vector point=result->vertices[result->indices[face.start+p]];int found=FALSE;
                for(int v=0;v<source->nv;v++) {
                    vector moved=source->vertices[v];if(fb_vertex_membership(item,v)) {
                        if(fb_vertex_matrix_enabled) moved=fb_affine_point(&fb_vertex_matrix,moved);else moved+=delta;
                    }
                    if(vlen(point-moved)>=FB_GEOM_MERGE) continue;
                    if(fabs(source->vertices[v]*source->faces[s].attr.planenormal-source->faces[s].attr.planedist)<FB_GEOM_PLANE) found=TRUE;
                }
                if(found) matches++;
            }
            float normal=source->faces[s].attr.planenormal*face.attr.planenormal;
            if(matches>bestMatches || (matches==bestMatches && normal>bestNormal)) {best=s;bestMatches=matches;bestNormal=normal;}
        }
        brushface_t *out=&result->faces[f].attr;vector normal=out->planenormal;float dist=out->planedist;
        strunzone(out->shadername);*out=source->faces[best].attr;out->shadername=strzone(source->faces[best].attr.shadername);
        out->planenormal=normal;out->planedist=dist;
        if(cvar("ftebroom_uv_lock")) {
            // Fit texture coordinates to corresponding source vertices on this
            // face. This preserves stationary UVs and carries moved corners.
            vector oldPoints[3],newPoints[3];int paired=0;
            for(int v=0;v<source->nv && paired<3;v++) {
                vector original=source->vertices[v];
                if(fabs(original*source->faces[best].attr.planenormal-source->faces[best].attr.planedist)>=FB_GEOM_PLANE)continue;
                vector moved=original;if(fb_vertex_membership(item,v)) {
                    if(fb_vertex_matrix_enabled)moved=fb_affine_point(&fb_vertex_matrix,moved);else moved+=delta;
                }
                int onFace=FALSE;for(int p=0;p<face.count;p++)if(vlen(result->vertices[result->indices[face.start+p]]-moved)<FB_GEOM_MERGE)onFace=TRUE;
                if(!onFace)continue;
                if(paired==2)if(vlen(crossproduct(newPoints[1]-newPoints[0],moved-newPoints[0]))<0.001)continue;
                oldPoints[paired]=original;newPoints[paired++]=moved;
            }
            if(paired==3) {
                vector uvEdgeU=newPoints[1]-newPoints[0],uvEdgeV=newPoints[2]-newPoints[0],crossed=crossproduct(uvEdgeU,uvEdgeV);
                float denominator=crossed*crossed;
                if(denominator>0.000001) {
                    vector dualU=crossproduct(uvEdgeV,crossed)/denominator,dualV=crossproduct(crossed,uvEdgeU)/denominator;
                    vector uvAxisS=out->sdir,uvAxisT=out->tdir;
                    out->sdir=dualU*((oldPoints[1]-oldPoints[0])*uvAxisS)+dualV*((oldPoints[2]-oldPoints[0])*uvAxisS);
                    out->tdir=dualU*((oldPoints[1]-oldPoints[0])*uvAxisT)+dualV*((oldPoints[2]-oldPoints[0])*uvAxisT);
                    out->sbias+=oldPoints[0]*uvAxisS-newPoints[0]*out->sdir;
                    out->tbias+=oldPoints[0]*uvAxisT-newPoints[0]*out->tdir;
                }
            }
        }
    }
}
int fb_vertex_build(vector delta,int remove,float snap)
{
    if(fb_vertex_mode==FB_TOOL_CONTROLPOINT) {
        if(remove)return FALSE;
        for(int b=0;b<fb_vertex_brushcount;b++) {
            fb_vertex_brush_t *item=&fb_vertex_brushes[b];
            for(int p=0;p<item->controlCount;p++) {
                item->controlPreview[p]=item->control[p];vector point=item->control[p].xyz;
                if(fb_vertex_selected(item->handles[p])) {
                    if(fb_vertex_matrix_enabled)point=fb_affine_point(&fb_vertex_matrix,point);else point+=delta;
                    if(snap>0)for(int axis=0;axis<3;axis++)point[axis]=rint(point[axis]/snap)*snap;
                }
                if(!fb_geom_point(point))return FALSE;
                item->controlPreview[p].xyz=point;
            }
        }
        fb_vertex_delta=delta;fb_vertex_preview_valid=TRUE;return TRUE;
    }
    fb_poly_t *results=memalloc(sizeof(fb_poly_t)*fb_vertex_brushcount);int *affected=memalloc(sizeof(int)*fb_vertex_brushcount);int valid=TRUE;
    for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_vertex_brush_t *item=&fb_vertex_brushes[b];fb_poly_t *source=&item->source;
        vector *points=memalloc(sizeof(vector)*(source->nv+1));int count=0,touches=snap>0;
        for(int v=0;v<source->nv;v++) {
            if(item->removed[v])continue;
            int selected=fb_vertex_membership(item,v);if(selected)touches=TRUE;
            if(remove && selected) continue;
            vector point=source->vertices[v];if(selected) {
                if(fb_vertex_matrix_enabled) point=fb_affine_point(&fb_vertex_matrix,point);else point+=delta;
            }
            if(snap>0) for(int axis=0;axis<3;axis++) point[axis]=rint(point[axis]/snap)*snap;
            points[count++]=point;
        }
        if(fb_vertex_adding) {
            int inside=TRUE,onSurface=FALSE;
            for(int f=0;f<source->nf;f++) {
                float d=fb_vertex_add_point*source->faces[f].attr.planenormal-source->faces[f].attr.planedist;
                if(d>FB_GEOM_PLANE) inside=FALSE;if(fabs(d)<FB_GEOM_PLANE) onSurface=TRUE;
            }
            if(inside && onSurface){points[count++]=fb_vertex_add_point+delta;touches=TRUE;}
        }
        if(!touches){memfree(points);continue;}affected[b]=TRUE;
        int ok=fb_hull(points,count,&results[b]);
        if(ok && !remove && !snap && fb_vertex_mode!=FB_TOOL_VERTEX && results[b].nv!=source->nv) ok=FALSE;
        if(ok) fb_vertex_materials(item,&results[b],delta);
        memfree(points);if(!ok) {valid=FALSE;break;}
    }
    if(valid) for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_vertex_brushes[b].changed=affected[b];
        fb_poly_free(&fb_vertex_brushes[b].preview);fb_vertex_brushes[b].preview=results[b];
        memfill8((__variant*)&results[b],0,sizeof(fb_poly_t));
    }
    for(int b=0;b<fb_vertex_brushcount;b++) fb_poly_free(&results[b]);memfree(results);memfree(affected);
    if(valid) {
        fb_vertex_delta=delta;fb_vertex_preview_valid=TRUE;fb_vertex_preview_epoch++;
        if(fb_vertex_drag==1 && fb_vertex_mode==FB_TOOL_VERTEX && !fb_vertex_matrix_enabled && !fb_vertex_adding) {
            for(int h=0;h<fb_vertex_count;h++)fb_vertex_handles[h].collapsed=TRUE;
            for(int b=0;b<fb_vertex_brushcount;b++) {
                fb_vertex_brush_t *item=&fb_vertex_brushes[b];
                if(!item->changed){for(int v=0;v<item->source.nv;v++)fb_vertex_handles[item->handles[v]].collapsed=FALSE;continue;}
                for(int v=0;v<item->source.nv;v++) {
                    if(item->removed[v])continue;
                    vector point=item->source.vertices[v];if(fb_vertex_membership(item,v))point+=delta;
                    // Once a moving vertex reaches a stationary one, keep only the moving
                    // handle for the rest of this gesture. Escape restores both originals.
                    int merged=FALSE;
                    if(!fb_vertex_membership(item,v))for(int m=0;m<item->source.nv;m++) {
                        if(item->removed[m] || !fb_vertex_membership(item,m))continue;
                        if(vlen(item->source.vertices[m]+delta-point)<FB_GEOM_MERGE)merged=TRUE;
                    }
                    int survives=FALSE;
                    if(!merged)for(int r=0;r<item->preview.nv;r++)if(vlen(item->preview.vertices[r]-point)<FB_GEOM_MERGE)survives=TRUE;
                    if(survives)fb_vertex_handles[item->handles[v]].collapsed=FALSE;else item->removed[v]=TRUE;
                }
            }
        }
    }
    return valid;
}
void fb_vertex_commit(string label)
{
    if(!fb_vertex_preview_valid) return;
    vector *selected=memalloc((fb_vertex_count+1)*sizeof(vector));int selectedCount=0;
    for(int h=0;h<fb_vertex_count;h++) if(fb_vertex_handles[h].selected) {
        vector point=fb_vertex_handles[h].point+fb_vertex_delta;
        if(fb_vertex_matrix_enabled) point=fb_affine_point(&fb_vertex_matrix,fb_vertex_handles[h].point);
        selected[selectedCount++]=point;
    }
    if(fb_vertex_adding) selected[selectedCount++]=fb_vertex_add_point+fb_vertex_delta;
    fb_tx_begin(label);int ok=TRUE;
    for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_vertex_brush_t *item=&fb_vertex_brushes[b];static brushface_t faces[64];
        if(!item->controlCount&&!item->changed)continue;
        if(item->controlCount) {
            if(!patch_history_edit(item->model,item->id,item->controlPreview,item->patch)){ok=FALSE;break;}
            continue;
        }
        for(int f=0;f<item->preview.nf;f++) faces[f]=item->preview.faces[f].attr;
        if(!brush_history_edit(item->model,item->id,faces,item->preview.nf,item->contents)) {ok=FALSE;break;}
    }
    if(ok) fb_tx_commit();else fb_tx_cancel();fb_vertex_rebuild();
    for(int h=0;h<fb_vertex_count;h++) for(int s=0;s<selectedCount;s++) if(vlen(fb_vertex_handles[h].point-selected[s])<FB_GEOM_MERGE) fb_vertex_handles[h].selected=TRUE;
    memfree(selected);fb_vertex_selection_count();
}
void fb_vertex_cancel()
{
    fb_vertex_drag=fb_vertex_preview_valid=fb_vertex_adding=fb_vertex_matrix_enabled=FALSE;fb_vertex_delta=fb_vertex_segment='0 0 0';
    for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_poly_free(&fb_vertex_brushes[b].preview);
        if(fb_vertex_brushes[b].removed)memfill8((__variant*)fb_vertex_brushes[b].removed,0,fb_vertex_brushes[b].source.nv*sizeof(int));
    }
    for(int h=0;h<fb_vertex_count;h++)fb_vertex_handles[h].collapsed=FALSE;
}
int fb_vertex_pick(vector pixel,int view)
{
    if(fb_vertex_mode==FB_TOOL_FACE && !fb_vertex_drag) {
        vector start=unproject(pixel),ray=normalize(unproject(pixel+'0 0 8192')-start);
        float nearest=100000000;int found=-1;
        for(int b=0;b<fb_vertex_brushcount;b++) {
            fb_vertex_brush_t *item=&fb_vertex_brushes[b];fb_poly_t *poly=&item->source;
            for(int f=0;f<poly->nf;f++) {
                brushface_t plane=poly->faces[f].attr;float denom=ray*plane.planenormal;
                if(denom>=-0.00001)continue;float distance=(plane.planedist-start*plane.planenormal)/denom;
                if(distance<0 || distance>=nearest)continue;vector point=start+ray*distance;int inside=TRUE;
                for(int side=0;side<poly->nf;side++)if(point*poly->faces[side].attr.planenormal>poly->faces[side].attr.planedist+.01){inside=FALSE;break;}
                if(inside){nearest=distance;found=item->handles[f];}
            }
        }
        if(found>=0)return found;
    }
    int result=-1;float distance=9;
    for(int h=0;h<fb_vertex_count;h++) {
        if(fb_vertex_handles[h].collapsed)continue;vector point=fb_vertex_handles[h].point;
        if(!view) {makevectors(getproperty(VF_ANGLES));if((point-getproperty(VF_ORIGIN))*v_forward<=0) continue;}
        vector screen=project(point);screen_z=0;float d=vlen(screen-pixel);
        if(d<distance) {distance=d;result=h;}
    }
    return result;
}
int fb_vertex_add_pick(vector pixel,int view)
{
    fb_vertex_add_hover=FALSE;
    if(fb_vertex_mode!=FB_TOOL_VERTEX || !cvar("ftebroom_shift")) return FALSE;
    vector start=unproject(pixel),end=unproject(pixel+'0 0 8192');
    if(!fb_pick_query(start,end,pixel,0,FB_HIT_BRUSH,FALSE)) return FALSE;
    for(int hit=0;hit<fb_hit_count;hit++) if(brush_isselected(fb_hits[hit].model,fb_hits[hit].id)) {
        fb_vertex_add_point=fb_snap_to_plane(fb_hits[hit].point,fb_hits[hit].normal,fb_hits[hit].normal*fb_hits[hit].point);
        fb_vertex_add_hover=TRUE;return TRUE;
    }
    return FALSE;
}
float fb_vertex_vertical_scale,fb_vertex_vertical_fallback;
float fb_vertex_vertical_distance(vector pixel)
{
    if(fb_vertex_vertical_fallback)return -pixel_y*fb_vertex_vertical_scale;
    vector rayStart=unproject(pixel),ray=normalize(unproject(pixel+'0 0 8192')-rayStart),offset=rayStart-fb_vertex_anchor;
    float denominator=1-ray_z*ray_z;if(denominator<0.0001)return 0;
    return (offset_z-ray_z*(ray*offset))/denominator;
}
void fb_vertex_down(vector pixel,int alt,int view)
{
    if(fb_vertex_epoch!=history || fb_vertex_mode!=cvar("ftebroom_tool")) fb_vertex_rebuild();
    int h=fb_vertex_pick(pixel,view),ctrl=cvar("ftebroom_ctrl");
    if(h>=0 && alt && cvar("ftebroom_shift") && fb_vertex_mode==FB_TOOL_VERTEX && fb_vertex_selection_count()) {
        vector origin=fb_vertex_handles[h].point;
        for(int s=0;s<fb_vertex_count;s++) if(fb_vertex_handles[s].selected) {origin=fb_vertex_handles[s].point;break;}
        if(fb_vertex_build(fb_vertex_handles[h].point-origin,FALSE,0)) fb_vertex_commit("Merge Vertices");
        return;
    }
    fb_vertex_pixel=fb_vertex_lastpixel=pixel;fb_vertex_view=view;fb_vertex_alt=alt;fb_vertex_delta=fb_vertex_segment='0 0 0';
    fb_vertex_drag=2;fb_vertex_preview_valid=FALSE;
    if(h>=0) {
        if(ctrl) fb_vertex_handles[h].selected=!fb_vertex_handles[h].selected;
        else if(!fb_vertex_handles[h].selected) {fb_vertex_deselect();fb_vertex_handles[h].selected=TRUE;}
        fb_vertex_anchor=fb_vertex_handles[h].point;fb_vertex_drag=1;
    }
    else if(fb_vertex_add_pick(pixel,view)) {
        fb_vertex_deselect();fb_vertex_adding=TRUE;fb_vertex_anchor=fb_vertex_add_point;fb_vertex_drag=1;
    }
    vector dragRay=normalize(unproject(pixel+'0 0 8192')-unproject(pixel)),viewport=getproperty(VF_SIZE);
    fb_vertex_vertical_fallback=1-dragRay_z*dragRay_z<0.01;
    fb_vertex_vertical_scale=2*max(1,vlen(fb_vertex_anchor-getproperty(VF_ORIGIN)))*tan(getproperty(VF_FOVY)*M_PI/360)/max(1,viewport_y);
    fb_vertex_selection_count();fb_active=TRUE;
}
void fb_vertex_move(vector pixel,int alt,int view)
{
    if(!fb_vertex_tool()) return;
    fb_vertex_hover=fb_vertex_pick(pixel,view);
    if(!fb_vertex_drag) fb_vertex_add_pick(pixel,view);
    if(!fb_vertex_drag || view!=fb_vertex_view) return;
    fb_vertex_lastpixel=pixel;
    if(fb_vertex_drag!=1 || vlen(pixel-fb_vertex_pixel)<3) return;
    if(alt!=fb_vertex_alt) {fb_vertex_segment=fb_vertex_delta;fb_vertex_pixel=pixel;fb_vertex_alt=alt;}
    vector delta='0 0 0';
    if(!view && alt) delta_z=fb_vertex_vertical_distance(pixel)-fb_vertex_vertical_distance(fb_vertex_pixel);
    else delta=fb_view_plane_point(pixel,view,fb_vertex_anchor)-fb_view_plane_point(fb_vertex_pixel,view,fb_vertex_anchor);
    delta=fb_vertex_segment+fb_snap_delta(delta);
    if((fb_vertex_mode==FB_TOOL_VERTEX || fb_vertex_mode==FB_TOOL_CONTROLPOINT) && cvar("ftebroom_ctrl")) delta=fb_snap(fb_vertex_anchor+delta)-fb_vertex_anchor;
    if(view)delta[fb_view_depth_axis(view)]=0;
    if(fb_vertex_mode==FB_TOOL_CONTROLPOINT && cvar("ftebroom_shift")) {
        int axis=0;for(int a=1;a<3;a++)if(fabs(delta[a])>fabs(delta[axis]))axis=a;
        for(int a=0;a<3;a++)if(a!=axis)delta[a]=0;
    }
    if(fb_vertex_preview_valid && delta==fb_vertex_delta)return;
    if(!fb_vertex_build(delta,FALSE,0)) cvar_set("ftebroom_selection_status","Move rejected: brush must remain a valid convex volume");
}
void fb_vertex_up(vector pixel,int alt,int view)
{
    fb_vertex_move(pixel,alt,view);
    if(fb_vertex_drag==2) {
        if(vlen(pixel-fb_vertex_pixel)<3) fb_vertex_deselect();
        else for(int h=0;h<fb_vertex_count;h++) {
            vector point=fb_vertex_handles[h].point;
            if(!view) {makevectors(getproperty(VF_ANGLES));if((point-getproperty(VF_ORIGIN))*v_forward<=0) continue;}
            vector p=project(point);
            if(p_x<min(pixel_x,fb_vertex_pixel_x) || p_x>max(pixel_x,fb_vertex_pixel_x) || p_y<min(pixel_y,fb_vertex_pixel_y) || p_y>max(pixel_y,fb_vertex_pixel_y)) continue;
            if(cvar("ftebroom_ctrl")) fb_vertex_handles[h].selected=TRUE;
            else fb_vertex_handles[h].selected=!fb_vertex_handles[h].selected;
        }
    } else if(fb_vertex_preview_valid && vlen(fb_vertex_delta)>0.0001) fb_vertex_commit("Move Geometry");
    fb_vertex_cancel();fb_active=FALSE;fb_vertex_selection_count();
}
int fb_vertex_replaces(int model,int id)
{
    if(!fb_vertex_preview_valid || (!fb_vertex_tool() && !fb_vertex_scope)) return FALSE;
    for(int b=0;b<fb_vertex_brushcount;b++) if(fb_vertex_brushes[b].model==model && fb_vertex_brushes[b].id==id && (fb_vertex_brushes[b].controlCount||fb_vertex_brushes[b].changed)) return TRUE;
    return FALSE;
}
void fb_vertex_draw()
{
    cvar_set("ftebroom_vertex_handles","0");if(!fb_vertex_tool() && !fb_vertex_scope) return;
    shaderforname("ftebroom/vertexwire","{\ncull none\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\nnodepthtest\n}\n}");
    if(!fb_vertex_drag && (fb_vertex_epoch!=history || (!fb_vertex_scope && fb_vertex_mode!=cvar("ftebroom_tool")))) fb_vertex_rebuild();
    if(fb_vertex_mode==FB_TOOL_CONTROLPOINT) {
        int rows=0,columns=0;
        if(fb_vertex_brushcount){rows=fb_vertex_brushes[0].patch.cpheight;columns=fb_vertex_brushes[0].patch.cpwidth;}
        for(int b=0;b<fb_vertex_brushcount;b++)if(fb_vertex_brushes[b].patch.cpheight!=rows || fb_vertex_brushes[b].patch.cpwidth!=columns){rows=columns=0;break;}
        cvar_set("ftebroom_patch_rows",itos(rows));cvar_set("ftebroom_patch_columns",itos(columns));
    }
    for(int h=0;h<fb_vertex_count;h++) {
        vector point=fb_vertex_handles[h].point;if(fb_vertex_handles[h].selected && fb_vertex_preview_valid) {
            if(fb_vertex_matrix_enabled) point=fb_affine_point(&fb_vertex_matrix,point);else point+=fb_vertex_delta;
        }
        vector p=project(point);int visible=!fb_vertex_handles[h].collapsed;
        if(!cvar("ca_brush_view")) {makevectors(getproperty(VF_ANGLES));if((point-getproperty(VF_ORIGIN))*v_forward<=0)visible=FALSE;}
        cvar_set(sprintf("ftebroom_vertex_handle_%i",h),sprintf("%g %g %i %i %i",p_x,p_y,fb_vertex_handles[h].selected,h==fb_vertex_hover,visible));
    }
    if(fb_vertex_mode!=FB_TOOL_VERTEX)for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_vertex_brush_t *item=&fb_vertex_brushes[b];fb_poly_t *poly=&item->source;
        int count=poly->nf;if(fb_vertex_mode==FB_TOOL_EDGE)count=poly->ne;
        for(int h=0;h<count;h++) {
            int handle=item->handles[h];
            vector color='1 1 0';if(handle==fb_vertex_hover)color='1 0.2 0.2';
            int segments=1;if(fb_vertex_mode==FB_TOOL_FACE)segments=poly->faces[h].count;
            for(int e=0;e<segments;e++) {
                int a,bv;
                if(fb_vertex_mode==FB_TOOL_EDGE){a=poly->edges[h].a;bv=poly->edges[h].b;}
                else {fb_poly_face_t face=poly->faces[h];a=poly->indices[face.start+e];bv=poly->indices[face.start+(e+1)%face.count];}
                vector start=poly->vertices[a],end=poly->vertices[bv];
                if(fb_vertex_preview_valid) {
                    if(fb_vertex_membership(item,a)){if(fb_vertex_matrix_enabled)start=fb_affine_point(&fb_vertex_matrix,start);else start+=fb_vertex_delta;}
                    if(fb_vertex_membership(item,bv)){if(fb_vertex_matrix_enabled)end=fb_affine_point(&fb_vertex_matrix,end);else end+=fb_vertex_delta;}
                }
                R_BeginPolygon("ftebroom/vertexwire");R_PolygonVertex(start,'0 0 0',color,1);R_PolygonVertex(end,'0 0 0',color,1);R_EndPolygon();
            }
        }
    }
    cvar_set("ftebroom_vertex_handles",itos(fb_vertex_count));
    cvar_set("ftebroom_vertex_new","");
    if(fb_vertex_add_hover || fb_vertex_adding) {
        vector added=fb_vertex_add_point;if(fb_vertex_adding && fb_vertex_preview_valid) added+=fb_vertex_delta;
        vector p=project(added);cvar_set("ftebroom_vertex_new",sprintf("%g %g",p_x,p_y));
    }
    cvar_set("ftebroom_vertex_lasso",fb_vertex_drag==2?sprintf("%g %g %g %g",fb_vertex_pixel_x,fb_vertex_pixel_y,fb_vertex_lastpixel_x,fb_vertex_lastpixel_y):"");
    if(fb_vertex_hover>=0 && fb_vertex_hover<fb_vertex_count) {
        vector labelPoint=fb_vertex_handles[fb_vertex_hover].point;
        if(fb_vertex_drag==1 && fb_vertex_preview_valid)labelPoint+=fb_vertex_delta;
        cvar_set("ftebroom_vertex_coordinates",sprintf("%g, %g, %g",labelPoint_x,labelPoint_y,labelPoint_z));
        cvar_set("ftebroom_selection_status",sprintf("Vertex: %v",labelPoint));
        if(fb_vertex_drag==1)for(int axis=0;axis<3;axis++) {
            vector extent='0 0 0';extent[axis]=8192;
            R_BeginPolygon("ftebroom/vertexwire",0,FALSE);R_PolygonVertex(labelPoint-extent,'0 0 0','1 1 0',0.5);R_PolygonVertex(labelPoint+extent,'0 0 0','1 1 0',0.5);R_EndPolygon();
        }
    }
    if(fb_vertex_preview_valid) for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_vertex_brush_t *item=&fb_vertex_brushes[b];
        if(item->controlCount) {
            fb_patch_preview(item->controlPreview,item->patch);
            continue;
        }
        if(!item->changed)continue;
        fb_poly_t *poly=&fb_vertex_brushes[b].preview;static brushface_t faces[64];
        for(int f=0;f<poly->nf;f++) faces[f]=poly->faces[f].attr;
        DrawQCBrushTextured(faces,poly->nf,'1 1 1',1);DrawQCBrushWireframe(faces,poly->nf,"ftebroom/vertexwire",'1 1 0',1);
    }
}
void fb_vertex_delete()
{
    if(fb_vertex_mode==FB_TOOL_CONTROLPOINT){cvar_set("ftebroom_selection_status","Control points cannot be deleted; change Rows or Columns instead");return;}
    if(!fb_vertex_tool() || !fb_vertex_selection_count()) return;
    if(fb_vertex_build('0 0 0',TRUE,0)) fb_vertex_commit("Delete Geometry");
    else cvar_set("ftebroom_selection_status","Cannot delete: a brush would become flat or empty");
}
void fb_vertex_nudge(vector delta)
{
    if(!fb_vertex_selection_count()) return;
    if(fb_vertex_build(delta,FALSE,0)) fb_vertex_commit("Move Geometry");
}
void fb_vertex_snap(float spacing)
{
    if(spacing<=0) return;
    int tool=cvar("ftebroom_tool");fb_tool_refresh();cvar_set("ftebroom_tool",itos(FB_TOOL_VERTEX));fb_vertex_rebuild();
    fb_tx_begin("Snap Vertices");
    for(int b=0;b<fb_vertex_brushcount;b++) {
        fb_vertex_brush_t *item=&fb_vertex_brushes[b];
        vector *points=memalloc(item->source.nv*sizeof(vector));
        for(int v=0;v<item->source.nv;v++) for(int a=0;a<3;a++) points[v][a]=rint(item->source.vertices[v][a]/spacing)*spacing;
        fb_poly_t hull={0};
        if(fb_hull(points,item->source.nv,&hull)) {
            fb_vertex_materials(item,&hull,'0 0 0');static brushface_t faces[64];
            for(int f=0;f<hull.nf;f++) faces[f]=hull.faces[f].attr;
            brush_history_edit(item->model,item->id,faces,hull.nf,item->contents);
        } else print("ReyondBroom: Snap skipped a brush that would become invalid.\n");
        fb_poly_free(&hull);memfree(points);
    }
    fb_tx_commit();cvar_set("ftebroom_tool",itos(tool));fb_vertex_rebuild();
}
void fb_vertex_merge()
{
    if(!fb_vertex_selection_count()) return;
    int capacity=0;for(int b=0;b<fb_vertex_brushcount;b++) capacity+=fb_vertex_brushes[b].source.nv;
    vector *points=memalloc(capacity*sizeof(vector));int count=0;
    for(int b=0;b<fb_vertex_brushcount;b++) for(int v=0;v<fb_vertex_brushes[b].source.nv;v++)
        if(fb_vertex_membership(&fb_vertex_brushes[b],v)) points[count++]=fb_vertex_brushes[b].source.vertices[v];
    fb_poly_t hull={0};int valid=fb_hull(points,count,&hull);memfree(points);
    if(valid) {
        static brushface_t faces[64];for(int f=0;f<hull.nf;f++) {
            faces[f]=hull.faces[f].attr;int found=FALSE;
            for(int b=0;b<fb_vertex_brushcount && !found;b++) for(int s=0;s<fb_vertex_brushes[b].source.nf;s++) {
                if(fb_geom_sameplane(&faces[f],&fb_vertex_brushes[b].source.faces[s].attr)) {faces[f]=fb_vertex_brushes[b].source.faces[s].attr;found=TRUE;break;}
            }
        }
        fb_tx_begin("Convex Merge Geometry");
        for(int b=0;b<fb_vertex_brushcount;b++) brush_history_delete(fb_vertex_brushes[b].model,fb_vertex_brushes[b].id);
        if(brush_history_create(fb_vertex_brushes[0].model,faces,hull.nf,fb_vertex_brushes[0].contents,TRUE)) fb_tx_commit();else fb_tx_cancel();
        fb_vertex_rebuild();
    } else cvar_set("ftebroom_selection_status","Select at least four non-coplanar vertices for Convex Merge");
    fb_poly_free(&hull);
}
int fb_vertex_bounds(vector *bounds)
{
    int count=0;bounds[0]='1000000 1000000 1000000';bounds[1]=-bounds[0];
    if(fb_vertex_mode==FB_TOOL_CONTROLPOINT) {
        for(int h=0;h<fb_vertex_count;h++)if(fb_vertex_handles[h].selected) {
            vector point=fb_vertex_handles[h].point;count++;
            for(int a=0;a<3;a++){bounds[0][a]=min(bounds[0][a],point[a]);bounds[1][a]=max(bounds[1][a],point[a]);}
        }
        return count;
    }
    for(int b=0;b<fb_vertex_brushcount;b++) for(int v=0;v<fb_vertex_brushes[b].source.nv;v++) {
        if(!fb_vertex_membership(&fb_vertex_brushes[b],v)) continue;
        vector point=fb_vertex_brushes[b].source.vertices[v];count++;
        for(int a=0;a<3;a++) {bounds[0][a]=min(bounds[0][a],point[a]);bounds[1][a]=max(bounds[1][a],point[a]);}
    }
    return count;
}
int fb_vertex_affine_command()
{
    vector bounds[2];if(!fb_vertex_bounds(bounds)) return TRUE;
    vector center=(bounds[0]+bounds[1])*0.5,axis='0 0 1';float angle=0;
    string kind=strzone(argv(1));int flip=kind=="fliph" || kind=="flipv";
    if(argv(0)=="ftebroom_rotate") {
        axis=[stof(argv(1)),stof(argv(2)),stof(argv(3))];angle=stof(argv(4));
        if(argv(7)!="") center=[stof(argv(5)),stof(argv(6)),stof(argv(7))];
    } else {
        int view=cvar("ca_brush_view");makevectors(getproperty(VF_ANGLES));
        vector forward=fb_affine_axis(v_forward),right=fb_affine_axis(v_right),up='0 0 1';
        if(view) {fb_view_axes(view);forward=fb_view_normal;right=fb_view_right;up=fb_view_up;}
        axis=forward;if(kind=="yaw") axis=up;if(kind=="pitch" || kind=="fliph") axis=right;if(kind=="flipv") axis=up;
        angle=stof(argv(2))*90;
    }
    strunzone(kind);if(vlen(axis)<0.001 || !fb_geom_finite(angle)) return TRUE;axis=normalize(axis);
    fb_affine_t m={0};
    if(flip) {m.x='1 0 0'-axis*(2*axis_x);m.y='0 1 0'-axis*(2*axis_y);m.z='0 0 1'-axis*(2*axis_z);}
    else {
        m.x=fb_affine_rotate_axis('1 0 0',axis,angle*M_PI/180);
        m.y=fb_affine_rotate_axis('0 1 0',axis,angle*M_PI/180);
        m.z=fb_affine_rotate_axis('0 0 1',axis,angle*M_PI/180);
    }
    m.translation=center-fb_affine_direction(&m,center);fb_vertex_matrix=m;fb_vertex_matrix_enabled=TRUE;
    if(fb_vertex_build('0 0 0',FALSE,0)) fb_vertex_commit(flip?"Flip Geometry":"Rotate Geometry");
    else cvar_set("ftebroom_selection_status","Transform rejected: invalid brush geometry");
    fb_vertex_cancel();return TRUE;
}
int fb_vertex_affine_preview(fb_affine_t *m)
{
    if(!fb_affine_prepare(m)) return FALSE;
    fb_affine_t previous=fb_vertex_matrix;int enabled=fb_vertex_matrix_enabled;
    fb_vertex_matrix=*m;fb_vertex_matrix_enabled=TRUE;
    if(fb_vertex_build('0 0 0',FALSE,0)) return TRUE;
    fb_vertex_matrix=previous;fb_vertex_matrix_enabled=enabled;return FALSE;
}
