// SPDX-License-Identifier: GPL-2.0-or-later
void fb_entity_expect(int valid,string label) {print(strcat(valid?"FTEBROOM_ENTITY_PASS ":"FTEBROOM_ENTITY_FAIL ",label,"\n"));}
string fb_entity_field(int id,string key)
{
    int count=fb_entity_tokenize(terrain_edit(TEREDIT_ENT_GET,id));for(int i=0;i<count;i+=2)if(argv(i)==key)return argv(i+1);return "";
}
float fb_entity_testcommand()
{
    if(argv(0)=="ftebroom_test_entities"&&argv(1)=="player72"&&cvar("ftebroom_skip_setup")){
        fb_selection_clear();fb_entity_create("info_player_start",FALSE,'-40 -64 96');fb_document_tick();int player72=fb_selected_entity();
        fb_entity_expect(fb_entity_model_name(player72)=="progs/player.mdl"&&fb_entity_model_ready(player72),"player72_loaded_after_resource_mount");
        // Lose a notification deliberately: the persistent epoch must repair the scene.
        cvar_set("ftebroom_entity_assets_epoch",ftos(cvar("ftebroom_entity_assets_epoch")+1));fb_document_tick();
        fb_entity_expect(fb_entity_model_name(player72)=="progs/player.mdl"&&fb_entity_model_ready(player72),"player72_recovers_without_command");
        fb_selection_clear();return TRUE;
    }
    if(argv(0)=="ftebroom_test_scene40" && cvar("ftebroom_skip_setup")) {
        fb_selection_clear();fb_entity_create("info_player_start",FALSE,'-40 -64 96');
        int player=fb_selected_entity();fb_document_tick();
        fb_entity_expect(fb_entity_model_name(player)=="progs/player.mdl","base_pak_player_model_resolved");
        fb_selection_clear();return TRUE;
    }
    if(argv(0)=="ftebroom_test_delete" && cvar("ftebroom_skip_setup")) {
        static int entityId,deleteHistory;int phase=stoi(argv(1));
        if(phase==0) {
            fb_tool_deactivate();fb_selection_clear();
            fb_entity_create("info_player_deathmatch",FALSE,'-128 -64 64');
            entityId=fb_selected_entity();deleteHistory=history;
            fb_entity_expect(entityId>0,"delete_point_setup");
        } else if(phase==1) {
            fb_entity_expect(terrain_edit(TEREDIT_ENT_GET,entityId)=="" && !fb_selected_entity_count,"point_keyboard_delete");
        } else if(phase==2) {
            fb_entity_expect(history==deleteHistory && fb_entity_selected(entityId) && terrain_edit(TEREDIT_ENT_GET,entityId)!="","point_keyboard_undo");
        } else if(phase==3) {
            fb_selection_clear();fb_createbox('128 0 64','64 64 64');
            fb_entity_create("func_wall",TRUE,'0 0 0');fb_entity_targets();
            entityId=fb_property_entities[0];deleteHistory=history;
        } else if(phase==4) {
            fb_entity_expect(terrain_edit(TEREDIT_ENT_GET,entityId)=="" && !selectedbrushcount,"brush_entity_keyboard_delete");
        } else if(phase==5) {
            fb_entity_expect(history==deleteHistory && selectedbrushcount==1 && terrain_edit(TEREDIT_ENT_GET,entityId)!="","brush_entity_keyboard_undo");
            fb_selection_clear();
        }
        return TRUE;
    }
    if(argv(0)!="ftebroom_test_entities" || !cvar("ftebroom_skip_setup"))return FALSE;
    fb_selection_clear();int marker=history;
    fb_entity_create("info_player_deathmatch",FALSE,'64 128 64');int player=fb_selected_entity();
    fb_entity_expect(player>0 && fb_entity_field(player,"classname")=="info_player_deathmatch","create_selected_point");
    fb_entity_expect(stov(fb_entity_field(player,"origin"))=='64 128 64',"point_origin");
    vector bounds[2];fb_entity_bounds(player,bounds);
    fb_entity_expect(bounds[1]-bounds[0]=='32 32 56',"q3_definition_bounds");
    brush_undo();fb_entity_expect(history==marker && terrain_edit(TEREDIT_ENT_GET,player)=="","create_single_undo");brush_redo();
    fb_entity_expect(fb_entity_selected(player),"redo_restores_entity_selection");
    fb_entity_property("set","targetname","player_one");fb_entity_expect(fb_entity_field(player,"targetname")=="player_one","set_property");
    fb_entity_property("rename","targetname","target");fb_entity_expect(fb_entity_field(player,"targetname")=="" && fb_entity_field(player,"target")=="player_one","rename_property");
    fb_entity_property("remove","target","");fb_entity_expect(fb_entity_field(player,"target")=="","remove_property");brush_undo();
    fb_entity_expect(fb_entity_field(player,"target")=="player_one","property_undo");
    fb_entity_create("light",FALSE,'128 128 96');int light=fb_selected_entity();fb_entity_select(player,TRUE);
    marker=history;fb_entity_property("set","targetname","shared");
    fb_entity_expect(fb_entity_field(player,"targetname")=="shared" && fb_entity_field(light,"targetname")=="shared","multi_entity_property");
    brush_undo();fb_entity_expect(history==marker && fb_entity_field(light,"targetname")=="","multi_property_single_undo");brush_redo();
    fb_entity_select(player,FALSE);
    fb_entity_apply_defaults("missing");fb_entity_expect(fb_entity_field(light,"light")=="300","missing_default_properties");
    fb_entity_property("set","light","450");fb_entity_apply_defaults("missing");fb_entity_expect(fb_entity_field(light,"light")=="450","missing_defaults_preserve_explicit");
    fb_entity_apply_defaults("existing");fb_entity_expect(fb_entity_field(light,"light")=="300","reset_existing_defaults");
    fb_entity_property("set","spawnflags","8");fb_entity_select(player,TRUE);
    fb_entity_property("setflag","spawnflags","2");
    fb_entity_expect(fb_entity_field(player,"spawnflags")=="2" && fb_entity_field(light,"spawnflags")=="10","flags_preserve_other_entities_bits");
    fb_entity_property("clearflag","spawnflags","2");
    fb_entity_expect(fb_entity_field(player,"spawnflags")=="0" && fb_entity_field(light,"spawnflags")=="8","flags_clear_only_requested_bit");
    fb_selection_clear();fb_entity_property("set","message","Entity test map");fb_entity_expect(fb_entity_field(0,"message")=="Entity test map","unselected_edits_worldspawn");
    fb_createbox('256 0 64','64 64 64');fb_createbox('384 0 64','64 64 64');
    fb_entity_create("func_door",TRUE,'0 0 0');fb_document_tick();int door=-1;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_nodes[n].name=="func_door")door=fb_nodes[n].id;
    fb_entity_expect(door>0 && selectedbrushcount==1 && selectedbrushes[0].model!=1,"create_brush_entity");
    fb_entity_property("set","speed","200");fb_entity_expect(fb_entity_field(door,"speed")=="200","selected_brush_edits_owner");
    fb_organize_action("structural",0,"");fb_document_tick();
    fb_entity_expect(selectedbrushcount==1 && selectedbrushes[0].model==1 && terrain_edit(TEREDIT_ENT_GET,door)=="","make_structural_removes_empty_entity");
    brush_undo();fb_entity_expect(terrain_edit(TEREDIT_ENT_GET,door)!="","structural_undo_restores_entity");
    fb_entity_publish();fb_entity_expect(fb_property_entity_count==1 && fb_property_entities[0]==door,"inspector_owner_targets");
    cvar_set("ftebroom_instantiate_defaults","1");
    marker=history;fb_entity_create("light",FALSE,'256 128 96');int defaultLight=fb_selected_entity();
    fb_entity_expect(fb_entity_field(defaultLight,"light")=="300","create_with_default_properties");
    brush_undo();fb_entity_expect(history==marker && terrain_edit(TEREDIT_ENT_GET,defaultLight)=="","defaults_creation_single_undo");
    cvar_set("ftebroom_instantiate_defaults","0");
    hashtable parameters=hash_createtab(8,EV_STRING);
    parameters["modelscale"]="2";parameters["modelscale_vec"]="1 2 3";
    fb_entity_expect(fb_definition_parameter("modelscale, modelscale_vec",parameters,"1")=="1 2 3","model_vector_scale_expression");
    fb_entity_expect(fb_definition_parameter("missing",parameters,"3")=="3" && fb_definition_parameter("2",parameters,"1")=="2","model_parameter_fallback_literal");
    hash_destroytab(parameters);
    fb_selection_clear();fb_entity_property("set","qa_a","A");fb_entity_property("set","qa_b","B");marker=history;
    tokenize("ftebroom_entity_remove qa_a qa_b");fb_entity_command();
    fb_entity_expect(fb_entity_field(0,"qa_a")=="" && fb_entity_field(0,"qa_b")=="","remove_multiple_properties");
    brush_undo();fb_entity_expect(history==marker && fb_entity_field(0,"qa_a")=="A" && fb_entity_field(0,"qa_b")=="B","remove_properties_single_undo");

    return TRUE;
}
