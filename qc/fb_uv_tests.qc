// SPDX-License-Identifier: GPL-2.0-or-later
void fb_uv_expect(int ok,string label){print(strcat(ok?"FTEBROOM_UV_PASS ":"FTEBROOM_UV_FAIL ",label,"\n"));}
int fb_uv_testcommand()
{
    if(argv(0)!="ftebroom_test_uv" || !cvar("ftebroom_skip_setup"))return FALSE;
    fb_tool_deactivate();int id=fb_createbox('0 0 32','128 128 64');brush_deselectall();brushface_select(1,id,6);
    brushface_t original[64],changed[64];brush_get(1,id,original,64,__NULL__);int marker=history;
    fb_uv_expect(fb_uv_begin(),"single_face_begin");fb_uv_matrix('1 0 16','0 1 0');
    memcpy(changed,fb_uv_preview,fb_uv_count*sizeof(brushface_t));
    fb_uv_expect(fabs(changed[5].sbias-original[5].sbias-16)<0.001,"offset_16_texels");
    brush_get(1,id,changed,64,__NULL__);fb_uv_expect(changed[5].sbias==original[5].sbias && history==marker,"drag_does_not_mutate_brush_or_history");
    float uv107Start=gettime(1);int uv107Id=fb_uv_id,uv107Geometry=fb_grid_geometry_epoch;
    for(int uv107Step=0;uv107Step<240;uv107Step++)fb_uv_matrix([1,0,uv107Step+.25],'0 1 0');
    fb_uv_expect(fb_uv_id==uv107Id && history==marker && fb_grid_geometry_epoch==uv107Geometry,"240_drag_updates_keep_geometry_and_history_stable");
    print(sprintf("UV107_PREVIEW updates=240 ms=%g\n",(gettime(1)-uv107Start)*1000));
    fb_uv_matrix('1 0 32','0 1 0');fb_uv_finish(TRUE);
    brush_get(selectedbrushes[0].model,selectedbrushes[0].id,changed,64,__NULL__);
    int same=TRUE;for(int f=0;f<6;f++)if(changed[f].planenormal!=original[f].planenormal || changed[f].planedist!=original[f].planedist)same=FALSE;
    fb_uv_expect(same && fabs(changed[5].sbias-original[5].sbias-32)<0.001,"absolute_preview_preserves_geometry");
    brush_undo();brush_get(selectedbrushes[0].model,selectedbrushes[0].id,changed,64,__NULL__);
    fb_uv_expect(history==marker && changed[5].sbias==original[5].sbias,"single_undo");
    int redo=history_max;fb_uv_begin();fb_uv_matrix('1 0 99','0 1 0');fb_uv_finish(FALSE);
    fb_uv_expect(history==marker && history_max==redo,"cancel_preserves_redo");
    fb_uv_begin();fb_uv_matrix('0 -1 0','1 0 0');memcpy(changed,fb_uv_preview,fb_uv_count*sizeof(brushface_t));
    fb_uv_expect(changed[5].sdir==-original[5].tdir && changed[5].tdir==original[5].sdir,"rotation_axes");fb_uv_finish(FALSE);
    fb_uv_begin();fb_uv_matrix('1 0.5 0','0 1 0');memcpy(changed,fb_uv_preview,fb_uv_count*sizeof(brushface_t));
    fb_uv_expect(changed[5].sdir==original[5].sdir+original[5].tdir*0.5,"valve220_shear");fb_uv_finish(FALSE);
    fb_uv_begin();int before=history;fb_uv_matrix('0 0 0','0 1 0');fb_uv_expect(history==before,"singular_matrix_rejected");fb_uv_finish(FALSE);
    fb_repeat_clear();fb_uv_begin();fb_uv_matrix('1 0 16','0 1 0');fb_uv_finish(TRUE);
    marker=history;fb_repeat_run();brush_get(selectedbrushes[0].model,selectedbrushes[0].id,changed,64,__NULL__);
    fb_uv_expect(fabs(changed[5].sbias-original[5].sbias-32)<0.001,"canvas_command_repeat");
    brush_undo();fb_uv_expect(history==marker,"repeat_single_undo");brush_undo();
    fb_uv_publish();fb_uv_expect(cvar("ftebroom_uv_face")==1,"face_canvas_published");return TRUE;
}
