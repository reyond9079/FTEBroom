// SPDX-License-Identifier: GPL-2.0-or-later
int fb_material_test_id;
float fb_material_testcommand()
{
    if(argv(0)!="ftebroom_test_materials" || !cvar("ftebroom_skip_setup")) return FALSE;
    if(argv(1)=="q2102probe") {
        fb_tool_deactivate();fb_selection_clear();cvar_set("ca_newbrushtexture","e1u1/grate2_5");
        fb_createbox('0 0 96','128 128 224');fb_material_test_id=selectedbrushes[0].id;fb_selection_clear();
        cvar_set("ftebroom_test_camera_lock","64 -192 176");
        localcmd("ftebroom_camera_restore 64 -192 176 0 90 0 90\n");
        string shader=fb_surface_preview_shader("e1u1/grate2_5",1);
        print(sprintf("Q2102_SIZE shader=%v raw=%v preview=%s\n",drawgetimagesize(shader),drawgetimagesize("textures/e1u1/grate2_5"),fb_shader_previews["e1u1/grate2_5"]));
        return TRUE;
    }
    if(argv(1)=="q2102select"){fb_selection_set(fb_document_brush_node(1,fb_material_test_id),TRUE);return TRUE;}
    if(argv(1)=="q2102clear"){fb_selection_clear();return TRUE;}
    if(argv(1)=="case79view") {
        fb_tool_deactivate();fb_selection_clear();cvar_set("ca_newbrushtexture",argv(2));
        fb_createbox('0 0 96','128 128 128');fb_group_create("Material case regression");fb_selection_clear();
        localcmd("ftebroom_camera_restore 0 -320 128 5 90 0 90\n");return TRUE;
    }
    if(argv(1)=="q1case79") {
        float savedQ1=cvar("ftebroom_quake1");string savedEpoch=strzone(cvar_string("ftebroom_material_tag_epoch"));
        cvar_set("ftebroom_quake1","1");
        fb_transform_expect(fb_material_key("Textures/TechTrim16_b")=="techtrim16_b","q1case79_prefix_and_case");
        if(fb_shader_previews)fb_transform_expect(fb_shader_previews[fb_material_key("TechTrim16_b")]!="","q1case79_actual_wad_preview");
        cvar_set("ftebroom_material_tag_epoch","7901");string first=strzone(fb_surface_material_shader("techtrim16_b",1));
        float oldShader=shaderforname(first,"{ { map textures/ftebroom/qa_red.tga } }");
        cvar_set("ftebroom_material_tag_epoch","7902");string second=fb_surface_material_shader("techtrim16_b",1);
        float newShader=shaderforname(second,"{ { map textures/ftebroom/qa_green.tga } }");
        fb_transform_expect(oldShader>0 && newShader>0 && oldShader!=newShader,"q1case79_resource_generation_changes_shader");
        cvar_set("ftebroom_quake1","0");fb_transform_expect(fb_material_key("Textures/TechTrim16_b")=="Textures/TechTrim16_b","q1case79_q3_names_unchanged");
        cvar_set("ftebroom_quake1",ftos(savedQ1));cvar_set("ftebroom_material_tag_epoch",savedEpoch);strunzone(savedEpoch);strunzone(first);return TRUE;
    }
    if(argv(1)=="drop63") {
        fb_tool_deactivate();fb_selection_clear();int drop63_id=fb_createbox('0 0 64','128 128 128');
        cvar_set("ca_brush_view","3");cvar_set("ca_brush_viewsize","512");fb_view_prepare();
        vector drop63_pixel=project('0 0 128');drop63_pixel_z=0;int drop63_before=history;
        fb_material_drop("ftebroom/qa_blue",drop63_pixel);
        brushface_t drop63_faces[6];brush_get(1,selectedbrushes[0].id,drop63_faces,6,__NULL__);int drop63_painted=0;
        for(int drop63_f=0;drop63_f<6;drop63_f++)if(drop63_faces[drop63_f].shadername=="ftebroom/qa_blue")drop63_painted++;
        fb_transform_expect(drop63_painted==1&&history>drop63_before,"drop63_paints_one_face");
        brush_undo();fb_transform_expect(history==drop63_before,"drop63_single_undo");brush_redo();
        return TRUE;
    }
    if(argv(1)=="transparent24") {
        fb_document_tick();fb_filter_prepare();fb_render_faces_prepare();int transparentFaces=0;
        for(int i=0;i<fb_render_face_count;i++)if(fb_render_faces[i].alpha>0 && fb_render_faces[i].alpha<1)transparentFaces++;
        fb_transform_expect(transparentFaces>=6,"selected_transparent_faces_keep_alpha");return TRUE;
    }
    if(argv(1)=="patch23") {
        fb_tool_deactivate();fb_selection_clear();fb_material_change("sx",0.25,FALSE);fb_material_change("sy",0.75,FALSE);fb_material_change("angle",30,FALSE);
        int test23_newbrush=fb_createbox('0 0 64','128 128 128');brushface_t test23_defaults[6];brush_get(1,test23_newbrush,test23_defaults,6,__NULL__);
        vector test23_initial=fb_uv_values(&test23_defaults[5]);fb_transform_expect(fabs(test23_initial_x-0.25)<0.001 && fabs(test23_initial_y-0.75)<0.001 && fabs(test23_initial_z-30)<0.001,"new_brush_uv_defaults");
        brush_deselectall();brushface_select(1,test23_newbrush,6);fb_patch_convert();
        fb_transform_expect(cvar("ftebroom_tool")==FB_TOOL_CONTROLPOINT,"patch_auto_control_tool");
        patchinfo_t test23_patch;patchvert_t test23_before[9],test23_after[9];patch_getcp(selectedbrushes[0].model,selectedbrushes[0].id,test23_before,9,&test23_patch);
        int test23_marker=history;cvar_set("ca_newbrushtexture","ftebroom/qa_blue");fb_material_edit("material",0);
        fb_material_edit("sx",2);fb_material_edit("sy",0.5);fb_material_edit("angle",45);fb_material_edit("ox",0.25);fb_material_edit("oy",0.75);
        patch_getcp(selectedbrushes[0].model,selectedbrushes[0].id,test23_after,9,&test23_patch);vector test23_uv=fb_patch_uv(test23_after,test23_patch);
        fb_transform_expect(test23_patch.shadername=="ftebroom/qa_blue","patch_material_applied");
        fb_transform_expect(fabs(test23_uv_x-2)<0.001 && fabs(test23_uv_y-0.5)<0.001 && fabs(test23_uv_z-45)<0.001,"patch_scale_angle_persist");
        fb_transform_expect(fabs(test23_after[0].s-0.25)<0.001 && fabs(test23_after[0].t-0.75)<0.001,"patch_offsets_persist");
        float test23_error=0;for(int test23_p=0;test23_p<9;test23_p++)test23_error=max(test23_error,vlen(test23_before[test23_p].xyz-test23_after[test23_p].xyz));fb_transform_expect(test23_error==0,"patch_uv_preserves_geometry");
        int test23_last=history;fb_material_edit("sx",0);fb_transform_expect(history==test23_last,"patch_zero_scale_rejected");
        for(int test23_u=0;test23_u<6;test23_u++)brush_undo();patch_getcp(selectedbrushes[0].model,selectedbrushes[0].id,test23_after,9,&test23_patch);test23_error=0;
        for(int test23_q=0;test23_q<9;test23_q++)test23_error=max(test23_error,fabs(test23_before[test23_q].s-test23_after[test23_q].s)+fabs(test23_before[test23_q].t-test23_after[test23_q].t));
        fb_transform_expect(history==test23_marker && test23_error<0.001,"patch_uv_undo");for(int test23_redo=0;test23_redo<6;test23_redo++)brush_redo();
        fb_material_state();fb_transform_expect(fabs(cvar("ftebroom_scale_x")-2)<0.001 && fabs(cvar("ftebroom_scale_y")-0.5)<0.001,"patch_inspector_readback");
        fb_vertex_rebuild();return TRUE;
    }
    if(argv(1)=="reload23") {
        fb_document_tick();fb_selection_clear();for(int test23_n=0;test23_n<fb_node_count;test23_n++)if(fb_nodes[test23_n].kind==FB_NODE_PATCH)brush_select(fb_nodes[test23_n].model,fb_nodes[test23_n].id);
        fb_material_state();fb_transform_expect(selectedbrushcount==1 && cvar_string("ftebroom_current_material")=="ftebroom/qa_blue" && fabs(cvar("ftebroom_scale_x")-2)<0.001 && fabs(cvar("ftebroom_scale_y")-0.5)<0.001 && fabs(cvar("ftebroom_angle")-45)<0.001,"patch_material_map_reload");return TRUE;
    }
    if(argv(1)=="edit") {
        fb_selection_clear();fb_material_test_id=fb_createbox('0 0 128','64 64 64');
        static brushface_t faces[6];int contents;
        brush_get(1,fb_material_test_id,faces,6,&contents);
        faces[0].sbias=0;faces[1].sbias=8;faces[2].sbias=20;
        fb_tx_begin("Fixture attributes");fb_material_test_id=brush_history_edit(1,fb_material_test_id,faces,6,contents);fb_tx_commit();
        brush_deselectall();for(int f=1;f<=3;f++) brushface_select(1,fb_material_test_id,f);
        fb_material_change("ox",16,TRUE);
        brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(faces[0].sbias==16 && faces[1].sbias==24 && faces[2].sbias==36,"offset_delta_preserves_differences");
        fb_material_edit("ox",32);brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(faces[0].sbias==32 && faces[1].sbias==32 && faces[2].sbias==32,"offset_absolute_three_faces");
        vector s=faces[0].sdir,t=faces[0].tdir;
        fb_material_change("angle",15,TRUE);fb_material_change("angle",15,TRUE);
        fb_material_change("angle",-30,TRUE);brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(vlen(faces[0].sdir-s)<0.0001 && vlen(faces[0].tdir-t)<0.0001,"angle_roundtrip_preserves_axes");
        fb_material_flags("surface",0x80000001i,FALSE);fb_material_flags("value",123456789i,FALSE);
        brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(faces[0].surfaceflags==0x80000001i && faces[2].surfaceflags==0x80000001i && faces[3].surfaceflags==0,"flags_exact_and_selected_only");
        fb_transform_expect(faces[0].surfacevalue==123456789i,"surface_value_exact_integer");
        fb_material_state();fb_transform_expect((int)cvar("ftebroom_uv_mixed")==0,"uniform_fields_not_mixed");
        int before=history;fb_material_edit("sx",0);
        fb_transform_expect(history==before,"zero_scale_rejected_atomically");
        fb_material_edit("sx",2);fb_material_edit("sy",0.5);
        brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(fabs(vlen(faces[0].sdir)-0.5)<0.0001 && fabs(vlen(faces[0].tdir)-2)<0.0001,"independent_scales");
        float bias=faces[0].tbias;
        fb_texture_key("vertical",1,0);fb_texture_key("vertical",-1,0);
        brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(fabs(faces[0].tbias-bias)<0.0001,"keyboard_texture_roundtrip");
        cvar_set("ca_newbrushtexture","ftebroom/qa_red");fb_material_edit("material",0);
        cvar_set("ftebroom_replace_from","ftebroom/qa_red");cvar_set("ftebroom_replace_to","ftebroom/qa_blue");
        int replaceBefore=history;fb_material_replace();
        fb_transform_expect(fb_faces_count()==3,"replace_selects_changed_faces");
        brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(faces[0].shadername=="ftebroom/qa_blue" && faces[2].shadername=="ftebroom/qa_blue" && faces[3].shadername=="ftebroom/grid","replace_selected_scope");
        brush_undo();brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(history==replaceBefore && faces[0].shadername=="ftebroom/qa_red","replace_one_undo");
        brush_redo();
        cvar_set("ftebroom_material_query","ftebroom/qa_blue");fb_material_select("faces");
        fb_transform_expect(fb_faces_count()==3,"select_material_faces");
        fb_material_usage();
        brush_deselectall();fb_createbox('256 0 128','128 64 32');fb_faces_convert();
        int fitId=selectedbrushes[0].id;brush_deselectall();brushface_select(1,fitId,3);
        cvar_set("ca_newbrushtexture","ftebroom/grid");fb_material_edit("material",0);fb_texture_key("world",1,0);
        vector image=drawgetimagesize("textures/ftebroom/grid");
        fb_texture_align("fit_h",1,FALSE);brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(image_x>0 && fabs(vlen(faces[2].sdir)-image_x/128)<0.0001,"fit_h_one_repeat");
        fb_texture_align("fit_h",1,FALSE);brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(fabs(vlen(faces[2].sdir)-image_x*2/128)<0.0001,"fit_h_two_repeats");
        fb_texture_align("fit_h",1,TRUE);brush_get(1,selectedbrushes[0].id,faces,6,&contents);
        fb_transform_expect(fabs(vlen(faces[2].sdir)-image_x/256)<0.0001,"fit_h_half_texture");
        fb_texture_align("auto",1,FALSE);
        fb_transform_expect(fb_faces_count()==1,"auto_fit_preserves_face_selection");
    }
    return TRUE;
}
