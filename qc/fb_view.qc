// SPDX-License-Identifier: GPL-2.0-or-later
vector fb_view_right,fb_view_up,fb_view_normal;
vector fb_view_centers[4],fb_view_lastmin,fb_view_lastmax;
int fb_view_has_selection;
void fb_view_axes(int view)
{
    fb_view_right='1 0 0';fb_view_up='0 1 0';fb_view_normal='0 0 1';
    if(view==1) {fb_view_right='0 1 0';fb_view_up='0 0 1';fb_view_normal='1 0 0';}
    if(view==2) {fb_view_right='1 0 0';fb_view_up='0 0 1';fb_view_normal='0 -1 0';}
}
int fb_view_depth_axis(int view)
{
    if(view==1) return 0;
    if(view==2) return 1;
    return 2;
}
void fb_view_prepare()
{
    int view=cvar("ca_brush_view");if(view<1 || view>3) return;
    vector size=getproperty(VF_SIZE);
    float span=bound(1,cvar("ca_brush_viewsize"),262144);
    vector fov=[span,span*size_y/size_x,131072];
    if(size_y>size_x) fov=[span*size_x/size_y,span,131072];
    vector angles='90 90 0';
    if(view==1) angles='0 180 0';
    if(view==2) angles='0 90 0';
    setproperty(VF_ORIGIN,fb_view_centers[view]);
    setproperty(VF_ANGLES,angles);setproperty(VF_VIEWENTITY,0);
    setproperty(VF_PERSPECTIVE,FALSE);setproperty(VF_FOV,fov);
    setproperty(VF_MINDIST,-131072);setproperty(VF_MAXDIST,131072);
    fb_view_axes(view);
    makevectors(angles);
}
vector fb_view_point(vector pixel,float depth)
{
    vector point=unproject(pixel);
    point[fb_view_depth_axis(cvar("ca_brush_view"))]=depth;
    return point;
}
void fb_view_remember_selection()
{
    static int selection=-1,geometry=-1,edits=-1,entities=-1;
    if(selection==fb_selection_epoch && geometry==fb_grid_geometry_epoch && edits==fb_incremental_epoch && entities==fb_entity_render_epoch)return;
    selection=fb_selection_epoch;geometry=fb_grid_geometry_epoch;edits=fb_incremental_epoch;entities=fb_entity_render_epoch;
    static vector bounds[2];int found=FALSE;
    for(int b=0;b<selectedbrushcount;b++) {
        if(brush_getfacepoints(selectedbrushes[b].model,selectedbrushes[b].id,0,bounds,2)!=2) continue;
        if(!found) {fb_view_lastmin=bounds[0];fb_view_lastmax=bounds[1];found=TRUE;}
        else for(int a=0;a<3;a++) {
            fb_view_lastmin[a]=min(fb_view_lastmin[a],bounds[0][a]);
            fb_view_lastmax[a]=max(fb_view_lastmax[a],bounds[1][a]);
        }
    }
    for(int entityIndex=0;entityIndex<fb_selected_entity_count;entityIndex++) if(fb_entity_bounds(fb_selected_entities[entityIndex],bounds)) {
        if(!found) {fb_view_lastmin=bounds[0];fb_view_lastmax=bounds[1];found=TRUE;}
        else for(int e=0;e<3;e++) {
            fb_view_lastmin[e]=min(fb_view_lastmin[e],bounds[0][e]);
            fb_view_lastmax[e]=max(fb_view_lastmax[e],bounds[1][e]);
        }
    }
    if(found) fb_view_has_selection=TRUE;
}
void fb_view_input(string kind,float dx,float dy,float scroll)
{
    int view=cvar("ca_brush_view");if(view<1 || view>3) return;
    fb_view_prepare();
    vector pixel=[cvar("ftebroom_mouse_x"),cvar("ftebroom_mouse_y"),0];
    vector before=unproject(pixel);
    if(kind=="pan" || kind=="look") fb_view_centers[view]+=before-unproject(pixel+[dx,dy,0]);
    if(kind=="wheel") {
        if(cvar("ftebroom_wheel_invert")) scroll=-scroll;
        cvar_set("ca_brush_viewsize",ftos(bound(1,cvar("ca_brush_viewsize")/max(0.01,1+scroll/50),262144)));
        fb_view_prepare();fb_view_centers[view]+=before-unproject(pixel);
    }
    // Share world coordinates; each orthographic camera uses its visible axes.
    for(int other=1;other<=3;other++) fb_view_centers[other]=fb_view_centers[view];
    fb_view_prepare();
}
void fb_view_background()
{
    int view=cvar("ca_brush_view");if(!view) return;
    fb_view_axes(view);
    vector size=getproperty(VF_SIZE),origin=getproperty(VF_MIN);
    vector lo=unproject(origin),hi=unproject(origin+size);
    float left=min(lo*fb_view_right,hi*fb_view_right),right=max(lo*fb_view_right,hi*fb_view_right);
    float bottom=min(lo*fb_view_up,hi*fb_view_up),top=max(lo*fb_view_up,hi*fb_view_up);
    float step=fb_grid(),units=(right-left)/size_x;
    while(step/units<4) step*=2;
    if(cvar("ftebroom_grid_visible")) {
        drawsetcliparea(origin_x,origin_y,size_x,size_y);
        for(int axis=0;axis<3;axis++) {
            vector basis='0 0 0';basis[axis]=1;vector n=fb_grid_direction_world(basis);
            float nr=n*fb_view_right,nu=n*fb_view_up,den=nr*nr+nu*nu;
            if(den<0.000001)continue;
            float depth=n*fb_view_normal*(lo*fb_view_normal)-n*autocvar_ftebroom_grid_origin;
            float low=min(nr*left,nr*right)+min(nu*bottom,nu*top)+depth;
            float high=max(nr*left,nr*right)+max(nu*bottom,nu*top)+depth;
            float spacing=step;while((high-low)/spacing>1024)spacing*=2;
            vector tangent=normalize(fb_view_right*(-nu)+fb_view_up*nr)*(right-left+top-bottom)*2;
            for(float level=ceil(low/spacing)*spacing;level<=high;level+=spacing) {
                vector center=(fb_view_right*nr+fb_view_up*nu)*((level-depth)/den)+fb_view_normal*(lo*fb_view_normal);
                drawline(1,project(center-tangent),project(center+tangent),'0.18 0.18 0.2',cvar("ftebroom_grid_alpha"),0);
            }
        }
        drawresetcliparea();
    }
    vector rc='1 0.2 0.2',uc='0.2 1 0.2';
    string rn="X",un="Y";
    if(view==1) {rc='0.2 1 0.2';rn="Y";}
    if(view!=3) {uc='0.3 0.5 1';un="Z";}
    drawline(2,project(fb_view_right*left),project(fb_view_right*right),rc,1,0);
    drawline(2,project(fb_view_up*bottom),project(fb_view_up*top),uc,1,0);
    vector p=origin+[76,size_y-65,0];
    drawline(2,p,p+'36 0 0',rc,1,0);drawstring(p+'40 -5 0',rn,'8 8 0',rc,1,0);
    drawline(2,p,p+'0 -36 0',uc,1,0);drawstring(p+'-4 -50 0',un,'8 8 0',uc,1,0);
    string bounds=cvar_string("ftebroom_map_bounds");
    int count=tokenize(bounds);
    if(count==6 && cvar("ftebroom_bounds_visible")) {
        vector mn=[stof(argv(0)),stof(argv(1)),stof(argv(2))],mx=[stof(argv(3)),stof(argv(4)),stof(argv(5))];
        float l=min(mn*fb_view_right,mx*fb_view_right),r=max(mn*fb_view_right,mx*fb_view_right);
        float b=min(mn*fb_view_up,mx*fb_view_up),t=max(mn*fb_view_up,mx*fb_view_up);
        vector a=project(fb_view_right*l+fb_view_up*b),c=project(fb_view_right*r+fb_view_up*t);
        drawline(2,a,[c_x,a_y,0],'1 0.5 0',1,0);drawline(2,[c_x,a_y,0],c,'1 0.5 0',1,0);
        drawline(2,c,[a_x,c_y,0],'1 0.5 0',1,0);drawline(2,[a_x,c_y,0],a,'1 0.5 0',1,0);
    }
}

// Apply a native viewport before projection, picking or camera input.
int fb_view_rect(int index)
{
    string rect=cvar_string(sprintf("ftebroom_view%i_rect",index));
    if(tokenize(rect)!=4) return FALSE;
    vector origin=[stof(argv(0)),stof(argv(1)),0],size=[stof(argv(2)),stof(argv(3)),0];
    if(size_x<1 || size_y<1) return FALSE;
    setproperty(VF_MIN,origin);setproperty(VF_SIZE,size);return TRUE;
}
void fb_view_reset()
{
    vector center='0 0 0';
    if(selectedbrushcount || fb_selected_entity()>=0) {fb_view_remember_selection();center=(fb_view_lastmin+fb_view_lastmax)*0.5;}
    for(int i=1;i<=3;i++) fb_view_centers[i]=center;
    cvar_set("ca_brush_viewsize","1024");
}
