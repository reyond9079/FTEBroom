// SPDX-License-Identifier: GPL-2.0-or-later
// Switch only the CSQC render world; the editable model and history stay alive.
int fb_build_preview_active;
float(float index) editor_renderworld = #0:editor_renderworld;
float fb_build_source_model;
float fb_build_source_fullbright,fb_build_source_clear;
string fb_build_source_clearcolour;
entity fb_build_entities;
.entity fb_build_next;
void fb_build_preview_leave()
{
    if(!fb_build_preview_active)return;
    if(!editor_renderworld(fb_build_source_model)){print("ReyondBroom: cannot restore editing world\n");return;}
    cvar_set("r_fullbright",ftos(fb_build_source_fullbright));
    cvar_set("gl_clear",ftos(fb_build_source_clear));cvar_set("r_clearcolour",fb_build_source_clearcolour);strunzone(fb_build_source_clearcolour);
    while(fb_build_entities){entity next=fb_build_entities.fb_build_next;remove(fb_build_entities);fb_build_entities=next;}
    fb_build_preview_active=FALSE;cvar_set("ftebroom_bsp_preview","0");
}
void fb_build_preview_enter(string path)
{
    fb_build_preview_leave();
    if(substring(path,0,20)!="maps/ftebroom-build/")return;
    // BSP lightmaps and shader permutations must be initialized in lit mode,
    // rather than inheriting the editable MAP's fullbright render state.
    fb_build_source_fullbright=cvar("r_fullbright");cvar_set("r_fullbright","0");
    float index=getmodelindex(path);
    if(!index){cvar_set("r_fullbright",ftos(fb_build_source_fullbright));cvar_set("ftebroom_build_error","Cannot load compiled BSP");return;}
    fb_build_source_model=world.modelindex;if(!fb_build_source_model)fb_build_source_model=1;
    if(!editor_renderworld(index)){cvar_set("r_fullbright",ftos(fb_build_source_fullbright));cvar_set("ftebroom_build_error","Cannot load compiled BSP");return;}
    fb_build_source_clear=cvar("gl_clear");fb_build_source_clearcolour=strzone(cvar_string("r_clearcolour"));
    cvar_set("r_clearcolour","0 0 0");cvar_set("gl_clear","1");
    fb_build_preview_active=TRUE;cvar_set("ftebroom_bsp_preview","1");
    path=strzone(path);int entityCount=0;
    getentitytoken("");
    while(getentitytoken()=="{") {
        string modelPath="",positionText="",anglesText="",yawText="",key;
        while((key=getentitytoken())!="}" && key!="") {
            key=strzone(key);string value=getentitytoken();
            if(key=="model")modelPath=strzone(value);
            if(key=="origin")positionText=strzone(value);
            if(key=="angles")anglesText=strzone(value);
            if(key=="angle")yawText=strzone(value);
            strunzone(key);
        }
        if(substring(modelPath,0,1)=="*") {
            entity item=spawn();setmodel(item,strcat(modelPath,":",path));item.origin=stov(positionText);entityCount++;
            item.angles=stov(anglesText);if(anglesText=="" && !cvar("ftebroom_quake1"))item.angles_y=stof(yawText);
            item.fb_build_next=fb_build_entities;fb_build_entities=item;
        }
        strunzone(modelPath);strunzone(positionText);strunzone(anglesText);strunzone(yawText);
    }
    strunzone(path);cvar_set("ftebroom_bsp_entities",itos(entityCount));
}
void fb_build_preview_draw()
{
    for(entity item=fb_build_entities;item;item=item.fb_build_next)addentity(item);
}

