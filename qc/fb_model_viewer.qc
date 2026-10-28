// SPDX-License-Identifier: GPL-2.0-or-later
vector(float model,float sequence,float mode) editor_model_info = #0:editor_model_info;
entity fb_modelviewer_entity;
string fb_modelviewer_path;
float fb_modelviewer_skeleton;
float fb_modelviewer_skinmodel;
string fb_modelviewer_customskin;
void(float model) editor_model_meshmeta = #0:editor_model_meshmeta;
void(entity item,float surface,float mode) editor_model_drawsurface = #0:editor_model_drawsurface;
vector fb_modeleditor_point(vector point,int node)
{
    string prefix=strcat("ftebroom_modeleditor_transform_",ftos(node));
    return stov(cvar_string(strcat(prefix,"_origin")))+stov(cvar_string(strcat(prefix,"_0")))*point_x+stov(cvar_string(strcat(prefix,"_1")))*point_y+stov(cvar_string(strcat(prefix,"_2")))*point_z;
}
void fb_modeleditor_guides(vector eye)
{
    string line="ftebroom/model-editor-guides",floorShader="ftebroom/model-editor-floor";
    shaderforname(line,"{\ncull none\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc blend\n}\n}");
    shaderforname(floorShader,"{\ncull none\nsort blend\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc blend\n}\n}");
    int lines=0;
    if(cvar("ftebroom_modeleditor_floor")){
        float step=max(1,fb_grid()),extent=max(1024,fabs(eye_z)*2);extent=min(extent,1000000);
        while(extent/step>64)step*=2;
        vector center=[floor(eye_x/step)*step,floor(eye_y/step)*step,0];
        extent=ceil(extent/step)*step;
        R_BeginPolygon(floorShader,3);
        R_PolygonVertex(center+[-extent,-extent,-.05],'0 0','0.18 0.20 0.24',.18);
        R_PolygonVertex(center+[extent,-extent,-.05],'0 0','0.18 0.20 0.24',.18);
        R_PolygonVertex(center+[extent,extent,-.05],'0 0','0.18 0.20 0.24',.18);
        R_PolygonVertex(center+[-extent,extent,-.05],'0 0','0.18 0.20 0.24',.18);R_EndPolygon();
        for(float offset=-extent;offset<=extent;offset+=step){
            R_BeginPolygon(line);R_PolygonVertex(center+[offset,-extent,0],'0 0','0.38 0.42 0.48',.45);R_PolygonVertex(center+[offset,extent,0],'0 0','0.38 0.42 0.48',.45);R_EndPolygon();
            R_BeginPolygon(line);R_PolygonVertex(center+[-extent,offset,0],'0 0','0.38 0.42 0.48',.45);R_PolygonVertex(center+[extent,offset,0],'0 0','0.38 0.42 0.48',.45);R_EndPolygon();lines+=2;
        }
    }
    if(cvar("ftebroom_modeleditor_axes"))for(int axis=0;axis<3;axis++){
        vector start='0 0 0',end='0 0 0',colour='0.25 0.5 1';start[axis]=-10000000;end[axis]=10000000;
        if(axis==0)colour='1 .18 .15';else if(axis==1)colour='.15 1 .25';
        R_BeginPolygon(line);R_PolygonVertex(start,'0 0',colour,1);R_PolygonVertex(end,'0 0',colour,1);R_EndPolygon();
    }
    cvar_set("ftebroom_modeleditor_grid_lines",itos(lines));
}
void fb_modelviewer_draw()
{
    if(!cvar("ftebroom_modelviewer_open")) {
        if(fb_modelviewer_entity){remove(fb_modelviewer_entity);fb_modelviewer_entity=world;}
        if(fb_modelviewer_skeleton){skel_delete(fb_modelviewer_skeleton);fb_modelviewer_skeleton=0;}
        return;
    }
    string path=cvar_string("ftebroom_modelviewer_path");int bsp=strtolower(substring(path,max(0,strlen(path)-4),-1))==".bsp";
    if(strcmp(path,fb_modelviewer_path) || !fb_modelviewer_entity) {
        if(fb_modelviewer_entity)remove(fb_modelviewer_entity);
        if(fb_modelviewer_skeleton)skel_delete(fb_modelviewer_skeleton);
        fb_modelviewer_entity=spawn();setmodel(fb_modelviewer_entity,path);
        fb_modelviewer_skinmodel=0;
        strunzone(fb_modelviewer_path);fb_modelviewer_path=strzone(path);
        fb_modelviewer_skeleton=0;if(!bsp)fb_modelviewer_skeleton=skel_create(fb_modelviewer_entity.modelindex);
    }
    entity item=fb_modelviewer_entity;float model=item.modelindex;
    float editing=cvar("ftebroom_modeleditor_active");
    if(editing&&!bsp&&!cvar("ftebroom_modeleditor_meshes"))editor_model_meshmeta(model);
    if(model!=fb_modelviewer_skinmodel) {
        fb_modelviewer_skinmodel=model;
        int skinCount=0;
        while(!bsp&&skinCount<256 && skintoname(model,skinCount)!="")skinCount++;
        skinCount=max(max(1,skinCount),cvar("ftebroom_modelviewer_embeddedskins"));
        cvar_set("ftebroom_modelviewer_skins",ftos(skinCount));
        for(int skinIndex=0;skinIndex<skinCount;skinIndex++)
            cvar_set(strcat("ftebroom_modelviewer_skin_name_",ftos(skinIndex)),skintoname(model,skinIndex));
    }
    item.skin=max(0,cvar("ftebroom_modelviewer_skin"));
    cvar_set("ftebroom_modelviewer_skinname",bsp?"":skintoname(model,item.skin));
    string customSkin=cvar_string("ftebroom_modelviewer_customskin");
    if(!bsp&&strcmp(customSkin,fb_modelviewer_customskin)) {
        setcustomskin(item,customSkin);
        strunzone(fb_modelviewer_customskin);fb_modelviewer_customskin=strzone(customSkin);
    }
    cvar_set("ftebroom_modelviewer_count",ftos(bsp?1:modelframecount(model)));
    int sequence=bound(0,cvar("ftebroom_modelviewer_sequence"),max(0,(bsp?1:modelframecount(model))-1));
    vector info=editor_model_info(model,sequence,0);
    cvar_set("ftebroom_modelviewer_poses",ftos(info_x));cvar_set("ftebroom_modelviewer_duration",ftos(info_y));
    cvar_set("ftebroom_modelviewer_name",bsp?"Static BSP":frametoname(model,sequence));
    float bones=fb_modelviewer_skeleton?skel_get_numbones(fb_modelviewer_skeleton):0;
    cvar_set("ftebroom_modelviewer_bones",ftos(bones));
    int selectedBone=bound(1,cvar("ftebroom_modelviewer_bone"),max(1,bones));
    cvar_set("ftebroom_modelviewer_bonename",bones?skel_get_bonename(fb_modelviewer_skeleton,selectedBone):"No skeletal bones");
    item.frame=sequence;item.frame1time=cvar("ftebroom_modelviewer_time");item.frame2=sequence;item.lerpfrac=0;
    vector low=editor_model_info(model,0,1),high=editor_model_info(model,0,2),center=(low+high)*0.5;
    cvar_set("ftebroom_modeleditor_low",vtos(low));cvar_set("ftebroom_modeleditor_high",vtos(high));
    vector viewAngles=[cvar("ftebroom_modelviewer_pitch"),cvar("ftebroom_modelviewer_yaw"),0];
    makevectors(viewAngles);vector eye=center-v_forward*max(8,vlen(high-low)*1.4)*max(.1,cvar("ftebroom_modelviewer_zoom"));
    float player=editing&&cvar("ftebroom_modeleditor_player");if(player){eye='0 0 0';viewAngles='0 0 0';}
    else if(editing&&cvar("ftebroom_modeleditor_freecamera"))eye=stov(cvar_string("ftebroom_modeleditor_camera"));
    cvar_set("ftebroom_modeleditor_eye",vtos(eye));cvar_set("ftebroom_modeleditor_angles",vtos(viewAngles));
    vector size=[max(64,cvar("ftebroom_modelviewer_width")),max(64,cvar("ftebroom_modelviewer_height")),0];
    float resolutionScale=min(1,min(1600/size_x,1200/size_y));size*=resolutionScale;size_x=max(1,floor(size_x));size_y=max(1,floor(size_y));
    vector oldOrigin=getproperty(VF_ORIGIN),oldAngles=getproperty(VF_ANGLES),oldMin=getproperty(VF_MIN),oldSize=getproperty(VF_SIZE),oldFov=getproperty(VF_FOV);
    float oldPerspective=getproperty(VF_PERSPECTIVE),oldNear=getproperty(VF_MINDIST),oldFar=getproperty(VF_MAXDIST);
    clearscene();setproperty(VF_PERSPECTIVE,TRUE);setproperty(VF_MINDIST,1);setproperty(VF_MAXDIST,0);setproperty(VF_RT_DESTCOLOUR,"ftebroom_modelviewer",-IMGFMT_R8G8B8A8,size);
    setproperty(VF_MIN,'0 0 0');setproperty(VF_SIZE,size);setproperty(VF_ORIGIN,eye);setproperty(VF_ANGLES,viewAngles);
    setproperty(VF_DRAWWORLD,0);setproperty(VF_FOVY,45);setproperty(VF_FOVX,atan(tan(45*M_PI/360)*size_x/size_y)*360/M_PI);
    if(player){setproperty(VF_FOVX,90);setproperty(VF_FOVY,atan(size_y/size_x)*360/M_PI);setproperty(VF_MINDIST,.1);}
    drawfill('0 0 0',size,'0.08 0.09 0.11',1,0);item.effects=EF_FULLBRIGHT;
    if(editing)fb_modeleditor_guides(eye);
    if(editing&&!bsp&&cvar("ftebroom_modeleditor_count")>0){
        item.renderflags=0;item.scale=1;setorigin(item,'0 0 0');item.angles='0 0 0';
        editor_model_drawsurface(item,-1,cvar("ftebroom_modeleditor_display"));
        item.renderflags=0;setorigin(item,'0 0 0');item.angles='0 0 0';
    }else{item.renderflags=0;setorigin(item,'0 0 0');addentity(item);}
    if(cvar("ftebroom_modelviewer_showbones"))for(int bone=1;bone<=min(bones,512);bone++) {
        int parent=skel_get_boneparent(fb_modelviewer_skeleton,bone);
        if(parent){vector a=gettaginfo(item,bone),b=gettaginfo(item,parent);if(editing){int node=cvar("ftebroom_modeleditor_selected");if(node==0&&cvar("ftebroom_modeleditor_count")==1)node=1;a=fb_modeleditor_point(a,node);b=fb_modeleditor_point(b,node);}fb_visual_line(a,b,'0.2 1 0.4',1,1,0);}
    }
    renderscene();setproperty(VF_RT_DESTCOLOUR,"");
    setproperty(VF_ORIGIN,oldOrigin);setproperty(VF_ANGLES,oldAngles);setproperty(VF_MIN,oldMin);setproperty(VF_SIZE,oldSize);setproperty(VF_FOV,oldFov);setproperty(VF_DRAWWORLD,1);setproperty(VF_PERSPECTIVE,oldPerspective);setproperty(VF_MINDIST,oldNear);setproperty(VF_MAXDIST,oldFar);
}

float(float model,float sequence,float frames,float fps,optional float skin) editor_model_extract = #0:editor_model_extract;
int fb_model_extract_command()
{
    if(argv(0)!="ftebroom_model_extract")return FALSE;
    entity model=spawn();setmodel(model,cvar_string("ftebroom_convert_source"));
    int valid=editor_model_extract(model.modelindex,cvar("ftebroom_convert_sequence"),cvar("ftebroom_convert_frames"),cvar("ftebroom_convert_fps"),cvar("ftebroom_convert_skin"));
    remove(model);cvar_set("ftebroom_convert_status",valid?"1":"-1");return TRUE;
}
