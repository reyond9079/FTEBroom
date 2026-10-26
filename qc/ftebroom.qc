void fb_texture_align(string operation,int direction,int fraction);
void fb_transfer_finish(int commit);
int fb_transfer_begin(vector pixel,int rotate,int materialOnly);
void fb_transfer_at(vector pixel,int allFaces);
void fb_material_replace();
void fb_material_drop(string material,vector pixel);
void fb_texture_key(string mode,int direction,int speed);
void fb_material_usage();
void fb_entity_publish();
void fb_uv_publish();
void fb_uv_finish(int commit);
float fb_entity_command();
int fb_export_map();
float fb_context_command();
void fb_entity_drop_preview();
void fb_filter_prepare();
void fb_filter_draw();
void fb_filter_tag(string type,int enabled);
int fb_render_visible(int model,int id);
void fb_material_select(string mode);
void fb_delete_selection();
void fb_csg(string operation);
int fb_primitive_create(vector low,vector high,int replace);
void fb_primitive_preview(vector low,vector high);
void fb_primitive_apply();
void fb_visibility_sync();
void fb_visibility_command(string operation);
void fb_group_create(string name);
void fb_organize_action(string operation,int target,string value);
void fb_lines_index();
void fb_select_line(int line);
void fb_select_command(string mode);
int fb_faces_count();
void fb_faces_click(int model,int id,int face,int ctrl);
void fb_faces_paint(vector pixel);
void fb_faces_double(vector pixel,int ctrl,int alt);
// SPDX-License-Identifier: GPL-2.0-or-later
// Gestures capture their originating view; perspective Alt locks world Z.
float fb_dragged, fb_moving, fb_alt;
int fb_model, fb_brush, fb_facecount, fb_face;
int fb_original_owned,fb_gesture_view,fb_constraint_axis;
float fb_hidden_source;
float fb_vertical_fallback,fb_vertical_scale;
float fb_face_select;
int fb_point_entity,fb_pick_kind;
vector fb_applied_offset;
vector fb_downpixel, fb_anchor, fb_offset, fb_segment, fb_lastpixel, fb_boxcenter, fb_boxsize;
brushface_t fb_original[128], fb_preview[128];
vector fb_view_plane_point(vector pixel, int view, vector depthAnchor)
{
    if(view) {vector point=unproject(pixel);point[fb_view_depth_axis(view)]=depthAnchor[fb_view_depth_axis(view)];return point;}
    float height=depthAnchor_z;
    vector a = unproject(pixel), b = unproject(pixel + '0 0 8192');
    vector ray = b-a;
    if (fabs(ray_z) < 0.0001) {
        vector p=a;
        if (fabs(ray_x)>fabs(ray_y)) p_x=depthAnchor_x; else p_y=depthAnchor_y;
        p_z=height; return p;
    }
    return a + ray * ((height-a_z)/ray_z);
}
float fb_vertical_drag(vector pixel,vector anchor)
{
    vector rayStart=unproject(pixel),rayEnd=unproject(pixel+'0 0 8192');
    vector ray=normalize(rayEnd-rayStart),w=rayStart-anchor;
    float denominator=1-ray_z*ray_z;
    if(fb_vertical_fallback)return -pixel_y*fb_vertical_scale;
    if(denominator<0.0001)return 0;
    return (w_z-ray_z*(ray*w))/denominator;
}
void fb_boxfaces(vector center, vector size)
{
    static const vector normals[6] = {'-1 0 0','0 -1 0','0 0 -1','1 0 0','0 1 0','0 0 1'};
    for (int i=0;i<6;i++) {
        fb_preview[i].planenormal=normals[i];
        float extent = (i==0 || i==3) ? size_x : ((i==1 || i==4) ? size_y : size_z);
        fb_preview[i].planedist=center*normals[i]+extent*0.5;
        fb_preview[i].shadername=cvar_string("ca_newbrushtexture");
        fb_preview[i].surfaceflags=0;fb_preview[i].surfacevalue=0;
        reset_texturecoords(&fb_preview[i]);
    }
}
int fb_createbox(vector center, vector size)
{
    for (int i=0;i<3;i++) if (!fb_geom_finite(size[i]) || !fb_geom_finite(center[i]) || size[i]<0.125 || fabs(center[i])+size[i]*0.5>autocvar_ftebroom_world_bound) return 0;
    fb_tx_begin("Create brush");
    fb_boxfaces(center,size);
    brush_deselectall();
    fb_restore_entity_selection(-1);
    int model=fb_creation_model();
    if(!model) {fb_tx_cancel();return 0;}
    int id=brush_history_create(model,fb_preview,6,CONTENTBIT_SOLID,TRUE);
    fb_tx_commit();return id;
}
void fb_cancel()
{
    fb_spin_cancel_drag();
    fb_polygon_cancel_drag();
    fb_bend_cancel_drag();
    fb_sculpt_finish(FALSE);
    fb_uv_finish(FALSE);
    fb_sweep_cancel();
    fb_moveface_cancel();
    fb_clip_cancel_drag();
    fb_vertex_cancel();
    fb_scale_cancel();
    fb_assemble_cancel();
    fb_transfer_finish(FALSE);
    fb_rotate_cancel();
    fb_extrude_clear();
    fb_transform_clear();
    if(fb_select_paint) {fb_select_paint=FALSE;fb_tx_cancel();}
    if(fb_hidden_source) {
        if(!brush_isselected(fb_model,fb_brush)) brush_selected(fb_model,fb_brush,-1,FALSE);
        fb_hidden_source=FALSE;
    }
    for(int f=0;f<fb_original_owned;f++) strunzone(fb_original[f].shadername);
    fb_original_owned=0; fb_active=FALSE; fb_dragged=FALSE; fb_owner=FB_OWNER_NONE;
}
// Camera claims RMB in the native frontend before this chain is reached.
// Extrude claims Shift gestures before this selection/move fallback.
int fb_permanent_owner()
{
    if(fb_brush) if(!fb_face_select) {
        if(cvar("ftebroom_drag_mode")) return FB_OWNER_MOVE;
        if(brush_isselected(fb_model,fb_brush)) return FB_OWNER_MOVE;
    }
    if(!fb_face_select) {
        if(fb_point_entity>=0 && fb_entity_selected(fb_point_entity)) return FB_OWNER_MOVE;
        if(!autocvar_ftebroom_create_with_selection && (selectedbrushcount || fb_selected_entity_count)) return FB_OWNER_SELECTION;
        return FB_OWNER_SHAPE;
    }
    return FB_OWNER_SELECTION;
}
void fb_move_numeric(vector offset)
{
    fb_tool_refresh();
    if(fb_transform_capture()) fb_transform_commit(offset);
    fb_transform_clear();
}

void fb_duplicate_numeric(vector offset)
{
    fb_tool_refresh();
    if(fb_transform_capture()) {fb_transform_duplicate=TRUE;fb_transform_commit(offset);}
    fb_transform_clear();
}
void fb_begin(vector pixel, float alt, int view)
{
    fb_cancel();
    fb_gesture_view=view;fb_constraint_axis=-1;
    if(cvar("ftebroom_shift") && alt && !cvar("ftebroom_ctrl"))if(fb_moveface_begin(pixel,view))return;
    if(cvar("ftebroom_shift")) if(fb_extrude_begin(pixel,view)) return;
    if(alt && !view) if(fb_transfer_begin(pixel,cvar("ftebroom_shift"),cvar("ftebroom_ctrl"))) {
        fb_owner=FB_OWNER_MATERIAL;fb_active=TRUE;fb_downpixel=fb_lastpixel=pixel;return;
    }
    if(view) alt=0;
    int hits=fb_pick(pixel);
    fb_model=selectedbrushmodel;fb_brush=0;fb_face=0;fb_point_entity=-1;fb_pick_kind=0;
    vector hitpoint='0 0 0';
    if(hits) {
        // Selected objects own a drag even when another surface occludes them.
        // The frontmost hit remains the target for a click without movement.
        int dragHit=0;
        if(!cvar("ftebroom_shift") && !cvar("ftebroom_ctrl"))
            for(int h=0;h<hits;h++) {
                int target=fb_selection_target(fb_hits[h].node);
                if(target>=0)if(fb_selection_selected(target)){dragHit=h;break;}
            }
        fb_hit_t hit=fb_hits[dragHit];fb_pick_kind=hit.kind;hitpoint=hit.point;
        if(hit.kind==FB_HIT_BRUSH || hit.kind==FB_HIT_PATCH) {fb_model=hit.model;fb_brush=hit.id;fb_face=hit.face;}
        else if(hit.kind==FB_HIT_ENTITY) fb_point_entity=hit.id;
        else if(hit.kind==FB_HIT_HANDLE) return;
    }
    cvar_set("ftebroom_selection_status","");
    fb_face_select=FALSE;if(!view) fb_face_select=cvar("ftebroom_shift");
    fb_face_ctrl=cvar("ftebroom_ctrl");fb_face_paint_ready=fb_faces_count()>0;
    fb_select_ctrl=cvar("ftebroom_ctrl") && !fb_face_select;
    fb_select_node=hits?fb_selection_target(fb_hits[0].node):-1;
    // Keep the function call out of a mixed logical expression on the legacy QC target.
    fb_select_blocked=FALSE;
    if(fb_select_ctrl) if(fb_select_node>=0) fb_select_blocked=fb_selection_selected(fb_select_node);
    fb_owner=fb_permanent_owner();
    if((fb_pick_kind==FB_HIT_PATCH || fb_point_entity>=0) && fb_owner!=FB_OWNER_MOVE) fb_owner=FB_OWNER_SELECTION;
    if(fb_select_ctrl) {
        if(fb_select_blocked) fb_owner=FB_OWNER_MOVE;
        else fb_owner=FB_OWNER_SELECTION;
    }
    fb_moving=(fb_owner==FB_OWNER_MOVE);
    fb_anchor=hits?hitpoint:[0,0,cvar("ftebroom_plane")];
    if (!hits) {
        vector rayStart=unproject(pixel),rayEnd=unproject(pixel+'0 0 8192');
        fb_anchor=rayStart+normalize(rayEnd-rayStart)*autocvar_ftebroom_default_distance;
        if(view || autocvar_ftebroom_project_creation_plane) {
            fb_anchor_z=cvar("ftebroom_plane");
            fb_anchor=fb_view_plane_point(pixel,fb_gesture_view,fb_anchor);
        }
    }
    if(fb_gesture_view && !fb_moving) {
        int depth=fb_view_depth_axis(fb_gesture_view);
        float level=0;if(fb_view_has_selection) level=fb_view_lastmin[depth];
        vector depthAnchor=fb_anchor;depthAnchor[depth]=level;
        fb_anchor=fb_view_plane_point(pixel,view,depthAnchor);
    }
    if (fb_moving) {
        if(cvar("ftebroom_drag_mode") && fb_brush && !brush_isselected(fb_model,fb_brush)) fb_selection_click(fb_select_node,FALSE);
        if(!fb_transform_capture()) return;
        fb_transform_duplicate=fb_select_ctrl;
    }
    // Keep the actual hit depth; snap the resulting displacement, not the anchor.
    vector dragRay=normalize(unproject(pixel+'0 0 8192')-unproject(pixel));
    vector viewport=getproperty(VF_SIZE);
    fb_vertical_fallback=1-dragRay_z*dragRay_z<0.01;
    fb_vertical_scale=2*max(1,vlen(fb_anchor-getproperty(VF_ORIGIN)))*tan(getproperty(VF_FOVY)*M_PI/360)/max(1,viewport_y);
    if(fb_gesture_view && !fb_moving && fb_view_has_selection)
        fb_anchor[fb_view_depth_axis(fb_gesture_view)]=fb_view_lastmin[fb_view_depth_axis(fb_gesture_view)];
    fb_downpixel=fb_lastpixel=pixel; fb_segment=fb_view_plane_point(pixel,fb_gesture_view,fb_anchor);
    fb_applied_offset=fb_offset='0 0 0'; fb_alt=alt; fb_active=TRUE;
}
void fb_update(vector pixel, float alt, int view)
{
    if (!fb_active || view!=fb_gesture_view) return;
    if(view) alt=0;
    if (vlen(pixel-fb_downpixel)>=4) fb_dragged=TRUE;
    if(fb_owner==FB_OWNER_EXTRUDE) {if(fb_dragged) fb_extrude_move(pixel);return;}
    if(fb_owner==FB_OWNER_MATERIAL) {
        vector transferDelta=pixel-fb_lastpixel;
        int transferSamples=min(256,max(1,ceil(vlen(transferDelta)/4)));
        if(vlen(transferDelta)>0) for(int i=1;i<=transferSamples;i++) fb_transfer_at(fb_lastpixel+transferDelta*((float)i/(float)transferSamples),FALSE);
        fb_lastpixel=pixel;return;
    }
    if(fb_owner==FB_OWNER_SELECTION) {
        if(fb_face_select && fb_face_ctrl && fb_face_paint_ready && fb_dragged) {
            vector faceDelta=pixel-fb_lastpixel;
            int faceSamples=min(256,max(1,ceil(vlen(faceDelta)/4)));
            if(!fb_select_paint || vlen(faceDelta)>0) for(int sampleFace=1;sampleFace<=faceSamples;sampleFace++)
                fb_faces_paint(fb_lastpixel+faceDelta*((float)sampleFace/(float)faceSamples));
            fb_lastpixel=pixel;
        }
        if(fb_select_ctrl && fb_dragged && !fb_select_blocked) {
            vector delta=pixel-fb_lastpixel;
            int samples=min(256,max(1,ceil(vlen(delta)/4)));
            if(!fb_select_paint || vlen(delta)>0) for(int sample=1;sample<=samples;sample++)
                fb_selection_paint(fb_lastpixel+delta*((float)sample/(float)samples));
            fb_lastpixel=pixel;
        }
        return;
    }
    if (alt != fb_alt) {
        fb_alt=alt; fb_segment=fb_view_plane_point(pixel,fb_gesture_view,fb_anchor+fb_offset); fb_lastpixel=pixel;
        return;
    }
    if(fb_gesture_view) {
        vector point2d=fb_view_plane_point(pixel,fb_gesture_view,fb_anchor);
        fb_offset+=point2d-fb_segment;fb_segment=point2d;
    } else if (fb_alt) {
        // Closest cursor-ray point on the captured world-Z axis, like TrenchBroom.
        fb_offset_z += fb_vertical_drag(pixel,fb_anchor)-fb_vertical_drag(fb_lastpixel,fb_anchor);
    } else {
        vector point=fb_view_plane_point(pixel,fb_gesture_view,fb_anchor+fb_offset);
        fb_offset_x += point_x-fb_segment_x;
        fb_offset_y += point_y-fb_segment_y;
        fb_segment=point;
    }
    fb_lastpixel=pixel;
    for (int axis=0;axis<3;axis++) fb_offset[axis]=bound(-autocvar_ftebroom_world_bound*2,fb_offset[axis],autocvar_ftebroom_world_bound*2);
    vector offset=fb_snap_delta(fb_offset);
    if(view)offset[fb_view_depth_axis(view)]=0;
    if(fb_moving && cvar("ftebroom_shift") && fb_dragged) {
        int depthAxis=2;if(view) depthAxis=fb_view_depth_axis(view);
        if(fb_constraint_axis<0 && vlen(offset)>0) {
            float largest=-1;
            for(int candidate=0;candidate<3;candidate++) if(!view || candidate!=depthAxis)
                if(fabs(offset[candidate])>largest) {largest=fabs(offset[candidate]);fb_constraint_axis=candidate;}
        }
        for(int locked=0;locked<3;locked++) if(fb_constraint_axis>=0 && locked!=fb_constraint_axis) offset[locked]=0;
    } else fb_constraint_axis=-1;
    if (fb_moving) {
        if(fb_transform_valid(offset)) {
            fb_applied_offset=offset;fb_transform_delta=offset;
        }
        fb_transform_preview=fb_dragged;
    } else {
        static vector shapeBounds[2];
        if(fb_shape_bounds(fb_anchor,fb_offset,fb_gesture_view,cvar("ftebroom_shift"),alt,getproperty(VF_ORIGIN),shapeBounds)) {
            fb_boxcenter=(shapeBounds[0]+shapeBounds[1])*0.5;
            fb_boxsize=shapeBounds[1]-shapeBounds[0];fb_boxfaces(fb_boxcenter,fb_boxsize);
            cvar_set("ftebroom_selection_status",sprintf("Size %g x %g x %g | Shift: square | Shift+Alt: cube | Alt: height",fb_boxsize_x,fb_boxsize_y,fb_boxsize_z));
        }
    }
}
void fb_finish(vector pixel, float alt, int view)
{
    if (!fb_active || view!=fb_gesture_view) return;
    if(view) alt=0;
    fb_update(pixel,alt,view);
    if(fb_owner==FB_OWNER_EXTRUDE) {
        if(fb_dragged) fb_extrude_commit();
        else if(!view) fb_faces_click(fb_model,fb_brush,fb_face,FALSE);
        fb_cancel();return;
    }
    if(fb_owner==FB_OWNER_MATERIAL) {fb_transfer_finish(TRUE);fb_cancel();return;}
    if(fb_select_paint) {fb_select_paint=FALSE;fb_tx_commit();}
    else if(fb_face_select) {
        if(!fb_dragged) {
            int pickedBrush=0,pickedFace=0;
            if(fb_pick_kind==FB_HIT_BRUSH) {pickedBrush=fb_brush;pickedFace=fb_face;}
            fb_faces_click(fb_model,pickedBrush,pickedFace,fb_face_ctrl);
        }
    }
    else if(fb_select_ctrl && fb_dragged && !fb_moving) {
        if(fb_select_blocked) print("ReyondBroom: duplicate and move is reserved for roadmap task 4.1.\n");
    } else if(!fb_dragged && !fb_face_select) fb_selection_click(fb_select_node,fb_select_ctrl);
    else if (!fb_moving && (fb_pick_kind==FB_HIT_PATCH || fb_point_entity>=0)) {
        fb_tx_begin("Select");
        brush_deselectall();
        fb_restore_entity_selection(fb_point_entity);
        if (fb_brush) {
            brush_select(fb_model,fb_brush);
        }
        fb_tx_commit();
    } else if(fb_owner==FB_OWNER_SELECTION) {
        // Dragging empty space with an existing selection does not create geometry.
    } else if (fb_moving) {
        if (vlen(fb_applied_offset)>0 || fb_transform_duplicate) {
            fb_transform_commit(fb_applied_offset);
        }
    } else {
        if(!fb_primitive_create(fb_boxcenter-fb_boxsize*0.5,fb_boxcenter+fb_boxsize*0.5,FALSE)) print("ReyondBroom: shape could not be created.\n");
    }
    int selectedPatch=!fb_dragged && fb_pick_kind==FB_HIT_PATCH && brush_isselected(fb_model,fb_brush);
    fb_cancel();
    if(selectedPatch)fb_tool_activate(FB_TOOL_CONTROLPOINT);
}

// Cache overlay vertices and indices; camera motion never rebuilds geometry.
typedef struct {
    int id,model,node,count,u,v,sky,brushEnd;
    vector center,normal;vector *points;
    float spacing,radius;
    trisoup_simple_vert_t *vertices;
    int *triangles,*lines;int occluder;
} fb_grid_face_t;
int fb_occlusion_revision;int fb_generator_count,fb_render_access_count;int *fb_render_access;
typedef struct {int stride,flags;string gridShader,edgeShader;int *visible;int revision;} fb_grid_native_params_t;
int(fb_grid_face_t *faces,int count,vector *bounds,int *access,int accessCount,fb_grid_native_params_t *params) ftebroom_draw_grid_compact=#0:ftebroom_draw_grid_compact;
int(fb_grid_face_t *faces,int count,vector *bounds,int *access,int accessCount,fb_grid_native_params_t *params) ftebroom_draw_grid_cached=#0:ftebroom_draw_grid_cached;
int fb_grid_upload_epoch;
int(fb_grid_face_t *faces,int count,int *access,int accessCount,int stride,int mode,int revision) ftebroom_prepare_occlusion=#0:ftebroom_prepare_occlusion;
var float autocvar_ftebroom_geometry_occlusion=1;
var float autocvar_ftebroom_occlusion_culling=1;
int fb_face_occluder(brushface_t *face,int contents,int node,int model);
int fb_grid_built_epoch;
fb_grid_face_t *fb_grid_faces;
int fb_grid_face_count,fb_grid_face_capacity;
int *fb_grid_visible;int fb_grid_visible_count;
vector *fb_grid_packed_points;
vector *fb_grid_brush_bounds;
trisoup_simple_vert_t *fb_grid_packed_vertices;
int *fb_grid_packed_triangles;
int *fb_grid_packed_lines;int fb_grid_vertex_capacity;
int fb_sky_material(string material);
var float autocvar_ftebroom_sky_overlays=0;
void fb_surfacegrid(int geometryOnly)
{
    static int epoch,valid,materialEpoch;
    static brushface_t gridPlanes[MAX_BRUSHFACES];
    static vector lastEye,lastAngles,lastSize,lastMin,lastFov;
    static float lastGrid,lastAlpha,lastRotation;
    static int lastFilters,lastVisibility,lastSky,wasPreview;
    vector eye=getproperty(VF_ORIGIN),angles=getproperty(VF_ANGLES),size=getproperty(VF_SIZE);
    vector viewport=getproperty(VF_MIN),fov=getproperty(VF_FOV);
    float grid=fb_grid(),alpha=bound(0,cvar("ftebroom_grid_alpha"),1);
    int currentMaterials=cvar("ftebroom_material_tag_epoch");int rebuild=!valid || epoch!=fb_grid_geometry_epoch || materialEpoch!=currentMaterials;
    if(rebuild) {
        epoch=fb_grid_geometry_epoch;valid=TRUE;materialEpoch=currentMaterials;
        // Four contiguous allocations replace four allocations per face. The
        // QC heap otherwise spends seconds walking hundreds of thousands of blocks.
        int required=0,vertexOffset=0;static vector gridBounds[2];
        for(int node=2;node<fb_node_count;node++)if(fb_nodes[node].kind==FB_NODE_BRUSH){
            int faces=brush_get(fb_nodes[node].model,fb_nodes[node].id,__NULL__,0,__NULL__);
            for(int face=1;face<=min(faces,MAX_BRUSHFACES);face++)required+=brush_getfacepoints(fb_nodes[node].model,fb_nodes[node].id,face,facepoints,MAX_FACEPOINTS);
        }
        if(required>fb_grid_vertex_capacity){
            fb_grid_vertex_capacity=max(required,max(1024,fb_grid_vertex_capacity*2));
            memfree(fb_grid_packed_points);memfree(fb_grid_packed_vertices);memfree(fb_grid_packed_triangles);memfree(fb_grid_packed_lines);
            fb_grid_packed_points=memalloc(fb_grid_vertex_capacity*sizeof(vector));
            fb_grid_packed_vertices=memalloc(fb_grid_vertex_capacity*2*sizeof(trisoup_simple_vert_t));
            fb_grid_packed_triangles=memalloc(fb_grid_vertex_capacity*3*sizeof(int));fb_grid_packed_lines=memalloc(fb_grid_vertex_capacity*2*sizeof(int));
        }
        memfree(fb_grid_brush_bounds);fb_grid_brush_bounds=memalloc(fb_node_count*2*sizeof(vector));
        fb_grid_face_count=0;
        for(int b=2;b<fb_node_count;b++) {
          if(fb_nodes[b].kind!=FB_NODE_BRUSH)continue;
          int model=fb_nodes[b].model,id=fb_nodes[b].id;
          if(brush_getfacepoints(model,id,0,gridBounds,2)==2){fb_grid_brush_bounds[b*2]=(gridBounds[0]+gridBounds[1])*.5;fb_grid_brush_bounds[b*2+1]=[vlen(gridBounds[1]-gridBounds[0])*.5+.1,0,0];}
          else fb_grid_brush_bounds[b*2+1]='1000000000 0 0';
          int firstFace=fb_grid_face_count;
          int gridContents;int planeCount=brush_get(model,id,gridPlanes,MAX_BRUSHFACES,&gridContents);
          for(int f=1;f<=min(planeCount,MAX_BRUSHFACES);f++) {
            int n=brush_getfacepoints(model,id,f,facepoints,MAX_FACEPOINTS);
            if(n<3) break;
            if(fb_grid_face_count==fb_grid_face_capacity) {
                int capacity=max(256,fb_grid_face_capacity*2);
                fb_grid_face_t *items=memalloc(sizeof(fb_grid_face_t)*capacity);
                if(fb_grid_face_count) memcpy(items,fb_grid_faces,sizeof(fb_grid_face_t)*fb_grid_face_count);
                if(fb_grid_faces) memfree(fb_grid_faces);
                fb_grid_faces=items;fb_grid_face_capacity=capacity;
            }
            fb_grid_face_t *face=&fb_grid_faces[fb_grid_face_count++];
            face.id=id;face.model=model;face.node=b;face.count=n;face.center='0 0 0';
            face.points=&fb_grid_packed_points[vertexOffset];face.vertices=&fb_grid_packed_vertices[vertexOffset*2];
            face.triangles=&fb_grid_packed_triangles[vertexOffset*3];face.lines=&fb_grid_packed_lines[vertexOffset*2];vertexOffset+=n;
            for(int j=0;j<n;j++) {
                int next=j+1;if(next>=n)next=0;
                face.lines[j*2]=j;face.lines[j*2+1]=next;
                if(j>=2){face.triangles[(j-2)*3]=0;face.triangles[(j-2)*3+1]=j-1;face.triangles[(j-2)*3+2]=j;}
            }
            memcpy(face.points,facepoints,sizeof(vector)*n);
            face.normal=gridPlanes[f-1].planenormal;
            face.sky=(gridPlanes[f-1].surfaceflags&4)!=0 || fb_sky_material(gridPlanes[f-1].shadername);
            face.occluder=fb_face_occluder(&gridPlanes[f-1],gridContents,b,model);
            face.u=0;face.v=1;
            if(fabs(face.normal[0])>fabs(face.normal[1]) && fabs(face.normal[0])>fabs(face.normal[2])) {face.u=1;face.v=2;}
            else if(fabs(face.normal[1])>fabs(face.normal[2])) {face.u=0;face.v=2;}
            for(int j=0;j<n;j++) face.center+=facepoints[j];
            face.center=face.center / (float)n;
            face.radius=0;for(int j=0;j<n;j++)face.radius=max(face.radius,vlen(face.points[j]-face.center));
        }
          for(int face=firstFace;face<fb_grid_face_count;face++)fb_grid_faces[face].brushEnd=fb_grid_face_count;
        }
        memfree(fb_grid_visible);fb_grid_visible=memalloc(max(1,fb_grid_face_count)*sizeof(int));fb_grid_built_epoch=fb_grid_geometry_epoch;fb_occlusion_revision++;
    }
    if(geometryOnly) {lastGrid=-1;wasPreview=TRUE;return;}
    static vector lastGridOrigin,lastGridX,lastGridY,lastGridZ;
    int gridChanged=lastGridOrigin!=autocvar_ftebroom_grid_origin||lastGridX!=autocvar_ftebroom_grid_basis_x||lastGridY!=autocvar_ftebroom_grid_basis_y||lastGridZ!=autocvar_ftebroom_grid_basis_z;
    lastGridOrigin=autocvar_ftebroom_grid_origin;lastGridX=autocvar_ftebroom_grid_basis_x;lastGridY=autocvar_ftebroom_grid_basis_y;lastGridZ=autocvar_ftebroom_grid_basis_z;
    if(rebuild || gridChanged || grid!=lastGrid || alpha!=lastAlpha || lastRotation!=autocvar_ftebroom_grid_rotation){fb_grid_upload_epoch++;for(int i=0;i<fb_grid_face_count;i++) {
        fb_grid_face_t *face=&fb_grid_faces[i];
        vector gridNormal=fb_grid_direction_local(face.normal);face.u=0;face.v=1;
        if(fabs(gridNormal_x)>fabs(gridNormal_y)&&fabs(gridNormal_x)>fabs(gridNormal_z)){face.u=1;face.v=2;}
        else if(fabs(gridNormal_y)>fabs(gridNormal_z)){face.u=0;face.v=2;}
        for(int j=0;j<face.count;j++) {
            for(int a=0;a<3;a++) {
                face.vertices[j].xyz[a]=face.vertices[j+face.count].xyz[a]=face.points[j][a]+face.normal[a]*0.02;
                face.vertices[j+face.count].rgba[a]=1;
            }
            face.vertices[j].rgba[0]=0.65;face.vertices[j].rgba[1]=0.68;face.vertices[j].rgba[2]=0.72;
            face.vertices[j].rgba[3]=alpha;face.vertices[j+face.count].rgba[3]=1;
            vector gridPoint=fb_grid_local(face.points[j]);face.vertices[j].st[0]=gridPoint[face.u]/grid;face.vertices[j].st[1]=gridPoint[face.v]/grid;
        }
    }
    }
    lastAlpha=alpha;lastRotation=autocvar_ftebroom_grid_rotation;
    int preview=fb_active;
    static int lastSelection,lastTool;
    int selectionChanged=lastSelection!=fb_selection_epoch || lastTool!=cvar("ftebroom_tool");
    lastSelection=fb_selection_epoch;lastTool=cvar("ftebroom_tool");
    int recull=rebuild || selectionChanged || eye!=lastEye || angles!=lastAngles || size!=lastSize || viewport!=lastMin || fov!=lastFov || lastFilters!=fb_filter_epoch || lastVisibility!=fb_visibility_epoch || lastSky!=autocvar_ftebroom_sky_overlays || preview || wasPreview;
    lastFilters=fb_filter_epoch;lastVisibility=fb_visibility_epoch;lastSky=autocvar_ftebroom_sky_overlays;wasPreview=preview;
    lastEye=eye;lastAngles=angles;lastSize=size;lastMin=viewport;lastFov=fov;lastGrid=grid;
    // Overlays are coplanar with world surfaces and draw after them. A bare
    // polygonoffset uses FTE's 25-LSB bias, which pushes distant grid lines in
    // front of nearer geometry; the default GL_LEQUAL depth test is enough.
    shaderforname("ftebroom/surfacegrid","{\ncull none\nprogram ftebroom_grid\n{\nmap textures/ftebroom/overlay.tga\nblendfunc blend\nrgbgen vertex\nalphagen vertex\n}\n}");
    int previous=-1,replaced=FALSE;
    makevectors(angles);vector forward=v_forward,right=v_right,up=v_up;
    float tx=tan(fov_x*M_PI/360),ty=tan(fov_y*M_PI/360);
    int gridVisible=cvar("ftebroom_grid_visible"),edges=cvar("ftebroom_show_edges");
    shaderforname("ftebroom/brushedges","{\ncull none\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\n}\n}");
    if(autocvar_ftebroom_native_overlays && checkbuiltin(ftebroom_draw_grid_compact) && !fb_active && !fb_transform_preview && !fb_vertex_preview_valid && !fb_generator_count && cvar("ftebroom_tool")!=FB_TOOL_CLIP){
        fb_render_allowed(0,FB_PICK_LOCKED);fb_grid_native_params_t params;params.stride=sizeof(fb_grid_face_t);params.flags=8|(gridVisible?1:0)|(edges?2:0)|(autocvar_ftebroom_sky_overlays?4:0);params.gridShader="ftebroom/surfacegrid";params.edgeShader="ftebroom/brushedges";params.visible=fb_grid_visible;params.revision=fb_grid_upload_epoch;
        int nativeCount;if(autocvar_ftebroom_cached_meshes && fb_grid_face_count>=30000 && checkbuiltin(ftebroom_draw_grid_cached))nativeCount=ftebroom_draw_grid_cached(fb_grid_faces,fb_grid_face_count,fb_grid_brush_bounds,fb_render_access,fb_render_access_count,&params);else nativeCount=ftebroom_draw_grid_compact(fb_grid_faces,fb_grid_face_count,fb_grid_brush_bounds,fb_render_access,fb_render_access_count,&params);
        if(nativeCount>=0){fb_grid_visible_count=nativeCount;return;}
    }
    // Visibility is shared by the grid and edge passes.
    if(recull){fb_grid_visible_count=0;
    for(int i=0;i<fb_grid_face_count;i++) {
        fb_grid_face_t *face=&fb_grid_faces[i];
        if(face.node!=previous) {
            previous=face.node;replaced=fb_preview_replaces(face.model,face.id);if(!fb_render_allowed(face.node,FB_PICK_LOCKED))replaced=TRUE;
            vector brushDelta=fb_grid_brush_bounds[face.node*2]-eye;float depth=brushDelta*forward,extent=fb_grid_brush_bounds[face.node*2+1][0];
            if(depth+extent<0||fabs(brushDelta*right)>depth*tx+extent*(1+tx)||fabs(brushDelta*up)>depth*ty+extent*(1+ty))replaced=TRUE;
        }
        if(replaced){i=face.brushEnd-1;continue;}
        if(face.sky && !autocvar_ftebroom_sky_overlays)continue;
        if((eye-face.center)*face.normal< -0.01)continue;
        vector delta=face.center-eye;float z=delta*forward,radius=face.radius;
        if(z+radius<0)continue;
        if(fabs(delta*right)>z*tx+radius*(1+tx) || fabs(delta*up)>z*ty+radius*(1+ty))continue;
        fb_grid_visible[fb_grid_visible_count++]=i;
    }
    }
    for(int pass=0;pass<2;pass++) {
        if(!pass && !gridVisible)continue;if(pass && !edges)continue;
        for(int i=0;i<fb_grid_visible_count;i++) {
            fb_grid_face_t *face=&fb_grid_faces[fb_grid_visible[i]];
            if(!pass)addtrisoup_simple("ftebroom/surfacegrid",3,face.vertices,face.triangles,(face.count-2)*3);
            else addtrisoup_simple("ftebroom/brushedges",3|DRAWFLAG_LINES,&face.vertices[face.count],face.lines,face.count*2);
        }
    }
}

void fb_prepare_occlusion()
{
    if(!checkbuiltin(ftebroom_prepare_occlusion))return;
    int enabled=autocvar_ftebroom_occlusion_culling && cvar("ftebroom_quake1") && !cvar("ca_brush_view") && !fb_active && !fb_transform_preview && !fb_vertex_preview_valid && !fb_generator_count && cvar("ftebroom_tool")!=FB_TOOL_CLIP && cvar("ftebroom_render_faces")!=2 && fb_grid_built_epoch==fb_grid_geometry_epoch;
    int mode=enabled?2:0;fb_render_allowed(0,FB_PICK_LOCKED);
    ftebroom_prepare_occlusion(fb_grid_faces,fb_grid_face_count,fb_render_access,fb_render_access_count,sizeof(fb_grid_face_t),mode,fb_occlusion_revision);
}
#include "ftebroom_edit.qc"

void ftebroom_prepare()
{
    fb_grid();
    fb_document_tick();
    fb_material_usage();
    fb_entity_publish();
    fb_uv_publish();
    fb_filter_prepare();
    fb_view_remember_selection();
    curmousepos=[cvar("ftebroom_mouse_x"),cvar("ftebroom_mouse_y"),0];
    fb_tool_pointer_move(curmousepos,cvar("ftebroom_alt"),cvar("ca_brush_view"));
    fb_visibility_sync();
}
void fb_trace_draw();
void fb_issues_tick();
float fb_frame_profile[4];
void fb_generator_draw();
void fb_symmetry_draw();
void fb_array_draw();
void ftebroom_frame()
{
    float profileTime=gettime(1);
    fb_trace_draw();
    fb_issues_tick();
    fb_filter_draw();
    fb_frame_profile[0]+=gettime(1)-profileTime;profileTime=gettime(1);
    fb_pick_draw_entities(FALSE);
    fb_frame_profile[1]+=gettime(1)-profileTime;profileTime=gettime(1);
    fb_transform_draw();
    fb_extrude_draw();
    fb_array_draw();fb_generator_draw();fb_symmetry_draw();
    if (fb_active && fb_dragged && fb_owner==FB_OWNER_SHAPE) {
        fb_primitive_preview(fb_boxcenter-fb_boxsize*0.5,fb_boxcenter+fb_boxsize*0.5);
    }
    if(!cvar("ca_brush_view"))if(cvar("ftebroom_grid_visible") || cvar("ftebroom_show_edges"))fb_surfacegrid(FALSE);
    fb_frame_profile[2]+=gettime(1)-profileTime;profileTime=gettime(1);
    if (cvar("ftebroom_grid_visible") && !cvar("ca_brush_view")) {
        shaderforname("ftebroom/worldgrid","{\n{\nmap $whiteimage\nblendfunc blend\nrgbgen vertex\nalphagen vertex\n}\n}");
        // Keep the construction plane just behind a coincident brush surface.
        float z=cvar("ftebroom_plane")-0.05;
        float g=fb_grid_display_step([0,0,z],'0 0 1');
        for (int i=-32;i<=32;i++) {
            R_BeginPolygon("ftebroom/worldgrid");
            R_PolygonVertex(fb_grid_world([i*g,-32*g,z]),'0 0','0.16 0.18 0.2',bound(0,cvar("ftebroom_grid_alpha"),1));
            R_PolygonVertex(fb_grid_world([i*g,32*g,z]),'0 0','0.16 0.18 0.2',bound(0,cvar("ftebroom_grid_alpha"),1)); R_EndPolygon();
            R_BeginPolygon("ftebroom/worldgrid");
            R_PolygonVertex(fb_grid_world([-32*g,i*g,z]),'0 0','0.16 0.18 0.2',bound(0,cvar("ftebroom_grid_alpha"),1));
            R_PolygonVertex(fb_grid_world([32*g,i*g,z]),'0 0','0.16 0.18 0.2',bound(0,cvar("ftebroom_grid_alpha"),1)); R_EndPolygon();
        }
    }
    fb_tool_draw();
    fb_tool_panel_state();
    fb_material_state();
    int selectionContext=0;
    if(fb_selected_entity_count)selectionContext=1;
    if(selectedbrushcount) {
        selectionContext=1;
        for(int b=0;b<selectedbrushcount;b++)
            if(!selbrush_whole(&selectedbrushes[b])) selectionContext=2;
    }
    cvar_set("ftebroom_ctx_selection",itos(selectionContext));
    int view=cvar("ca_brush_view");
    cvar_set("ftebroom_ctx_view",itos(view==3?1:(view==2?2:(view==1?3:0))));
    cvar_set("ftebroom_selection_count",itos(selectedbrushcount));
    cvar_set("ftebroom_entity_selection_count",itos(fb_selected_entity_count));
    int openNode=fb_selection_open_node();
    cvar_set("ftebroom_open_group",openNode>=0?fb_nodes[openNode].name:"");
    fb_frame_profile[3]+=gettime(1)-profileTime;
}
float() ftebroom_command =
{
    if(argv(0)=="ftebroom_bsp") {
        if(argv(1)=="edit")fb_build_preview_leave();
        else fb_build_preview_enter(argv(1));
        return TRUE;
    }
    // Editing a document always restores its world before touching geometry.
    if(fb_build_preview_active && substring(argv(0),0,9)=="ftebroom_" &&
        substring(argv(0),0,16)!="ftebroom_camera_" && argv(0)!="ftebroom_camera") {
        if(argv(0)=="ftebroom_pointer")return TRUE;
        fb_build_preview_leave();
    }
    if(fb_document_command()) return TRUE;
    if(argv(0)=="ftebroom_csg") {fb_csg(argv(1));return TRUE;}
    if(argv(0)=="ftebroom_shape_apply") {fb_primitive_apply();return TRUE;}
    if(argv(0)=="ftebroom_visibility") {fb_visibility_command(argv(1));return TRUE;}
    if(fb_entity_command()) return TRUE;
    if(fb_context_command())return TRUE;
    if(argv(0)=="ftebroom_tag") {fb_filter_tag(argv(1),stoi(argv(2)));return TRUE;}
    if(argv(0)=="ftebroom_export_map") {fb_export_map();return TRUE;}
    if(argv(0)=="ftebroom_group") {fb_group_create(argv(1));return TRUE;}
    if(argv(0)=="ftebroom_organize") {fb_organize_action(argv(1),stoi(argv(2)),argv(3));return TRUE;}
    if(argv(0)=="ftebroom_vertex_delete") {fb_vertex_delete();return TRUE;}
    if(argv(0)=="ftebroom_vertex_snap") {float spacing=1;if(argv(1)=="grid") spacing=fb_grid();fb_vertex_snap(spacing);return TRUE;}
    if(argv(0)=="ftebroom_scale") {fb_scale_numeric([stof(argv(2)),stof(argv(3)),stof(argv(4))],argv(1)=="size");return TRUE;}
    if(argv(0)=="ftebroom_line_index") {fb_lines_index();return TRUE;}
    if(argv(0)=="ftebroom_select_line") {fb_select_line(stoi(argv(1)));return TRUE;}
    if(argv(0)=="ftebroom_select") {fb_select_command(argv(1));return TRUE;}
    if(fb_grid_command()) return TRUE;
    if(fb_tools_command()) return TRUE;
    if(argv(0)=="ftebroom_deselect") {fb_tool_refresh();brush_history_deselect();return TRUE;}
    if(argv(0)=="ftebroom_move") {fb_move_numeric([stof(argv(1)),stof(argv(2)),stof(argv(3))]);return TRUE;}
    if(argv(0)=="ftebroom_undo") {fb_tool_refresh();brush_undo();return TRUE;}
    if(argv(0)=="ftebroom_redo") {fb_tool_refresh();brush_redo();return TRUE;}
    if(argv(0)=="ftebroom_delete" || argv(0)=="brushedit_delete") {
        if(fb_vertex_tool()) {fb_vertex_delete();return TRUE;}
        fb_delete_selection();return TRUE;
    }
    if(argv(0)=="ftebroom_duplicate") {fb_duplicate_numeric('0 0 0');return TRUE;}
    if(argv(0)=="ftebroom_nudge" || argv(0)=="ftebroom_duplicate_nudge") {
        vector offset='0 0 0';int axis=stoi(argv(1));
        if(axis<0 || axis>2) return TRUE;
        offset=fb_transform_nudge_axis(axis,cvar("ca_brush_view"),getproperty(VF_ANGLES))*stof(argv(2))*fb_grid();
        if(cvar("ftebroom_tool")==FB_TOOL_ROTATE) {
            vector pivot=fb_rotate_center+offset;if(fb_geom_point(pivot)) {fb_rotate_center=pivot;fb_rotate_publish();}
        }
        else if(fb_rotating_grid()){vector before=fb_rotate_center;fb_rotate_center+=offset;fb_rotate_record_center(before,fb_rotate_center);fb_rotate_publish();}
        else if(fb_vertex_tool()) fb_vertex_nudge(offset);
        else if(argv(0)=="ftebroom_duplicate_nudge") fb_duplicate_numeric(offset);
        else {fb_tx_begin("Nudge");fb_move_numeric(offset);fb_tx_commit();}
        return TRUE;
    }
    if (argv(0)=="ftebroom_pointer") {
        vector pixel=[stof(argv(2)),stof(argv(3)),0]; float alt=stof(argv(4));
        if(cvar("ftebroom_terrain_edit")) {
            if(argv(1)=="down")cvar_set("ftebroom_terrain_paint","1");
            if(argv(1)=="up"||argv(1)=="cancel")cvar_set("ftebroom_terrain_paint","0");
            return TRUE;
        }
        float oldCtrl=cvar("ftebroom_ctrl"),oldShift=cvar("ftebroom_shift");
        if(argv(6)!="") {
            int modifiers=stoi(argv(6));
            cvar_set("ftebroom_ctrl",(modifiers&1)?"1":"0");
            cvar_set("ftebroom_shift",(modifiers&2)?"1":"0");
        }
        if(cvar("ftebroom_test_pointer_trace")) print(sprintf("FTEBROOM_POINTER %s alt=%g ctrl=%g shift=%g\n",argv(1),alt,cvar("ftebroom_ctrl"),cvar("ftebroom_shift")));
        if(argv(1)=="down") {
            fb_tool_pointer_down(pixel,alt,cvar("ca_brush_view"));
        }
        else if(argv(1)=="up") fb_tool_pointer_up(pixel,alt,cvar("ca_brush_view"));
        else if(argv(1)=="dblclick" && cvar("ftebroom_tool")==FB_TOOL_CLIP) fb_clip_double(pixel,cvar("ca_brush_view"));
        else if(argv(1)=="dblclick" && cvar("ftebroom_tool")==FB_TOOL_BRUSH) fb_assemble_double(pixel,cvar("ca_brush_view"));
        else if(argv(1)=="dblclick" && !cvar("ftebroom_tool")) {
            int transferred=FALSE;
            if(alt && !cvar("ca_brush_view") && fb_faces_count()==1) {
                if(fb_transfer_begin(pixel,cvar("ftebroom_shift"),cvar("ftebroom_ctrl"))) {fb_transfer_at(pixel,TRUE);fb_transfer_finish(TRUE);transferred=TRUE;}
            }
            if(!transferred) {
                if(cvar("ftebroom_shift") && !cvar("ca_brush_view")) fb_faces_double(pixel,cvar("ftebroom_ctrl"),alt);
                else fb_selection_double(pixel,cvar("ftebroom_ctrl"));
            }
        }
        else if(argv(1)=="drill" && !fb_active && !cvar("ftebroom_tool")) {fb_pick(pixel);fb_selection_drill_hits(alt>0?1:-1);}
        else if(argv(1)=="cancel") {fb_cancel();fb_owner=FB_OWNER_NONE;}
        cvar_set("ftebroom_ctrl",ftos(oldCtrl));cvar_set("ftebroom_shift",ftos(oldShift));
        return TRUE;
    }
    if(argv(0)=="ftebroom_material_drop"){fb_material_drop(argv(1),[stof(argv(2)),stof(argv(3)),0]);return TRUE;}
    if(argv(0)=="ftebroom_material_replace") {fb_material_replace();return TRUE;}
    if(argv(0)=="ftebroom_texture_align") {fb_texture_align(argv(1),stoi(argv(2)),stoi(argv(3)));return TRUE;}
    if(argv(0)=="ftebroom_texture_key") {fb_texture_key(argv(1),stoi(argv(2)),stoi(argv(3)));return TRUE;}
    if(argv(0)=="ftebroom_material_select") {fb_material_select(argv(1));return TRUE;}
    if(argv(0)=="ftebroom_material") {if(argv(1)!="") cvar_set("ca_newbrushtexture",argv(1));fb_material_edit("material",0);return TRUE;}
    if(argv(0)=="ftebroom_uv") {fb_material_change(argv(1),stof(argv(2)),stoi(argv(3)));return TRUE;}
    if(argv(0)=="ftebroom_flags") {fb_material_flags(argv(1),stoi(argv(2)),stoi(argv(3)));return TRUE;}
    if(argv(0)=="ftebroom_clip") {if(argv(1)=="apply") fb_clip_apply(); else fb_clip_count=0; return TRUE;}
    if (argv(0)=="ftebroom_look") {
        fb_camera_input("look",stof(argv(1)),stof(argv(2)),0,0);
        return TRUE;
    }
    if (argv(0)!="ftebroom_box") return FALSE;
    if(argv(6)=="") { print("Usage: ftebroom_box x y z width depth height\n"); return TRUE; }
    vector center=[stof(argv(1)),stof(argv(2)),stof(argv(3))];
    vector size=[stof(argv(4)),stof(argv(5)),stof(argv(6))];
    int id=fb_createbox(center,size);
    if(id) print(sprintf("ReyondBroom: created brush %i.\n",id));
    else print("ReyondBroom: invalid box dimensions or position.\n");
    return TRUE;
};
