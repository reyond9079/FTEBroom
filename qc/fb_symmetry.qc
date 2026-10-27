// SPDX-License-Identifier: GPL-3.0-or-later
// Symmetry instances use the persistent linked-group transaction machinery.
void fb_symmetry_matrix(vector center,float angle,int side,int sides,int rotateCopies,fb_affine_t *matrix)
{
    fb_linked_matrix("",matrix);
    float step=2*M_PI/sides;
    if(rotateCopies || !(side&1)) {
        float rotation=step*side;
        matrix->x=[cos(rotation),sin(rotation),0];matrix->y=[-sin(rotation),cos(rotation),0];
    } else {
        float mirror=angle+step*(float)((side+1i)/2i);
        matrix->x=[cos(2*mirror),sin(2*mirror),0];matrix->y=[sin(2*mirror),-cos(2*mirror),0];
    }
    matrix->translation=center-fb_affine_direction(matrix,center);fb_affine_prepare(matrix);
}
int fb_symmetry_relink(int sides,int rotateCopies);
var float autocvar_ftebroom_symmetry_relink=1;
int fb_symmetry_apply(int sides,int rotateCopies)
{
    if(sides!=2&&sides!=4&&sides!=8)return FALSE;
    fb_tool_deactivate();fb_document_tick();
    if(!selectedbrushcount&&!fb_selected_entity_count){cvar_set("ftebroom_selection_status","Select objects for symmetry");return FALSE;}
    if(autocvar_ftebroom_symmetry_relink)if(fb_symmetry_relink(sides,rotateCopies))return TRUE;
    if(!fb_transform_capture())return FALSE;
    if(fb_transform_count*sides>2048){fb_transform_clear();cvar_set("ftebroom_selection_status","Symmetry exceeds 2048 objects");return FALSE;}
    vector low='1000000000 1000000000 1000000000',high=-low;
    for(int i=0;i<fb_transform_count;i++)for(int axis=0;axis<3;axis++){low[axis]=min(low[axis],fb_transform_items[i].low[axis]);high[axis]=max(high[axis],fb_transform_items[i].high[axis]);}
    vector center=(low+high)*.5;fb_transform_clear();
    int parent=fb_selection_open_node();if(parent<0)parent=fb_organize_node(fb_active_layer);
    int sourceId=-1,selectedGroups=0;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_GROUP&&fb_selection_selected(n)&&fb_nodes[n].parent==parent){sourceId=fb_nodes[n].id;selectedGroups++;}
    // Reconfigure an existing symmetry set; otherwise wrap the complete selection.
    int existing=selectedGroups==1&&sourceId>=0;
    if(existing)existing=fb_nodes[fb_organize_node(sourceId)].fields["_ftebroom_symmetry"]!="";
    if(existing)for(int n=2;n<fb_node_count;n++)if(fb_selection_leaf(n)&&fb_selection_selected(n)&&!fb_selection_descendant(n,fb_organize_node(sourceId)))existing=FALSE;
    float angle=cvar("ftebroom_symmetry_angle")*M_PI/180;
    if(existing&&!cvar("ftebroom_symmetry_custom_center")){
        string savedCenter=fb_nodes[fb_organize_node(sourceId)].fields["_ftebroom_symmetry_center"];
        if(savedCenter!="")center=stov(savedCenter);
    }
    if(cvar("ftebroom_symmetry_custom_center"))center=[cvar("ftebroom_symmetry_center_x"),cvar("ftebroom_symmetry_center_y"),cvar("ftebroom_symmetry_center_z")];
    if(!fb_geom_point(center)){cvar_set("ftebroom_selection_status","Symmetry center is outside world bounds");return FALSE;}
    fb_tx_begin("Symmetry");fb_linked_busy=TRUE;int valid=TRUE;
    if(!existing){
        sourceId=fb_organize_new("group","Symmetry source",parent);fb_document_rebuild();
        valid=fb_organize_move(sourceId,fb_organize_node(sourceId),FALSE);
    }
    fb_document_rebuild();int source=fb_organize_node(sourceId);
    string link=strzone(existing?(string)fb_nodes[source].fields["_tb_linked_group_id"]:fb_linked_uuid());
    if(existing){
        // Rebuilding the set is atomic, including deletion of the old instances.
        int *remove=memalloc(fb_node_count*sizeof(int));
        for(int n=2;n<fb_node_count;n++)if(n!=source&&fb_nodes[n].kind==FB_NODE_GROUP&&fb_nodes[n].fields["_tb_linked_group_id"]==link)
            for(int child=2;child<fb_node_count;child++)if(fb_selection_descendant(child,n))remove[child]=TRUE;
        for(int n=fb_node_count-1;n>=2;n--)if(remove[n]){if(fb_nodes[n].kind>=FB_NODE_BRUSH)brush_history_delete(fb_nodes[n].model,fb_nodes[n].id);else fb_tx_entity_set(fb_nodes[n].id,"",HIST_TREE);}
        memfree(remove);
    }
    fb_affine_t identity;fb_linked_matrix("",&identity);
    fb_tx_set_key(sourceId,"_tb_linked_group_id",link,HIST_TREE);
    fb_tx_set_key(sourceId,"_tb_transformation",fb_linked_matrix_text(&identity),HIST_TREE);
    fb_tx_set_key(sourceId,"_ftebroom_symmetry",sprintf("%g %g",(float)sides,(float)rotateCopies),HIST_TREE);
    fb_tx_set_key(sourceId,"_ftebroom_symmetry_center",vtos(center),HIST_TREE);
    fb_tx_set_key(sourceId,"_ftebroom_symmetry_angle",ftos(angle),HIST_TREE);
    for(int side=1;side<sides&&valid;side++){
        fb_document_rebuild();source=fb_organize_node(sourceId);parent=fb_nodes[source].parent;
        int copy=fb_organize_new("group",sprintf("Symmetry side %g",(float)(side+1)),parent);
        fb_affine_t matrix;fb_symmetry_matrix(center,angle,side,sides,rotateCopies,&matrix);
        fb_tx_set_key(copy,"_ftebroom_symmetry_center",vtos(center),HIST_TREE);
        fb_tx_set_key(copy,"_ftebroom_symmetry_angle",ftos(angle),HIST_TREE);
        fb_tx_set_key(copy,"_tb_linked_group_id",link,HIST_TREE);
        fb_tx_set_key(copy,"_tb_transformation",fb_linked_matrix_text(&matrix),HIST_TREE);
        fb_tx_set_key(copy,"_ftebroom_symmetry",sprintf("%g %g",(float)sides,(float)rotateCopies),HIST_TREE);
    }
    fb_document_rebuild();source=fb_organize_node(sourceId);
    if(valid)valid=fb_linked_sync_source(source);
    fb_linked_busy=TRUE;
    if(valid){fb_selection_clear();fb_document_rebuild();fb_selection_set(fb_organize_node(sourceId),TRUE);fb_tx_commit();}else fb_tx_cancel();
    fb_linked_busy=FALSE;strunzone(link);fb_document_tick();
    cvar_set("ftebroom_selection_status",valid?"Symmetry created. Open any side's group to edit all instances.":"Symmetry could not be created; no changes applied.");return valid;
}
int fb_symmetry_command()
{
    if(argv(0)!="ftebroom_symmetry")return FALSE;
    int sides=stoi(argv(1)),rotateCopies=stoi(argv(2));fb_symmetry_apply(sides,rotateCopies);return TRUE;
}

// Rebuild guide bounds only when the document or selection changes.
var float autocvar_ftebroom_symmetry_guides=1;
var int fb_symmetry_guide_revision=-1,fb_symmetry_guide_selection=-1,fb_symmetry_guide_open=-1;
int fb_symmetry_guide_sides,fb_symmetry_guide_filters,fb_symmetry_guide_visibility;
int fb_container_bounds(int node,vector *bounds);
vector fb_symmetry_guide_center;
float fb_symmetry_guide_angle,fb_symmetry_guide_radius,fb_symmetry_guide_height;
void fb_symmetry_guides_update()
{
    fb_symmetry_guide_filters=fb_filter_epoch;fb_symmetry_guide_visibility=fb_visibility_epoch;
    fb_symmetry_guide_revision=fb_grid_geometry_epoch;fb_symmetry_guide_selection=fb_selection_epoch;fb_symmetry_guide_open=fb_open_group;fb_symmetry_guide_sides=0;
    int root=-1;
    if(fb_open_group)root=fb_organize_node(fb_open_group);
    for(int steps=0;root>=2&&root<fb_node_count&&steps<fb_node_count;steps++){if(fb_nodes[root].fields["_ftebroom_symmetry"]!="")break;root=fb_nodes[root].parent;}
    // QC logical expressions evaluate every operand. Reject ordinary nodes
    // before querying a group's descendant selection on large documents.
    if(root<2)for(int n=2;n<fb_node_count;n++) {
        if(fb_nodes[n].kind!=FB_NODE_GROUP)continue;
        if(fb_nodes[n].fields["_ftebroom_symmetry"]=="")continue;
        if(fb_selection_selected(n)){root=n;break;}
    }
    if(root<2)return;
    vector bounds[2];if(!fb_container_bounds(root,bounds))return;
    string stored=fb_nodes[root].fields["_ftebroom_symmetry_center"];
    fb_symmetry_guide_center=stored!=""?stov(stored):(bounds[0]+bounds[1])*.5;
    fb_symmetry_guide_angle=stof(fb_nodes[root].fields["_ftebroom_symmetry_angle"]);
    fb_symmetry_guide_sides=stoi(fb_nodes[root].fields["_ftebroom_symmetry"]);
    if(fb_symmetry_guide_sides!=2&&fb_symmetry_guide_sides!=4&&fb_symmetry_guide_sides!=8){fb_symmetry_guide_sides=0;return;}
    vector extent=bounds[1]-bounds[0];fb_symmetry_guide_radius=bound(512,max(extent_x,extent_y)*3,32768);fb_symmetry_guide_height=max(128,extent_z*1.25);
}
void fb_symmetry_draw()
{
    if(!autocvar_ftebroom_symmetry_guides)return;
    if(fb_symmetry_guide_filters!=fb_filter_epoch||fb_symmetry_guide_visibility!=fb_visibility_epoch||fb_symmetry_guide_revision!=fb_grid_geometry_epoch||fb_symmetry_guide_selection!=fb_selection_epoch||fb_symmetry_guide_open!=fb_open_group)fb_symmetry_guides_update();
    if(!fb_symmetry_guide_sides)return;
    for(int side=0;side<fb_symmetry_guide_sides;side++){
        float angle=fb_symmetry_guide_angle+2*M_PI*side/fb_symmetry_guide_sides;
        fb_visual_line(fb_symmetry_guide_center,fb_symmetry_guide_center+[cos(angle),sin(angle),0]*fb_symmetry_guide_radius,'0 .85 1',.75,1,FALSE);
    }
    fb_visual_line(fb_symmetry_guide_center-'0 0 1'*fb_symmetry_guide_height,fb_symmetry_guide_center+'0 0 1'*fb_symmetry_guide_height,'0 .85 1',.75,1,FALSE);
}

#include "fb_symmetry_relink.qc"
