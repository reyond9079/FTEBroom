// SPDX-License-Identifier: GPL-2.0-or-later
// Remote BSPs and avatars are render entities, never editable document nodes.
entity fb_online_items;
.entity fb_online_next;
vector fb_online_low,fb_online_high;
void fb_online_clear()
{
    while(fb_online_items){entity next=fb_online_items.fb_online_next;remove(fb_online_items);fb_online_items=next;}
}
int fb_online_before_commit()
{
    if(!cvar("ftebroom_online_active"))return TRUE;
    fb_document_rebuild();int geometry=0;
    for(int n=2;n<fb_node_count;n++) {
        fb_node_t node=fb_nodes[n];vector bounds[2];int valid=FALSE;
        if(node.kind>=FB_NODE_BRUSH){geometry++;valid=brush_getfacepoints(node.model,node.id,0,bounds,2)==2;}
        else if(node.kind==FB_NODE_ENTITY){bounds[0]=bounds[1]=fb_entity_origin(node.id);valid=TRUE;}
        if(valid)for(int axis=0;axis<3;axis++)if(bounds[0][axis]<fb_online_low[axis]-.01 || bounds[1][axis]>fb_online_high[axis]+.01){print("Online: edit cancelled outside your plot.\n");return FALSE;}
    }
    if(geometry>2048){print("Online: at most 2048 brushes per plot.\n");return FALSE;}
    return TRUE;
}
void fb_online_draw()
{
    if(!cvar("ftebroom_online_active"))return;
    for(entity item=fb_online_items;item;item=item.fb_online_next) {
        if(item.modelindex)addentity(item);
        else fb_visual_box(item.origin+'-16 -16 -24',item.origin+'16 16 32','0.2 0.8 1',1,FALSE,TRUE);
    }
    fb_visual_box(fb_online_low,fb_online_high,'0.2 0.8 1',.7,FALSE,TRUE);
}
float fb_online_command()
{
    if(argv(0)=="ftebroom_test_online" && cvar("ftebroom_skip_setup")) {
        if(argv(1)=="guard") {
            fb_document_rebuild();int before=fb_node_count;vector center=(fb_online_low+fb_online_high)*.5;center_z=96;
            fb_createbox(center,'32 32 32');fb_document_rebuild();int inside=fb_node_count;
            print(inside==before+1?"FTEBROOM_ONLINE_PASS inside_plot_edit\n":"FTEBROOM_ONLINE_FAIL inside_plot_edit\n");
            center_x=fb_online_high_x+64;fb_createbox(center,'32 32 32');fb_document_rebuild();
            print(fb_node_count==inside?"FTEBROOM_ONLINE_PASS outside_plot_rolled_back\n":"FTEBROOM_ONLINE_FAIL outside_plot_rolled_back\n");
            brush_undo();fb_document_rebuild();
            print(fb_node_count==before?"FTEBROOM_ONLINE_PASS guard_undo\n":"FTEBROOM_ONLINE_FAIL guard_undo\n");
        } else if(argv(1)=="add") {
            vector added=(fb_online_low+fb_online_high)*.5;added_z=96;fb_createbox(added,'32 32 32');
        } else if(argv(1)=="scene") {
            int avatars=0,models=0;
            for(entity item=fb_online_items;item;item=item.fb_online_next)if(item.modelindex){if(item.model=="progs/player.mdl")avatars++;else models++;}
            print(avatars>0?"FTEBROOM_ONLINE_PASS remote_avatar_loaded\n":"FTEBROOM_ONLINE_FAIL remote_avatar_loaded\n");
            print(models>0?"FTEBROOM_ONLINE_PASS remote_bsp_loaded\n":"FTEBROOM_ONLINE_FAIL remote_bsp_loaded\n");
        }
        return TRUE;
    }
    if(argv(0)=="ftebroom_online_clear"){fb_online_clear();return TRUE;}
    if(argv(0)=="ftebroom_online_enable") {
        for(int i=0;i<3;i++){fb_online_low[i]=cvar(sprintf("ftebroom_online_bound%i",i));fb_online_high[i]=cvar(sprintf("ftebroom_online_bound%i",(int)(i+3)));}
        precache_model("progs/player.mdl");
        filestream material=fopen("data/online/default-material.txt",FILE_READ);
        if((float)material>=0){string chosen=fgets(material);if(chosen!="")cvar_set("ca_newbrushtexture",chosen);fclose(material);}
        vector spawnpoint=(fb_online_low+fb_online_high)*.5;spawnpoint_z=128;
        localcmd(sprintf("setpos %g %g %g 15 90 0\n",spawnpoint_x,spawnpoint_y,spawnpoint_z));return TRUE;
    }
    if(argv(0)!="ftebroom_online_scene")return FALSE;
    if(!cvar("ftebroom_online_active"))return TRUE;
    filestream file=fopen("data/online/scene.txt",FILE_READ);if((float)file<0)return TRUE;
    fb_online_clear();string line;int count=0;
    while((line=fgets(file)) && count<16) {
        int tokens=tokenize(line);string kind=argv(0);
        entity item;
        if(kind=="model" && tokens==2 && substring(argv(1),0,strlen("maps/ftebroom-build/online-"))=="maps/ftebroom-build/online-") {
            item=spawn();setmodel(item,argv(1));item.origin='0 0 0';item.fb_online_next=fb_online_items;fb_online_items=item;count++;
        } else if(kind=="avatar" && tokens==6) {
            item=spawn();item.origin=[stof(argv(1)),stof(argv(2)),stof(argv(3))-24];item.angles=[-stof(argv(4)),stof(argv(5)),0];
            setmodel(item,"progs/player.mdl");item.fb_online_next=fb_online_items;fb_online_items=item;count++;
        }
    }
    fclose(file);return TRUE;
}
