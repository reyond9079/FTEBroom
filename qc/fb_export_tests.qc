// SPDX-License-Identifier: GPL-2.0-or-later
void fb_export_expect(int ok,string label){print(strcat(ok?"FTEBROOM_EXPORT_PASS ":"FTEBROOM_EXPORT_FAIL ",label,"\n"));}
int fb_export_testcommand()
{
    if(argv(0)!="ftebroom_test_export" || !cvar("ftebroom_skip_setup"))return FALSE;
    if(argv(1)=="bsp90"){
        fb_tool_deactivate();brush_deselectall();
        fb_createbox('16 32 48','64 64 64');brush_deselectall();
        int id90=fb_createbox('160 320 480','64 64 64');brush_deselectall();brushface_select(1,id90,6);
        int before90=history,count90=selectedbrushcount;
        fb_export_expect(fb_export_selected_bsp(),"bsp90_export_selected");
        fb_export_expect(history==before90&&selectedbrushcount==count90&&selectedbrushes[0].id==id90,"bsp90_no_document_or_selection_changes");
        fb_export_expect(vlen(stov(cvar_string("ftebroom_bsp_export_pivot"))-'160 320 480')<.001,"bsp93_centered_pivot");
        static vector originalBounds[2];brush_getfacepoints(1,id90,0,originalBounds,2);
        fb_export_expect(originalBounds[0]=='128 288 448'&&originalBounds[1]=='192 352 512',"bsp93_source_bounds_unchanged");
        brush_deselectall();fb_export_expect(!fb_export_selected_bsp(),"bsp90_empty_selection_rejected");brushface_select(1,id90,6);return TRUE;
    }
    fb_tool_deactivate();int marker=history;
    fb_export_expect(fb_trace_count==5 && fb_trace_portal_count==1,"diagnostic_files_loaded");
    for(int p=0;p<5;p++)fb_trace_step(1);
    fb_export_expect(fb_trace_index==4 && fb_camera_eye=='64 32 80',"five_leak_points");
    fb_trace_step(-1);fb_export_expect(fb_camera_eye=='48 32 64',"previous_leak_point");
    fb_export_expect(history==marker,"diagnostics_do_not_change_document");
    fb_trace_unload(FALSE);fb_trace_unload(TRUE);
    fb_export_expect(fb_trace_count==0 && fb_trace_portal_count==0,"unload_diagnostics");
    int id=fb_createbox('0 0 128','64 64 32');brush_deselectall();brushface_select(1,id,6);fb_patch_convert();
    fb_createbox('128 0 128','64 64 32');
    fb_organize_action("layernew",0,"No export");int layer=fb_active_layer;
    fb_createbox('10000 0 0','64 64 32');fb_organize_action("layeromit",layer,"1");
    marker=history;fb_export_expect(fb_obj_export(),"obj_export");
    fb_export_expect(history==marker,"obj_export_preserves_history");
    return TRUE;
}
