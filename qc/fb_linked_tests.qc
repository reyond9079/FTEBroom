// SPDX-License-Identifier: GPL-2.0-or-later
void fb_linked_expect(int ok,string label){print(strcat(ok?"FTEBROOM_LINKED_PASS ":"FTEBROOM_LINKED_FAIL ",label,"\n"));}
int fb_linked_test_brush(int entityIndex)
{
    fb_document_tick();
    int root=fb_organize_node(entityIndex);
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH && fb_nodes[n].parent==root)return n;
    return -1;
}
vector fb_linked_test_min(int node)
{
    static vector bounds[2];if(node<0)return '0 0 -99999';
    brush_getfacepoints(fb_nodes[node].model,fb_nodes[node].id,0,bounds,2);return bounds[0];
}
int fb_linked_testcommand()
{
    if(argv(0)!="ftebroom_test_linked" || !cvar("ftebroom_skip_setup"))return FALSE;
    fb_cancel();fb_open_group=0;fb_createbox('0 0 64','64 64 64');fb_group_create("Linked A");fb_document_tick();
    int a=-1,b=-1;for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].name=="Linked A")a=fb_nodes[n].id;
    int marker=history;fb_linked_action("duplicate");fb_document_tick();
    int count=0;for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP && fb_nodes[n].fields["_tb_linked_group_id"]!="") {
        count++;if(fb_nodes[n].id!=a)b=fb_nodes[n].id;
    }
    fb_linked_expect(count==2 && b>=0 && selectedbrushcount==1,"linked_duplicate");
    if(b<0)return TRUE;
    fb_link_peers_prepare();fb_linked_expect(fb_link_peer_count==2,"peer_index_contains_both_instances");
    brush_undo();fb_document_tick();fb_linked_expect(history==marker && fb_nodes[fb_organize_node(a)].fields["_tb_linked_group_id"]=="","duplicate_single_undo");
    fb_link_peers_prepare();fb_linked_expect(fb_link_peer_count==0,"peer_index_removes_undone_instance");
    brush_redo();fb_document_tick();
    fb_link_peers_prepare();fb_linked_expect(fb_link_peer_count==2,"peer_index_restores_redone_instance");
    fb_move_numeric('128 0 0');fb_document_tick();fb_affine_t map;
    fb_linked_matrix(fb_nodes[fb_organize_node(b)].fields["_tb_transformation"],&map);
    fb_linked_expect(map.translation=='128 0 0',"instance_translation_matrix");
    fb_open_group=a;fb_selection_clear();fb_selection_set(fb_linked_test_brush(a),TRUE);
    marker=history;fb_move_numeric('0 0 32');fb_document_tick();
    int na=fb_linked_test_brush(a),nb=fb_linked_test_brush(b);
    vector minA=fb_linked_test_min(na),minB=fb_linked_test_min(nb);
    fb_linked_expect(na>=0 && nb>=0 && minB-minA=='128 0 0' && minA_z==64,"open_instance_sync");
    brush_undo();fb_document_tick();na=fb_linked_test_brush(a);nb=fb_linked_test_brush(b);
    minA=fb_linked_test_min(na);minB=fb_linked_test_min(nb);
    fb_linked_expect(history==marker && minA_z==32 && minB_z==32,"sync_single_undo");
    brush_redo();fb_document_tick();fb_open_group=0;fb_selection_clear();fb_selection_set(fb_organize_node(b),TRUE);
    fb_transform_capture();fb_affine_rotate('0 0 1',90,'128 0 0');fb_transform_clear();fb_document_tick();
    fb_open_group=a;fb_selection_clear();fb_selection_set(fb_linked_test_brush(a),TRUE);fb_move_numeric('16 0 0');fb_document_tick();
    na=fb_linked_test_brush(a);nb=fb_linked_test_brush(b);
    minA=fb_linked_test_min(na);minB=fb_linked_test_min(nb);
    fb_linked_expect(fabs(minB_y-minA_x)<0.01,"rotated_instance_sync");
    string merged=fb_linked_merge_properties("\"speed\" \"200\"\n\"targetname\" \"shared\"", "\"speed\" \"100\"\n\"targetname\" \"local\"\n\"_tb_protected_properties\" \"targetname\"");
    fb_linked_expect(fb_clip_field(merged,"speed")=="200" && fb_clip_field(merged,"targetname")=="local","protected_properties");strunzone(merged);
    fb_open_group=a;fb_selection_clear();fb_entity_create("info_player_deathmatch",FALSE,'0 0 80');fb_document_tick();
    int sourceEntity=fb_selected_entity(),targetEntity=-1;
    fb_entity_property("set","targetname","shared");fb_entity_property("set","speed","100");fb_document_tick();
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_selection_descendant(n,fb_organize_node(b)))targetEntity=fb_nodes[n].id;
    fb_linked_expect(targetEntity>=0,"point_entity_sync");
    if(targetEntity>=0) {
        fb_open_group=b;fb_selection_clear();fb_entity_select(targetEntity,TRUE);
        fb_linked_protect("on","targetname");fb_entity_property("set","targetname","local");fb_document_tick();
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_selection_descendant(n,fb_organize_node(a)))sourceEntity=fb_nodes[n].id;
        fb_open_group=a;fb_selection_clear();fb_entity_select(sourceEntity,TRUE);
        fb_entity_property("set","targetname","shared2");fb_entity_property("set","speed","200");fb_document_tick();
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_selection_descendant(n,fb_organize_node(b)))targetEntity=fb_nodes[n].id;
        string fields=strzone(terrain_edit(TEREDIT_ENT_GET,targetEntity));
        fb_linked_expect(fb_clip_field(fields,"targetname")=="local" && fb_clip_field(fields,"speed")=="200","protected_entity_sync");strunzone(fields);
        fb_open_group=b;fb_selection_clear();fb_entity_select(targetEntity,TRUE);fb_linked_protect("off","targetname");
        fields=strzone(terrain_edit(TEREDIT_ENT_GET,targetEntity));fb_linked_expect(fb_clip_field(fields,"targetname")=="shared2","unprotect_restores_peer");strunzone(fields);
        fb_linked_protect("on","angle");fb_entity_property("set","angle","77");fb_document_tick();
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_selection_descendant(n,fb_organize_node(a)))sourceEntity=fb_nodes[n].id;
        fb_open_group=a;fb_selection_clear();fb_entity_select(sourceEntity,TRUE);fb_entity_property("set","angle","0");fb_document_tick();
        for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_selection_descendant(n,fb_organize_node(b)))targetEntity=fb_nodes[n].id;
        fb_open_group=b;fb_selection_clear();fb_entity_select(targetEntity,TRUE);fb_linked_protect("off","angle");
        fields=strzone(terrain_edit(TEREDIT_ENT_GET,targetEntity));fb_linked_expect(fabs(stof(fb_clip_field(fields,"angle"))-90)<0.01,"unprotect_angle_uses_relative_transform");strunzone(fields);
    }
    fb_open_group=a;fb_selection_clear();fb_selection_set(fb_linked_test_brush(a),TRUE);marker=history;
    fb_linked_extract();fb_document_tick();int extracted=0;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP && fb_nodes[n].name=="Extracted group" && fb_nodes[n].parent==1)extracted++;
    fb_linked_expect(extracted==2 && fb_linked_test_brush(a)<0 && fb_linked_test_brush(b)<0,"extract_counterparts");
    brush_undo();fb_document_tick();fb_linked_expect(history==marker && fb_linked_test_brush(a)>=0 && fb_linked_test_brush(b)>=0,"extract_single_undo");
    fb_open_group=0;fb_selection_clear();fb_selection_set(fb_organize_node(b),TRUE);fb_linked_action("separate");fb_document_tick();
    fb_linked_expect(fb_nodes[fb_organize_node(b)].fields["_tb_linked_group_id"]=="","separate_singleton");
    fb_open_group=0;fb_createbox('512 0 64','32 32 32');fb_group_create("Outer source");fb_document_tick();
    int outer=-1,other=-1;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].name=="Outer source")outer=fb_nodes[n].id;
    fb_linked_action("duplicate");fb_document_tick();
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].name=="Outer source" && fb_nodes[n].id!=outer)other=fb_nodes[n].id;
    fb_open_group=outer;fb_selection_clear();fb_selection_set(fb_linked_test_brush(outer),TRUE);fb_group_create("Nested plain group");fb_document_tick();
    int inner=-1;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].name=="Nested plain group" && fb_selection_descendant(n,fb_organize_node(outer)))inner=fb_nodes[n].id;
    fb_open_group=inner;fb_selection_clear();fb_selection_set(fb_linked_test_brush(inner),TRUE);marker=history;
    fb_move_numeric('0 0 16');fb_document_tick();int peerBrush=-1;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH && fb_selection_descendant(n,fb_organize_node(other)))peerBrush=n;
    vector peerMin=fb_linked_test_min(peerBrush);
    fb_linked_expect(inner>=0 && peerBrush>=0 && peerMin_z==64,"nested_plain_group_syncs_linked_ancestor");
    brush_undo();fb_linked_expect(history==marker,"nested_sync_single_undo");
    return TRUE;
}
