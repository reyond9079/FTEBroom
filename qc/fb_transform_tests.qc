// SPDX-License-Identifier: GPL-2.0-or-later
void fb_transform_expect(int condition,string name)
{
    print(strcat(condition?"FTEBROOM_TRANSFORM_PASS ":"FTEBROOM_TRANSFORM_FAIL ",name,"\n"));
}
float fb_transform_testcommand()
{
    if(argv(0)!="ftebroom_test_transform" || !cvar("ftebroom_skip_setup")) return FALSE;
    if(argv(1)=="multi") {
        int ids[3];
        for(int i=0;i<3;i++) ids[i]=fb_createbox([i*64,0,64],'32 32 32');
        fb_selection_clear();
        for(int i=0;i<3;i++) brush_select(1,ids[i]);
        int before=history;
        fb_transform_expect(fb_transform_capture() && fb_transform_count==3,"capture_three");
        fb_transform_commit('48 -16 0');fb_transform_clear();
        static vector bounds[2];
        int valid=selectedbrushcount==3;
        for(int b=0;b<selectedbrushcount;b++) {
            brush_getfacepoints(1,selectedbrushes[b].id,0,bounds,2);
            if(vlen(bounds[0]-[b*64+32,-32,48])>0.01) valid=FALSE;
        }
        fb_transform_expect(valid,"move_three_exact_delta");
        brush_undo();fb_transform_expect(history==before && selectedbrushcount==3,"multi_one_undo");
        brush_redo();fb_transform_expect(selectedbrushcount==3,"multi_redo");
        fb_transform_capture();before=history;
        fb_transform_commit('65536 0 0');fb_transform_clear();
        fb_transform_expect(history==before,"world_reject_atomic");
        fb_transform_expect(vlen(fb_transform_nudge_axis(1,0,'0 90 0')-'0 1 0')<0.01,"nudge_forward_y");
        fb_transform_expect(vlen(fb_transform_nudge_axis(1,0,'0 180 0')-'-1 0 0')<0.01,"nudge_forward_negative_x");
        fb_transform_expect(vlen(fb_transform_nudge_axis(2,2,'0 0 0')-'0 -1 0')<0.01,"xz_depth_toward_viewer");
        before=history;fb_duplicate_numeric('16 0 0');
        fb_transform_expect(selectedbrushcount==3,"duplicate_three");
        brush_undo();fb_transform_expect(history==before && selectedbrushcount==3,"duplicate_one_undo");
        static vector movedBounds[2];
        brush_getfacepoints(1,selectedbrushes[0].id,0,movedBounds,2);
        vector pixel=project((movedBounds[0]+movedBounds[1])*0.5);pixel_z=0;
        cvar_set("ftebroom_ctrl","0");cvar_set("ftebroom_shift","0");cvar_set("ca_brush_view","0");
        fb_begin(pixel,1,0);fb_update(pixel+'0 -16 0',1,0);
        fb_transform_expect(fb_transform_preview && fb_transform_count==3 && fb_transform_delta_z>0 && fb_transform_delta_x==0 && fb_transform_delta_y==0 && fabs(fb_transform_delta_z/fb_grid()-rint(fb_transform_delta_z/fb_grid()))<.001,"three_object_alt_preview_snapped");
        fb_cancel();fb_transform_expect(history==before && selectedbrushcount==3,"multi_cancel_no_history");
    } else if(argv(1)=="mixed") {
        fb_document_tick();fb_selection_clear();
        int patch=-1,entityIndex=-1;
        for(int n=0;n<fb_node_count;n++) {
            if(fb_nodes[n].kind==FB_NODE_PATCH) {patch=n;brush_select(fb_nodes[n].model,fb_nodes[n].id);}
            if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_selection_leaf(n)) {entityIndex=fb_nodes[n].id;fb_entity_select(entityIndex,TRUE);}
        }
        vector origin=fb_entity_origin(entityIndex);
        fb_transform_expect(patch>=0 && entityIndex>=0 && fb_transform_capture(),"mixed_capture");
        int mixedBefore=history;
        fb_transform_commit('16 -32 48');fb_transform_clear();
        fb_transform_expect(vlen(fb_entity_origin(entityIndex)-origin-'16 -32 48')<0.01,"entity_moves_with_patch");
        brush_undo();
        fb_transform_expect(history==mixedBefore && vlen(fb_entity_origin(entityIndex)-origin)<0.01,"mixed_one_undo");
        fb_duplicate_numeric('0 0 0');
        fb_transform_expect(selectedbrushcount==1 && fb_selected_entity_count==1 && fb_selected_entities[0]!=entityIndex,"mixed_duplicate");
        fb_transform_expect(terrain_edit(TEREDIT_ENT_GET,fb_selected_entities[0])!="","mixed_duplicate_persists_fields");
        brush_undo();
        fb_transform_expect(history==mixedBefore && fb_entity_selected(entityIndex),"mixed_duplicate_undo");
        fb_delete_selection();
        fb_transform_expect(selectedbrushcount==0 && fb_selected_entity_count==0,"mixed_delete");
        brush_undo();
        fb_transform_expect(history==mixedBefore && selectedbrushcount==1 && fb_entity_selected(entityIndex),"mixed_delete_undo");
    }
    return TRUE;
}
