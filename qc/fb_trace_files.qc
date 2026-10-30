// SPDX-License-Identifier: GPL-2.0-or-later
vector *fb_trace_points;
vector *fb_trace_portals;
int *fb_trace_sizes;
int fb_trace_count,fb_trace_portal_count,fb_trace_index;
var int fb_trace_visible=TRUE;
trisoup_simple_vert_t *fb_trace_vertices;int *fb_trace_lines;
void fb_trace_unload(int portals)
{
    if(portals){memfree(fb_trace_portals);memfree(fb_trace_sizes);fb_trace_portals=__NULL__;fb_trace_sizes=__NULL__;fb_trace_portal_count=0;}
    else {memfree(fb_trace_vertices);memfree(fb_trace_lines);fb_trace_vertices=__NULL__;fb_trace_lines=__NULL__;memfree(fb_trace_points);fb_trace_points=__NULL__;fb_trace_count=0;fb_trace_index=-1;}
}
void fb_trace_load(int portals)
{
    filestream file=fopen(portals?"portals.txt":"leak.txt",FILE_READ);if((float)file<0)return;
    int count=0,polygons=0;string line;
    while((line=fgets(file))) {
        if(portals){int n=stoi(line);if(n<3 || n>1024 || count+n>262144){fclose(file);return;}
            polygons++;count+=n;for(int i=0;i<n;i++)if(!fgets(file)){fclose(file);return;}}
        else if(++count>262144){fclose(file);return;}
    }
    fclose(file);fb_trace_unload(portals);if(!count)return;
    file=fopen(portals?"portals.txt":"leak.txt",FILE_READ);if((float)file<0)return;
    if(portals) {
        fb_trace_portals=memalloc(sizeof(vector)*count);fb_trace_sizes=memalloc(sizeof(int)*polygons);
        fb_trace_portal_count=polygons;int offset=0;
        for(int p=0;p<polygons;p++){int vertices=stoi(fgets(file));fb_trace_sizes[p]=vertices;
            for(int v=0;v<vertices;v++)fb_trace_portals[offset++]=stov(fgets(file));}
    }else {fb_trace_points=memalloc(sizeof(vector)*count);fb_trace_count=count;
        for(int j=0;j<count;j++)fb_trace_points[j]=stov(fgets(file));
        fb_trace_visible=TRUE;fb_trace_vertices=memalloc(sizeof(trisoup_simple_vert_t)*count);fb_trace_lines=memalloc(sizeof(int)*max(2,(count-1)*2));
        for(int j=0;j<count;j++){for(int a=0;a<3;a++){fb_trace_vertices[j].xyz[a]=fb_trace_points[j][a];fb_trace_vertices[j].rgba[a]=a==0?1:.12;}fb_trace_vertices[j].rgba[3]=1;}
        for(int j=0;j<count-1;j++){fb_trace_lines[j*2]=j;fb_trace_lines[j*2+1]=j+1;}}
    fclose(file);
}
void fb_trace_step(int step)
{
    if(!fb_trace_count)return;
    fb_trace_index=bound(0,fb_trace_index+step,fb_trace_count-1);fb_focus_active=FALSE;
    fb_camera_position(fb_trace_points[fb_trace_index]);
    int next=min(fb_trace_count-1,fb_trace_index+1),previous=max(0,fb_trace_index-1);
    vector direction=fb_trace_points[next]-fb_trace_points[fb_trace_index];
    if(vlen(direction)<0.001)direction=fb_trace_points[previous]-fb_trace_points[fb_trace_index];
    if(vlen(direction)>0.001)setproperty(VF_CL_VIEWANGLES,fb_camera_angles(direction));
}
void fb_trace_goto(int index){if(!fb_trace_count)return;fb_trace_index=bound(0,index,fb_trace_count-1);fb_trace_step(0);}
void fb_trace_dot(vector point,vector color){
    makevectors(getproperty(VF_ANGLES));float size=bound(3,vlen(point-(vector)getproperty(VF_ORIGIN))*.007,40);
    vector right=v_right*size,up=v_up*size;
    R_BeginPolygon("ftebroom/leakmarker",3);R_PolygonVertex(point-right-up,'0 0 0',color,1);R_PolygonVertex(point+right-up,'1 0 0',color,1);R_PolygonVertex(point+right+up,'1 1 0',color,1);R_PolygonVertex(point-right+up,'0 1 0',color,1);R_EndPolygon();
}
void fb_trace_draw()
{
    if(fb_trace_visible && fb_trace_count) {
        shaderforname("ftebroom/leak","{\nnodepthtest\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\n}\n}");
        shaderforname("ftebroom/leakmarker","{\ncull none\nnodepthtest\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\n}\n}");
        if(fb_trace_count>1)addtrisoup_simple("ftebroom/leak",3|DRAWFLAG_LINES,fb_trace_vertices,fb_trace_lines,(fb_trace_count-1)*2);
        fb_trace_dot(fb_trace_points[0],'0.2 1 0.35');if(fb_trace_count>1)fb_trace_dot(fb_trace_points[fb_trace_count-1],'0.2 0.6 1');
        if(fb_trace_index>=0){
            vector point=fb_trace_points[fb_trace_index];fb_trace_dot(point,'1 0.85 0.1');
            int next=fb_trace_index+1;while(next<fb_trace_count && vlen(fb_trace_points[next]-point)<.001)next++;
            if(next<fb_trace_count){vector dir=normalize(fb_trace_points[next]-point),side=normalize(crossproduct(dir,point-(vector)getproperty(VF_ORIGIN)))*6;
                R_BeginPolygon("ftebroom/leakmarker",3);R_PolygonVertex(point+dir*28,'0 0 0','1 0.85 0.1',1);R_PolygonVertex(point+dir*12+side,'0 0 0','1 0.85 0.1',1);R_PolygonVertex(point+dir*12-side,'0 0 0','1 0.85 0.1',1);R_EndPolygon();}
        }
    }
    if(fb_trace_portal_count) {
        shaderforname("ftebroom/portals","{\ncull none\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc blend\n}\n}");
        int offset=0;
        for(int p=0;p<fb_trace_portal_count;p++) {
            R_BeginPolygon("ftebroom/portals",0);
            for(int v=0;v<fb_trace_sizes[p];v++)R_PolygonVertex(fb_trace_portals[offset++],'0 0 0','0.2 0.7 1',0.25);
            R_EndPolygon();
        }
    }
}
int fb_trace_command()
{
    if(argv(0)!="ftebroom_trace")return FALSE;
    string kind=argv(1);
    if(kind=="loadpoints")fb_trace_load(FALSE);else if(kind=="loadportals")fb_trace_load(TRUE);
    else if(kind=="unloadpoints")fb_trace_unload(FALSE);else if(kind=="unloadportals")fb_trace_unload(TRUE);
    else if(kind=="visible")fb_trace_visible=stoi(argv(2))!=0;else if(kind=="goto")fb_trace_goto(stoi(argv(2)));else fb_trace_step(kind=="previous"?-1:1);return TRUE;
}
