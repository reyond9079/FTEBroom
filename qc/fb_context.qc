void fb_terrain_create(vector position,string material);
// SPDX-License-Identifier: GPL-2.0-or-later
vector fb_entity_drop_position(string classname,vector pixel)
{
    vector depth=[0,0,cvar("ftebroom_plane")];if(cvar("ca_brush_view") && fb_visual_has_bounds)depth=fb_visual_max;
    vector point,normal='0 0 1';
    if(fb_pick(pixel)>0){point=fb_hits[0].point;normal=fb_hits[0].normal;}
    else point=fb_view_plane_point(pixel,cvar("ca_brush_view"),depth);
    vector low='-8 -8 -8',high='8 8 8';
    for(int d=0;d<fb_entity_definition_count;d++)if(fb_entity_definitions[d].classname==classname){low=fb_entity_definitions[d].mins;high=fb_entity_definitions[d].maxs;break;}
    float support=0;for(int a=0;a<3;a++){if(normal[a]>=0)support+=normal[a]*low[a];else support+=normal[a]*high[a];}
    vector origin=fb_snap(point-normal*support);float penetration=point*normal-(origin*normal+support);if(penetration>0)origin+=normal*penetration;
    return origin;
}
void fb_context_query(vector pixel)
{
    fb_document_tick();cvar_set("ftebroom_context_group","0");cvar_set("ftebroom_context_entity","0");cvar_set("ftebroom_context_brush_entity","0");cvar_set("ftebroom_context_layer","0");cvar_set("ftebroom_context_material","");cvar_set("ftebroom_context_class","");
    vector point=fb_view_plane_point(pixel,cvar("ca_brush_view"),[0,0,cvar("ftebroom_plane")]);
    if(fb_pick(pixel)>0) {
        fb_hit_t hit=fb_hits[0];point=hit.point;
        if(hit.kind==FB_HIT_BRUSH && hit.face>0){static brushface_t faces[64];int count=brush_get(hit.model,hit.id,faces,64,__NULL__);if(hit.face<=count)cvar_set("ftebroom_context_material",faces[hit.face-1].shadername);}
        if(hit.node>=0)for(int n=hit.node,steps=0;n>=2 && steps++<fb_node_count;n=fb_nodes[n].parent) {
            if(fb_nodes[n].kind==FB_NODE_GROUP && !cvar("ftebroom_context_group"))cvar_set("ftebroom_context_group",itos(fb_nodes[n].id));
            if(fb_nodes[n].kind==FB_NODE_LAYER)cvar_set("ftebroom_context_layer",itos(fb_nodes[n].id));
            if(fb_nodes[n].kind==FB_NODE_ENTITY){cvar_set("ftebroom_context_class",fb_nodes[n].fields["classname"]);cvar_set("ftebroom_context_entity",itos(fb_nodes[n].id));if(!fb_selection_leaf(n))cvar_set("ftebroom_context_brush_entity","1");}
        }
    }
    cvar_set("ftebroom_context_point",vtos(fb_snap(point)));cvar_set("ftebroom_context_serial",ftos(cvar("ftebroom_context_serial")+1));
}
void fb_entity_move_to(int id)
{
    fb_tool_refresh();fb_document_tick();int node=fb_organize_node(id);if(node<2 || fb_nodes[node].kind!=FB_NODE_ENTITY || fb_selection_leaf(node))return;
    int count=selectedbrushcount;selbrush_t *items=memalloc(count*sizeof(selbrush_t));memcpy(items,selectedbrushes,count*sizeof(selbrush_t));
    fb_tx_begin("Move Brushes to Entity");int model=fb_organize_model(id),valid=model!=0;
    for(int b=0;b<count && valid;b++)if(items[b].model!=model)valid=fb_organize_move_brush(items[b].model,items[b].id,model,FALSE);
    if(valid){fb_organize_cleanup();fb_tx_commit();}else fb_tx_cancel();memfree(items);
}
void fb_entity_select_class(string classname)
{
    string name=strzone(classname);fb_document_tick();fb_tx_begin("Select Entity Class");fb_selection_clear();
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_ENTITY && fb_nodes[n].fields["classname"]==name && fb_pick_allowed(n,0))fb_selection_set(n,TRUE);
    fb_tx_commit();strunzone(name);
}
void fb_entity_drop_preview()
{
    string classname=cvar_string("ftebroom_entity_ghost");if(classname=="")return;
    vector mn=getproperty(VF_MIN),sz=getproperty(VF_SIZE);if(curmousepos_x<mn_x || curmousepos_y<mn_y || curmousepos_x>=mn_x+sz_x || curmousepos_y>=mn_y+sz_y)return;
    vector point=fb_entity_drop_position(classname,curmousepos),low='-8 -8 -8',high='8 8 8';
    for(int d=0;d<fb_entity_definition_count;d++)if(fb_entity_definitions[d].classname==classname){low=fb_entity_definitions[d].mins;high=fb_entity_definitions[d].maxs;break;}
    fb_visual_box(point+low,point+high,'1 0.8 0',0.7,FALSE);
}
float fb_context_command()
{
    if(argv(0)=="ftebroom_entity_at_cursor") {
        string classname=strzone(argv(1));vector cursorPosition=fb_entity_drop_position(classname,curmousepos);
        fb_entity_create(classname,FALSE,cursorPosition);strunzone(classname);return TRUE;
    }

    if(argv(0)=="ftebroom_context"){fb_context_query([stof(argv(1)),stof(argv(2)),0]);return TRUE;}
    if(argv(0)=="ftebroom_terrain_position") {vector point=fb_entity_drop_position("func_model",[stof(argv(1)),stof(argv(2)),0]);cvar_set("ftebroom_terrain_origin",vtos(point));return TRUE;}
    if(argv(0)=="ftebroom_terrain_create") {fb_terrain_create(stov(argv(1)),argv(2));return TRUE;}
    if(argv(0)=="ftebroom_model_drop") {
        string model=strzone(argv(1)),modelClass="func_model";if(argv(4)=="misc_model"||argv(4)=="cycler"||argv(4)=="cycler_sprite")modelClass=argv(4);
        vector modelPixel=[stof(argv(2)),stof(argv(3)),0];
        vector modelPosition=fb_entity_drop_position(modelClass,modelPixel);
        for(int axis=0;axis<3;axis++)if(!fb_geom_finite(modelPosition[axis])||fabs(modelPosition[axis])>autocvar_ftebroom_world_bound){strunzone(model);return TRUE;}
        fb_tx_begin("Import model");fb_entity_create(modelClass,FALSE,modelPosition);
        fb_entity_property("set","model",model);fb_tx_commit();strunzone(model);return TRUE;
    }
    if(argv(0)=="ftebroom_entity_drop"){
        string name=strzone(argv(1));vector pixel=[stof(argv(2)),stof(argv(3)),0];vector position=fb_entity_drop_position(name,pixel);fb_entity_create(name,FALSE,position);strunzone(name);return TRUE;
    }
    if(argv(0)=="ftebroom_entity_move_to"){fb_entity_move_to(stoi(argv(1)));return TRUE;}
    if(argv(0)=="ftebroom_entity_select_class"){fb_entity_select_class(argv(1));return TRUE;}
    return FALSE;
}
