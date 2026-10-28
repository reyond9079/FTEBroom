// SPDX-License-Identifier: GPL-2.0-or-later
int *fb_delete92_ids;vector *fb_delete92_bounds;int fb_delete92_count,fb_delete92_epoch;
int fb_delete92_testcommand()
{
    if(argv(1)=="delete92fixture"){
        for(int i=0;i<64;i++)fb_createbox([512+(i%8)*40,512+(int)(i/8)*40,32],'24 24 24');return TRUE;
    }
    if(argv(1)=="delete92select"){
        fb_tool_refresh();fb_document_tick();fb_selection_clear();int limit=max(32,stoi(argv(2)));
        memfree(fb_delete92_ids);memfree(fb_delete92_bounds);fb_delete92_ids=memalloc(limit*2*sizeof(int));fb_delete92_bounds=memalloc(limit*2*sizeof(vector));fb_delete92_count=0;
        selbrush_t *items=memalloc(limit*sizeof(selbrush_t));
        for(int n=2;n<fb_node_count&&fb_delete92_count<limit;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH){
            int at=fb_delete92_count++;items[at].model=fb_nodes[n].model;items[at].id=fb_nodes[n].id;selbrush_reset_mask(&items[at],TRUE);items[at].faceonly=FALSE;
            fb_delete92_ids[at*2]=items[at].model;fb_delete92_ids[at*2+1]=items[at].id;brush_getfacepoints(items[at].model,items[at].id,0,fb_delete92_bounds+at*2,2);
        }
        history_restore_list(items,fb_delete92_count,-1);memfree(items);fb_delete92_epoch=fb_grid_geometry_epoch;
        print(sprintf("RB_DELETE92 selected=%i\n",selectedbrushcount));return TRUE;
    }
    if(argv(1)=="delete92apply"){
        float startApply=gettime(1);fb_delete_selection();fb_document_tick();
        print(sprintf("RB_DELETE92 delete_document_ms=%g count=%i retained=%i\n",(gettime(1)-startApply)*1000,fb_delete92_count,fb_delete92_epoch==fb_grid_geometry_epoch));
        int okApply=fb_delete92_count>0&&selectedbrushcount==0&&fb_delete92_epoch==fb_grid_geometry_epoch;
        for(int i=0;i<fb_delete92_count;i++)if(brush_get(fb_delete92_ids[i*2],fb_delete92_ids[i*2+1],__NULL__,0,__NULL__)>0)okApply=FALSE;
        fb_transform_expect(okApply,"delete92_all_removed_retained_cache");return TRUE;
    }
    if(argv(1)=="delete92undo"){
        float startUndo=gettime(1);brush_undo();fb_document_tick();print(sprintf("RB_DELETE92 undo_document_ms=%g selected=%i\n",(gettime(1)-startUndo)*1000,selectedbrushcount));
        int okUndo=selectedbrushcount==fb_delete92_count;static vector box[2];
        for(int i=0;i<selectedbrushcount&&i<fb_delete92_count;i++){
            if(brush_getfacepoints(selectedbrushes[i].model,selectedbrushes[i].id,0,box,2)!=2||box[0]!=fb_delete92_bounds[i*2]||box[1]!=fb_delete92_bounds[i*2+1])okUndo=FALSE;
            fb_delete92_ids[i*2]=selectedbrushes[i].model;fb_delete92_ids[i*2+1]=selectedbrushes[i].id;
        }
        fb_transform_expect(okUndo,"delete92_undo_exact_bounds_selection");fb_delete92_epoch=fb_grid_geometry_epoch;return TRUE;
    }
    if(argv(1)=="delete92redo"){
        float startRedo=gettime(1);brush_redo();fb_document_tick();print(sprintf("RB_DELETE92 redo_document_ms=%g\n",(gettime(1)-startRedo)*1000));
        int okRedo=selectedbrushcount==0&&fb_delete92_epoch==fb_grid_geometry_epoch;
        for(int i=0;i<fb_delete92_count;i++)if(brush_get(fb_delete92_ids[i*2],fb_delete92_ids[i*2+1],__NULL__,0,__NULL__)>0)okRedo=FALSE;
        fb_transform_expect(okRedo,"delete92_redo_removed_retained_cache");return TRUE;
    }
    if(argv(1)=="visual93reference"){fb_visual_test_reference=stoi(argv(2));return TRUE;}
    if(argv(1)=="drop93prepare"){
        fb_document_tick();cvar_set("ftebroom_test_drop_epoch",itos(fb_grid_geometry_epoch));cvar_set("ftebroom_test_drop_start",ftos(gettime(1)));return TRUE;
    }
    if(argv(1)=="drop93check"){
        fb_document_tick();int node=fb_organize_node(fb_selected_entity());
        int retained=fb_grid_geometry_epoch==stoi(cvar_string("ftebroom_test_drop_epoch"));
        fb_transform_expect(retained,"drop93_retains_world_geometry");
        int validModel=FALSE;if(node>=2)validModel=fb_nodes[node].fields["model"]!="";
        fb_transform_expect(validModel,"drop93_model_property_updated");
        print(sprintf("RB_DROP93 retained=%i epoch=%i nodes=%i\n",retained,fb_grid_geometry_epoch,fb_node_count));return TRUE;
    }
    if(argv(1)=="drop93history"){
        int epoch=fb_grid_geometry_epoch;brush_undo();fb_document_tick();brush_redo();fb_document_tick();
        fb_transform_expect(fb_grid_geometry_epoch==epoch,"drop93_undo_redo_retains_geometry");return TRUE;
    }
    if(argv(1)=="viewer92info"){
        print(sprintf("RB_VIEWER92 count=%g open=%g path=%s low=%s high=%s preview=%g\n",cvar("ftebroom_modelviewer_count"),cvar("ftebroom_modelviewer_open"),cvar_string("ftebroom_modelviewer_path"),cvar_string("ftebroom_modeleditor_low"),cvar_string("ftebroom_modeleditor_high"),cvar("ftebroom_mod_preview")));return TRUE;
    }
    if(argv(1)=="alpha92q3"){
        fb_filter_materials_load();fb_transform_expect(fabs(fb_surface_opacity("qa92/glass",FB_TAG_TRANSLUCENT)-.25)<.001,"alpha92_q3_loaded_opacity_metadata");
        fb_transform_expect(fb_shader_masked["qa92/grate"]&&fb_shader_alpha_test["qa92/grate"]=="ge128","alpha92_q3_loaded_mask_metadata");return TRUE;
    }
    if(argv(1)=="alpha92scene"){
        cvar_set("ca_newbrushtexture","checker");fb_createbox('2000 2048 64','512 8 128');fb_createbox('2000 2000 -8','512 320 16');
        cvar_set("ca_newbrushtexture",cvar("ftebroom_quake1")?"{tc_256vine":"qa92/grate");fb_createbox('1880 2000 64','176 4 128');
        cvar_set("ca_newbrushtexture",cvar("ftebroom_quake1")?"clip":"qa92/glass");fb_createbox('2120 2000 64','176 4 128');
        brush_deselectall();cvar_set("ca_newbrushtexture","fb_grey");fb_camera_position('2000 1550 170');setproperty(VF_CL_VIEWANGLES,'12 90 0');return TRUE;
    }
    if(argv(1)=="faces92"){
        fb_selection_clear();int first=fb_createbox('128 128 128','32 32 32');brush_deselectall();int other=fb_createbox('192 128 128','32 32 32');brush_deselectall();brush_select(1,first);
        cvar_set("ftebroom_shift","1");fb_faces_click(1,other,1,FALSE);
        fb_transform_expect(selectedbrushcount==1&&brush_isselected(1,first)&&!brush_isselected(1,other),"faces92_shift_excludes_unselected_brush");
        fb_faces_click(1,first,1,FALSE);fb_transform_expect(brushface_isselected(1,first,1),"faces92_shift_accepts_selected_face");
        cvar_set("ftebroom_shift","0");fb_faces_click(1,other,1,FALSE);fb_transform_expect(brushface_isselected(1,other,1),"faces92_plain_face_tool_selects_others");return TRUE;
    }
    if(argv(1)=="drop92"){
        string expected=strzone(argv(2));int selected=fb_selected_entity();string fields=terrain_edit(TEREDIT_ENT_GET,selected);
        hashtable parsed=hash_createtab(16,EV_STRING);int fieldsCount=fb_entity_tokenize(fields);for(int field=0;field<fieldsCount;field+=2)parsed[argv(field)]=argv(field+1);fb_transform_expect(selected>0&&parsed["classname"]==expected&&parsed["model"]!="","drop92_profile_class_and_model");hash_destroytab(parsed);strunzone(expected);return TRUE;
    }
    if(argv(1)=="alpha92"){
        brushface_t face={0};face.shadername="clip";int tags=fb_filter_face_tags(&face,0);
        fb_transform_expect((tags&FB_TAG_CLIP)&&fabs(fb_surface_opacity("clip",tags)-.35)<.001,"alpha92_clip_service_opacity");
        face.shadername="{tc_256vine";tags=fb_filter_face_tags(&face,0);
        fb_transform_expect(fb_surface_opacity(face.shadername,tags)==1,"alpha92_fence_keeps_opaque_texels");
        string key="alpha92/glass";fb_shader_opacity[key]=.27;fb_shader_blend[key]="blend";
        fb_transform_expect(fabs(fb_surface_opacity(key,FB_TAG_TRANSLUCENT)-.27)<.001,"alpha92_explicit_shader_opacity");
        fb_shader_opacity[key]=1;fb_shader_masked[key]=TRUE;
        fb_transform_expect(fb_surface_opacity(key,FB_TAG_TRANSLUCENT)==1,"alpha92_alpha_test_no_arbitrary_fade");
        return TRUE;
    }
    return FALSE;
}
