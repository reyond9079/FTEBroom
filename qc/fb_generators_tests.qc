vector *fb_move89_origins;int *fb_move89_ids;int fb_move89_count,fb_move89_history,fb_move89_epoch;
vector() ftebroom_cached_mesh_stats=#0:ftebroom_cached_mesh_stats;
float fb_benchmark_start,fb_benchmark_frames;
#include "fb_bevel_extended_tests.qc"
// SPDX-License-Identifier: GPL-3.0-or-later
int fb_generators_test_history;
#include "fb_performance_tests.qc"
#include "fb_delete_tests.qc"
void fb_polygon_test_rectangle(vector center,float half,float height)
{
    fb_polygon_count=4;fb_polygon_closed=TRUE;fb_polygon_normal='0 0 1';
    fb_polygon_points[0]=center+[-half,-half,height];fb_polygon_points[1]=center+[half,-half,height];fb_polygon_points[2]=center+[half,half,height];fb_polygon_points[3]=center+[-half,half,height];
}

void fb_fullbench_tick()
{
    static int started;
    if(!cvar("ftebroom_skip_setup") || !cvar("ftebroom_fullbench")){started=FALSE;return;}
    if(cvar("ftebroom_fullbench_qa")==13){
        if(!started && fb_node_count>=2 && strstrofs(cvar_string("mapname"),"_ftebroom_open",0)>=0){started=TRUE;localcmd("in 2 exec profile82-stages.cfg\n");}return;
    }
    if(cvar("ftebroom_fullbench_qa")==10){
        if(started || fb_node_count<2)return;
        started=TRUE;localcmd("in 8 ftebroom_camera_restore 800 -512 256 20 90 0 90\nin 10 ftebroom_test_generators startup83\nin 12 ftebroom_test_generators layout83\nin 14 screenshot startup83.png\nin 15 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")==11){
        static float model84Ready;
        if(!started){started=TRUE;model84Ready=0;localcmd("ftebroom_test_desktop mods63check\n");}
        if(cvar("ftebroom_modeleditor_drawn")>0){
            if(!model84Ready)model84Ready=gettime(0);
            if(model84Ready>0 && gettime(0)-model84Ready>3){model84Ready=-1;localcmd(strcat("ftebroom_test_generators model84\nscreenshot ",cvar_string("ftebroom_test_capture"),"\nin 1 quit force\n"));}
        }
        return;
    }
    if(cvar("ftebroom_fullbench_qa")==12){
        if(!started && strstrofs(cvar_string("mapname"),"_ftebroom_open",0)>=0){started=TRUE;localcmd("in 3 ftebroom_test_generators spikes87\nin 7 quit force\n");}return;
    }
    if(started || fb_node_count<50000 || !cvar("ftebroom_large_map_ready"))return;
    started=TRUE;print("RB_FULLBENCH_READY\n");
    string name=cvar_string("ftebroom_fullbench_name");if(name=="")name="full76";
    if(cvar("ftebroom_fullbench_qa")==9){localcmd("exec profile82-stages.cfg\n");return;}
    if(cvar("ftebroom_fullbench_qa")==8){
        localcmd("ftebroom_test_generators frame76\nin 2 ftebroom_test_generators perf80 1\nin 4 ftebroom_test_generators incremental81finish\nin 7 ftebroom_test_generators perf80 2\nin 9 ftebroom_test_generators incremental81finish\nin 12 ftebroom_test_generators create78undo\nin 14 ftebroom_test_generators create78redo\nin 17 ftebroom_test_generators perf80 3\nin 19 ftebroom_test_generators incremental81finish\nin 21 ftebroom_test_generators incremental81check\nin 21.5 ftebroom_test_generators incremental81stress\nin 24 ftebroom_camera_focus\nin 24 set ftebroom_show_models 0\nin 27 screenshot incremental81-live.png\nin 28 ftebroom_test_generators document80\nin 38 screenshot incremental81-rebuilt.png\nin 39 ftebroom_test_generators incremental81check\nin 41 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")==7){
        localcmd("ftebroom_test_generators frame76\nin 2 set ftebroom_benchmark_name idle\nin 2 set ftebroom_benchmark 1\nin 14 ftebroom_test_generators perf80 1\nin 16 set ftebroom_benchmark_name create\nin 16 set ftebroom_benchmark 1\nin 28 ftebroom_test_generators perf80 0\nin 31 ftebroom_test_generators perf80 2\nin 33 set ftebroom_benchmark_name move\nin 33 set ftebroom_benchmark 1\nin 45 ftebroom_test_generators perf80 0\nin 48 ftebroom_test_generators perf80 3\nin 50 set ftebroom_benchmark_name vertex\nin 50 set ftebroom_benchmark 1\nin 62 ftebroom_test_generators perf80 0\nin 65 set ftebroom_benchmark_name release\nin 65 set ftebroom_benchmark 1\nin 66 ftebroom_test_generators create78begin\nin 66.1 ftebroom_test_generators create78update\nin 66.2 ftebroom_test_generators create78finish\nin 130 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")==6){
        localcmd("ftebroom_test_generators frame76\nin 2 ftebroom_test_generators create78begin\nin 4 ftebroom_test_generators create78update\nin 6 ftebroom_test_generators create78finish\nin 10 ftebroom_test_generators create78check 30328\nin 12 ftebroom_test_generators create78undo\nin 16 ftebroom_test_generators create78check 30327\nin 18 ftebroom_test_generators create78redo\nin 22 ftebroom_test_generators create78check 30328\nin 24 ftebroom_test_generators create78delete\nin 28 ftebroom_test_generators create78check 30327\nin 30 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")==5){
        localcmd("ftebroom_test_generators frame76\nin 2 screenshot restart77-before.png\nin 3 set ftebroom_fullbench 0\nin 4 set vid_multisample 8\nin 4 ftebroom_renderer_restart\nin 20 screenshot restart77-after.png\nin 21 ftebroom_test_generators bench76\nin 22 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")==4){
        localcmd("ftebroom_test_generators frame76\nset ftebroom_show_entity_names 1\nset ftebroom_cached_meshes 1\nset ftebroom_merge_missing_materials 1\nin 2 set ftebroom_geometry_occlusion 0\nin 3 screenshot pose77-0-reference.png\nin 4 set ftebroom_geometry_occlusion 1\nin 5 screenshot pose77-0-fast.png\nin 7 ftebroom_camera_restore -9715.52 -8403.52 -5358.29 -24.6845 45 0 90\nin 7 set ftebroom_geometry_occlusion 0\nin 8 screenshot pose77-1-reference.png\nin 9 set ftebroom_geometry_occlusion 1\nin 10 screenshot pose77-1-fast.png\nin 12 ftebroom_camera_restore -448 288 8 0 45 0 90\nin 12 set ftebroom_geometry_occlusion 0\nin 13 screenshot pose77-2-reference.png\nin 14 set ftebroom_geometry_occlusion 1\nin 15 screenshot pose77-2-fast.png\nin 17 ftebroom_camera_restore 1000 -1000 128 0 135 0 90\nin 17 set ftebroom_geometry_occlusion 0\nin 18 screenshot pose77-3-reference.png\nin 19 set ftebroom_geometry_occlusion 1\nin 20 screenshot pose77-3-fast.png\nin 22 ftebroom_camera_restore -8000 0 128 0 0 0 90\nin 22 set ftebroom_geometry_occlusion 0\nin 23 screenshot pose77-4-reference.png\nin 24 set ftebroom_geometry_occlusion 1\nin 25 screenshot pose77-4-fast.png\nin 27 ftebroom_camera_restore 0 -7400 1600 15 90 0 90\nin 27 set ftebroom_geometry_occlusion 0\nin 28 screenshot pose77-5-reference.png\nin 29 set ftebroom_geometry_occlusion 1\nin 30 screenshot pose77-5-fast.png\nin 33 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")==3){
        localcmd("ftebroom_test_generators frame76\nin 2 set ftebroom_show_entity_names 0\nin 2 set ftebroom_cached_meshes 0\nin 2 set ftebroom_merge_missing_materials 0\nin 4 ftebroom_test_generators cam77\nin 4 screenshot qa77-base.png\nin 6 set ftebroom_cached_meshes 1\nin 8 ftebroom_test_generators cam77\nin 8 screenshot qa77-cache.png\nin 10 set ftebroom_cached_meshes 0\nin 10 set ftebroom_merge_missing_materials 1\nin 12 ftebroom_test_generators cam77\nin 12 screenshot qa77-shaders.png\nin 14 set ftebroom_cached_meshes 1\nin 14 set ftebroom_geometry_occlusion 1\nin 16 ftebroom_test_generators cam77\nin 16 screenshot qa77-both.png\nin 17 ftebroom_test_generators bench76\nin 18 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")==2){
        localcmd("ftebroom_test_generators frame76\nin 2 set ftebroom_show_entity_names 0\nin 2 set ftebroom_cached_meshes 0\nin 2 set ftebroom_merge_missing_materials 0\nin 4 screenshot full77-reference.png\nin 6 set ftebroom_cached_meshes 1\nin 6 set ftebroom_merge_missing_materials 1\nin 8 screenshot full77-retained.png\nin 10 quit force\n");return;
    }
    if(cvar("ftebroom_fullbench_qa")){
        localcmd("ftebroom_test_generators frame76\nin 3 set ftebroom_show_entity_names 0\nin 3 set ftebroom_occlusion_culling 0\nin 4 screenshot full76-baseline.png\nin 7 set ftebroom_occlusion_culling 2\nin 8 screenshot full76-geometry.png\nin 11 set ftebroom_show_entity_names 1\nin 11 set ftebroom_occlusion_culling 1\nin 12 screenshot full76-labels.png\nin 14 quit force\n");return;
    }
    localcmd(strcat("ftebroom_test_generators frame76\nin 3 screenshot ",name,".png\nin 4 set ftebroom_benchmark 1\nin 15 set ftebroom_right 1\nin 15 set ftebroom_benchmark 1\nin 26 set ftebroom_right 0\nin 26 screenshot full77-end.png\nin 27 ftebroom_test_generators bench76\nin 28 quit force\n"));
}
int fb_generators_testcommand()
{
    if(argv(0)!="ftebroom_test_generators"||!cvar("ftebroom_skip_setup"))return FALSE;
    if(fb_delete92_testcommand())return TRUE;
    if(argv(1)=="preview89"){
        fb_transform_expect(fb_transform_capture(),"preview89_capture");
        fb_transform_preview=TRUE;fb_transform_delta='16 0 0';
        print(sprintf("RB_PREVIEW89 begin count=%i\n",fb_transform_count));return TRUE;
    }
    if(argv(1)=="preview89check"){
        int ok89=fb_transform_preview && fb_transform_count>0 && fb_transform_count==fb_selected_entity_count && fb_transform_point_count==fb_selected_entity_count;
        for(int i=0;i<fb_transform_count;i++){
            fb_transform_item_t *item89=&fb_transform_items[i];
            if(!fb_transform_replaces(item89->model,item89->id) || fb_entity_origin(item89->id)!=item89->origin)ok89=FALSE;
        }
        if(fb_transform_replaces(0,-1) || fb_transform_replaces(0,fb_transform_entity_capacity) || fb_transform_replaces(1,1))ok89=FALSE;
        fb_transform_expect(ok89,"preview89_all_points_indexed_originals_unchanged");
        fb_transform_expect(checkbuiltin(ftebroom_preview_boxes),"preview89_native_batch_available");return TRUE;
    }
    if(argv(1)=="cancel89"){fb_transform_clear();print("RB_PREVIEW89 cancelled\n");return TRUE;}
    if(argv(1)=="batch91guard"){
        int cursor91=history;
        int rejected91=!fb_incremental_apply(-1,0,FALSE);
        if(fb_incremental_apply(history_max,history_max+1,TRUE))rejected91=FALSE;
        fb_document_loaded();
        fb_transform_expect(rejected91 && history==cursor91,"batch91_reload_invalid_ranges_safe");return TRUE;
    }
    if(argv(1)=="load91begin"){cvar_set("ftebroom_fullbench_name",sprintf("%.9g",gettime(1)));print("RB_LOAD91_BEGIN\n");return TRUE;}
    if(argv(1)=="load91end"){print(sprintf("RB_LOAD91_READY ms=%g nodes=%i faces=%i\n",(gettime(1)-stof(cvar_string("ftebroom_fullbench_name")))*1000,fb_node_count,fb_render_face_count));return TRUE;}
    if(argv(1)=="brushes91select"){
        fb_tool_refresh();fb_document_tick();fb_selection_clear();
        for(int n=2;n<fb_node_count && selectedbrushcount<1024;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH && fb_nodes[n].model==1)brush_select(1,fb_nodes[n].id);
        cvar_set("ftebroom_auto_integer_snap","0");cvar_set("ftebroom_texture_lock","1");
        print(sprintf("RB_BRUSH91 selected=%i\n",selectedbrushcount));return TRUE;
    }
    if(argv(1)=="brushes91move"){
        int count91=selectedbrushcount,epoch91=fb_grid_geometry_epoch;
        int *ids91=memalloc(count91*sizeof(int));vector *perfBounds91=memalloc(count91*2*sizeof(vector));
        for(int i=0;i<count91;i++){ids91[i]=selectedbrushes[i].id;brush_getfacepoints(1,ids91[i],0,perfBounds91+i*2,2);}
        float start91=gettime(1);fb_move_numeric('16 -32 8');fb_document_tick();
        print(sprintf("RB_BRUSH91 move_document_ms=%g brushes=%i retained=%i\n",(gettime(1)-start91)*1000,count91,fb_grid_geometry_epoch==epoch91));
        int ok91=count91>0 && fb_grid_geometry_epoch==epoch91;static vector actual91[2];
        for(int i=0;i<count91;i++)if(brush_getfacepoints(1,ids91[i],0,actual91,2)!=2 || vlen(actual91[0]-perfBounds91[i*2]-'16 -32 8')>.001)ok91=FALSE;
        fb_transform_expect(ok91,"batch91_large_brush_bounds_ids");
        for(int i=0;i<fb_render_face_count;i++)if(fabs(fb_render_faces[i].distance-fb_render_faces[i].normal*fb_render_faces[i].points[0])>.1)ok91=FALSE;
        fb_transform_expect(ok91,"batch91_cached_face_planes");
        brush_undo();fb_document_tick();ok91=fb_grid_geometry_epoch==epoch91;
        for(int i=0;i<count91;i++)if(brush_getfacepoints(1,ids91[i],0,actual91,2)!=2 || actual91[0]!=perfBounds91[i*2] || actual91[1]!=perfBounds91[i*2+1])ok91=FALSE;
        fb_transform_expect(ok91,"batch91_large_brush_undo_exact");brush_redo();fb_document_tick();
        memfree(ids91);memfree(perfBounds91);return TRUE;
    }
    if(argv(1)=="batch91brush"){
        float oldSnap=cvar("ftebroom_auto_integer_snap"),oldLock=cvar("ftebroom_texture_lock");
        cvar_set("ftebroom_auto_integer_snap","0");cvar_set("ftebroom_texture_lock","1");
        int ids[3];for(int i=0;i<3;i++)ids[i]=fb_createbox([i*64,0,64],'32 32 32');
        fb_selection_clear();for(int i=0;i<3;i++)brush_select(1,ids[i]);
        brushface_t original[6],changed[6];brush_get(1,ids[0],original,6,__NULL__);
        int marker91=history;fb_move_numeric('.125 -.25 .5');fb_document_tick();
        history_t *record=&historyring[(history-1)&(historyring_length-1)];
        int fast=FALSE;if(record->kind==HIST_MOVE_BATCH)fast=record->batch.brushCount==3;
        fb_transform_expect(fast,"batch91_native_brush_path");
        int valid91=fast;static vector bounds91[2];
        for(int i=0;i<3;i++)if(brush_getfacepoints(1,ids[i],0,bounds91,2)!=2 || vlen(bounds91[0]-[i*64-16,-16,48]-'.125 -.25 .5')>.00001)valid91=FALSE;
        fb_transform_expect(valid91,"batch91_stable_ids_fractional_bounds");
        brush_get(1,ids[0],changed,6,__NULL__);valid91=TRUE;
        for(int f=0;f<6;f++)if(fabs(changed[f].sbias-original[f].sbias+original[f].sdir*'.125 -.25 .5')>.00001 || fabs(changed[f].tbias-original[f].tbias+original[f].tdir*'.125 -.25 .5')>.00001)valid91=FALSE;
        fb_transform_expect(valid91,"batch91_texture_lock");
        for(int i=0;i<16;i++){brush_undo();brush_redo();}
        brush_undo();brush_get(1,ids[0],changed,6,__NULL__);valid91=history==marker91;
        for(int f=0;f<6;f++)if(changed[f].planedist!=original[f].planedist || changed[f].sbias!=original[f].sbias || changed[f].tbias!=original[f].tbias)valid91=FALSE;
        fb_transform_expect(valid91,"batch91_repeated_undo_exact");brush_redo();
        if(fast){
            int saved=record->batch.ids[1],cursor=history;record->batch.ids[1]=-1;
            brush_undo();fb_transform_expect(history==cursor,"batch91_invalid_history_cancelled");record->batch.ids[1]=saved;
        }
        brush_get(1,ids[0],changed,6,__NULL__);changed[0].sbias+=4;
        fb_tx_begin("UV test");brush_history_edit(1,ids[0],changed,6,1);fb_tx_commit();brush_undo();brush_undo();
        fb_transform_expect(history==marker91,"batch91_remapped_geometry_undo");brush_redo();
        cvar_set("ftebroom_auto_integer_snap",ftos(oldSnap));cvar_set("ftebroom_texture_lock",ftos(oldLock));return TRUE;
    }
    if(argv(1)=="move89" || argv(1)=="move91"){
        int release91=argv(1)=="move91";
        memfree(fb_move89_origins);memfree(fb_move89_ids);fb_move89_count=fb_selected_entity_count;
        fb_move89_origins=memalloc(max(1,fb_move89_count)*sizeof(vector));fb_move89_ids=memalloc(max(1,fb_move89_count)*sizeof(int));
        for(int i=0;i<fb_move89_count;i++){fb_move89_ids[i]=fb_selected_entities[i];fb_move89_origins[i]=fb_entity_origin(fb_selected_entities[i]);}
        fb_move89_history=history;fb_move89_epoch=fb_grid_geometry_epoch;
        if(release91)fb_transform_capture();
        float started89=gettime(1);if(release91){fb_transform_commit('16 0 0');fb_transform_clear();}else fb_move_numeric('16 0 0');fb_document_tick();
        fb_transform_expect(fb_grid_geometry_epoch==fb_move89_epoch,"move89_retains_brush_geometry");
        print(sprintf("RB_MOVE89 commit_ms=%g entities=%i release_only=%i records=%i\n",(gettime(1)-started89)*1000,fb_selected_entity_count,release91,history-fb_move89_history));return TRUE;
    }
    if(argv(1)=="undo89" || argv(1)=="redo89" || argv(1)=="check89"){
        int redo89=argv(1)=="redo89",undo89=argv(1)=="undo89";if(undo89)brush_undo();else if(redo89)brush_redo();
        fb_document_tick();int valid89=fb_move89_count>0 && fb_selected_entity_count==fb_move89_count && fb_grid_geometry_epoch==fb_move89_epoch;
        if(undo89 && history!=fb_move89_history)valid89=FALSE;
        if(redo89 && history<=fb_move89_history)valid89=FALSE;
        vector delta89=undo89?'0 0 0':'16 0 0';
        for(int i=0;i<fb_move89_count;i++)if(vlen(fb_entity_origin(fb_move89_ids[i])-fb_move89_origins[i]-delta89)>.001)valid89=FALSE;
        fb_transform_expect(valid89,redo89?"move89_redo_all_origins":(undo89?"move89_undo_all_origins":"move89_exact_all_origins"));return TRUE;
    }
    if(argv(1)=="pointcolors88"){
        fb_point_render_prepare();int errors88=0,checked88=0;
        int words88=sizeof(trisoup_simple_vert_t)/sizeof(int);int *saved88=memalloc(32*sizeof(trisoup_simple_vert_t));
        for(int i=0;i<min(32,fb_point_render_count);i++){
            fb_point_render_t *point88=&fb_point_render[i];fb_point_geometry_t *geometry88=&fb_point_geometry[i];
            memcpy(saved88,geometry88.wire,8*sizeof(trisoup_simple_vert_t));memcpy(saved88+8*words88,geometry88.solid,24*sizeof(trisoup_simple_vert_t));
            fb_entity_native_color(point88.id,'-1 -1 -1');fb_entity_draw_box(point88.id,point88.color,FALSE,FALSE,"");
            int *wire88=(int*)geometry88.wire;int *solid88=(int*)geometry88.solid;
            for(int word88=0;word88<32*words88;word88++)if(saved88[word88]!=(word88<8*words88?wire88[word88]:solid88[word88-8*words88]))errors88++;
            checked88++;
        }
        memfree(saved88);fb_transform_expect(checked88>0 && !errors88,"pointcolors88_matches_script_bytes");
        print(sprintf("RB_POINTCOLORS88 points=%i mismatched_words=%i\n",checked88,errors88));return TRUE;
    }
    if(argv(1)=="bounds88"){
        int valid88=FALSE;static vector bounds88[2];vector low88='0 0 0',high88='0 0 0';
        for(int i=0;i<fb_selected_entity_count;i++)if(fb_entity_bounds(fb_selected_entities[i],bounds88)){
            if(!valid88){low88=bounds88[0];high88=bounds88[1];valid88=TRUE;}
            else for(int a=0;a<3;a++){low88[a]=min(low88[a],bounds88[0][a]);high88[a]=max(high88[a],bounds88[1][a]);}
        }
        if(!valid88 || selectedbrushcount){fb_transform_expect(FALSE,"bounds88_point_fixture");return TRUE;}
        float captured88=cvar("ftebroom_camera_captured");cvar_set("ftebroom_camera_captured","1");
        fb_view_remember_selection();fb_visual_prepare();
        fb_transform_expect(fb_view_lastmin==low88 && fb_view_lastmax==high88 && fb_visual_min==low88 && fb_visual_max==high88,"bounds88_matches_full_reference");
        fb_view_remember_selection();fb_visual_prepare();
        fb_transform_expect(fb_visual_min==low88 && fb_visual_max==high88,"bounds88_reuses_static_selection");
        cvar_set("ftebroom_camera_captured",ftos(captured88));return TRUE;
    }
    if(argv(1)=="peers88"){
        fb_document_tick();fb_link_peers_prepare();
        if(fb_node_count>1024){fb_transform_expect(fb_link_peer_count==stoi(argv(2)),"peers88_large_map_candidate_count");return TRUE;}
        int peer88Errors=0,peer88Groups=0;
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP && fb_nodes[n].fields["_tb_linked_group_id"]!=""){
            string key88=fb_nodes[n].fields["_tb_linked_group_id"];int actual88=(int)fb_link_peer_heads[key88],others88=0;
            for(int peer88=2;peer88<fb_node_count;peer88++)if(fb_nodes[peer88].kind==FB_NODE_GROUP && fb_nodes[peer88].fields["_tb_linked_group_id"]==key88){
                if(actual88!=peer88)peer88Errors++;if(actual88)actual88=fb_link_peer_next[actual88];if(peer88!=n)others88++;
            }
            if(actual88)peer88Errors++;if(others88)peer88Groups++;
        }
        fb_transform_expect(!peer88Errors && peer88Groups==fb_link_peer_count,"peers88_matches_full_scan_order");
        int builds88=fb_link_peer_builds;fb_link_peers_prepare();
        fb_transform_expect(builds88==fb_link_peer_builds,"peers88_reuses_unchanged_document");return TRUE;
    }
    if(argv(1)=="spikes87"){
        fb_document_tick();fb_render_allowed(0,FB_PICK_LOCKED);
        static vector spike87Bounds[2];int compared87=0,mismatch87=0;
        for(int n=2;n<fb_node_count && compared87<8;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH && brush_getfacepoints(fb_nodes[n].model,fb_nodes[n].id,0,spike87Bounds,2)==2){
            fb_visual_spike_batch(spike87Bounds[0],spike87Bounds[1]);
            for(int vertex=0;vertex<8;vertex++)for(int axis=0;axis<3;axis++){
                int index=vertex*3+axis;vector start='0 0 0',end;
                for(int a=0;a<3;a++)start[a]=(vertex&(1i<<a))?spike87Bounds[1][a]:spike87Bounds[0][a];
                end=start;end[axis]+=((vertex&(1i<<axis))?1:-1)*256;
                vector tip=fb_visual_spike_trace(start,end,TRUE);
                if(vlen(tip-fb_visual_spike_tips[index])>.01 || fb_visual_spike_hit!=fb_visual_spike_hits[index])mismatch87++;
            }
            compared87++;
        }
        fb_transform_expect(compared87>0 && mismatch87==0,"spikes87_native_matches_reference");
        print(sprintf("RB_SPIKES87 brushes=%i mismatches=%i\n",compared87,mismatch87));return TRUE;
    }
    if(argv(1)=="group87"){
        fb_tool_refresh();fb_document_tick();fb_selection_clear();
        int group87=-1;int *sizes87=memalloc(fb_node_count*sizeof(int));
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH && fb_nodes[n].model!=1)sizes87[fb_nodes[n].parent]++;
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP && sizes87[n]>0 && sizes87[n]<128 && (group87<0 || sizes87[n]>sizes87[group87]))group87=n;
        memfree(sizes87);if(group87<0){fb_transform_expect(FALSE,"group87_fixture");return TRUE;}
        fb_selection_set(group87,TRUE);int epoch87=fb_grid_geometry_epoch,brushes87=selectedbrushcount;
        float start87=gettime(1);fb_duplicate_numeric('0 0 0');fb_document_tick();
        fb_transform_expect(fb_grid_geometry_epoch==epoch87 && selectedbrushcount==brushes87,"group87_duplicate_retains_base");
        print(sprintf("RB_GROUP87 duplicate_ms=%g brushes=%i\n",(gettime(1)-start87)*1000,brushes87));
        start87=gettime(1);fb_move_numeric('16 0 0');fb_document_tick();
        fb_transform_expect(fb_grid_geometry_epoch==epoch87,"group87_move_retains_base");
        print(sprintf("RB_GROUP87 move_ms=%g\n",(gettime(1)-start87)*1000));
        brush_undo();fb_document_tick();brush_undo();fb_document_tick();brush_redo();fb_document_tick();brush_redo();fb_document_tick();
        fb_transform_expect(fb_grid_geometry_epoch==epoch87,"group87_undo_redo_retains_base");
        return TRUE;
    }
    if(argv(1)=="select87"){
        fb_tool_refresh();fb_document_tick();fb_selection_clear();
        int select87=-1;int *pointSizes87=memalloc(fb_node_count*sizeof(int));
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY)pointSizes87[fb_nodes[n].parent]++;
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP && (select87<0 || pointSizes87[n]>pointSizes87[select87]))select87=n;
        memfree(pointSizes87);
        if(select87<0){fb_transform_expect(FALSE,"select87_fixture");return TRUE;}
        fb_selection_set(fb_selection_target(select87),TRUE);
        float selectTime87=gettime(1);int ok87=fb_visual_spike_batch('-5800 -6700 -1300','2800 6400 1300');
        fb_transform_expect(ok87,"select87_native_guides_large_map");
        print(sprintf("RB_SELECT87 entities=%i brushes=%i guides_ms=%g\n",fb_selected_entity_count,selectedbrushcount,(gettime(1)-selectTime87)*1000));return TRUE;
    }
    if(argv(1)=="selection86check") {
        int expected86=stoi(argv(2)),check86Ok=fb_selected_entity_count==expected86;
        for(int i=0;i<fb_selected_entity_count;i++)if(!fb_entity_selected(fb_selected_entities[i]) || terrain_edit(TEREDIT_ENT_GET,fb_selected_entities[i])=="")check86Ok=FALSE;
        fb_transform_expect(check86Ok,"selection86_complete_live_entity_membership");
        print(sprintf("RB_SELECTION86 points=%i slots=%i cacheCapacity=%i\n",fb_selected_entity_count,(int)terrain_edit(TEREDIT_ENT_COUNT),editents_capacity));return TRUE;
    }
    if(argv(1)=="duplicate86" || argv(1)=="select86") {
        fb_tool_refresh();fb_document_tick();fb_selection_clear();
        int *sizes=memalloc(sizeof(int)*fb_node_count);int largest=-1;
        for(int n=2;n<fb_node_count;n++)if(fb_selection_leaf(n))
            for(int parent=fb_nodes[n].parent,steps=0;parent>=2 && parent<fb_node_count && steps<fb_node_count;steps++) {
                sizes[parent]++;parent=fb_nodes[parent].parent;
            }
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP && (largest<0 || sizes[n]>sizes[largest]))largest=n;
        if(largest<0){memfree(sizes);fb_transform_expect(FALSE,"duplicate86_group_fixture");return TRUE;}
        fb_selection_set(largest,TRUE);int selected=selectedbrushcount,duplicate86Marker=history;
        print(sprintf("RB_DUPLICATE86 group=%i direct=%i selected=%i entities=%i nodes=%i\n",largest,sizes[largest],selected,fb_selected_entity_count,fb_node_count));memfree(sizes);
        if(argv(1)=="select86") {
            fb_transform_expect(selectedbrushcount+fb_selected_entity_count>1000,"select86_large_group");
            fb_selection_set(largest,FALSE);
            fb_transform_expect(!selectedbrushcount && !fb_selected_entity_count,"select86_large_group_deselect");
            return TRUE;
        }
        float started=gettime(1);fb_duplicate_numeric('0 0 0');
        fb_transform_expect(selectedbrushcount==selected && history>duplicate86Marker,"duplicate86_group_count");
        print(sprintf("RB_DUPLICATE86 commit_ms=%g selected=%i history=%i\n",(gettime(1)-started)*1000,selectedbrushcount,history));
        return TRUE;
    }
    if(argv(1)=="material83"){
        fb_render_faces_prepare();string material83Shader=fb_surface_material_shader(argv(2),1);
        for(int material83Index=0;material83Index<fb_render_face_count;material83Index++)if(fb_render_faces[material83Index].shader==material83Shader){
            fb_render_face_t *material83Face=&fb_render_faces[material83Index];fb_node_t material83Node=fb_nodes[material83Face.node];static brushface_t material83Attrs[64];brush_get(material83Node.model,material83Node.id,material83Attrs,64,__NULL__);
            brushface_t material83Attr=material83Attrs[material83Index-fb_render_first[material83Face.node]+1];vector material83Size=drawgetimagesize(material83Shader),material83Point=material83Face.points[0];vector material83Expected=[(material83Point*material83Attr.sdir+material83Attr.sbias)/max(1,material83Size_x),(material83Point*material83Attr.tdir+material83Attr.tbias)/max(1,material83Size_y),0];
            print(sprintf("RB_MATERIAL83 shader=%s size=%v uv=%v expected=%v node=%i model=%i id=%i\n",material83Shader,material83Size,material83Face.uv[0],material83Expected,material83Face.node,material83Node.model,material83Node.id));
            if(argv(3)==""){
                vector material83Pixel=(vector)getproperty(VF_MIN)+(vector)getproperty(VF_SIZE)*.5;material83Pixel_z=0;fb_pick(material83Pixel);
                for(int material83HitIndex=0;material83HitIndex<min(8,fb_hit_count);material83HitIndex++){fb_hit_t *material83Hit=&fb_hits[material83HitIndex];if(material83Hit.kind!=FB_HIT_BRUSH)continue;static brushface_t material83HitFaces[64];brush_get(material83Hit.model,material83Hit.id,material83HitFaces,64,__NULL__);print(sprintf("RB_MATERIAL83_HIT model=%i id=%i face=%i distance=%g point=%v shader=%s\n",material83Hit.model,material83Hit.id,material83Hit.face,material83Hit.distance,material83Hit.point,material83HitFaces[bound(0,material83Hit.face-1,63)].shadername));}
            }
            if(argv(3)=="focus"){vector material83Eye=material83Face.center+material83Face.normal*96,material83Angles=fb_camera_angles(-material83Face.normal);cvar_set("ftebroom_test_camera_lock",vtos(material83Eye));localcmd(sprintf("ftebroom_camera_restore %g %g %g %g %g 0 90\n",material83Eye_x,material83Eye_y,material83Eye_z,material83Angles_x,material83Angles_y));}
            if(argv(3)=="select"){fb_selection_clear();brush_select(material83Node.model,material83Node.id);}return TRUE;
        }
        print(strcat("RB_MATERIAL83_NOT_FOUND ",material83Shader,"\n"));return TRUE;
    }
    if(argv(1)=="model84"){
        fb_document_tick();fb_cancel();fb_tool_deactivate();fb_selection_clear();int model84Id=-1;
        fb_document_lists_prepare();for(int i=0;i<fb_point_node_count;i++){int candidate84=fb_nodes[fb_point_nodes[i]].id;if(fb_entity_has_model(candidate84)){model84Id=candidate84;break;}}
        fb_transform_expect(model84Id>=0,"model84_fixture_has_model");if(model84Id<0)return TRUE;
        string original84=strzone(terrain_edit(TEREDIT_ENT_GET,model84Id));vector origin84=fb_entity_origin(model84Id);
        fb_entity_select(model84Id,TRUE);fb_transform_expect(fb_transform_capture(),"model84_capture");
        fb_transform_delta='32 -16 8';fb_transform_preview=TRUE;cvar_set("ftebroom_show_models","1");
        int draws84=fb_entity_instance_draws;fb_transform_draw();
        fb_transform_expect(fb_entity_instance_draws==draws84+1 && vlen(fb_entity_instance_origin-origin84-fb_transform_delta)<.001,"model84_drag_draws_translated_model");
        fb_transform_expect(original84==terrain_edit(TEREDIT_ENT_GET,model84Id),"model84_preview_preserves_properties");
        fb_transform_clear();fb_selection_clear();strunzone(original84);return TRUE;
    }
    if(argv(1)=="layout83"){
        fb_document_tick();fb_surfacegrid(TRUE);fb_render_faces_prepare();fb_selection_clear();fb_createbox('0 0 160','64 64 64');fb_document_tick();
        int layout83Node=fb_document_brush_node(selectedbrushes[0].model,selectedbrushes[0].id);fb_incremental_brush_t *layout83Entry=fb_incremental_build(layout83Node);
        print(sprintf("RB_LAYOUT83 variant=%i word=%i header=%i node=%i grid=%i faces=%i\n",sizeof(__variant),sizeof(int),sizeof(fb_incremental_brush_t),layout83Entry->node,layout83Entry->gridCount,layout83Entry->faceCount));
        fb_transform_expect(layout83Entry->node==layout83Node && layout83Entry->gridCount==6 && layout83Entry->faceCount==6,"layout83_header_and_arrays");
        fb_transform_expect((int*)layout83Entry->grid==(int*)layout83Entry+sizeof(fb_incremental_brush_t)/sizeof(int),"layout83_exact_header_offset");
        fb_transform_expect(layout83Entry->grid[0].id==selectedbrushes[0].id && layout83Entry->triangles[0]==0,"layout83_geometry_not_header_alias");memfree(layout83Entry);return TRUE;
    }
    if(argv(1)=="startup83"){
        fb_document_tick();fb_tool_deactivate();fb_selection_clear();int startup83Count=0;for(int startup83N=2;startup83N<fb_node_count;startup83N++)if(fb_nodes[startup83N].kind==FB_NODE_BRUSH)startup83Count++;
        cvar_set("ftebroom_tool","1");vector startup83Pixel=(vector)getproperty(VF_MIN)+(vector)getproperty(VF_SIZE)*.5;startup83Pixel_z=0;fb_begin(startup83Pixel,0,0);print(sprintf("RB_STARTUP83 owner=%i active=%g kind=%i nodes=%i viewport=%v\n",fb_owner,fb_active,fb_pick_kind,fb_node_count,(vector)getproperty(VF_SIZE)));fb_update(startup83Pixel+'48 32 0',0,0);print(sprintf("RB_STARTUP83_SHAPE size=%v anchor=%v offset=%v eye=%v shape=%s model=%i\n",fb_boxsize,fb_anchor,fb_offset,(vector)getproperty(VF_ORIGIN),autocvar_ftebroom_shape,fb_active_layer_model()));fb_finish(startup83Pixel+'48 32 0',0,0);fb_document_tick();
        int startup83After=0;for(int startup83J=2;startup83J<fb_node_count;startup83J++)if(fb_nodes[startup83J].kind==FB_NODE_BRUSH)startup83After++;fb_transform_expect(startup83After==startup83Count+1,"startup83_mouse_creation");print(strcat("RB_STARTUP83_STATUS ",cvar_string("ftebroom_selection_status")," / ",fb_geom_error,"\n"));
        brush_undo();fb_document_tick();brush_redo();fb_document_tick();fb_entity_publish();fb_transform_expect(fb_node_count>0,"startup83_undo_redo_inspector");return TRUE;
    }
    if(argv(1)=="state82"){
        fb_static_preview_prepare();print(sprintf("RB_VERTEX82_DELTA delta=%v drag=%i mode=%i\n",fb_vertex_delta,fb_vertex_drag,fb_perf80_mode));int live=0,base=0,masked=0;
        for(int n=2;n<fb_render_access_count;n++){if(fb_render_access[n]&1)live++;if(fb_static_access[n]&1)base++;if((fb_render_access[n]&1)&&!(fb_static_access[n]&1))masked++;}
        print(sprintf("RB_STATE82 nodes=%i selected=%i vertexBrushes=%i vertexValid=%i faces=%i grid=%i\n",fb_node_count,selectedbrushcount,fb_vertex_brushcount,fb_vertex_preview_valid,fb_render_face_count,fb_grid_face_count));
        print(sprintf("RB_ACCESS82 live=%i base=%i masked=%i active=%g tool=%g native=%s\n",live,base,masked,fb_active,cvar("ftebroom_tool"),cvar_string("ftebroom_native_faces_drawn")));return TRUE;
    }
    if(argv(1)=="cube82"||argv(1)=="cube82drag"){
        fb_cancel();fb_tool_deactivate();fb_selection_clear();fb_createbox('-6500 0 128','128 128 128');fb_document_tick();
        fb_perf80_model=1;fb_perf80_id=selectedbrushes[0].id;fb_perf80_start(3);fb_perf80_mode=0;
        fb_vertex_deselect();fb_vertex_handles[0].selected=TRUE;fb_vertex_drag=1;fb_vertex_anchor=fb_vertex_handles[0].point;
        int cube82Valid=fb_vertex_build('0 0 8',FALSE,0);print(sprintf("RB_CUBE82 valid=%i brushes=%i handles=%i\n",cube82Valid,fb_vertex_brushcount,fb_vertex_count));print(strcat("RB_CUBE82_ERROR ",fb_geom_error,"\n"));if(argv(1)=="cube82drag"&&cube82Valid){fb_perf80_pixel=project(fb_vertex_anchor);fb_perf80_pixel_z=0;fb_vertex_pixel=fb_perf80_pixel;fb_perf80_since=gettime(0);fb_perf80_mode=3;}return TRUE;
    }
    if(argv(1)=="vertex82"){fb_perf80_start(3);fb_perf80_mode=0;int vertex82Valid=fb_vertex_build('0 0 8',FALSE,0);print(sprintf("RB_VERTEX82 valid=%i\n",vertex82Valid));return TRUE;}
    if(argv(1)=="state81"){print(sprintf("RB_STATE81 now=%g start=%g frames=%g benchmark=%g\n",gettime(0),fb_benchmark_start,fb_benchmark_frames,cvar("ftebroom_benchmark")));print(sprintf("RB_VIEW81 show=%g mod=%g draw=%g tool=%g owner=%i active=%g\n",cvar("ca_show"),cvar("ftebroom_mod_preview"),getproperty(VF_DRAWWORLD),cvar("ftebroom_tool"),fb_owner,fb_active));return TRUE;}
    if(argv(1)=="incremental81stress"){fb_perf81_stress();return TRUE;}
    if(argv(1)=="incremental81finish"){fb_perf81_finish();return TRUE;}
    if(argv(1)=="incremental81check"){fb_perf81_validate();return TRUE;}
    if(argv(1)=="perf80"){fb_perf80_start(stoi(argv(2)));return TRUE;}
    if(argv(1)=="document80"){fb_perf80_document();return TRUE;}
    if(argv(1)=="spike80"){fb_perf80_spikes();return TRUE;}

    if(argv(1)=="create78undo"){brush_undo();return TRUE;}
    if(argv(1)=="create78redo"){brush_redo();return TRUE;}
    if(argv(1)=="create78delete"){fb_delete_selection();return TRUE;}
    if(argv(1)=="create78check"){
        fb_document_tick();int create78Count=0;for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH)create78Count++;
        fb_transform_expect(create78Count==stoi(argv(2)),strcat("create78_brush_count_",argv(2)));
        print(sprintf("RB_CREATE78 capacity=%i brushes=%i selected=%i\n",fb_grid_vertex_capacity,create78Count,selectedbrushcount));return TRUE;
    }
    if(argv(1)=="selection78"){
        fb_document_tick();int differences=0;for(int n=0;n<fb_node_count;n++)if(fb_selection_selected(n)!=fb_selection_selected_uncached(n))differences++;
        fb_transform_expect(differences==0,"selection78_matches_reference");
        fb_entity_links_prepare();int edge=0;
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY){
            string target=fb_nodes[n].fields["target"],kill=fb_nodes[n].fields["killtarget"];
            for(int other=2;other<fb_node_count;other++)if(other!=n && fb_nodes[other].kind==FB_NODE_ENTITY && fb_nodes[other].fields["targetname"]!="" && (fb_nodes[other].fields["targetname"]==target || fb_nodes[other].fields["targetname"]==kill)) {
                if(edge>=fb_entity_connection_count){differences++;continue;}
                if(fb_entity_connections[edge].from!=n || fb_entity_connections[edge].to!=other)differences++;edge++;
            }
        }
        fb_transform_expect(differences==0 && edge==fb_entity_connection_count,"links78_matches_reference_order");return TRUE;
    }
    if(argv(1)=="create78begin"){
        brush_deselectall();fb_restore_entity_selection(-1);cvar_set("ftebroom_tool","1");
        vector pixel=project('-3500 -3500 1408');pixel_z=0;fb_begin(pixel,0,0);
        print(sprintf("RB_CREATE78 begin owner=%i active=%g pixel=%v\n",fb_owner,fb_active,pixel));return TRUE;
    }
    if(argv(1)=="create78update"){
        fb_update(fb_downpixel+'60 45 0',0,0);print(sprintf("RB_CREATE78 update owner=%i active=%g size=%v\n",fb_owner,fb_active,fb_boxsize));return TRUE;
    }
    if(argv(1)=="create78finish"){
        float confirm81Time=gettime(1);fb_finish(fb_downpixel+'60 45 0',0,0);print(sprintf("RB_CREATE78 finish ms=%g\n",(gettime(1)-confirm81Time)*1000));return TRUE;
    }
    if(argv(1)=="cam77"){print(sprintf("RB_CAMERA77 eye=%v angles=%v viewport=%v %v\n",fb_camera_eye,(vector)getproperty(VF_ANGLES),(vector)getproperty(VF_MIN),(vector)getproperty(VF_SIZE)));return TRUE;}
    if(argv(1)=="bench76"){int brushes=0,patches=0,entities=0;for(int node=0;node<fb_node_count;node++){if(fb_nodes[node].kind==FB_NODE_BRUSH)brushes++;if(fb_nodes[node].kind==FB_NODE_PATCH)patches++;if(fb_nodes[node].fields)entities++;}print(sprintf("RB_MAP_COUNTS brushes=%i patches=%i entities=%i nodes=%i\n",brushes,patches,entities,fb_node_count));if(checkbuiltin(ftebroom_cached_mesh_stats))print(sprintf("RB_GPU_CACHES %v enabled=%g faces=%s\n",ftebroom_cached_mesh_stats(),cvar("ftebroom_cached_meshes"),cvar_string("ftebroom_native_faces_drawn")));print(sprintf("RB_BENCH_STATE time=%g start=%g frames=%g enabled=%g labels=%s\n",gettime(0),fb_benchmark_start,fb_benchmark_frames,cvar("ftebroom_benchmark"),cvar_string("ftebroom_label_stats")));return TRUE;}
    if(argv(1)=="frame76"){
        fb_document_tick();static vector allBounds[2];
        if(fb_container_bounds(0,allBounds)){
            vector center=(allBounds[0]+allBounds[1])*.5;
            float radius=vlen(allBounds[1]-allBounds[0])*.5;
            vector eye=center+normalize('-1 -1 0.65')*radius*1.6;
            vector direction=center-eye,angles='0 0 0';
            angles_x=atan2(-direction_z,vlen([direction_x,direction_y,0]))*180/M_PI;angles_y=atan2(direction_y,direction_x)*180/M_PI;
            print(sprintf("FTEBROOM_FULLMAP_BOUNDS %v %v\n",allBounds[0],allBounds[1]));
            localcmd(sprintf("ftebroom_camera_restore %g %g %g %g %g 0 90\n",eye_x,eye_y,eye_z,angles_x,angles_y));
        }
        return TRUE;
    }
    if(argv(1)=="labels75"){
        int mismatches=0,hitsBefore=fb_label_native_traces;float savedOcclusion=cvar("ftebroom_occlusion_culling");cvar_set("ftebroom_occlusion_culling","0");fb_prepare_occlusion();
        cvar_set("ftebroom_native_labels","1");
        fb_point_render_prepare();
        for(int i=0;i<2048;i++){
            float a=(float)i*.017;
            vector target='-448 288 8',start=target+[cos(a)*1024,sin(a)*1024,128];
            if(i&1i)target_z+=1024;
            if(fb_point_render_count>0)if(i&2i){int sample=i*31i;while(sample>=fb_point_render_count)sample-=fb_point_render_count;target=fb_point_render[sample].label;}
            if((i%8)==0)start='-448 288 8';
            int actual=fb_label_visible(start,target,1);
            traceline(start,target,TRUE,world);
            if(actual!=(trace_fraction>=.999))mismatches++;
        }
        fb_transform_expect(mismatches==0,"labels75_matches_full_traces");
        fb_transform_expect(fb_label_native_traces>hitsBefore,"labels75_native_path_exercised");cvar_set("ftebroom_occlusion_culling",ftos(savedOcclusion));
return TRUE;
    }
    if(argv(1)=="map75"){
        fb_document_tick();int mapBrushes=0;
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH)mapBrushes++;
        print(sprintf("FTEBROOM_MAP_COUNTS %i brushes, %i entities\n",mapBrushes,terrain_edit(TEREDIT_ENT_COUNT)));
        int *counts=memalloc(fb_node_count*sizeof(int));
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH)counts[fb_nodes[n].parent]++;
        filestream mapStats=fopen("map-counts.txt",FILE_WRITE);
        for(int n=0;n<fb_node_count;n++)if(fb_nodes[n].kind<FB_NODE_BRUSH && fb_nodes[n].kind!=FB_NODE_LAYER)fputs(mapStats,sprintf("%i %i\n",fb_nodes[n].id,n?counts[n]:counts[1]));
        fclose(mapStats);memfree(counts);
        fb_transform_expect(mapBrushes==(int)cvar("ftebroom_test_map_brushes"),"map75_all_brushes_loaded");
        fb_transform_expect(terrain_edit(TEREDIT_ENT_COUNT)==(int)cvar("ftebroom_test_map_entities"),"map75_all_entities_loaded");
        fb_transform_expect(cvar("ftebroom_large_map_ready")==1,"map75_cache_warmup_finished");return TRUE;
    }
    if(argv(1)=="large74"){
        fb_document_tick();fb_document_lists_prepare();int largeBrushes=0,largeLast=-1;
        for(int largeNode=2;largeNode<fb_node_count;largeNode++)if(fb_nodes[largeNode].kind==FB_NODE_BRUSH){largeBrushes++;largeLast=largeNode;}
        fb_transform_expect(largeBrushes==(int)cvar("ftebroom_test_large_count"),"large74_loaded_brush_count");
        fb_transform_expect(fb_line_count==largeBrushes+2,"large74_linear_source_line_index");
        fb_transform_expect(fb_point_node_count==1&&fb_patch_node_count==0,"large74_typed_node_lists");
        fb_pick_query('32 -64 32','32 96 32','0 0 0',0,FB_HIT_BRUSH,FALSE);
        fb_transform_expect(fb_hit_count==1&&vlen(fb_hits[0].point-'32 0 32')<.01,"large74_exact_pick");
        fb_selection_clear();int largeModel=fb_nodes[largeLast].model,largeId=fb_nodes[largeLast].id;brush_select(largeModel,largeId);fb_visual_candidates_prepare();
        int largeFound=FALSE;for(int largeC=0;largeC<fb_visual_candidate_count;largeC++)if(fb_visual_candidates[largeC]==largeLast)largeFound=TRUE;
        fb_transform_expect(largeFound,"large74_selection_cache_invalidation");
        static vector largeBounds[2];brush_getfacepoints(largeModel,largeId,0,largeBounds,2);vector largeBefore=largeBounds[0];
        static brushface_t largeFaces[64];int largeContents,largeFacesCount=brush_get(largeModel,largeId,largeFaces,64,&largeContents);
        for(int largeF=0;largeF<largeFacesCount;largeF++)largeFaces[largeF].planedist+=largeFaces[largeF].planenormal[0]*16;
        fb_tx_begin("Large map cache test");brush_history_edit(largeModel,largeId,largeFaces,largeFacesCount,largeContents);fb_tx_commit();fb_document_tick();
        brush_getfacepoints(largeModel,selectedbrushes[0].id,0,largeBounds,2);fb_transform_expect(vlen(largeBounds[0]-largeBefore-'16 0 0')<.01,"large74_edited_id_lookup");
        brush_undo();fb_document_tick();brush_getfacepoints(largeModel,selectedbrushes[0].id,0,largeBounds,2);fb_transform_expect(vlen(largeBounds[0]-largeBefore)<.01,"large74_undo_lookup");
        brush_redo();brush_undo();fb_document_tick();fb_selection_clear();fb_visual_candidates_prepare();fb_transform_expect(fb_visual_candidate_count==0,"large74_deselection_cache_invalidation");return TRUE;
    }
    if(argv(1)=="rubble72setup"){
        fb_cancel();fb_selection_clear();fb_createbox('0 0 -16','2048 2048 32');fb_selection_clear();
        if(cvar("ftebroom_test_rubble_horizontal"))fb_createbox('0 0 256','512 512 16');else fb_createbox('0 0 160','256 32 256');
        fb_generators_test_history=history;fb_document_tick();return TRUE;
    }
    if(argv(1)=="rubble72check"){
        fb_transform_expect(cvar("ftebroom_plugin_status")==1&&cvar("ftebroom_settle_applied")==1&&selectedbrushcount==12,"rubble72_pipeline_complete");
        fb_document_tick();int rubbleVisible=0;
        for(int rubbleS=0;rubbleS<selectedbrushcount;rubbleS++)for(int rubbleN=2;rubbleN<fb_node_count;rubbleN++)if(fb_nodes[rubbleN].kind==FB_NODE_BRUSH&&fb_nodes[rubbleN].model==selectedbrushes[rubbleS].model&&fb_nodes[rubbleN].id==selectedbrushes[rubbleS].id&&fb_render_allowed(rubbleN,0))rubbleVisible++;
        fb_transform_expect(rubbleVisible==12,"rubble73_fragments_in_render_tree");
        int rubbleGroup=fb_organize_node(cvar("ftebroom_plugin_result_group")),rubbleChildren=0;
        if(rubbleGroup>=2&&fb_nodes[rubbleGroup].kind==FB_NODE_GROUP)for(int rubbleChild=2;rubbleChild<fb_node_count;rubbleChild++)if(fb_nodes[rubbleChild].parent==rubbleGroup&&fb_nodes[rubbleChild].kind==FB_NODE_ENTITY)rubbleChildren++;
        fb_transform_expect(rubbleChildren==3,"rubble73_grouped_plug_entities");
        if(rubbleVisible!=12)print(sprintf("RUBBLE_DEBUG visible=%i nodes=%i entities=%i\n",rubbleVisible,fb_node_count,terrain_edit(TEREDIT_ENT_COUNT)));
        fb_transform_expect(cvar("ftebroom_settle_moving")==0,"rubble73_no_flying_result");
        fb_transform_expect(cvar("ftebroom_settle_dropped")==12,"rubble73_drop_diagnostics");
        if(cvar("ftebroom_test_rubble_horizontal"))fb_transform_expect(cvar("ftebroom_plugin_actual_axis")==2&&cvar("ftebroom_plugin_release_negative")==1,"rubble73_horizontal_downward_release");
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
    if(argv(1)=="rubble73guard"){
        fb_selection_clear();fb_createbox('616 0 128','96 96 16');fb_document_tick();int guardHistory=history;
        fb_plugin_selection_command();brush_deselectall();
        tokenize("ftebroom_plugin_batch guard perforate fracture release");fb_plugin_batch_command();
        fb_transform_expect(cvar("ftebroom_plugin_status")==-2&&history==guardHistory,"rubble73_stale_selection_rejected");
        brush_undo();fb_createbox('616 0 128','96 96 16');fb_document_tick();guardHistory=history;fb_plugin_selection_command();
        cvar_set("ftebroom_hole_axis","2");cvar_set("ftebroom_hole_count","2");cvar_set("ftebroom_hole_sides","6");cvar_set("ftebroom_hole_min","24");cvar_set("ftebroom_hole_max","48");cvar_set("ftebroom_hole_spacing","8");cvar_set("ftebroom_hole_keep","1");
        cvar_set("ftebroom_plugin_hole_layout","596 0 12 636 0 12");cvar_set("ftebroom_fracture_seed","1337");
        int layoutAtomic=TRUE;
        for(int layoutSeed=0;layoutSeed<16;layoutSeed++){
            cvar_set("ftebroom_fracture_seed",itos(1337+layoutSeed*104729));tokenize("ftebroom_plugin_batch guard perforate fracture release");fb_plugin_batch_command();
            if(cvar("ftebroom_plugin_status")==1)break;
            if(history!=guardHistory||selectedbrushcount!=1)layoutAtomic=FALSE;
        }
        cvar_set("ftebroom_plugin_hole_layout","");fb_transform_expect(layoutAtomic,"rubble73_layout_retries_atomic");
        if(cvar("ftebroom_plugin_status")!=1)print(strcat("RUBBLE_LAYOUT_ERROR ",cvar_string("ftebroom_plugin_error"),"\n"));
        fb_transform_expect(cvar("ftebroom_plugin_status")==1&&selectedbrushcount==8,"rubble73_certified_layout_real_geometry");
        brush_undo();fb_transform_expect(history==guardHistory&&selectedbrushcount==1,"rubble73_certified_layout_undo");return TRUE;
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
