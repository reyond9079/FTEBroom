// SPDX-License-Identifier: GPL-2.0-or-later
void fb_vertex_expect(int ok,string label) {print(strcat(ok?"FTEBROOM_VERTEX_PASS ":"FTEBROOM_VERTEX_FAIL ",label,"\n"));}
float fb_vertex_testcommand()
{
    if(argv(0)!="ftebroom_test_vertex" || !cvar("ftebroom_skip_setup")) return FALSE;
    if(argv(1)=="subset82"){
        fb_cancel();fb_tool_deactivate();fb_selection_clear();int subset82A=fb_createbox('10000 10000 128','128 128 128'),subset82B=fb_createbox('10400 10000 128','128 128 128');
        brush_deselectall();brush_select(1,subset82A);brush_select(1,subset82B);fb_tool_activate(FB_TOOL_VERTEX);fb_vertex_rebuild();fb_vertex_deselect();fb_vertex_handles[0].selected=TRUE;fb_vertex_drag=1;
        int subset82Valid=fb_vertex_build('0 0 8',FALSE,0),subset82Changed=0,subset82Replaced=0;
        for(int i=0;i<fb_vertex_brushcount;i++){subset82Changed+=fb_vertex_brushes[i].changed;subset82Replaced+=fb_vertex_replaces(fb_vertex_brushes[i].model,fb_vertex_brushes[i].id);}
        fb_vertex_expect(subset82Valid&&fb_vertex_brushcount==2&&subset82Changed==1&&subset82Replaced==1,"subset82_only_affected_brush_replaced");
        fb_poly_t subset82Before={0},subset82After={0};fb_poly_from_brush(1,subset82B,&subset82Before);fb_vertex_commit("Vertex subset");fb_document_tick();fb_poly_from_brush(1,subset82B,&subset82After);
        fb_vertex_expect(subset82Before.nv==subset82After.nv&&subset82Before.nf==subset82After.nf,"subset82_unaffected_topology");int subset82Equal=TRUE;for(int v=0;v<subset82Before.nv;v++)if(subset82Before.vertices[v]!=subset82After.vertices[v])subset82Equal=FALSE;fb_vertex_expect(subset82Equal,"subset82_unaffected_positions");fb_poly_free(&subset82Before);fb_poly_free(&subset82After);fb_vertex_cancel();return TRUE;
    }
    if(substring(argv(1),0,4)=="cone") {
        int coneMode=0;if(argv(1)=="cone2d")coneMode=2;if(argv(1)=="conealt")coneMode=1;
        fb_cancel();fb_selection_clear();
        cvar_set("ftebroom_shape","cone");cvar_set("ftebroom_shape_sides","32");cvar_set("ftebroom_shape_axis","2");
        cvar_set("ftebroom_shape_circle","edge");cvar_set("ftebroom_shape_hollow","0");
        int made=fb_primitive_create('-64 -64 0','64 64 128',FALSE);
        fb_tool_activate(FB_TOOL_VERTEX);
        int apexHandle=-1;for(int h=0;h<fb_vertex_count;h++)if(fb_vertex_handles[h].point[2]>127)apexHandle=h;
        fb_vertex_expect(made && apexHandle>=0,"cone32_apex_found");
        if(apexHandle>=0 && coneMode) {
            int gestureHistory=history;int gestureView=0;if(coneMode==2)gestureView=2;
            cvar_set("ca_brush_view",itos(gestureView));setproperty(VF_PERSPECTIVE,TRUE);fb_view_prepare();
            if(!gestureView){setproperty(VF_ORIGIN,'320 -400 280');setproperty(VF_ANGLES,'25 130 0');setproperty(VF_AFOV,90);setproperty(VF_MINDIST,1);}
            vector gestureStart=project('0 0 128'),gestureEnd=project('0 0 192');gestureStart_z=gestureEnd_z=0;
            int gestureAlt=coneMode==1;
            fb_vertex_down(gestureStart,gestureAlt,gestureView);
            fb_vertex_move(gestureEnd,gestureAlt,gestureView);
            fb_vertex_up(gestureEnd,gestureAlt,gestureView);
            fb_vertex_expect(history>gestureHistory && fb_vertex_selection_count()==1 && fabs(fb_vertex_handles[fb_vertex_count-1].point[2]-192)<0.02,coneMode==2?"cone32_2d_drag":"cone32_alt_vertical_drag");
            return TRUE;
        }
        if(apexHandle>=0) {
            fb_vertex_handles[apexHandle].selected=TRUE;
            int ok=fb_vertex_build('0 0 64',FALSE,0);
            fb_vertex_expect(ok,"cone32_apex_up");
            if(!ok)print(strcat("CONE_ERROR ",fb_geom_error,"\n"));
            if(ok) {
                int coneHistory=history;fb_vertex_commit("Move cone apex");
                fb_poly_t cone={0};fb_poly_from_brush(selectedbrushes[0].model,selectedbrushes[0].id,&cone);
                float high=-9999;for(int v=0;v<cone.nv;v++)high=max(high,cone.vertices[v][2]);
                fb_vertex_expect(cone.nf==33 && cone.nv==33 && fabs(high-192)<0.01,"cone32_commit_preserves_topology");fb_poly_free(&cone);
                brush_undo();fb_vertex_rebuild();fb_vertex_expect(history==coneHistory,"cone32_single_undo");

            }
        }
        return TRUE;
    }
    vector oldEye=getproperty(VF_ORIGIN),oldAngles=getproperty(VF_ANGLES);
    setproperty(VF_PERSPECTIVE,TRUE);setproperty(VF_AFOV,90);setproperty(VF_ORIGIN,'256 -256 128');setproperty(VF_ANGLES,'19.471 135 0');
    fb_vertex_anchor='0 0 0';fb_vertex_vertical_fallback=FALSE;
    vector screenAnchor=project(fb_vertex_anchor);screenAnchor_z=0;
    float nearVertical=fb_vertex_vertical_distance(screenAnchor-'0 10 0')-fb_vertex_vertical_distance(screenAnchor);
    float nearHorizontal=vlen(fb_view_plane_point(screenAnchor+'10 0 0',0,fb_vertex_anchor)-fb_view_plane_point(screenAnchor,0,fb_vertex_anchor));
    setproperty(VF_ORIGIN,'1024 -1024 512');screenAnchor=project(fb_vertex_anchor);screenAnchor_z=0;
    float farVertical=fb_vertex_vertical_distance(screenAnchor-'0 10 0')-fb_vertex_vertical_distance(screenAnchor);
    float farHorizontal=vlen(fb_view_plane_point(screenAnchor+'10 0 0',0,fb_vertex_anchor)-fb_view_plane_point(screenAnchor,0,fb_vertex_anchor));
    fb_vertex_expect(fabs(farVertical/nearVertical-4)<0.01,"vertical_drag_scales_with_camera_distance");
    fb_vertex_expect(fabs(farHorizontal/nearHorizontal-4)<0.01,"horizontal_drag_scales_with_camera_distance");
    setproperty(VF_ORIGIN,oldEye);setproperty(VF_ANGLES,oldAngles);
    fb_cancel();fb_selection_clear();fb_createbox('0 0 64','128 128 128');
    fb_tool_activate(FB_TOOL_VERTEX);fb_vertex_expect(fb_vertex_count==8,"cube_eight_handles");
    int handle=-1;for(int h=0;h<fb_vertex_count;h++) if(fb_vertex_handles[h].point=='64 64 128') handle=h;
    fb_vertex_handles[handle].selected=TRUE;int marker=history;
    int valid=fb_vertex_build('0 0 -16',FALSE,0);
    fb_vertex_expect(valid && fb_vertex_brushes[0].preview.nf==7,"nonplanar_face_triangulated");
    fb_vertex_expect(history==marker,"preview_does_not_change_history");
    cvar_set("ftebroom_uv_lock","1");fb_vertex_build('0 0 -16',FALSE,0);
    brushface_t top={0};fb_poly_t *source=&fb_vertex_brushes[0].source;fb_poly_t *preview=&fb_vertex_brushes[0].preview;
    for(int f=0;f<source->nf;f++)if(source->faces[f].attr.planenormal=='0 0 1')top=source->faces[f].attr;
    int uvValid=TRUE,uvSamples=0;
    for(int f=0;f<preview->nf;f++) {
        fb_poly_face_t face=preview->faces[f];if(face.attr.planenormal[2]<0.5)continue;
        for(int p=0;p<face.count;p++) {
            vector point=preview->vertices[preview->indices[face.start+p]],original=point;
            if(vlen(point-'64 64 112')<0.01)original_z+=16;
            if(fabs(point*face.attr.sdir+face.attr.sbias-original*top.sdir-top.sbias)>0.01)uvValid=FALSE;
            if(fabs(point*face.attr.tdir+face.attr.tbias-original*top.tdir-top.tbias)>0.01)uvValid=FALSE;
            uvSamples++;
        }
    }
    fb_vertex_expect(uvValid && uvSamples==6,"uv_lock_preserves_triangle_corner_coordinates");cvar_set("ftebroom_uv_lock","0");

    fb_vertex_cancel();fb_vertex_expect(!fb_vertex_preview_valid && history==marker,"cancel_preview");
    valid=fb_vertex_build('-128 0 0',FALSE,0);
    fb_vertex_expect(valid && fb_vertex_brushes[0].preview.nv==7,"merge_vertices");fb_vertex_cancel();
    fb_vertex_drag=1;fb_vertex_build('-128 0 0',FALSE,0);
    valid=fb_vertex_build('-144 0 0',FALSE,0);
    fb_vertex_expect(valid && fb_vertex_brushes[0].preview.nv==7,"merged_vertex_continues_without_splitting");
    fb_vertex_cancel();fb_vertex_build('0 0 0',FALSE,0);
    fb_vertex_expect(fb_vertex_brushes[0].preview.nv==8,"cancel_restores_merged_pair");fb_vertex_cancel();
    valid=fb_vertex_build('-64 -64 -64',FALSE,0);
    fb_vertex_expect(valid && fb_vertex_brushes[0].preview.nv==7,"interior_vertex_disappears");fb_vertex_cancel();
    fb_vertex_drag=1;fb_vertex_build('-64 -64 -64',FALSE,0);fb_vertex_build('0 0 16',FALSE,0);
    fb_vertex_expect(fb_vertex_brushes[0].preview.nv==7,"collapsed_vertex_does_not_reappear_during_drag");
    fb_vertex_cancel();fb_vertex_build('0 0 16',FALSE,0);
    fb_vertex_expect(fb_vertex_brushes[0].preview.nv==8,"cancel_restores_collapsed_vertex");fb_vertex_cancel();
    valid=fb_vertex_build('0 0 -16',FALSE,0);fb_vertex_commit("Move Geometry");
    fb_vertex_expect(fb_vertex_brushes[0].source.nf==7,"commit_geometry");
    brush_undo();fb_vertex_rebuild();fb_vertex_expect(history==marker && fb_vertex_brushes[0].source.nf==6,"undo_geometry");
    fb_vertex_handles[0].selected=TRUE;fb_vertex_delete();fb_vertex_expect(fb_vertex_brushes[0].source.nv==7,"delete_one_vertex");
    brush_undo();fb_vertex_rebuild();
    for(int h=0;h<5;h++) fb_vertex_handles[h].selected=TRUE;
    marker=history;fb_vertex_delete();fb_vertex_expect(history==marker,"reject_flat_delete_atomically");
    fb_vertex_deselect();cvar_set("ca_brush_view","2");cvar_set("ca_brush_viewsize","512");fb_view_prepare();
    vector a=project('-80 -64 144'),b=project('80 -64 112');a_z=b_z=0;
    fb_vertex_down(a,0,2);fb_vertex_up(b,0,2);fb_vertex_expect(fb_vertex_selection_count()==4,"lasso_four_vertices");
    fb_vertex_down(a,0,2);fb_vertex_up(b,0,2);fb_vertex_expect(!fb_vertex_selection_count(),"lasso_toggles_selection");
    fb_tool_activate(FB_TOOL_EDGE);fb_vertex_expect(fb_vertex_count==12,"cube_twelve_edges");
    fb_tool_activate(FB_TOOL_FACE);fb_vertex_expect(fb_vertex_count==6,"cube_six_faces");
    for(int h=0;h<fb_vertex_count;h++) if(fb_vertex_handles[h].point[2]==128) fb_vertex_handles[h].selected=TRUE;
    valid=fb_vertex_build('16 0 0',FALSE,0);fb_vertex_expect(valid && fb_vertex_brushes[0].preview.nf==6,"move_face_shears_cube");
    fb_vertex_cancel();
    fb_tool_activate(FB_TOOL_VERTEX);
    fb_vertex_adding=TRUE;fb_vertex_add_point='0 0 128';
    valid=fb_vertex_build('0 0 32',FALSE,0);
    fb_vertex_expect(valid && fb_vertex_brushes[0].preview.nv==9,"add_roof_vertex");
    fb_vertex_commit("Add Vertex");fb_vertex_expect(fb_vertex_selection_count()==1,"new_vertex_stays_selected");
    brush_undo();fb_vertex_rebuild();
    fb_vertex_handles[0].selected=TRUE;fb_vertex_nudge('16 0 0');
    fb_vertex_expect(fb_vertex_selection_count()==1,"nudge_preserves_handle_selection");
    brush_undo();fb_vertex_rebuild();
    fb_vertex_handles[0].selected=TRUE;
    fb_tool_activate(FB_TOOL_SCALE);
    fb_vertex_expect(fb_vertex_scope && fb_vertex_selection_count()==1,"scale_retains_vertex_scope");
    fb_tool_activate(FB_TOOL_EDGE);
    fb_vertex_expect(!fb_vertex_scope && fb_vertex_count==12,"edge_exits_affine_scope");
    fb_tool_deactivate();fb_hit_t hit={0};hit.point='64 0 64';hit.normal='1 0 0';
    vector start=project(hit.point),end=project('64 0 80');start_z=end_z=0;
    marker=history;
    int prepared=fb_moveface_prepare(hit,start,2);fb_moveface_move(end,2);
    fb_vertex_expect(prepared && fb_vertex_preview_valid && history==marker,"move_face_preview");
    fb_tool_escape();fb_vertex_expect(!fb_moveface_active && !fb_vertex_scope && history==marker,"move_face_escape");
    fb_moveface_prepare(hit,start,2);fb_moveface_up(end,2);
    fb_poly_t moved={0};fb_poly_from_brush(selectedbrushes[0].model,selectedbrushes[0].id,&moved);
    int sheared=moved.nv==8;
    for(int v=0;v<moved.nv;v++) if(moved.vertices[v][0]>0 && moved.vertices[v][2]!=16 && moved.vertices[v][2]!=144) sheared=FALSE;
    fb_poly_free(&moved);fb_vertex_expect(sheared,"move_face_2d_shear");
    brush_undo();fb_vertex_expect(history==marker,"move_face_single_undo");
    return TRUE;
}
