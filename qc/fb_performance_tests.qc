// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic cursor motion drives the production gesture handlers every frame.
int fb_perf80_mode,fb_perf80_model,fb_perf80_id;vector fb_perf80_pixel;float fb_perf80_since;
void fb_perf80_tree(string path)
{
    filestream file=fopen(path,FILE_WRITE);if((float)file<0)return;
    for(int n=0;n<fb_node_count;n++){
        fb_node_t *node=&fb_nodes[n];
        fputs(file,sprintf("%i %i %i %i %i %i ",node->kind,node->model,node->id,node->parent,node->layer,node->flags));fputs(file,strcat(fb_map_quote(node->name),"\n"));
        if(node->fields)for(int key=0;;key++){string name=hash_getkey(node->fields,key);if isnull(name)break;fputs(file,strcat(fb_map_quote(name)," ",fb_map_quote(node->fields[name]),"\n"));}
    }
    fclose(file);
}
void fb_perf80_document()
{
    fb_document_tick();fb_perf80_tree("data/tree80-fast.txt");
    fb_document_rebuild();fb_perf80_tree("data/tree80-full.txt");
    print("RB_DOCUMENT80 snapshots ready\n");
}
void fb_perf80_spikes()
{
    static int step,checked,hits;static vector bounds[2];
    int n=2+(int)(step/6)*max(1,(int)(fb_node_count/10));
    while(n<fb_node_count && fb_nodes[n].kind!=FB_NODE_BRUSH)n++;
    if(n<fb_node_count && brush_getfacepoints(fb_nodes[n].model,fb_nodes[n].id,0,bounds,2)==2){
        int axis=(int)(step%6)/2,side=step%2;
        vector start=(bounds[0]+bounds[1])*.5,end=start;start[axis]=bounds[side][axis];end=start;end[axis]+=side?256:-256;
        vector reference=fb_visual_spike_trace(start,end,TRUE);int referenceHit=fb_visual_spike_hit;
        vector fast=fb_visual_spike_trace(start,end);int fastHit=fb_visual_spike_hit;
        fb_transform_expect(vlen(reference-fast)<.01 && referenceHit==fastHit,"spike80_exact_candidate_trace");checked++;hits+=referenceHit;
    }
    step++;if(step<48)localcmd("in .04 ftebroom_test_generators spike80\n");
    else {print(sprintf("RB_SPIKE80 checked=%i hits=%i\n",checked,hits));step=checked=hits=0;}
}
void fb_perf80_pointer()
{
    if(!cvar("ftebroom_skip_setup") || !fb_perf80_mode)return;
    float elapsed=gettime(0)-fb_perf80_since;vector pixel=fb_perf80_pixel+[40+sin(elapsed*1.3)*24,24+cos(elapsed*.9)*16,0];
    cvar_set("ftebroom_mouse_x",ftos(pixel_x));cvar_set("ftebroom_mouse_y",ftos(pixel_y));
}
void fb_perf80_start(int mode)
{
    if(fb_perf80_id && selectedbrushcount && brush_get(fb_perf80_model,fb_perf80_id,__NULL__,0,__NULL__)<=0){fb_perf80_model=selectedbrushes[0].model;fb_perf80_id=selectedbrushes[0].id;}
    fb_cancel();fb_perf80_mode=0;fb_tool_deactivate();fb_selection_clear();
    if(!mode)return;
    cvar_set("ca_brush_view","0");cvar_set("ftebroom_shift","0");cvar_set("ftebroom_alt","0");cvar_set("ftebroom_ctrl","0");
    if(mode==1){cvar_set("ftebroom_tool","0");fb_perf80_pixel=project('-3500 -3500 1408');fb_perf80_pixel_z=0;fb_begin(fb_perf80_pixel,0,0);fb_transform_expect(fb_owner==FB_OWNER_SHAPE,"perf80_create_owner");}
    else {
        if(!fb_perf80_id){static vector b[2];float score=1000000000;for(int node=2;node<fb_node_count;node++)if(fb_nodes[node].kind==FB_NODE_BRUSH && fb_nodes[node].model==1){
            if(brush_get(1,fb_nodes[node].id,__NULL__,0,__NULL__)!=6)continue;
            if(brush_getfacepoints(1,fb_nodes[node].id,0,b,2)!=2)continue;
            if(vlen(b[1]-b[0])>512)continue;vector center=(b[0]+b[1])*.5;float distance=vlen(center-'-3000 -3000 1000');
            if(distance<score){score=distance;fb_perf80_model=1;fb_perf80_id=fb_nodes[node].id;fb_perf80_pixel=project(center);}
        }print(sprintf("RB_PERF80_BRUSH model=%i id=%i\n",fb_perf80_model,fb_perf80_id));}
        brush_select(fb_perf80_model,fb_perf80_id);
        static vector bounds[2];brush_getfacepoints(fb_perf80_model,fb_perf80_id,0,bounds,2);fb_perf80_pixel=project((bounds[0]+bounds[1])*.5);fb_perf80_pixel_z=0;
        if(mode==2){cvar_set("ftebroom_tool","0");fb_begin(fb_perf80_pixel,0,0);fb_transform_expect(fb_moving,"perf80_move_owner");}
        else {cvar_set("ftebroom_tool",itos(FB_TOOL_VERTEX));fb_vertex_rebuild();fb_perf80_pixel=project(fb_vertex_handles[0].point);fb_perf80_pixel_z=0;fb_vertex_down(fb_perf80_pixel,0,0);fb_transform_expect(fb_vertex_drag,"perf80_vertex_owner");}
    }
    fb_perf80_mode=mode;fb_perf80_since=gettime(0);
}

void fb_perf81_finish()
{
    int mode=fb_perf80_mode,epoch=fb_grid_geometry_epoch;float began=gettime(1);
    vector pixel=[cvar("ftebroom_mouse_x"),cvar("ftebroom_mouse_y"),0];
    int before=fb_document_revision();
    if(mode==3){
        fb_vertex_up(pixel,0,0);
        if(before==fb_document_revision()){
            fb_vertex_rebuild();fb_vertex_deselect();if(fb_vertex_count)fb_vertex_handles[0].selected=TRUE;
            int valid=fb_vertex_build('0 0 8',FALSE,0);fb_transform_expect(valid,"incremental81_vertex_preview");if(valid)fb_vertex_commit("Move Geometry");
        }
    }else fb_finish(pixel,0,0);
    fb_transform_expect(before!=fb_document_revision(),"incremental81_gesture_committed");
    fb_perf80_mode=0;fb_document_tick();
    if(mode!=1 && selectedbrushcount){fb_perf80_model=selectedbrushes[0].model;fb_perf80_id=selectedbrushes[0].id;}
    fb_transform_expect(epoch==fb_grid_geometry_epoch,"incremental81_keeps_base_geometry");
    print(sprintf("RB_INCREMENTAL81 mode=%i confirm_ms=%g edited=%i epoch=%i revision=%i\n",mode,(gettime(1)-began)*1000,fb_incremental_brush_count,fb_grid_geometry_epoch,fb_document_revision()));
}
void fb_perf81_validate()
{
    fb_document_tick();int brushes=0,dead=0;hashtable counts=hash_createtab(512,EV_INTEGER);static brushface_t faces[128];
    for(int n=0;n<fb_node_count;n++){
        if(fb_nodes[n].kind<0){dead++;continue;}
        if(fb_nodes[n].kind!=FB_NODE_BRUSH)continue;brushes++;
        int total=brush_get(fb_nodes[n].model,fb_nodes[n].id,faces,128,__NULL__);
        for(int f=0;f<min(128,total);f++){string name=fb_material_key(faces[f].shadername);counts[name]=(int)counts[name]+1i;}
    }
    int differences=0;for(int i=0;;i++){string name=hash_getkey(counts,i);if isnull(name)break;if((int)counts[name]!=(int)fb_material_counts[name]){differences++;if(differences<8)print(sprintf("RB_MATERIAL81 name=%s reference=%i retained=%i\n",name,(int)counts[name],(int)fb_material_counts[name]));}}
    for(int i=0;;i++){string name=hash_getkey(fb_material_counts,i);if isnull(name)break;if((int)counts[name]!=(int)fb_material_counts[name]){differences++;if(differences<8)print(sprintf("RB_MATERIAL81 name=%s reference=%i retained=%i\n",name,(int)counts[name],(int)fb_material_counts[name]));}}
    fb_visual_candidates_prepare();int visual=0,visualErrors=0;
    for(int n=2;n<fb_node_count;n++){
        if(!fb_selection_leaf(n)||!fb_render_allowed(n,FB_PICK_LOCKED))continue;
        int selected=fb_nodes[n].kind==FB_NODE_ENTITY?fb_entity_selected(fb_nodes[n].id):brush_isselected(fb_nodes[n].model,fb_nodes[n].id);
        if(!selected && !fb_visual_locked(n) && !(autocvar_ftebroom_hover_highlight&&fb_visual_hover>=0&&fb_selection_descendant(n,fb_visual_hover)))continue;
        if(visual>=fb_visual_candidate_count || fb_visual_candidates[visual]!=n)visualErrors++;visual++;
    }
    fb_transform_expect(!visualErrors&&visual==fb_visual_candidate_count,"incremental81_overlay_candidates_match_full_scan");
    fb_transform_expect(differences==0,"incremental81_material_counts_match_full_scan");hash_destroytab(counts);
    print(sprintf("RB_INCREMENTAL81_CHECK brushes=%i dead=%i edited=%i\n",brushes,dead,fb_incremental_brush_count));
}

void fb_perf81_stress()
{
    static int step,epoch,ids[16],savedModel,savedId;
    if(!step){epoch=fb_grid_geometry_epoch;savedModel=fb_perf80_model;savedId=fb_perf80_id;fb_cancel();fb_tool_deactivate();}
    if(step<16){
        fb_tx_lasttime=-100;int id=fb_createbox([10000+step*32,10000,64],'16 16 16');fb_document_tick();
        brush_undo();fb_document_tick();brush_redo();fb_document_tick();
        fb_transform_expect(id && selectedbrushcount==1 && epoch==fb_grid_geometry_epoch,"incremental81_repeated_create_undo_redo");
        if(selectedbrushcount)ids[step]=selectedbrushes[0].id;step++;localcmd("in .05 ftebroom_test_generators incremental81stress\n");return;
    }
    brush_deselectall();for(int i=0;i<16;i++)brush_select(1,ids[i]);fb_delete_selection();fb_document_tick();
    fb_transform_expect(epoch==fb_grid_geometry_epoch,"incremental81_batch_delete_keeps_base");
    brush_select(savedModel,savedId);step=0;fb_perf81_validate();
}
