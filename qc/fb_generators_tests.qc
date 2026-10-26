#include "fb_bevel_extended_tests.qc"
// SPDX-License-Identifier: GPL-3.0-or-later
int fb_generators_test_history;
void fb_polygon_test_rectangle(vector center,float half,float height)
{
    fb_polygon_count=4;fb_polygon_closed=TRUE;fb_polygon_normal='0 0 1';
    fb_polygon_points[0]=center+[-half,-half,height];fb_polygon_points[1]=center+[half,-half,height];fb_polygon_points[2]=center+[half,half,height];fb_polygon_points[3]=center+[-half,half,height];
}

int fb_generators_testcommand()
{
    if(argv(0)!="ftebroom_test_generators"||!cvar("ftebroom_skip_setup"))return FALSE;
    if(argv(1)=="rubble72setup"){
        fb_cancel();fb_selection_clear();fb_createbox('0 0 160','256 32 256');fb_generators_test_history=history;fb_document_tick();return TRUE;
    }
    if(argv(1)=="rubble72check"){
        fb_transform_expect(cvar("ftebroom_plugin_status")==1&&cvar("ftebroom_settle_applied")==1&&selectedbrushcount==12,"rubble72_pipeline_complete");
        float rubbleCenter=0;for(int rubbleB=0;rubbleB<selectedbrushcount;rubbleB++){fb_poly_t rubblePoly={0};if(fb_poly_from_brush(selectedbrushes[rubbleB].model,selectedbrushes[rubbleB].id,&rubblePoly)){vector rubbleMid='0 0 0';for(int rubbleP=0;rubbleP<rubblePoly.nv;rubbleP++)rubbleMid+=rubblePoly.vertices[rubbleP];rubbleCenter+=rubbleMid_z/(float)rubblePoly.nv;}fb_poly_free(&rubblePoly);}
        fb_transform_expect(rubbleCenter/max(1,selectedbrushcount)<cvar("ftebroom_plugin_result_center_z")-2,"rubble72_fragments_fell");
        brush_undo();fb_transform_expect(selectedbrushcount==12,"rubble72_undo_settle");brush_undo();fb_transform_expect(history==fb_generators_test_history&&selectedbrushcount==1,"rubble72_undo_geometry");
        brush_redo();brush_redo();fb_transform_expect(selectedbrushcount==12,"rubble72_redo_pipeline");return TRUE;
    }
    if(argv(1)=="rubble72invalid"){
        fb_selection_clear();fb_createbox('600 0 160','64 32 64');int rubbleBefore=history;
        cvar_set("ftebroom_hole_min","10000");cvar_set("ftebroom_hole_max","10000");
        tokenize("ftebroom_plugin_batch perforate fracture");fb_plugin_batch_command();
        fb_transform_expect(history==rubbleBefore&&selectedbrushcount==1&&cvar("ftebroom_plugin_status")==-1,"rubble72_invalid_request_atomic");return TRUE;
    }
    if(argv(1)=="symmetry54patch"){fb_symmetry_patch_match_tests();return TRUE;}
    if(argv(1)=="symmetry54reload"){fb_symmetry_metadata_reload_test();return TRUE;}
    if(argv(1)=="symmetry54relink"){fb_symmetry_relink_tests();return TRUE;}
    if(argv(1)=="symmetry54"){fb_symmetry_settings_tests();return TRUE;}
    if(argv(1)=="bevel53"){fb_bevel_extended_tests();return TRUE;}
    if(argv(1)=="conversion50"){fb_conversion_tests();return TRUE;}
    if(argv(1)=="spawn49reload"){fb_spawn_reload_tests();return TRUE;}
    if(argv(1)=="spawn49"){fb_spawn_tests();return TRUE;}
    if(argv(1)=="array48"){fb_array_preview_tests();return TRUE;}
    if(argv(1)=="spin48"){fb_spin_tests();return TRUE;}
    if(argv(1)=="polygon47visible") {
        fb_transform_expect(fb_polygon_captured&&fb_polygon_valid&&fb_generator_count>0&&fb_polygon_count==4,"polygon47_persistent_preview");return TRUE;
    }
    if(argv(1)=="symmetry47reload"){fb_symmetry_reload_test();return TRUE;}
    if(argv(1)=="symmetry47"){fb_symmetry_tests();return TRUE;}
    if(argv(1)=="polygon47gesture") {
        fb_tool_deactivate();fb_selection_clear();fb_createbox('0 0 64','256 256 128');fb_document_tick();cvar_set("ca_brush_view","3");cvar_set("ca_brush_viewsize","512");fb_view_prepare();fb_tool_activate(FB_TOOL_POLYGONCUT);int polygonGestureHistory=history;cvar_set("ftebroom_polygon_depth","0");cvar_set("ftebroom_polygon_keep","0");
        vector corners[4];corners[0]='-64 -64 64';corners[1]='64 -64 64';corners[2]='64 64 64';corners[3]='-64 64 64';
        for(int i=0;i<4;i++){vector screen=project(corners[i]);screen_z=0;fb_tool_pointer_down(screen,0,3);fb_tool_pointer_up(screen,0,3);}
        fb_transform_expect(fb_polygon_count==4&&!fb_polygon_closed&&history==polygonGestureHistory,"polygon47_pointer_points");
        vector polygonFirst=project(corners[0]);polygonFirst_z=0;fb_tool_pointer_down(polygonFirst,0,3);fb_tool_pointer_up(polygonFirst,0,3);fb_transform_expect(fb_polygon_closed&&fb_polygon_valid&&history==polygonGestureHistory,"polygon47_click_close");
        vector second=project(corners[1]),moved=project(corners[1]+'16 0 0');second_z=moved_z=0;fb_tool_pointer_down(second,0,3);fb_polygon_lasttime=-100;fb_tool_pointer_move(moved,0,3);
        fb_transform_expect(fb_polygon_drag==2&&fb_polygon_points[1]=='80 -64 64',"polygon47_drag_point");fb_tool_escape();fb_transform_expect(!fb_polygon_drag&&fb_polygon_points[1]==corners[1]&&fb_polygon_valid,"polygon47_escape_drag");
        tokenize("ftebroom_polygon back");fb_polygon_command();fb_transform_expect(!fb_polygon_closed&&fb_polygon_count==4&&!fb_generator_count,"polygon47_reopen_contour");
        tokenize("ftebroom_polygon back");fb_polygon_command();fb_transform_expect(fb_polygon_count==3,"polygon47_remove_point");fb_tool_deactivate();fb_transform_expect(!fb_polygon_captured&&history==polygonGestureHistory,"polygon47_cancel_tool");
        fb_tool_activate(FB_TOOL_POLYGONCUT);fb_polygon_test_rectangle('0 0 0',48,128);fb_polygon_build();fb_camera_position('360 -420 320');setproperty(VF_CL_VIEWANGLES,'25 130 0');cvar_set("ca_brush_view","0");return TRUE;
    }
    if(argv(1)=="polygon47apply") {
        fb_tool_deactivate();fb_selection_clear();cvar_set("ca_grid","16");fb_createbox('20000 0 64','128 128 128');fb_document_tick();int polygonHistory=history;
        fb_polygon_capture(FALSE);fb_polygon_test_rectangle('20000 0 0',32,128);cvar_set("ftebroom_polygon_depth","0");cvar_set("ftebroom_polygon_keep","0");
        int cutOK=fb_polygon_build();float cutVolume=0;for(int i=0;i<fb_csg_count;i++)cutVolume+=fb_csg_results[i].poly.volume;
        fb_transform_expect(cutOK&&history==polygonHistory&&fabs(cutVolume-(128*128*128-64*64*128))<16,"polygon47_cut_preview");
        cvar_set("ftebroom_polygon_depth","1");cutOK=fb_polygon_build();cutVolume=0;for(int i=0;i<fb_csg_count;i++)cutVolume+=fb_csg_results[i].poly.volume;
        fb_transform_expect(cutOK&&fabs(cutVolume-(128*128*128-64*64*16))<16,"polygon47_cut_depth");
        cvar_set("ftebroom_polygon_keep","1");cutOK=fb_polygon_build();cutVolume=0;for(int i=0;i<fb_csg_count;i++)cutVolume+=fb_csg_results[i].poly.volume;
        fb_transform_expect(cutOK&&fabs(cutVolume-128*128*128)<16,"polygon47_keep_cutout");
        cutOK=fb_polygon_apply();fb_transform_expect(cutOK&&selectedbrushcount>1,"polygon47_apply");brush_undo();fb_transform_expect(history==polygonHistory&&selectedbrushcount==1,"polygon47_single_undo");
        fb_polygon_capture(TRUE);fb_polygon_test_rectangle('21000 0 0',32,0);int spawnHistory=history;cutOK=fb_polygon_apply();fb_poly_t spawnResult={0};fb_poly_from_brush(selectedbrushes[0].model,selectedbrushes[0].id,&spawnResult);
        fb_transform_expect(cutOK&&selectedbrushcount==1&&fabs(spawnResult.volume-64*64*16)<1,"polygon47_spawn_apply");fb_poly_free(&spawnResult);brush_undo();fb_transform_expect(history==spawnHistory,"polygon47_spawn_undo");return TRUE;
    }
    if(argv(1)=="polygon47") {
        fb_polygon_count=4;fb_polygon_closed=TRUE;fb_polygon_normal='0 0 1';fb_polygon_points[0]='0 0 0';fb_polygon_points[1]='128 0 0';fb_polygon_points[2]='128 128 0';fb_polygon_points[3]='0 128 0';
        int polygonOK=fb_polygon_decompose();fb_transform_expect(polygonOK&&fb_polygon_piece_count==1&&fb_polygon_pieces[0].count==4,"polygon47_convex_merge");
        if(!polygonOK)return TRUE;fb_poly_t polygonPrism={0};polygonOK=fb_polygon_prism(&fb_polygon_pieces[0],0,16,&polygonPrism);fb_transform_expect(polygonOK&&fabs(polygonPrism.volume-128*128*16)<1,"polygon47_spawn_volume");fb_poly_free(&polygonPrism);
        fb_polygon_count=6;fb_polygon_points[0]='0 0 0';fb_polygon_points[1]='128 0 0';fb_polygon_points[2]='128 64 0';fb_polygon_points[3]='64 64 0';fb_polygon_points[4]='64 128 0';fb_polygon_points[5]='0 128 0';
        polygonOK=fb_polygon_decompose();float polygonVolume=0;int polygonPieces=fb_polygon_piece_count;
        for(int i=0;i<fb_polygon_piece_count&&polygonOK;i++){polygonOK=fb_polygon_prism(&fb_polygon_pieces[i],0,16,&polygonPrism);polygonVolume+=polygonPrism.volume;fb_poly_free(&polygonPrism);}
        fb_transform_expect(polygonOK&&polygonPieces==2&&fabs(polygonVolume-12288*16)<1,"polygon47_concave_partition");
        for(int i=0;i<3;i++){vector swap=fb_polygon_points[i];fb_polygon_points[i]=fb_polygon_points[5-i];fb_polygon_points[5-i]=swap;}
        polygonOK=fb_polygon_decompose();fb_transform_expect(polygonOK&&fb_polygon_piece_count==2,"polygon47_reverse_winding");
        fb_polygon_count=4;fb_polygon_points[0]='0 0 0';fb_polygon_points[1]='128 128 0';fb_polygon_points[2]='128 0 0';fb_polygon_points[3]='0 128 0';polygonOK=fb_polygon_decompose();fb_transform_expect(!polygonOK&&!fb_polygon_piece_count,"polygon47_reject_self_intersection");
        fb_polygon_count=5;fb_polygon_points[0]='0 0 0';fb_polygon_points[1]='64 0 0';fb_polygon_points[2]='128 0 0';fb_polygon_points[3]='128 128 0';fb_polygon_points[4]='0 128 0';polygonOK=fb_polygon_decompose();fb_transform_expect(polygonOK&&fb_polygon_piece_count==1&&fb_polygon_pieces[0].count==4,"polygon47_collinear_point");fb_polygon_free_pieces();return TRUE;
    }
    if(argv(1)=="autosnap46") {
        fb_tool_deactivate();fb_selection_clear();int snapHistory=history;cvar_set("ftebroom_auto_integer_snap","1");fb_createbox('10000.25 0.25 64.25','128 128 128');
        fb_poly_t snapped={0};fb_poly_from_brush(selectedbrushes[0].model,selectedbrushes[0].id,&snapped);int integral=TRUE;
        for(int v=0;v<snapped.nv;v++)for(int axis=0;axis<3;axis++)if(fabs(snapped.vertices[v][axis]-rint(snapped.vertices[v][axis]))>.01)integral=FALSE;
        fb_transform_expect(integral&&fabs(snapped.volume-128*128*128)<16,"autosnap46_integral");fb_poly_free(&snapped);brush_undo();fb_transform_expect(history==snapHistory,"autosnap46_one_undo");brush_redo();
        fb_createbox('10000.25 256.25 64.25','.125 128 128');fb_poly_from_brush(selectedbrushes[0].model,selectedbrushes[0].id,&snapped);fb_transform_expect(snapped.nv==8&&snapped.volume>0,"autosnap46_preserves_thin_brush");fb_poly_free(&snapped);
        cvar_set("ftebroom_auto_integer_snap","0");return TRUE;
    }
    if(argv(1)=="subtract46") {
        fb_cancel();fb_tool_deactivate();fb_selection_clear();fb_createbox('12000 0 64','128 128 128');
        fb_poly_t subSource={0};fb_poly_from_brush(selectedbrushes[0].model,selectedbrushes[0].id,&subSource);
        fb_createbox('12000 0 64','64 64 64');int subHistory=history;fb_document_tick();
        print(sprintf("Subtract inputs %g nodes %g\n",(float)selectedbrushcount,(float)fb_node_count));fb_csg("subtract2");float subVolume=0;int subMaterials=TRUE;
        for(int i=0;i<selectedbrushcount;i++){fb_poly_t subPiece={0};fb_poly_from_brush(selectedbrushes[i].model,selectedbrushes[i].id,&subPiece);subVolume+=subPiece.volume;for(int f=0;f<subPiece.nf;f++)if(subPiece.faces[f].attr.shadername!=subSource.faces[0].attr.shadername)subMaterials=FALSE;fb_poly_free(&subPiece);}
        print(sprintf("Subtract output %g volume %g difference %g\n",(float)selectedbrushcount,subVolume,subVolume-(128*128*128-64*64*64)));fb_transform_expect(selectedbrushcount==26&&fabs(subVolume-(128*128*128-64*64*64))<16,"subtract46_regular_cells_volume");
        fb_transform_expect(subMaterials,"subtract46_source_materials");brush_undo();fb_transform_expect(history==subHistory&&selectedbrushcount==1,"subtract46_single_undo");brush_redo();fb_transform_expect(selectedbrushcount==26,"subtract46_redo");brush_undo();
        fb_csg_item_t *subCutters=memalloc(6*sizeof(fb_csg_item_t));
        static brushface_t subFaces[64];for(int f=0;f<subSource.nf;f++)subFaces[f]=subSource.faces[f].attr;
        fb_affine_t subRotation={0};subRotation.x='1 0 0';subRotation.y='0 .70710678 .70710678';subRotation.z='0 -.70710678 .70710678';subRotation.translation='0 45.254834 18.745166';fb_affine_prepare(&subRotation);
        for(int f=0;f<subSource.nf;f++)fb_affine_face(&subRotation,&subFaces[f],TRUE);
        fb_poly_from_faces(subFaces,subSource.nf,&subCutters[0].poly);int subFallback=fb_csg_grid_difference(&subSource,subCutters,1,0,0);
        fb_transform_expect(subFallback==-1&&!fb_csg_count,"subtract46_rotated_fallback");fb_poly_free(&subCutters[0].poly);
        for(int c=0;c<6;c++){
            for(int f=0;f<subSource.nf;f++){subFaces[f]=subSource.faces[f].attr;subFaces[f].planedist-=8+c*8;}
            fb_poly_from_faces(subFaces,subSource.nf,&subCutters[c].poly);
        }
        subFallback=fb_csg_grid_difference(&subSource,subCutters,6,0,0);fb_transform_expect(subFallback==-1&&!fb_csg_count,"subtract46_grid_budget_fallback");
        for(int c=0;c<6;c++)fb_poly_free(&subCutters[c].poly);memfree(subCutters);fb_poly_free(&subSource);return TRUE;
    }
    if(argv(1)=="bend46gesture") {
        fb_tool_deactivate();fb_selection_clear();fb_createbox('0 0 128','512 128 128');fb_document_tick();cvar_set("ca_brush_view","2");cvar_set("ca_brush_viewsize","1024");fb_view_prepare();fb_tool_activate(FB_TOOL_BEND);
        fb_bend_state.subdivisions=2;fb_bend_build();int gestureHistory=history;cvar_set("ftebroom_bend_mode","0");cvar_set("ftebroom_bend_component","0");
        vector bendStart=project(fb_bend_state.points[4]),bendEnd=project(fb_bend_state.points[4]+'0 0 64');bendStart_z=bendEnd_z=0;
        fb_bend_down(bendStart,0,2);fb_bend_lasttime=-100;fb_bend_move(bendEnd,0,2);
        fb_transform_expect(fb_bend_drag&&fb_bend_valid&&fabs(fb_bend_state.points[4][2]-192)<.05&&history==gestureHistory,"bend46_drag_preview");
        fb_tool_escape();fb_transform_expect(!fb_bend_drag&&fb_bend_state.points[4][2]==128&&history==gestureHistory,"bend46_escape_drag");
        fb_bend_down(bendStart,0,2);fb_bend_up(bendEnd,0,2);fb_transform_expect(fabs(fb_bend_state.points[4][2]-192)<.05&&history==gestureHistory,"bend46_release_keeps_preview");
        tokenize("ftebroom_undo");fb_bend_command();fb_transform_expect(fb_bend_state.points[4][2]==128,"bend46_local_undo");tokenize("ftebroom_redo");fb_bend_command();fb_transform_expect(fabs(fb_bend_state.points[4][2]-192)<.05,"bend46_local_redo");
        fb_tool_deactivate();fb_transform_expect(history==gestureHistory&&!fb_generator_count,"bend46_gesture_cancel_tool");
        fb_camera_position('480 -640 440');setproperty(VF_CL_VIEWANGLES,'22 126 0');cvar_set("ca_brush_view","0");fb_view_prepare();fb_tool_activate(FB_TOOL_BEND);fb_bend_state.subdivisions=3;fb_bend_state.points[4]+='0 0 128';fb_bend_handle=4;fb_bend_build();return TRUE;
    }
    if(argv(1)=="bend46") {
        fb_cancel();fb_tool_deactivate();fb_selection_clear();fb_createbox('16000 0 64','512 128 128');fb_document_tick();int bendHistory=history;
        fb_tool_activate(FB_TOOL_BEND);fb_transform_expect(fb_bend_valid&&fb_generator_count==1,"bend46_capture_straight");
        fb_bend_state.subdivisions=2;int bendOK=fb_bend_build();float bendVolume=0;for(int pieceIndex=0;pieceIndex<fb_generator_count;pieceIndex++)bendVolume+=fb_generator_items[pieceIndex].poly.volume;
        fb_transform_expect(bendOK&&fb_generator_count==4&&fabs(bendVolume-512*128*128)<16,"bend46_subdivision_volume");
        fb_bend_save();fb_bend_state.points[4]+='0 0 128';bendOK=fb_bend_build();int bendShared=0;
        if(bendOK)for(int part=0;part<fb_generator_count-1;part++){fb_poly_t *left=&fb_generator_items[part].poly;fb_poly_t *right=&fb_generator_items[part+1].poly;for(int aVertex=0;aVertex<left->nv;aVertex++)for(int bVertex=0;bVertex<right->nv;bVertex++)if(vlen(left->vertices[aVertex]-right->vertices[bVertex])<.05)bendShared++;}
        fb_transform_expect(bendOK&&bendShared==12&&history==bendHistory,"bend46_keep_joints_preview");
        fb_bend_state.scales[4]='.5 1.5 2';fb_bend_state.rotations[4]='45 0 0';bendOK=fb_bend_build();fb_transform_expect(bendOK,"bend46_scale_rotation");
        fb_bend_state.joints=FALSE;bendOK=fb_bend_build();fb_transform_expect(bendOK,"bend46_rigid_pieces");
        fb_bend_state.joints=TRUE;fb_bend_state.scales[4]='1 1 1';fb_bend_state.rotations[4]='0 0 0';bendOK=fb_bend_apply();
        fb_transform_expect(bendOK&&selectedbrushcount==4&&!fb_bend_count&&!cvar("ftebroom_tool"),"bend46_apply");brush_undo();fb_transform_expect(history==bendHistory&&selectedbrushcount==1,"bend46_single_undo");
        fb_tool_activate(FB_TOOL_BEND);fb_bend_save();fb_bend_state.points[4]+='0 0 32';fb_bend_drag=TRUE;fb_bend_cancel_drag();fb_transform_expect(fb_bend_state.points[4][2]==64&&history==bendHistory,"bend46_cancel_handle");
        fb_tool_deactivate();fb_transform_expect(!fb_generator_count&&!fb_bend_count&&history==bendHistory,"bend46_cancel_tool");return TRUE;
    }
    if(argv(1)=="array45") {
        cvar_set("ftebroom_array_spacing","128");cvar_set("ftebroom_array_centered","1");cvar_set("ftebroom_array_cross","0");cvar_set("ftebroom_array_mirror_offsets","1");cvar_set("ftebroom_array_lines","3");
        print(sprintf("Array line samples %v / %v ordinal %g / %g\n",fb_array_pattern(3,0,4,0,1,'0 0 16'),fb_array_pattern(3,1,4,0,1,'0 0 16'),fb_array_ordinal(1),fb_array_ordinal(2)));
        fb_transform_expect(fb_array_pattern(3,0,4,0,1,'0 0 16')=='128 0 16'&&fb_array_pattern(3,1,4,0,1,'0 0 16')=='-128 0 -16',"array45_centered_offsets");
        cvar_set("ftebroom_array_mirror_offsets","0");fb_transform_expect(fb_array_pattern(3,1,4,0,1,'0 0 16')=='-128 0 16',"array45_same_side_offsets");
        cvar_set("ftebroom_array_cross","1");cvar_set("ftebroom_array_lines","2");fb_transform_expect(vlen(fb_array_pattern(3,4,4,2,1,'0 0 0')-'0 128 0')<.01,"array45_cross_lines");
        fb_transform_expect(vlen(fb_array_pattern(4,4,4,2,1,'0 0 16')-'256 0 32')<.01,"array45_multiple_rings");
        cvar_set("ftebroom_array_radius","256");int randomInside=TRUE;for(int sample=0;sample<64;sample++){vector randomPoint=fb_array_pattern(5,sample,64,2,1337,'0 0 0');if(randomPoint_z!=0||vlen(randomPoint)>256)randomInside=FALSE;}
        fb_transform_expect(randomInside,"array45_random_disk");
        fb_affine_t facingMatrix={0};facingMatrix.x='1 0 0';facingMatrix.y='0 1 0';facingMatrix.z='0 0 1';cvar_set("ftebroom_array_face_center","1");cvar_set("ftebroom_array_facing_side","0");fb_array_facing(&facingMatrix,'100 0 0','0 128 0');
        fb_transform_expect(vlen(facingMatrix.x-'0 -1 0')<.001&&vlen(fb_affine_point(&facingMatrix,'100 0 0')-'100 128 0')<.001,"array45_facing_preserves_center");
        fb_cancel();fb_selection_clear();fb_createbox('8000 0 64','32 32 32');int arrayHistory=history;
        cvar_set("ftebroom_array_mode","4");cvar_set("ftebroom_array_count","4");cvar_set("ftebroom_array_rings","2");cvar_set("ftebroom_array_axis","2");cvar_set("ftebroom_array_x","0");cvar_set("ftebroom_array_y","0");cvar_set("ftebroom_array_z","0");
        int arrayOK=fb_array_apply();fb_transform_expect(arrayOK&&selectedbrushcount==8,"array45_ring_geometry");brush_undo();fb_transform_expect(history==arrayHistory&&selectedbrushcount==1,"array45_single_undo");
        cvar_set("ftebroom_array_rings","64");cvar_set("ftebroom_array_count","64");arrayOK=fb_array_apply();fb_transform_expect(!arrayOK&&history==arrayHistory,"array45_budget_atomic");return TRUE;
    }
    if(argv(1)=="sculpt45") {
        fb_cancel();fb_selection_clear();fb_tool_deactivate();
        fb_createbox('6000 0 64','128 128 128');int sculptModel=selectedbrushes[0].model,sculptTest_first=selectedbrushes[0].id;
        fb_createbox('6128 0 64','128 128 128');int sculptTest_second=selectedbrushes[0].id,sculptHistory=history;brush_select(sculptModel,sculptTest_first);
        cvar_set("ftebroom_sculpt_radius","100");cvar_set("ftebroom_sculpt_intensity","8");cvar_set("ftebroom_sculpt_mode","0");cvar_set("ftebroom_sculpt_prevent_collapse","1");
        fb_tx_begin("Sculpt test");fb_sculpt_stroke=TRUE;fb_sculpt_changed=FALSE;
        int sculptResult=fb_sculpt_at('6064 0 128','0 0 1');fb_sculpt_changed=sculptResult>0;
        fb_transform_expect(sculptResult==2,"sculpt45_raise_neighbours");
        fb_poly_t sculptA={0},sculptB={0};fb_poly_from_brush(sculptModel,selectedbrushes[1].id,&sculptA);fb_poly_from_brush(sculptModel,selectedbrushes[0].id,&sculptB);
        int sculptTest_seam=0,sculptTest_bottom=0;float sculptTest_top=0;
        for(int i=0;i<sculptA.nv;i++){if(sculptA.vertices[i][2]<.01)sculptTest_bottom++;if(fabs(sculptA.vertices[i][0]-6064)<.01&&sculptA.vertices[i][2]>128)for(int j=0;j<sculptB.nv;j++)if(vlen(sculptA.vertices[i]-sculptB.vertices[j])<.01){sculptTest_seam++;sculptTest_top=max(sculptTest_top,sculptA.vertices[i][2]);}}
        fb_transform_expect(sculptTest_seam==2&&sculptTest_bottom==4&&sculptTest_top>128,"sculpt45_shared_vertices_and_back_fixed");fb_poly_free(&sculptA);fb_poly_free(&sculptB);
        fb_sculpt_finish(TRUE);brush_undo();fb_transform_expect(history==sculptHistory,"sculpt45_single_undo");brush_redo();
        cvar_set("ftebroom_sculpt_mode","2");cvar_set("ftebroom_sculpt_radius","256");fb_tx_begin("Smooth test");sculptResult=fb_sculpt_at('6064 0 128','0 0 1');fb_transform_expect(sculptResult==2,"sculpt45_smooth");fb_tx_cancel();
        fb_tx_begin("Lower test");cvar_set("ftebroom_sculpt_mode","1");cvar_set("ftebroom_sculpt_intensity","1024");sculptResult=fb_sculpt_at([6064,0,sculptTest_top],'0 0 1');
        fb_poly_from_brush(sculptModel,selectedbrushes[1].id,&sculptA);float sculptTest_lowest=10000;for(int v=0;v<sculptA.nv;v++)if(sculptA.vertices[v][2]>.01)sculptTest_lowest=min(sculptTest_lowest,sculptA.vertices[v][2]);
        fb_transform_expect(sculptResult>0&&sculptA.volume>0&&sculptTest_lowest>=.99,"sculpt45_prevent_collapse");fb_poly_free(&sculptA);fb_tx_cancel();
        int sculptTest_cancelHistory=history;fb_tx_begin("Cancelled stroke");fb_sculpt_stroke=TRUE;fb_sculpt_changed=TRUE;fb_sculpt_at([6064,0,sculptTest_top],'0 0 1');fb_tool_activate(FB_TOOL_CLIP);
        fb_transform_expect(!fb_sculpt_stroke&&!fb_tx_depth&&history==sculptTest_cancelHistory&&cvar("ftebroom_tool")==FB_TOOL_CLIP,"sculpt45_tool_change_cancels");fb_tool_deactivate();return TRUE;
    }
    if(argv(1)=="rock45") {
        fb_cancel();fb_selection_clear();fb_createbox('0 0 256','512 512 512');fb_document_tick();int rockHistory=history;
        cvar_set("ftebroom_rock_segments","10");cvar_set("ftebroom_rock_seed","1337");cvar_set("ftebroom_rock_hollow","0");cvar_set("ftebroom_rock_group","0");
        fb_tx_begin("Rock test");int rockOK=fb_rock_apply();print(strcat("Rock: ",fb_geom_error,"\n"));fb_transform_expect(rockOK&&selectedbrushcount>0,"rock45_solid_valid");
        float rockVolume=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t rockPoly={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&rockPoly))rockVolume+=rockPoly.volume;fb_poly_free(&rockPoly);}
        if(rockOK)fb_tx_commit();else fb_tx_cancel();brush_undo();fb_transform_expect(history==rockHistory&&selectedbrushcount==1,"rock45_single_undo");
        fb_tx_begin("Repeat rock");rockOK=fb_rock_apply();float repeatVolume=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t rockPoly2={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&rockPoly2))repeatVolume+=rockPoly2.volume;fb_poly_free(&rockPoly2);}
        fb_transform_expect(rockOK&&fabs(repeatVolume-rockVolume)<1,"rock45_seed_repeat");fb_tx_cancel();
        cvar_set("ftebroom_rock_segments","48");fb_tx_begin("Detailed rock");float rockStart=gettime(1);rockOK=fb_rock_apply();print(sprintf("Detailed rock: %s, %g ms, %i pieces\n",fb_geom_error,(gettime(1)-rockStart)*1000,selectedbrushcount));fb_transform_expect(rockOK&&selectedbrushcount>1,"rock45_detail_convex_partition");fb_tx_cancel();
        cvar_set("ftebroom_rock_segments","8");cvar_set("ftebroom_rock_hollow","1");cvar_set("ftebroom_rock_thickness","16");fb_tx_begin("Hollow rock");rockOK=fb_rock_apply();print(strcat("Shell: ",fb_geom_error,"\n"));fb_transform_expect(rockOK&&selectedbrushcount==40,"rock45_hollow_shell");fb_tx_cancel();
        cvar_set("ftebroom_rock_hollow","0");cvar_set("ftebroom_rock_group","1");fb_tx_begin("Grouped rock");rockOK=fb_rock_apply();print(sprintf("Grouped rock: %s, depth %i, tool %g\n",fb_geom_error,fb_tx_depth,cvar("ftebroom_tool")));fb_transform_expect(rockOK&&fb_tx_depth>0,"rock45_group_atomic");if(fb_tx_depth)fb_tx_cancel();fb_transform_expect(history==rockHistory&&selectedbrushcount==1,"rock45_cancel_restores_source");return TRUE;
    }
    if(argv(1)=="settle44stale") {int staleHistory=history;int captured=fb_settle_export();fb_selection_clear();int accepted=fb_settle_preview();fb_settle_clear();fb_transform_expect(captured&&!accepted&&history==staleHistory&&!fb_generator_count,"settle44_stale_result_rejected");return TRUE;}
    if(argv(1)=="subdivision44texture") {
        fb_cancel();fb_selection_clear();fb_createbox('64 64 64','128 128 128');int textureModel=selectedbrushes[0].model,textureId=selectedbrushes[0].id,textureContents,textureFace=-1;
        static brushface_t textureFaces[64];int textureCount=brush_get(textureModel,textureId,textureFaces,64,&textureContents);
        for(int f=0;f<textureCount;f++)if(textureFaces[f].planenormal[2]>.99){textureFace=f;textureFaces[f].sdir='2 0 0';textureFaces[f].tdir='0 0.5 0';textureFaces[f].sbias=textureFaces[f].tbias=0;}
        fb_tx_begin("Texture basis fixture");brush_history_edit(textureModel,textureId,textureFaces,textureCount,textureContents);fb_tx_commit();fb_document_tick();
        selbrush_reset_mask(&selectedbrushes[0],FALSE);selbrush_set_face(&selectedbrushes[0],textureFace,TRUE);
        cvar_set("ftebroom_subdivide_texture","1");cvar_set("ftebroom_subdivide_grid","1");cvar_set("ftebroom_subdivide_cell","64");fb_tx_begin("Texture subdivision");int textureOK=fb_subdivision_apply();
        fb_transform_expect(textureOK&&selectedbrushcount==4,"subdivision44_texture_cell_scale");float textureVolume=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t texturePart={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&texturePart))textureVolume+=texturePart.volume;fb_poly_free(&texturePart);}
        fb_transform_expect(textureOK&&fabs(textureVolume-128*128*128)<1,"subdivision44_texture_volume");fb_tx_cancel();cvar_set("ftebroom_subdivide_texture","0");return TRUE;
    }
    if(argv(1)=="inset44taper") {
        fb_cancel();fb_selection_clear();fb_createbox('0 0 0','128 128 128');int model=selectedbrushes[0].model,id=selectedbrushes[0].id,contents;
        static brushface_t taper[64];int nf=brush_get(model,id,taper,64,&contents);
        for(int f=0;f<nf;f++)if(fabs(taper[f].planenormal[2])<.01){taper[f].planenormal[2]=.5;float length=vlen(taper[f].planenormal);taper[f].planenormal/=length;taper[f].planedist=96/length;}
        fb_tx_begin("Tapered inset fixture");id=brush_history_edit(model,id,taper,nf,contents);fb_tx_commit();fb_document_tick();
        selbrush_reset_mask(&selectedbrushes[0],FALSE);for(int f=0;f<nf;f++)if(taper[f].planenormal[2]>.99)selbrush_set_face(&selectedbrushes[0],f,TRUE);
        fb_poly_t before={0};fb_poly_from_brush(model,id,&before);float expected=before.volume;fb_poly_free(&before);
        cvar_set("ftebroom_inset_amount","16");cvar_set("ftebroom_inset_snap","0");fb_tx_begin("Tapered inset");int taperValid=fb_inset_apply();float actual=0;
        for(int b=0;b<selectedbrushcount;b++){fb_poly_t part={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&part))actual+=part.volume;fb_poly_free(&part);}
        fb_transform_expect(taperValid&&selectedbrushcount==5&&fabs(actual-expected)<expected*.001,"inset44_wider_back_preserved");fb_tx_cancel();return TRUE;
    }
    if(argv(1)=="settle44debug") {print(sprintf("Settle state history %i %i selection %i %i geometry %i %i\n",history,fb_settle_history,fb_selection_epoch,fb_settle_selection,fb_grid_geometry_epoch,fb_settle_geometry));print(sprintf("Settle count %i preview %i valid %g error %s\n",fb_settle_count,fb_generator_count,cvar("ftebroom_settle_valid"),cvar_string("ftebroom_settle_error")));return TRUE;}
    if(argv(1)=="settle44setup"||argv(1)=="settle44patch") {
        int usePatch=argv(1)=="settle44patch";fb_cancel();fb_selection_clear();float position=usePatch?4096:2048;
        if(usePatch){static patchvert_t ground[9];patchinfo_t groundInfo={0};groundInfo.cpwidth=groundInfo.cpheight=3;groundInfo.tesswidth=groundInfo.tessheight=-1;groundInfo.shadername="ftebroom/grid";groundInfo.contents=CONTENTBIT_SOLID;
            for(int y=0;y<3;y++)for(int x=0;x<3;x++){ground[y*3+x].xyz=[position+(x-1)*256,position+(y-1)*256,0];ground[y*3+x].rgb='1 1 1';}
            fb_tx_begin("Patch floor");patch_history_create(1,ground,groundInfo,FALSE);fb_tx_commit();
        }else fb_createbox([position,position,-16],'512 512 32');
        fb_selection_clear();fb_createbox([position,position,72],'16 16 16');fb_document_tick();fb_generators_test_history=history;return TRUE;
    }
    if(argv(1)=="settle44verify") {
        vector limits[2];brush_getfacepoints(selectedbrushes[0].model,selectedbrushes[0].id,0,limits,2);fb_transform_expect(limits[0][2]>-.25&&limits[0][2]<.25,"settle44_applied_contact");
        brush_undo();brush_getfacepoints(selectedbrushes[0].model,selectedbrushes[0].id,0,limits,2);fb_transform_expect(history==fb_generators_test_history&&fabs(limits[0][2]-64)<.01,"settle44_single_undo");
        brush_redo();brush_getfacepoints(selectedbrushes[0].model,selectedbrushes[0].id,0,limits,2);fb_transform_expect(limits[0][2]>-.25&&limits[0][2]<.25,"settle44_redo");return TRUE;
    }
    if(argv(1)=="viewport44") {
        fb_camera_position('256 -384 320');setproperty(VF_CL_VIEWANGLES,'25 124 0');
        fb_cancel();fb_selection_clear();fb_createbox('0 0 128','128 128 128');fb_document_tick();
        static brushface_t viewFaces[64];int viewCount=brush_get(selectedbrushes[0].model,selectedbrushes[0].id,viewFaces,64,__NULL__);
        selbrush_reset_mask(&selectedbrushes[0],FALSE);for(int f=0;f<viewCount;f++)if(viewFaces[f].planenormal[2]>.99)selbrush_set_face(&selectedbrushes[0],f,TRUE);return TRUE;
    }
    if(argv(1)=="spin44") {
        fb_cancel();fb_selection_clear();fb_createbox('0 0 128','128 128 128');fb_document_tick();int spinMarker=history,spinOriginal=selectedbrushes[0].id;
        static brushface_t spinFaces[64];int spinCount=brush_get(selectedbrushes[0].model,spinOriginal,spinFaces,64,__NULL__);
        selbrush_reset_mask(&selectedbrushes[0],FALSE);for(int f=0;f<spinCount;f++)if(spinFaces[f].planenormal[2]>.99)selbrush_set_face(&selectedbrushes[0],f,TRUE);
        cvar_set("ftebroom_spin_segments","3");cvar_set("ftebroom_spin_auto","1");cvar_set("ftebroom_spin_scale","1");cvar_set("ftebroom_spin_flex","0");cvar_set("ca_grid","16");
        fb_tx_begin("Spin test");int spinOK=fb_spin_apply();print(strcat("Spin: ",fb_geom_error,"\n"));fb_transform_expect(spinOK&&selectedbrushcount==3,"spin44_segment_count");
        float spinVolume=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t measureSpin={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&measureSpin))spinVolume+=measureSpin.volume;fb_poly_free(&measureSpin);}
        fb_transform_expect(spinOK&&fabs(spinVolume-128*128*48)<1,"spin44_straight_volume");fb_transform_expect(brush_get(1,spinOriginal,__NULL__,0,__NULL__)>0,"spin44_preserves_source");
        if(spinOK)fb_tx_commit();else fb_tx_cancel();brush_undo();fb_transform_expect(history==spinMarker&&selectedbrushcount==1,"spin44_undo");
        cvar_set("ftebroom_spin_auto","0");cvar_set("ftebroom_spin_x","64");cvar_set("ftebroom_spin_z","128");cvar_set("ftebroom_spin_flex",".5");cvar_set("ftebroom_spin_scale",".7");cvar_set("ftebroom_spin_rz","30");
        fb_tx_begin("Curved Spin");spinOK=fb_spin_apply();print(strcat("Curved Spin: ",fb_geom_error,"\n"));fb_transform_expect(spinOK&&selectedbrushcount==3,"spin44_curve_rotate_scale");fb_tx_cancel();return TRUE;
    }
    if(argv(1)=="partition44") {
        fb_cancel();fb_selection_clear();fb_createbox('0 0 128','128 128 128');fb_document_tick();int partitionMarker=history;
        static brushface_t partitionFaces[64];int partitionCount=brush_get(selectedbrushes[0].model,selectedbrushes[0].id,partitionFaces,64,__NULL__);
        selbrush_reset_mask(&selectedbrushes[0],FALSE);for(int f=0;f<partitionCount;f++)if(partitionFaces[f].planenormal[2]>.99)selbrush_set_face(&selectedbrushes[0],f,TRUE);
        cvar_set("ftebroom_inset_amount","16");cvar_set("ftebroom_inset_snap","0");fb_tx_begin("Inset test");int partitionOK=fb_inset_apply();
        print(strcat("Inset: ",fb_geom_error,"\n"));fb_transform_expect(partitionOK&&selectedbrushcount==5,"inset44_inner_and_four_borders");
        float partitionVolume=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t measure={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&measure))partitionVolume+=measure.volume;fb_poly_free(&measure);}
        fb_transform_expect(partitionOK&&fabs(partitionVolume-128*128*128)<1,"inset44_volume_conserved");
        if(partitionOK)fb_tx_commit();else fb_tx_cancel();brush_undo();fb_transform_expect(history==partitionMarker&&selectedbrushcount==1,"inset44_undo");
        cvar_set("ftebroom_inset_amount","1024");fb_tx_begin("Invalid inset");partitionOK=fb_inset_apply();fb_tx_cancel();fb_transform_expect(!partitionOK&&history==partitionMarker,"inset44_invalid_atomic");
        selbrush_reset_mask(&selectedbrushes[0],TRUE);cvar_set("ftebroom_subdivide_u","3");cvar_set("ftebroom_subdivide_v","2");cvar_set("ftebroom_subdivide_grid","0");cvar_set("ftebroom_subdivide_texture","0");
        for(int plane=0;plane<3;plane++){cvar_set("ftebroom_subdivide_plane",itos(plane));fb_tx_begin("Subdivision test");partitionOK=fb_subdivision_apply();print(strcat("Subdivision: ",fb_geom_error,"\n"));fb_transform_expect(partitionOK&&selectedbrushcount==6,"subdivision44_six_pieces");
            partitionVolume=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t measure={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&measure))partitionVolume+=measure.volume;fb_poly_free(&measure);}fb_transform_expect(partitionOK&&fabs(partitionVolume-128*128*128)<1,"subdivision44_volume_conserved");fb_tx_cancel();fb_transform_expect(history==partitionMarker&&selectedbrushcount==1,"subdivision44_cancel_atomic");}
        cvar_set("ftebroom_subdivide_grid","1");cvar_set("ftebroom_subdivide_cell","32");fb_tx_begin("Grid subdivision");partitionOK=fb_subdivision_apply();fb_transform_expect(partitionOK&&selectedbrushcount==16,"subdivision44_world_cells");fb_tx_cancel();
        cvar_set("ftebroom_subdivide_cell",".125");fb_tx_begin("Excessive subdivision");partitionOK=fb_subdivision_apply();fb_tx_cancel();fb_transform_expect(!partitionOK&&history==partitionMarker,"subdivision44_budget");return TRUE;
    }
    if(argv(1)=="holes43") {
        fb_cancel();fb_selection_clear();fb_createbox('0 0 128','512 512 128');fb_document_tick();int limitMarker=history;
        cvar_set("ftebroom_hole_count","64");cvar_set("ftebroom_hole_sides","32");cvar_set("ftebroom_hole_min","8");cvar_set("ftebroom_hole_max","12");
        fb_tx_begin("Over budget holes");int limitValid=fb_perforate_apply(FALSE);fb_tx_cancel();fb_transform_expect(!limitValid&&history==limitMarker&&selectedbrushcount==1,"holes43_over_budget_rejected");
        cvar_set("ftebroom_hole_count","8");cvar_set("ftebroom_hole_sides","16");cvar_set("ftebroom_hole_min","64");cvar_set("ftebroom_hole_max","72");fb_tx_begin("Boundary holes");limitValid=fb_perforate_apply(FALSE);print(strcat("Boundary holes: ",fb_geom_error,"\n"));fb_tx_cancel();fb_transform_expect((limitValid||fb_geom_error!="")&&history==limitMarker&&selectedbrushcount==1,"holes43_boundary_atomic");return TRUE;
    }
    if(argv(1)=="array43") {
        fb_cancel();fb_selection_clear();fb_createbox('256 0 128','32 32 32');fb_document_tick();int arrayMarker=history;
        cvar_set("ftebroom_array_count","3");cvar_set("ftebroom_array_mode","0");cvar_set("ftebroom_array_x","64");cvar_set("ftebroom_array_y","0");cvar_set("ftebroom_array_z","0");
        int arrayValid=fb_array_apply();fb_transform_expect(arrayValid&&selectedbrushcount==3,"array43_linear_count");
        vector bounds[2];brush_getfacepoints(selectedbrushes[2].model,selectedbrushes[2].id,0,bounds,2);fb_transform_expect(bounds[0][0]>=400,"array43_linear_bounds");
        brush_undo();fb_transform_expect(history==arrayMarker&&selectedbrushcount==1,"array43_single_undo");
        cvar_set("ftebroom_array_mode","1");cvar_set("ftebroom_array_angle","90");cvar_set("ftebroom_array_axis","2");cvar_set("ftebroom_array_rotate","1");arrayValid=fb_array_apply();fb_transform_expect(arrayValid&&selectedbrushcount==3,"array43_radial_count");brush_undo();
        cvar_set("ftebroom_array_mode","2");cvar_set("ftebroom_array_seed","1337");arrayValid=fb_array_apply();brush_getfacepoints(selectedbrushes[0].model,selectedbrushes[0].id,0,bounds,2);vector first=bounds[0];brush_undo();arrayValid=fb_array_apply();brush_getfacepoints(selectedbrushes[0].model,selectedbrushes[0].id,0,bounds,2);fb_transform_expect(arrayValid&&vlen(first-bounds[0])<.01,"array43_random_seed");brush_undo();return TRUE;
    }
    if(argv(1)=="perforate") {
        fb_cancel();fb_selection_clear();fb_createbox('0 0 128','128 128 128');fb_document_tick();int holeMarker=history;
        cvar_set("ftebroom_hole_count","2");cvar_set("ftebroom_hole_sides","5");cvar_set("ftebroom_hole_axis","2");cvar_set("ftebroom_hole_seed","1337");
        cvar_set("ftebroom_hole_min","16");cvar_set("ftebroom_hole_max","24");cvar_set("ftebroom_hole_spacing","4");cvar_set("ftebroom_hole_keep","1");
        fb_tx_begin("Perforate test");int holeOK=fb_perforate_apply(FALSE);print(strcat("Perforate test: ",fb_geom_error,"\n"));
        fb_transform_expect(holeOK&&selectedbrushcount==2,"perforate_selects_only_plugs");
        int holeShape=TRUE;for(int b=0;b<selectedbrushcount;b++){fb_poly_t poly={0};holeShape&=fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&poly)&&poly.nf==7;fb_poly_free(&poly);}
        fb_transform_expect(holeOK&&holeShape,"perforate_requested_sides");
        if(holeOK)fb_tx_commit();else fb_tx_cancel();brush_undo();
        fb_transform_expect(history==holeMarker&&selectedbrushcount==1,"perforate_single_undo");
        cvar_set("ftebroom_hole_min","1024");cvar_set("ftebroom_hole_max","1024");fb_tx_begin("Impossible holes");holeOK=fb_perforate_apply(FALSE);fb_tx_cancel();
        fb_transform_expect(!holeOK&&history==holeMarker&&selectedbrushcount==1,"perforate_rejects_unplaceable_holes");return TRUE;
    }
    if(argv(1)=="fracture") {
        fb_cancel();fb_selection_clear();fb_createbox('0 0 128','128 128 128');fb_document_tick();
        int fractureMarker=history;
        cvar_set("ftebroom_fracture_count","8");cvar_set("ftebroom_fracture_seed","1337");cvar_set("ftebroom_fracture_separation","0");cvar_set("ftebroom_fracture_entities","0");
        fb_tx_begin("Fracture test");int valid=fb_fracture_apply();
        print(strcat("Fracture test: ",fb_geom_error,"\n"));
        fb_transform_expect(valid&&selectedbrushcount==8,"fracture_exact_count");
        float volume=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t poly={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&poly))volume+=poly.volume;fb_poly_free(&poly);}
        fb_transform_expect(valid&&fabs(volume-128*128*128)<128*128*128*.001,"fracture_volume_conserved");
        if(valid)fb_tx_commit();else fb_tx_cancel();
        brush_undo();fb_transform_expect(history==fractureMarker&&selectedbrushcount==1,"fracture_undo_original");
        brush_redo();fb_transform_expect(selectedbrushcount==8,"fracture_redo_fragments");brush_undo();
        cvar_set("ftebroom_fracture_entities","1");fb_tx_begin("Fracture entity test");valid=fb_fracture_apply();
        int independent=TRUE;for(int a=0;a<selectedbrushcount;a++)for(int b=a+1;b<selectedbrushcount;b++)if(selectedbrushes[a].model==selectedbrushes[b].model)independent=FALSE;
        fb_transform_expect(valid&&selectedbrushcount==8&&independent,"fracture_independent_brush_entities");
        fb_tx_cancel();cvar_set("ftebroom_fracture_entities","0");return TRUE;
    }
    if(argv(1)=="patch") {
        fb_selection_clear();static patchvert_t points[9];patchinfo_t info={0};info.cpwidth=info.cpheight=3;info.tesswidth=info.tessheight=-1;info.shadername="ftebroom/grid";info.contents=CONTENTBIT_SOLID;
        for(int y=0;y<3;y++)for(int x=0;x<3;x++){int at=y*3+x;points[at].xyz=[x*32,y*32,256];points[at].s=x;points[at].t=y;points[at].rgb='1 1 1';points[at].a=1;}
        fb_tx_begin("Patch fixture");patch_history_create(1,points,info,TRUE);fb_tx_commit();int marker=history;
        cvar_set("ftebroom_quake1","0");cvar_set("ftebroom_land_patch_u","5");cvar_set("ftebroom_land_patch_v","7");
        fb_tx_begin("Terrain patch");int patchOK=fb_land_apply();if(patchOK)fb_tx_commit();else fb_tx_cancel();
        patchinfo_t after;int count=patch_getcp(selectedbrushes[0].model,selectedbrushes[0].id,__NULL__,0,&after);
        fb_transform_expect(patchOK&&count==35&&after.cpwidth==5&&after.cpheight==7&&after.shadername=="ftebroom/grid","terrain_patch_resizes_and_preserves_material");
        brush_undo();count=patch_getcp(selectedbrushes[0].model,selectedbrushes[0].id,__NULL__,0,&after);
        fb_transform_expect(history==marker&&count==9,"terrain_patch_single_undo");return TRUE;
    }
    if(argv(1)=="preview") {
        fb_transform_expect(fb_generator_count==8&&selectedbrushcount==1&&history==fb_generators_test_history,"preview_owns_geometry_without_document_change");
        fb_transform_expect(fb_generator_replaces(selectedbrushes[0].model,selectedbrushes[0].id),"preview_hides_original");return TRUE;
    }
    if(argv(1)=="cancelled") {
        fb_transform_expect(fb_generator_count==0&&selectedbrushcount==1&&history==fb_generators_test_history,"preview_cancel_restores_original");return TRUE;
    }
    fb_cancel();fb_selection_clear();fb_createbox('0 0 128','128 128 128');fb_document_tick();
    cvar_set("ftebroom_bevel_segments","3");cvar_set("ftebroom_bevel_profile","0.5");cvar_set("ftebroom_bevel_mode","0");cvar_set("ftebroom_bevel_radius","8");cvar_set("ftebroom_bevel_clamp","1");
    fb_tx_begin("Test bevel");int ok=fb_bevel_apply();
    print(strcat("GENERATOR bevel result: ",fb_geom_error,"\n"));
    fb_transform_expect(ok,"bevel_cube_valid");
    if(ok){fb_poly_t p={0};fb_poly_from_brush(selectedbrushes[0].model,selectedbrushes[0].id,&p);fb_transform_expect(p.nf>6&&p.volume<128*128*128,"bevel_adds_convex_faces");fb_poly_free(&p);}
    fb_tx_cancel();
    cvar_set("ftebroom_bevel_radius","1024");fb_tx_begin("Test clamp");ok=fb_bevel_apply();print(strcat("GENERATOR clamp: ",fb_geom_error,"\n"));fb_transform_expect(ok,"bevel_clamp_large_radius");fb_tx_cancel();
    cvar_set("ftebroom_bevel_radius","8");
    for(int mode=1;mode<=2;mode++) {
        fb_tool_activate(mode==1?FB_TOOL_EDGE:FB_TOOL_VERTEX);fb_vertex_deselect();fb_vertex_handles[0].selected=TRUE;cvar_set("ftebroom_bevel_mode",itos(mode));
        fb_tx_begin("Partial bevel");ok=fb_bevel_apply();fb_transform_expect(ok,mode==1?"bevel_selected_edge":"bevel_selected_vertex");fb_tx_cancel();
    }
    fb_tool_activate(0);cvar_set("ftebroom_bevel_mode","0");
    cvar_set("ftebroom_land_u","2");cvar_set("ftebroom_land_v","2");cvar_set("ftebroom_land_frequency","4");cvar_set("ftebroom_land_height","32");cvar_set("ftebroom_land_intensity","1");cvar_set("ftebroom_land_roughness","1");cvar_set("ftebroom_land_triangles","1");
    fb_tx_begin("Test terrain");ok=fb_land_apply();fb_transform_expect(ok&&selectedbrushcount==8,"terrain_eight_triangular_prisms");fb_tx_cancel();
    fb_transform_expect(selectedbrushcount==1,"generator_cancel_restores_selection");
    fb_generators_test_history=history;
    return TRUE;
}
