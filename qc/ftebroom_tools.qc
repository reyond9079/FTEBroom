int fb_modal_handle_at(vector pixel,int view);
void fb_array_clear();
int fb_selection_target(int node);
void fb_selection_click(int node,int ctrl);
int fb_spin_capture();
void fb_spin_clear();
void fb_spin_cancel_drag();
void fb_spin_down(vector pixel,int alt,int view);
void fb_spin_move(vector pixel,int alt,int view);
void fb_spin_up(vector pixel,int alt,int view);
void fb_spin_draw();
int fb_spin_tool_apply();
int fb_polygon_capture(int spawnMode);
void fb_polygon_clear();
void fb_polygon_cancel_drag();
void fb_polygon_down(vector pixel,int view);
void fb_polygon_move(vector pixel,int view);
void fb_polygon_up(vector pixel,int view);
void fb_polygon_draw();
int fb_polygon_apply();
int fb_bend_capture();
void fb_bend_clear();
void fb_bend_cancel_drag();
void fb_bend_down(vector pixel,int alt,int view);
void fb_bend_move(vector pixel,int alt,int view);
void fb_bend_up(vector pixel,int alt,int view);
void fb_bend_draw();
int fb_bend_apply();
void fb_sculpt_finish(int apply);
void fb_sculpt_down(vector pixel);
void fb_sculpt_move(vector pixel);
void fb_sculpt_draw();
void fb_rotate_reset();
void fb_rotate_cancel();
void fb_rotate_down(vector pixel,int alt,int view);
void fb_rotate_move(vector pixel,int alt,int view);
void fb_rotate_up(vector pixel,int alt,int view);
void fb_rotate_draw();
int fb_vertex_tool();
int fb_sweep_capture();
void fb_sweep_clear();
void fb_sweep_cancel();
void fb_sweep_down(vector pixel,int alt,int view);
void fb_sweep_move(vector pixel,int alt,int view);
void fb_sweep_up(vector pixel,int alt,int view);
void fb_sweep_draw();
int fb_sweep_escape();
void fb_vertex_rebuild();
void fb_vertex_clear();
void fb_vertex_deselect();
void fb_vertex_down(vector pixel,int alt,int view);
void fb_vertex_move(vector pixel,int alt,int view);
void fb_vertex_up(vector pixel,int alt,int view);
void fb_vertex_draw();
int fb_vertex_selection_count();
int fb_vertex_scope,fb_vertex_preserve;
int fb_moveface_active;
void fb_moveface_move(vector pixel,int view);
void fb_moveface_up(vector pixel,int view);
int fb_scale_tool();
void fb_scale_down(vector pixel,int alt,int view);
void fb_scale_move(vector pixel,int alt,int view);
void fb_scale_up(vector pixel,int alt,int view);
void fb_scale_draw();
void fb_assemble_down(vector pixel,int view);
void fb_assemble_move(vector pixel,int view);
void fb_assemble_up(vector pixel,int view);
void fb_assemble_draw();
void fb_assemble_apply();
// SPDX-License-Identifier: GPL-2.0-or-later
// Modal lifecycle and gesture ownership. Geometry tools are implemented in later tasks.
enum : int { FB_TOOL_NONE, FB_TOOL_BRUSH, FB_TOOL_CLIP, FB_TOOL_ROTATE,
    FB_TOOL_SWEEP, FB_TOOL_SCALE, FB_TOOL_SHEAR, FB_TOOL_VERTEX, FB_TOOL_EDGE,
    FB_TOOL_FACE, FB_TOOL_CONTROLPOINT, FB_TOOL_SCULPT, FB_TOOL_BEND, FB_TOOL_POLYGONCUT, FB_TOOL_POLYGONSPAWN, FB_TOOL_SPIN, FB_TOOL_GRIDROTATE };
enum : int { FB_OWNER_NONE, FB_OWNER_MOVE, FB_OWNER_SHAPE, FB_OWNER_SELECTION, FB_OWNER_MODAL, FB_OWNER_MATERIAL, FB_OWNER_EXTRUDE };
int fb_owner, fb_handle_count, fb_shape_count;
float fb_active;
vector fb_clip_points[3];
int fb_clip_count;
void fb_cancel();
void fb_begin(vector pixel,float alt,int view);
void fb_update(vector pixel,float alt,int view);
void fb_finish(vector pixel,float alt,int view);
void fb_clip_pick(vector pixel,int view);
void fb_clip_down(vector pixel,int view);
void fb_clip_move(vector pixel,int view);
void fb_clip_up(vector pixel,int view);
void fb_clip_double(vector pixel,int view);
void fb_clip_cancel_drag();
void fb_clip_apply();
void fb_clip_preview();
void fb_tool_panel_state()
{
    cvar_set("ftebroom_handle_count",itos(fb_handle_count));
    cvar_set("ftebroom_shape_points",itos(fb_shape_count));
    cvar_set("ftebroom_gesture_owner",itos(fb_owner));
}
float fb_tool_can_activate(int tool) { return tool>=FB_TOOL_NONE && tool<=FB_TOOL_GRIDROTATE && !(tool==FB_TOOL_CONTROLPOINT && cvar("ftebroom_quake1")); }
void fb_tool_refresh()
{
    fb_cancel(); fb_owner=FB_OWNER_NONE;
    fb_array_clear();fb_sweep_clear();fb_bend_clear();fb_polygon_clear();fb_spin_clear();
    fb_clip_count=fb_shape_count=fb_handle_count=0;
    fb_pick_clear_handles();
    cvar_set("ftebroom_scale_handles","0");cvar_set("ftebroom_vertex_handles","0");
    if(!fb_vertex_preserve) {fb_vertex_clear();fb_vertex_scope=FALSE;}
    vertedit.selectedvert1=vertedit.selectedvert2=-1;
    fb_tool_panel_state();
}
void fb_tool_deactivate()
{
    // Terrain owns pointer events independently of the brush tool ID.
    // Release that ownership before activating any other tool.
    fb_terrain_finish();
    cvar_set("ftebroom_terrain_paint","0");cvar_set("ftebroom_terrain_edit","0");
    cvar_set("ca_editormode",ftos(MODE_BRUSHEDIT));
    fb_tool_refresh(); cvar_set("ftebroom_tool","0");
}
void fb_tool_activate(int tool)
{
    if(!fb_tool_can_activate(tool)) return;
    int affine=tool==FB_TOOL_ROTATE || tool==FB_TOOL_SCALE || tool==FB_TOOL_SHEAR;
    fb_vertex_preserve=FALSE;
    if(affine) if(fb_vertex_tool() || fb_vertex_scope) if(fb_vertex_selection_count()>0) fb_vertex_preserve=TRUE;
    fb_tool_deactivate(); cvar_set("ftebroom_tool",itos(tool));
    fb_vertex_scope=fb_vertex_preserve;fb_vertex_preserve=FALSE;
    if(fb_vertex_scope) fb_vertex_selection_count();
    // QC boolean expressions may evaluate both operands; gate capture side effects explicitly.
    if(tool==FB_TOOL_SPIN) if(!fb_spin_capture()){cvar_set("ftebroom_tool","0");cvar_set("ftebroom_selection_status",fb_geom_error);}
    if(tool==FB_TOOL_POLYGONCUT||tool==FB_TOOL_POLYGONSPAWN) if(!fb_polygon_capture(tool==FB_TOOL_POLYGONSPAWN)){cvar_set("ftebroom_tool","0");cvar_set("ftebroom_selection_status",fb_geom_error);}
    if(tool==FB_TOOL_BEND) if(!fb_bend_capture()){cvar_set("ftebroom_tool","0");cvar_set("ftebroom_selection_status",fb_geom_error);}
    if(tool==FB_TOOL_ROTATE||tool==FB_TOOL_GRIDROTATE) fb_rotate_reset();
    if(fb_vertex_tool()) fb_vertex_rebuild();
    if(tool==FB_TOOL_SWEEP) if(!fb_sweep_capture()){cvar_set("ftebroom_tool","0");cvar_set("ftebroom_selection_status","Select faces before activating Sweep");}
}
int fb_tool_id(string name)
{
    if(name=="gridrotate")return FB_TOOL_GRIDROTATE;
    if(name=="spin")return FB_TOOL_SPIN;
    if(name=="polygoncut")return FB_TOOL_POLYGONCUT;
    if(name=="polygonspawn")return FB_TOOL_POLYGONSPAWN;
    if(name=="bend") return FB_TOOL_BEND;
    if(name=="sculpt") return FB_TOOL_SCULPT;
    if(name=="default") return FB_TOOL_NONE;
    if(name=="brush") return FB_TOOL_BRUSH;
    if(name=="clip") return FB_TOOL_CLIP;
    if(name=="rotate") return FB_TOOL_ROTATE;
    if(name=="sweep") return FB_TOOL_SWEEP;
    if(name=="scale") return FB_TOOL_SCALE;
    if(name=="shear") return FB_TOOL_SHEAR;
    if(name=="vertex") return FB_TOOL_VERTEX;
    if(name=="edge") return FB_TOOL_EDGE;
    if(name=="face") return FB_TOOL_FACE;
    if(name=="controlpoint") return FB_TOOL_CONTROLPOINT;
    return -1;
}
float fb_tool_cancel()
{
    int tool=cvar("ftebroom_tool");
    if(tool==FB_TOOL_CLIP && fb_clip_count>0) {fb_clip_count--; return TRUE;}
    if(tool==FB_TOOL_SWEEP)return fb_sweep_escape();
    if(tool==FB_TOOL_BRUSH && fb_shape_count>0) {fb_shape_count=0; return TRUE;}
    if(fb_handle_count>0) {fb_vertex_deselect();fb_handle_count=0; vertedit.selectedvert1=vertedit.selectedvert2=-1; return TRUE;}
    return FALSE;
}
void fb_tool_escape()
{
    if(cvar("ftebroom_terrain_edit")){fb_tool_deactivate();return;}
    if(fb_active) {fb_cancel();fb_owner=FB_OWNER_NONE;return;}
    if(cvar("ftebroom_tool")) {
        if(!fb_tool_cancel()) fb_tool_deactivate();
    } else brush_history_deselect();
    fb_tool_panel_state();
}
void fb_tool_pointer_down(vector pixel,float alt,int view)
{
    int tool=cvar("ftebroom_tool");
    fb_owner=FB_OWNER_MODAL;
    if(tool==FB_TOOL_ROTATE || tool==FB_TOOL_SCALE || tool==FB_TOOL_SHEAR || fb_vertex_tool()) {
        if(!fb_modal_handle_at(pixel,view))if(fb_pick_query(unproject(pixel),unproject(pixel+'0 0 8192'),pixel,0,FB_HIT_BRUSH|FB_HIT_PATCH,view!=0)) {
            fb_hit_t hit=fb_hits[0];
            if(!brush_isselected(hit.model,hit.id)) {
                fb_vertex_scope=FALSE;fb_selection_click(fb_selection_target(hit.node),cvar("ftebroom_ctrl"));
                if(fb_vertex_tool())fb_vertex_rebuild();
                if(tool==FB_TOOL_ROTATE)fb_rotate_reset();
                return;
            }
        }
    }
    if(tool==FB_TOOL_SPIN)fb_spin_down(pixel,alt,view);
    else if(tool==FB_TOOL_POLYGONCUT||tool==FB_TOOL_POLYGONSPAWN)fb_polygon_down(pixel,view);
    else if(tool==FB_TOOL_BEND)fb_bend_down(pixel,alt,view);
    else if(tool==FB_TOOL_SCULPT) fb_sculpt_down(pixel);
    else if(tool==FB_TOOL_CLIP) fb_clip_down(pixel,view);
    else if(tool==FB_TOOL_SWEEP) fb_sweep_down(pixel,alt,view);
    else if(fb_vertex_tool()) fb_vertex_down(pixel,alt,view);
    else if(fb_scale_tool()) fb_scale_down(pixel,alt,view);
    else if(tool==FB_TOOL_NONE) fb_begin(pixel,alt,view);
    else if(tool==FB_TOOL_ROTATE||tool==FB_TOOL_GRIDROTATE) fb_rotate_down(pixel,alt,view);
    else if(tool==FB_TOOL_BRUSH) fb_assemble_down(pixel,view);
    else print("ReyondBroom: modal tool framework active; geometry editing follows in its roadmap task.\n");
}
void fb_tool_pointer_move(vector pixel,float alt,int view)
{
    static vector lastPixel,lastOrigin,lastAngles;static int lastTool=-1,lastHistory=-1,lastSelection=-1,lastView=-1,lastAlt=-1,lastCtrl=-1,lastShift=-1;
    int tool=cvar("ftebroom_tool"),ctrl=cvar("ftebroom_ctrl"),shift=cvar("ftebroom_shift");
    vector origin=getproperty(VF_ORIGIN),angles=getproperty(VF_ANGLES);
    if(!fb_active && pixel==lastPixel && origin==lastOrigin && angles==lastAngles && tool==lastTool && history==lastHistory && fb_selection_epoch==lastSelection && view==lastView && alt==lastAlt && ctrl==lastCtrl && shift==lastShift)return;
    lastPixel=pixel;lastOrigin=origin;lastAngles=angles;lastTool=tool;lastHistory=history;lastSelection=fb_selection_epoch;lastView=view;lastAlt=alt;lastCtrl=ctrl;lastShift=shift;

    if(fb_owner==FB_OWNER_SHAPE){fb_update(pixel,alt,view);return;}
    if(cvar("ftebroom_tool")==FB_TOOL_SPIN)fb_spin_move(pixel,alt,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_POLYGONCUT||cvar("ftebroom_tool")==FB_TOOL_POLYGONSPAWN)fb_polygon_move(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_BEND)fb_bend_move(pixel,alt,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_SCULPT)fb_sculpt_move(pixel);
    else if(fb_moveface_active)fb_moveface_move(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_CLIP) fb_clip_move(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_SWEEP) fb_sweep_move(pixel,alt,view);
    else if(fb_vertex_tool()) fb_vertex_move(pixel,alt,view);
    else if(fb_scale_tool()) fb_scale_move(pixel,alt,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_BRUSH) fb_assemble_move(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_ROTATE||cvar("ftebroom_tool")==FB_TOOL_GRIDROTATE) fb_rotate_move(pixel,alt,view);
    else if(fb_owner!=FB_OWNER_NONE) fb_update(pixel,alt,view);
}
void fb_tool_pointer_up(vector pixel,float alt,int view)
{
    if(cvar("ftebroom_tool")==FB_TOOL_SPIN)fb_spin_up(pixel,alt,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_POLYGONCUT||cvar("ftebroom_tool")==FB_TOOL_POLYGONSPAWN)fb_polygon_up(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_BEND)fb_bend_up(pixel,alt,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_SCULPT)fb_sculpt_finish(TRUE);
    else if(fb_moveface_active)fb_moveface_up(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_CLIP) fb_clip_up(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_SWEEP) fb_sweep_up(pixel,alt,view);
    else if(fb_vertex_tool()) fb_vertex_up(pixel,alt,view);
    else if(fb_scale_tool()) fb_scale_up(pixel,alt,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_BRUSH) fb_assemble_up(pixel,view);
    else if(cvar("ftebroom_tool")==FB_TOOL_ROTATE||cvar("ftebroom_tool")==FB_TOOL_GRIDROTATE) fb_rotate_up(pixel,alt,view);
    else if(fb_owner!=FB_OWNER_MODAL) fb_finish(pixel,alt,view);
    fb_owner=FB_OWNER_NONE;
}
void fb_tool_double_click(vector pixel,float alt,int view) { fb_tool_pointer_down(pixel,alt,view); fb_tool_pointer_up(pixel,alt,view); }
void fb_tool_scroll(float delta) { }
void fb_tool_draw() { fb_clip_preview();fb_rotate_draw();fb_vertex_draw();fb_scale_draw();fb_assemble_draw();fb_sweep_draw();fb_sculpt_draw();fb_bend_draw();fb_polygon_draw();fb_spin_draw(); }
void fb_tools_test()
{
    string mode=argv(1);
    if(mode=="points") {
        fb_clip_points[0]='0 -64 64';fb_clip_points[1]='0 64 64';fb_clip_points[2]='0 64 128';fb_clip_count=3;
    } else if(mode=="handles") fb_handle_count=2;
    else if(mode=="state") print(sprintf("FTEBROOM_TOOLS %s tool=%g points=%i handles=%i selected=%i active=%g\n",argv(2),cvar("ftebroom_tool"),fb_clip_count,fb_handle_count,selectedbrushcount,fb_active));
}
void fb_tool_key(string key)
{
    if(cvar("ftebroom_tool")==FB_TOOL_BEND && key=="apply"){fb_bend_apply();return;}
    if(cvar("ftebroom_tool")==FB_TOOL_BRUSH && key=="apply") {fb_assemble_apply();return;}
    if(cvar("ftebroom_tool")!=FB_TOOL_CLIP) return;
    if(key=="apply") fb_clip_apply();
    else if(key=="side") cvar_set("ftebroom_clip_keep",itos((cvar("ftebroom_clip_keep")+1)%3));
    else if(key=="back") {
        if(fb_clip_count>0) fb_tool_cancel();

    }
}
float fb_tools_command()
{
    string cmd=argv(0);
    if(cmd=="ftebroom_test_tools") {
        if(cvar("ftebroom_skip_setup")) fb_tools_test();
    } else if(cmd=="ftebroom_tool_toggle") {
        int tool=fb_tool_id(argv(1));
        if(!fb_tool_can_activate(tool)) print("ReyondBroom: unknown tool.\n");
        else if(tool==cvar("ftebroom_tool")) fb_tool_deactivate();
        else fb_tool_activate(tool);
    } else if(cmd=="ftebroom_tool_deactivate") fb_tool_deactivate();
    else if(cmd=="ftebroom_cancel") fb_tool_escape();
    else if(cmd=="ftebroom_tool_key") fb_tool_key(argv(1));
    else return FALSE;
    return TRUE;
}
