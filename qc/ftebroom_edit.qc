// SPDX-License-Identifier: GPL-2.0-or-later
// Material edits and clipping share the brush history transaction boundary.
#include "fb_patch_material.qc"
vector fb_clip_normal;
float fb_clip_dist;

vector fb_uv_values(brushface_t *face)
{
    brushface_t base = *face;
    fb_texture_basecoords(&base);
    float u=face->sdir*base.sdir, v=face->sdir*base.tdir;
    float sx=1/max(0.000001,vlen(face->sdir));
    float sy=1/max(0.000001,vlen(face->tdir));
    if(u*(face->tdir*base.tdir)-v*(face->tdir*base.sdir)<0) sy=-sy;
    return [sx,sy,atan2(-v,u)*180/M_PI];
}
int fb_uv_revision();
int fb_uv_preview_attribute(int model,int id,int face,brushface_t *out);
void fb_material_state()
{
    static int stateGeometry=-1,stateRevision=-1,stateSelection=-1,stateUV=-1;static string stateDefaults;
    string defaults=sprintf("%s %.9g %.9g %.9g %.9g %.9g",cvar_string("ca_newbrushtexture"),cvar("ftebroom_new_scale_x"),cvar("ftebroom_new_scale_y"),cvar("ftebroom_new_angle"),cvar("ftebroom_new_offset_x"),cvar("ftebroom_new_offset_y"));
    int revision=fb_document_revision();
    if(stateGeometry==fb_grid_geometry_epoch&&stateRevision==revision&&stateSelection==fb_selection_epoch&&defaults==stateDefaults&&stateUV==fb_uv_revision())return;
    stateUV=fb_uv_revision();stateGeometry=fb_grid_geometry_epoch;stateRevision=revision;stateSelection=fb_selection_epoch;strunzone(stateDefaults);stateDefaults=strzone(defaults);
    static brushface_t faces[128];
    int count=0,contents,firstContents=0,firstFlags=0,firstValue=0,mixedFields=0;
    string material="";vector values='1 1 0',bias='0 0 0';int mixed=FALSE;
    for(int b=0;b<selectedbrushcount;b++) {
        patchinfo_t patch;int cpcount=patch_getcp(selectedbrushes[b].model,selectedbrushes[b].id,__NULL__,0,&patch);
        if(cpcount>0) {
            patchvert_t *cp=memalloc(cpcount*sizeof(patchvert_t));patch_getcp(selectedbrushes[b].model,selectedbrushes[b].id,cp,cpcount,&patch);
            vector puv=fb_patch_uv(cp,patch),pb=fb_patch_bias;
            if(!count){material=strzone(patch.shadername);values=puv;bias=pb;firstContents=patch.contents;}
            else {
                if(material!=patch.shadername)mixed=TRUE;
                if(fabs(puv_x-values_x)>0.0001)mixedFields|=1;
                if(fabs(puv_y-values_y)>0.0001)mixedFields|=2;
                if(fabs(puv_z-values_z)>0.0001)mixedFields|=4;
                if(fabs(pb_x-bias_x)>0.0001)mixedFields|=8;
                if(fabs(pb_y-bias_y)>0.0001)mixedFields|=16;
            }
            count++;memfree(cp);continue;
        }
        int n=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,faces,128,&contents);
        for(int f=0;f<min(n,128);f++) if(selbrush_face(&selectedbrushes[b],f)) {
            fb_uv_preview_attribute(selectedbrushes[b].model,selectedbrushes[b].id,f,&faces[f]);
            vector uv=fb_uv_values(&faces[f]);
            if(!count) {
                material=strzone(faces[f].shadername);values=uv;bias=[faces[f].sbias,faces[f].tbias,0];
                firstContents=contents;firstFlags=faces[f].surfaceflags;firstValue=faces[f].surfacevalue;
            } else {
                if(material!=faces[f].shadername) mixed=TRUE;
                if(fabs(uv_x-values_x)>0.0001) mixedFields|=1;
                if(fabs(uv_y-values_y)>0.0001) mixedFields|=2;
                if(fabs(uv_z-values_z)>0.0001) mixedFields|=4;
                if(fabs(faces[f].sbias-bias_x)>0.0001) mixedFields|=8;
                if(fabs(faces[f].tbias-bias_y)>0.0001) mixedFields|=16;
                if(firstFlags!=faces[f].surfaceflags) mixedFields|=32;
                if(firstContents!=contents) mixedFields|=64;
                if(firstValue!=faces[f].surfacevalue) mixedFields|=128;
            }
            count++;
        }
    }
    if(!count) {
        material=cvar_string("ca_newbrushtexture");
        values=[cvar("ftebroom_new_scale_x"),cvar("ftebroom_new_scale_y"),cvar("ftebroom_new_angle")];
        bias=[cvar("ftebroom_new_offset_x"),cvar("ftebroom_new_offset_y"),0];
    }
    cvar_set("ftebroom_current_material",mixed?"<multiple materials>":material);
    cvar_set("ftebroom_face_count",itos(count));cvar_set("ftebroom_uv_mixed",itos(mixedFields));
    cvar_set("ftebroom_scale_x",ftos(values_x));cvar_set("ftebroom_scale_y",ftos(values_y));
    cvar_set("ftebroom_angle",ftos(values_z));cvar_set("ftebroom_offset_x",ftos(bias_x));cvar_set("ftebroom_offset_y",ftos(bias_y));
    cvar_set("ftebroom_surface_flags",itos(firstFlags));cvar_set("ftebroom_content_flags",itos(firstContents));cvar_set("ftebroom_surface_value",itos(firstValue));
    if(count && !mixed && fb_faces_count()) cvar_set("ca_newbrushtexture",material);
    if(count) strunzone(material);
}
vector fb_uv_rotate(vector axis,vector normal,float angle)
{
    // Rodrigues rotation preserves skew and the normal component of Valve 220 axes.
    float c=cos(angle),s=sin(angle);
    return axis*c+crossproduct(normal,axis)*s+normal*(normal*axis)*(1-c);
}
void fb_material_change(string operation,float value,int relative)
{
    if(operation!="material" && operation!="sx" && operation!="sy" && operation!="angle" && operation!="ox" && operation!="oy") return;
    if(operation!="material" && (!fb_geom_finite(value) || fabs(value)>1000000)) return;
    if(!selectedbrushcount) {
        string setting="";
        if(operation=="sx")setting="ftebroom_new_scale_x";
        if(operation=="sy")setting="ftebroom_new_scale_y";
        if(operation=="angle")setting="ftebroom_new_angle";
        if(operation=="ox")setting="ftebroom_new_offset_x";
        if(operation=="oy")setting="ftebroom_new_offset_y";
        if(setting!="") {
            if(relative)value+=cvar(setting);
            if((operation=="sx" || operation=="sy") && fabs(value)<0.001)return;
            cvar_set(setting,ftos(value));
        }
        fb_material_state();return;
    }
    static brushface_t faces[128];int contents;
    string material=strzone(cvar_string("ca_newbrushtexture"));
    // Validate every selected face before opening a transaction.
    for(int candidate=0;candidate<selectedbrushcount;candidate++) {
        int n=brush_get(selectedbrushes[candidate].model,selectedbrushes[candidate].id,faces,128,&contents);
        if(n>128) {strunzone(material);return;}
        for(int f=0;f<n;f++) if(selbrush_face(&selectedbrushes[candidate],f)) {
            vector uv=fb_uv_values(&faces[f]);float scale=value;
            if(relative && operation=="sx") scale+=uv_x;
            if(relative && operation=="sy") scale+=uv_y;
            if((operation=="sx" || operation=="sy") && fabs(scale)<0.001) {strunzone(material);return;}
        }
    }
    fb_tx_begin(operation=="material"?"Set Material":strcat("Texture ",operation));
    for(int b=0;b<selectedbrushcount;b++) {
        if(patch_getcp(selectedbrushes[b].model,selectedbrushes[b].id,__NULL__,0,__NULL__)>0) {
            if(!fb_patch_material_change(selectedbrushes[b].model,selectedbrushes[b].id,operation,value,relative,material)) {
                fb_tx_cancel();strunzone(material);return;
            }
            continue;
        }
        int n=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,faces,128,&contents);
        if(n<=0 || n>128) continue;
        for(int f=0;f<n;f++) if(selbrush_face(&selectedbrushes[b],f)) {
            if(operation=="material") faces[f].shadername=material;
            else {
                vector uv=fb_uv_values(&faces[f]);
                if(operation=="sx") faces[f].sdir*=uv_x/(relative?uv_x+value:value);
                else if(operation=="sy") faces[f].tdir*=uv_y/(relative?uv_y+value:value);
                else if(operation=="ox") faces[f].sbias=relative?faces[f].sbias+value:value;
                else if(operation=="oy") faces[f].tbias=relative?faces[f].tbias+value:value;
                else if(operation=="angle") {
                    brushface_t base=faces[f];fb_texture_basecoords(&base);
                    float delta=(relative?value:value-uv_z)*M_PI/180;
                    if(crossproduct(base.sdir,base.tdir)*faces[f].planenormal>0) delta=-delta;
                    faces[f].sdir=fb_uv_rotate(faces[f].sdir,faces[f].planenormal,delta);
                    faces[f].tdir=fb_uv_rotate(faces[f].tdir,faces[f].planenormal,delta);
                }
            }
        }
        brush_history_edit(selectedbrushes[b].model,selectedbrushes[b].id,faces,n,contents);
    }
    fb_tx_commit();strunzone(material);fb_material_state();
}
void fb_material_edit(string operation,float value) {fb_material_change(operation,value,FALSE);}
void fb_material_flags(string operation,int value,int toggle)
{
    if(operation!="surface" && operation!="contents" && operation!="value") return;
    static brushface_t faces[128];int contents;
    for(int b=0;b<selectedbrushcount;b++) if(brush_get(selectedbrushes[b].model,selectedbrushes[b].id,__NULL__,0,__NULL__)>128) return;
    fb_tx_begin("Face Attributes");
    for(int b=0;b<selectedbrushcount;b++) {
        int count=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,faces,128,&contents);
        if(count<4) continue;
        if(operation=="contents") contents=toggle==1?(contents|value):(toggle==2?(contents&~value):value);
        for(int f=0;f<count;f++) if(selbrush_face(&selectedbrushes[b],f)) {
            if(operation=="surface") faces[f].surfaceflags=toggle==1?(faces[f].surfaceflags|value):(toggle==2?(faces[f].surfaceflags&~value):value);
            if(operation=="value") faces[f].surfacevalue=value;
        }
        brush_history_edit(selectedbrushes[b].model,selectedbrushes[b].id,faces,count,contents);
    }
    fb_tx_commit();fb_material_state();
}
vector fb_clip_surface_normal;
vector fb_clip_help[3],fb_clip_saved_points[3],fb_clip_saved_help[3];
var int fb_clip_source_view=-1;
var int fb_clip_handle=-1;
int fb_clip_dragging,fb_clip_saved_count,fb_clip_newpoint,fb_clip_exact,fb_clip_saved_exact;
vector fb_clip_downpixel,fb_clip_exact_normal,fb_clip_saved_normal;
float fb_clip_exact_dist,fb_clip_saved_dist;
vector fb_clip_hover,fb_clip_hover_help;
int fb_clip_hover_valid;
vector fb_clip_facepoints[MAX_FACEPOINTS];
int fb_clip_facecount;
float fb_clip_plane()
{
    if(fb_clip_count<3) fb_clip_exact=FALSE;
    if(fb_clip_count<2) return FALSE;
    if(fb_clip_exact) {fb_clip_normal=fb_clip_exact_normal;fb_clip_dist=fb_clip_exact_dist;return TRUE;}
    vector third=fb_clip_points[2];
    if(fb_clip_count==2) {
        int votes[6];for(int k=0;k<6;k++) votes[k]=0;
        for(int point=0;point<2;point++) {
            vector help=fb_clip_help[point];
            if(vlen(help)<0.5) help=fb_clip_surface_normal;
            if(vlen(help)<0.5) help='0 0 1';
            int axis=2;
            if(fabs(help_x)>fabs(help_z)) axis=0;
            if(fabs(help_y)>fabs(help[axis])) axis=1;
            int slot=axis*2;if(help[axis]<0) slot++;
            votes[slot]++;
        }
        int order[6]={4,5,0,1,2,3};int best=4;
        for(int j=0;j<6;j++) if(votes[order[j]]>votes[best]) best=order[j];
        vector direction='0 0 0';direction[(int)(best/2)]=(best&1)?-1:1;
        third=fb_clip_points[1]+128*direction;
    }
    vector normal=crossproduct(fb_clip_points[1]-fb_clip_points[0],third-fb_clip_points[0]);
    if(vlen(normal)<0.01) return FALSE;
    fb_clip_normal=normalize(normal);fb_clip_dist=fb_clip_normal*fb_clip_points[0];
    return TRUE;
}
int fb_clip_position(vector pixel,int view)
{
    fb_clip_hover_valid=FALSE;
    if(view) {
        fb_view_remember_selection();
        vector anchor='0 0 0';
        if(fb_view_has_selection) anchor=(fb_view_lastmin+fb_view_lastmax)*0.5;
        if(fb_clip_count) anchor=fb_clip_points[0];
        fb_view_axes(view);
        fb_clip_hover_help=fb_view_normal;
        fb_clip_hover=fb_snap_to_plane(fb_view_plane_point(pixel,view,anchor),fb_view_normal,anchor*fb_view_normal);
    } else {
        vector start=unproject(pixel),end=unproject(pixel+'0 0 8192');
        if(!fb_pick_query(start,end,pixel,0,FB_HIT_BRUSH,FALSE)) return FALSE;
        if(fb_hits[0].face<1) return FALSE;
        fb_clip_hover_help=fb_hits[0].normal;
        fb_clip_hover=fb_snap_to_plane(fb_hits[0].point,fb_clip_hover_help,fb_hits[0].point*fb_clip_hover_help);
    }
    fb_clip_hover_valid=TRUE;return TRUE;
}
void fb_clip_pick(vector pixel,int view)
{
    if(fb_clip_count>=3) return;
    if(!fb_clip_position(pixel,view)) return;
    if(!fb_clip_count) {fb_clip_surface_normal=fb_clip_hover_help;fb_clip_source_view=view;}
    fb_clip_help[fb_clip_count]=fb_clip_hover_help;
    fb_clip_points[fb_clip_count++]=fb_clip_hover;fb_clip_exact=FALSE;
}
void fb_clip_cancel_drag()
{
    if(!fb_clip_dragging) return;
    fb_clip_count=fb_clip_saved_count;
    for(int i=0;i<3;i++) {fb_clip_points[i]=fb_clip_saved_points[i];fb_clip_help[i]=fb_clip_saved_help[i];}
    fb_clip_exact=fb_clip_saved_exact;fb_clip_exact_normal=fb_clip_saved_normal;fb_clip_exact_dist=fb_clip_saved_dist;
    fb_clip_dragging=FALSE;fb_clip_handle=-1;
}
void fb_clip_down(vector pixel,int view)
{
    fb_clip_handle=-1;
    for(int i=0;i<fb_clip_count;i++) {
        vector screen=project(fb_clip_points[i]);screen_z=0;
        if(vlen(screen-pixel)<=9) fb_clip_handle=i;
    }
    if(fb_clip_handle<0 && fb_clip_count==3) return;
    fb_clip_saved_count=fb_clip_count;fb_clip_saved_exact=fb_clip_exact;
    fb_clip_saved_normal=fb_clip_exact_normal;fb_clip_saved_dist=fb_clip_exact_dist;
    for(int j=0;j<3;j++) {fb_clip_saved_points[j]=fb_clip_points[j];fb_clip_saved_help[j]=fb_clip_help[j];}
    fb_clip_newpoint=fb_clip_handle<0;
    if(fb_clip_newpoint) {
        fb_clip_pick(pixel,view);
        if(fb_clip_count==fb_clip_saved_count) return;
        fb_clip_handle=fb_clip_count-1;
    }
    fb_clip_downpixel=pixel;fb_clip_dragging=TRUE;fb_active=TRUE;
}
void fb_clip_move(vector pixel,int view)
{
    fb_clip_position(pixel,view);
    if(!fb_clip_dragging || !fb_clip_hover_valid) return;
    if(vlen(pixel-fb_clip_downpixel)<4) return;
    int handle=fb_clip_handle;
    if(fb_clip_newpoint) {
        if(fb_clip_saved_count>=2) return;
        handle=fb_clip_saved_count+1;fb_clip_count=handle+1;
    }
    vector point=fb_clip_hover;
    if(view && cvar("ftebroom_shift")) {
        vector anchor=fb_clip_saved_points[handle];
        if(fb_clip_newpoint) anchor=fb_clip_points[handle-1];
        vector delta=point-anchor;int axis=0;
        for(int a=1;a<3;a++) if(fabs(delta[a])>fabs(delta[axis])) axis=a;
        point=anchor;point[axis]+=delta[axis];
    }
    fb_clip_points[handle]=point;fb_clip_help[handle]=fb_clip_hover_help;fb_clip_exact=FALSE;
}
void fb_clip_up(vector pixel,int view)
{
    fb_clip_move(pixel,view);fb_clip_dragging=FALSE;fb_active=FALSE;fb_clip_handle=-1;
}
void fb_clip_double(vector pixel,int view)
{
    if(view) return;
    if(!fb_clip_position(pixel,view)) return;
    static brushface_t faces[64];static vector points[MAX_FACEPOINTS];
    int n=brush_get(fb_hits[0].model,fb_hits[0].id,faces,64,__NULL__),face=fb_hits[0].face-1;
    if(n<4 || n>64 || face<0 || face>=n) return;
    int count=brush_calcfacepoints(face+1,faces,n,points,MAX_FACEPOINTS);
    if(count<3) return;
    fb_clip_dragging=FALSE;fb_active=FALSE;fb_clip_exact=TRUE;fb_clip_count=3;
    fb_clip_exact_normal=faces[face].planenormal;fb_clip_exact_dist=faces[face].planedist;
    fb_clip_facecount=count;for(int p=0;p<count;p++) fb_clip_facepoints[p]=points[p];
    for(int i=0;i<3;i++) {fb_clip_points[i]=points[i];fb_clip_help[i]=faces[face].planenormal;}
}
void fb_clip_inherit(fb_poly_t *poly,brushface_t *source,int n,vector normal,float distance)
{
    for(int f=0;f<poly->nf;f++) {
        brushface_t *face=&poly->faces[f].attr;
        if(vlen(face->planenormal-normal)>0.0001 || fabs(face->planedist-distance)>0.001) continue;
        int best=0;for(int i=1;i<n;i++) if(source[i].planenormal*normal>source[best].planenormal*normal) best=i;
        strunzone(face->shadername);*face=source[best];face->shadername=strzone(source[best].shadername);
        face->planenormal=normal;face->planedist=distance;
    }
}
float fb_clip_candidate(brushface_t *source,int n,vector normal,float dist,brushface_t *result)
{
    memcpy(result,source,n*sizeof(brushface_t));
    int nearest=0;for(int i=1;i<n;i++) if(source[i].planenormal*normal>source[nearest].planenormal*normal) nearest=i;
    result[n]=source[nearest];result[n].planenormal=normal;result[n].planedist=dist;
    static vector points[MAX_FACEPOINTS];
    if(brush_calcfacepoints(n+1,result,n+1,points,MAX_FACEPOINTS)<3) return FALSE;
    int valid=0;
    for(int f=1;f<=n+1;f++) if(brush_calcfacepoints(f,result,n+1,points,MAX_FACEPOINTS)>=3) valid++;
    return valid>=4;
}
void fb_clip_apply()
{
    if(!fb_clip_plane()) {print("ReyondBroom: place two distinct cut points (or three non-collinear points).\n");return;}
    static brushface_t source[64], result[64];
    int total=selectedbrushcount;
    selbrush_t *selection=memalloc(total*sizeof(selbrush_t));
    memcpy(selection,selectedbrushes,total*sizeof(selbrush_t));
    fb_poly_t *parts=memalloc(total*2*sizeof(fb_poly_t));
    int *contents=memalloc(total*sizeof(int));
    int mode=cvar("ftebroom_clip_keep"), applied=0,valid=TRUE;
    for(int b=0;b<total;b++) {
        int n=brush_get(selection[b].model,selection[b].id,__NULL__,0,__NULL__);
        if(n<4) continue;
        if(n>64) {valid=FALSE;fb_geom_error="Clip supports at most 64 faces per brush";break;}
        brush_get(selection[b].model,selection[b].id,source,64,&contents[b]);
        fb_poly_t original={0};
        if(!fb_poly_from_faces(source,n,&original)) {valid=FALSE;break;}
        float lo=1000000,hi=-1000000;
        for(int v=0;v<original.nv;v++) {float d=original.vertices[v]*fb_clip_normal-fb_clip_dist;lo=min(lo,d);hi=max(hi,d);}
        fb_poly_free(&original);
        if(lo>=-FB_GEOM_PLANE || hi<=FB_GEOM_PLANE) continue;
        if(!fb_split(source,n,fb_clip_normal,fb_clip_dist,&parts[b*2],&parts[b*2+1])) {valid=FALSE;break;}
        fb_clip_inherit(&parts[b*2],source,n,-fb_clip_normal,-fb_clip_dist);
        fb_clip_inherit(&parts[b*2+1],source,n,fb_clip_normal,fb_clip_dist);
    }
    if(valid) {
        fb_tx_begin("Clip");
        for(int b=0;b<total;b++) {
            if(!parts[b*2].nf) continue;
            int side=b*2+1;if(mode==2) side=b*2;
            for(int f=0;f<parts[side].nf;f++) result[f]=parts[side].faces[f].attr;
            if(!brush_history_edit(selection[b].model,selection[b].id,result,parts[side].nf,contents[b])) {valid=FALSE;break;}
            if(mode==1) {
                for(int f=0;f<parts[b*2].nf;f++) result[f]=parts[b*2].faces[f].attr;
                if(!brush_history_create(selection[b].model,result,parts[b*2].nf,contents[b],TRUE)) {valid=FALSE;break;}
            }
            applied++;
        }
        if(valid) fb_tx_commit();else fb_tx_cancel();
    }
    for(int p=0;p<total*2;p++) fb_poly_free(&parts[p]);memfree(parts);memfree(contents);memfree(selection);
    if(valid) {fb_clip_count=0;fb_clip_exact=FALSE;print(sprintf("ReyondBroom: clipped %i brushes.\n",applied));}
    else {cvar_set("ftebroom_selection_status",strcat("Clip cancelled: ",fb_geom_error));print(strcat("ReyondBroom: Clip cancelled: ",fb_geom_error,"\n"));}
}
int fb_generator_replaces(int model,int id);
int fb_generator_count;
float fb_preview_replaces(int model,int id)
{
    if(fb_generator_count&&fb_generator_replaces(model,id))return TRUE;
    if(fb_transform_replaces(model,id)) return TRUE;
    if(fb_vertex_replaces(model,id)) return TRUE;
    if(cvar("ftebroom_tool")!=FB_TOOL_CLIP) return FALSE;
    if(!brush_isselected(model,id)) return FALSE;
    if(!fb_clip_plane()) return FALSE;
    int n=brush_get(model,id,__NULL__,0,__NULL__);
    if(n<4 || n>64) return FALSE;
    static brushface_t source[64],part[65];
    brush_get(model,id,source,64,__NULL__);
    if(!fb_clip_candidate(source,n,fb_clip_normal,fb_clip_dist,part)) return FALSE;
    if(!fb_clip_candidate(source,n,-fb_clip_normal,-fb_clip_dist,part)) return FALSE;
    static vector vertices[MAX_FACEPOINTS];
    float lo=1000000,hi=-1000000;
    for(int f=1;f<=n;f++) {
        int count=brush_calcfacepoints(f,source,n,vertices,MAX_FACEPOINTS);
        for(int v=0;v<count;v++) {float d=vertices[v]*fb_clip_normal-fb_clip_dist;lo=min(lo,d);hi=max(hi,d);}
    }
    return lo < -0.01 && hi > 0.01;
}
void fb_clip_preview()
{
    cvar_set("ftebroom_clip_points",itos(fb_clip_count));
    if(cvar("ftebroom_tool")!=FB_TOOL_CLIP) {fb_clip_count=0;return;}
    cvar_set("ftebroom_clip_hover_valid",itos(fb_clip_hover_valid));
    if(fb_clip_hover_valid) {vector hover=project(fb_clip_hover);cvar_set("ftebroom_clip_hover",sprintf("%g %g",hover_x,hover_y));}
    for(int i=0;i<fb_clip_count;i++) {
        vector p=project(fb_clip_points[i]);
        cvar_set(sprintf("ftebroom_clip_point_%i",i),sprintf("%g %g",p_x,p_y));
    }
    if(!fb_clip_plane()) return;
    shaderforname("ftebroom/clipwire","{\ncull none\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\nnodepthtest\n}\n}");
    shaderforname("ftebroom/discarded","{\ncull none\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\n}\n}");
    if(fb_clip_exact) for(int p=1;p<fb_clip_facecount-1;p++) {
        R_BeginPolygon("ftebroom/clipwire",3);
        R_PolygonVertex(fb_clip_facepoints[0],'0 0','1 0.45 0',0.25);
        R_PolygonVertex(fb_clip_facepoints[p],'0 0','1 0.45 0',0.25);
        R_PolygonVertex(fb_clip_facepoints[p+1],'0 0','1 0.45 0',0.25);R_EndPolygon();
    }
    static brushface_t source[64], candidate[65];
    int mode=cvar("ftebroom_clip_keep");
    for(int b=0;b<selectedbrushcount;b++) {
        if(!fb_preview_replaces(selectedbrushes[b].model,selectedbrushes[b].id)) continue;
        int contents,n=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,source,64,&contents);
        for(int side=1;side<=2;side++) {
            float sign=side==1?-1:1;
            if(!fb_clip_candidate(source,n,fb_clip_normal*sign,fb_clip_dist*sign,candidate)) continue;
            float keep=(mode==1 || (mode==0 && side==2) || (mode==2 && side==1));
            if(keep) DrawQCBrushTextured(candidate,n+1,'1 1 1',1);
            else DrawQCBrushSolid(candidate,n+1,"ftebroom/discarded",'1 0.35 0.15',0.18);
            DrawQCBrushWireframe(candidate,n+1,"ftebroom/clipwire",keep?'0 0.8 1':'1 0.35 0.15',keep?1:0.3);
        }
    }
}
