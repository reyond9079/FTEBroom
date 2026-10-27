var float autocvar_ftebroom_rotate_entity_properties=1;
// SPDX-License-Identifier: GPL-2.0-or-later
// Columns of the linear map and its inverse transpose act on points and covectors.
typedef struct {vector x,y,z,translation,ix,iy,iz;float determinant;} fb_affine_t;
void fb_linked_transform(fb_affine_t *delta);
void fb_repeat_affine(fb_affine_t *matrix,vector center);
int fb_repeat_fixed_pivot;
int fb_vertex_affine_command();
int fb_vertex_bounds(vector *bounds);
int fb_vertex_affine_preview(fb_affine_t *m);
void fb_vertex_commit(string label);
vector fb_affine_point(fb_affine_t *m,vector p) {return m->x*p_x+m->y*p_y+m->z*p_z+m->translation;}
vector fb_affine_direction(fb_affine_t *m,vector p) {return m->x*p_x+m->y*p_y+m->z*p_z;}
vector fb_affine_covector(fb_affine_t *m,vector p) {return m->ix*p_x+m->iy*p_y+m->iz*p_z;}
int fb_affine_prepare(fb_affine_t *m)
{
    m->determinant=m->x*crossproduct(m->y,m->z);
    if(!fb_geom_finite(m->determinant) || fabs(m->determinant)<0.000001) return FALSE;
    m->ix=crossproduct(m->y,m->z)/m->determinant;
    m->iy=crossproduct(m->z,m->x)/m->determinant;
    m->iz=crossproduct(m->x,m->y)/m->determinant;
    return TRUE;
}
void fb_affine_face(fb_affine_t *m,brushface_t *face,int textureLock)
{
    vector n=fb_affine_covector(m,face->planenormal);float length=vlen(n);
    face->planedist=(face->planedist+n*m->translation)/length;face->planenormal=n/length;
    if(textureLock) {
        face->sdir=fb_affine_covector(m,face->sdir);face->tdir=fb_affine_covector(m,face->tdir);
        face->sbias-=face->sdir*m->translation;face->tbias-=face->tdir*m->translation;
    }
}
vector fb_affine_center()
{
    if(fb_vertex_scope) {vector bounds[2];if(fb_vertex_bounds(bounds)) return (bounds[0]+bounds[1])*0.5;}
    vector low='1000000000 1000000000 1000000000',high=-low;
    for(int i=0;i<fb_transform_count;i++) for(int axis=0;axis<3;axis++) {
        low[axis]=min(low[axis],fb_transform_items[i].low[axis]);high[axis]=max(high[axis],fb_transform_items[i].high[axis]);
    }
    return fb_snap((low+high)*0.5);
}
void fb_affine_angles(int id,fb_affine_t *m)
{
    vector bounds[2],origin=fb_entity_origin(id);
    if(fb_entity_bounds(id,bounds)) {
        vector offset=(bounds[0]+bounds[1])*0.5-origin;
        if(fabs(offset_x)>0.001 || fabs(offset_y)>0.001) return;
    }
    // Scaling and shearing change positions, not entity orientation properties.
    if(fabs(vlen(m->x)-1)>0.0001 || fabs(vlen(m->y)-1)>0.0001 || fabs(vlen(m->z)-1)>0.0001
        || fabs(m->x*m->y)>0.0001 || fabs(m->x*m->z)>0.0001 || fabs(m->y*m->z)>0.0001) return;
    hashtable fields=hash_createtab(32,EV_STRING);int count=fb_entity_tokenize(terrain_edit(TEREDIT_ENT_GET,id));
    for(int k=0;k<count;k+=2) fields[argv(k)]=argv(k+1);
    string key="angles";if(fields[key]=="") key="mangle";
    if(fields[key]!="") {
        int lightMangle=key=="mangle" && substring(fields["classname"],0,5)=="light";
        string model=fields["model"];if(model=="") model=fb_entity_model_name(id);
        int mdl=strtolower(substring(model,strlen(model)-4,4))==".mdl";
        vector angles=stov(fields[key]);
        if(lightMangle) angles=[angles_y,angles_x,angles_z];
        if(mdl && !lightMangle) angles_x=-angles_x;
        makevectors(angles);
        vector forward=fb_affine_direction(m,v_forward),up=fb_affine_direction(m,v_up);
        angles=vectoangles(normalize(forward),normalize(up));angles_x=-angles_x;
        for(int component=0;component<3;component++) angles[component]-=360*floor((angles[component]+180)/360);
        if(mdl && !lightMangle) angles_x=-angles_x;
        if(lightMangle) angles=[angles_y,angles_x,angles_z];
        fb_tx_set_key(id,key,sprintf("%.9g %.9g %.9g",angles_x,angles_y,angles_z),HIST_ENTITY);
    } else if(fields["angle"]!="") {
        float yaw=stof(fields["angle"]);vector yawForward;
        if(yaw==-1) yawForward='0 0 1';else if(yaw==-2) yawForward='0 0 -1';else {makevectors([0,yaw,0]);yawForward=v_forward;}
        yawForward=normalize(fb_affine_direction(m,yawForward));
        if(yawForward_z>0.99999) yaw=-1;else if(yawForward_z< -0.99999) yaw=-2;else yaw=atan2(yawForward_y,yawForward_x)*180/M_PI;
        fb_tx_set_key(id,"angle",sprintf("%.9g",yaw),HIST_ENTITY);
    } else if(vlen(m->z-'0 0 1')<0.0001 && m->determinant>0) {
        float newYaw=atan2(m->x[1],m->x[0])*180/M_PI;
        if(fabs(newYaw)>0.0001) fb_tx_set_key(id,"angle",sprintf("%.9g",newYaw),HIST_ENTITY);
    }
    hash_destroytab(fields);
}
int fb_affine_valid(fb_affine_t *m)
{
    if(!fb_affine_prepare(m)) return FALSE;
    int textureLock=cvar("ftebroom_texture_lock");
    // Validate the complete transformed selection before making any history records.
    for(int i=0;i<fb_transform_count;i++) {
        fb_transform_item_t *item=&fb_transform_items[i];
        if(item->kind==FB_NODE_BRUSH) {
            brushface_t *faces=memalloc(sizeof(brushface_t)*item->count);memcpy(faces,item->faces,sizeof(brushface_t)*item->count);
            for(int f=0;f<item->count;f++) fb_affine_face(m,&faces[f],textureLock);
            int valid=fb_brush_valid(faces,item->count);memfree(faces);if(!valid) return FALSE;
        } else if(item->kind==FB_NODE_PATCH) {
            for(int p=0;p<item->count;p++) if(!fb_geom_point(fb_affine_point(m,item->points[p].xyz))) return FALSE;
        } else {
            for(int corner=0;corner<8;corner++) {
                vector point='0 0 0';
                for(int axis=0;axis<3;axis++) point[axis]=(corner&(1i<<axis))?item->high[axis]:item->low[axis];
                if(!fb_geom_point(fb_affine_point(m,point))) return FALSE;
            }
        }
    }
    return TRUE;
}
int fb_affine_apply(fb_affine_t *m,string label)
{
    if(fb_vertex_scope) {if(!fb_vertex_affine_preview(m)) return FALSE;fb_vertex_commit(label);return TRUE;}
    if(!fb_affine_valid(m)) return FALSE;
    int textureLock=cvar("ftebroom_texture_lock");
    vector repeatCenter=fb_affine_center();
    fb_tx_begin(label);
    fb_linked_transform(m);
    for(int i=0;i<fb_transform_count;i++) {
        fb_transform_item_t *item=&fb_transform_items[i];
        if(item->kind==FB_NODE_BRUSH) {
            brushface_t *faces=memalloc(sizeof(brushface_t)*item->count);memcpy(faces,item->faces,sizeof(brushface_t)*item->count);
            for(int f=0;f<item->count;f++) fb_affine_face(m,&faces[f],textureLock);
            brush_history_edit(item->model,item->id,faces,item->count,item->contents);memfree(faces);
        } else if(item->kind==FB_NODE_PATCH) {
            patchvert_t *points=memalloc(sizeof(patchvert_t)*item->count);
            for(int p=0;p<item->count;p++) {
                int destination=p;
                if(m->determinant<0) {int row=p/item->patch.cpwidth,col=p-row*item->patch.cpwidth;destination=row*item->patch.cpwidth+item->patch.cpwidth-1-col;}
                points[destination]=item->points[p];points[destination].xyz=fb_affine_point(m,item->points[p].xyz);
            }
            patch_history_edit(item->model,item->id,points,item->patch);memfree(points);
        } else {
            vector origin=fb_affine_point(m,item->origin);
            fb_tx_set_key(item->id,"origin",sprintf("%.9g %.9g %.9g",origin_x,origin_y,origin_z),HIST_ENTITY);
            if(cvar("ftebroom_rotate_entity_properties")) fb_affine_angles(item->id,m);
        }
    }
    fb_tx_commit();fb_repeat_affine(m,repeatCenter);return TRUE;
}
vector fb_affine_axis(vector direction)
{
    int axis=0;for(int a=1;a<3;a++) if(fabs(direction[a])>fabs(direction[axis])) axis=a;
    vector result='0 0 0';result[axis]=direction[axis]<0?-1:1;return result;
}
vector fb_affine_rotate_axis(vector point,vector axis,float angle)
{
    return point*cos(angle)+crossproduct(axis,point)*sin(angle)+axis*(axis*point)*(1-cos(angle));
}
void fb_affine_rotate(vector axis,float degrees,vector center)
{
    if(!fb_geom_finite(degrees) || vlen(axis)<0.001) return;
    axis=normalize(axis);float angle=degrees*M_PI/180;
    fb_affine_t m={0};m.x=fb_affine_rotate_axis('1 0 0',axis,angle);
    m.y=fb_affine_rotate_axis('0 1 0',axis,angle);m.z=fb_affine_rotate_axis('0 0 1',axis,angle);
    if(fabs(degrees/90-rint(degrees/90))<0.000001 && (fabs(axis_x)==1 || fabs(axis_y)==1 || fabs(axis_z)==1)) {
        for(int i=0;i<3;i++) {m.x[i]=rint(m.x[i]);m.y[i]=rint(m.y[i]);m.z[i]=rint(m.z[i]);}
    }
    m.translation=center-fb_affine_direction(&m,center);
    fb_repeat_fixed_pivot=fb_repeat_fixed_pivot || vlen(center-fb_affine_center())>0.001;
    if(!fb_affine_apply(&m,"Rotate")) cvar_set("ftebroom_selection_status","Rotation would create invalid geometry");
    fb_repeat_fixed_pivot=FALSE;
}
float fb_affine_command()
{
    if(argv(0)!="ftebroom_rotate" && argv(0)!="ftebroom_transform_key") return FALSE;
    if(fb_vertex_tool() || fb_vertex_scope) return fb_vertex_affine_command();
    string kind=strzone(argv(1));int direction=stoi(argv(2));float degrees=stof(argv(4));
    vector requestedAxis=[stof(argv(1)),stof(argv(2)),stof(argv(3))];int explicitCenter=argv(7)!="";
    vector requestedCenter=[stof(argv(5)),stof(argv(6)),stof(argv(7))];
    int numeric=argv(0)=="ftebroom_rotate",rotateTool=cvar("ftebroom_tool")==FB_TOOL_ROTATE;
    fb_tool_refresh();if(!fb_transform_capture()) {strunzone(kind);return TRUE;}
    vector center=fb_affine_center();
    if(rotateTool) center=[cvar("ftebroom_rotate_center_x"),cvar("ftebroom_rotate_center_y"),cvar("ftebroom_rotate_center_z")];
    if(numeric) {if(explicitCenter) center=requestedCenter;fb_repeat_fixed_pivot=explicitCenter;fb_affine_rotate(requestedAxis,degrees,center);}
    else {
        int view=cvar("ca_brush_view");makevectors(getproperty(VF_ANGLES));
        vector forward=fb_affine_axis(v_forward),right=fb_affine_axis(v_right),up='0 0 1';
        if(view) {fb_view_axes(view);forward=fb_view_normal;right=fb_view_right;up=fb_view_up;}
        if(kind=="fliph" || kind=="flipv") {
            vector axis=right;if(kind=="flipv") axis=up;
            fb_affine_t m={0};m.x='1 0 0'-axis*(2*axis_x);m.y='0 1 0'-axis*(2*axis_y);m.z='0 0 1'-axis*(2*axis_z);
            m.translation=center-fb_affine_direction(&m,center);
            if(!fb_affine_apply(&m,"Flip")) cvar_set("ftebroom_selection_status","Flip would create invalid geometry");
        } else {
            vector rotateAxis=forward;if(kind=="yaw") rotateAxis=up;if(kind=="pitch") rotateAxis=right;
            float step=90;if(rotateTool) step=bound(0.01,cvar("ftebroom_rotate_step"),360);
            fb_affine_rotate(rotateAxis,direction*step,center);
        }
    }
    fb_transform_clear();strunzone(kind);return TRUE;
}

fb_affine_t fb_affine_preview_matrix;
void fb_affine_draw()
{
    fb_affine_t *m=&fb_affine_preview_matrix;
    for(int i=0;i<fb_transform_count;i++) {
        fb_transform_item_t *item=&fb_transform_items[i];
        if(item->kind==FB_NODE_BRUSH) {
            brushface_t *faces=memalloc(sizeof(brushface_t)*item->count);memcpy(faces,item->faces,sizeof(brushface_t)*item->count);
            for(int f=0;f<item->count;f++) fb_affine_face(m,&faces[f],cvar("ftebroom_texture_lock"));
            if(!cvar("ca_brush_view")) DrawQCBrushTextured(faces,item->count,'1 1 1',0.65);
            DrawQCBrushWireframe(faces,item->count,"chop",'1 0.5 0',1);memfree(faces);
        } else if(item->kind==FB_NODE_PATCH) {
            patchinfo_t info;int count=patch_getmesh(item->model,item->id,__NULL__,0,&info);
            patchvert_t *mesh=memalloc(sizeof(patchvert_t)*count);patch_getmesh(item->model,item->id,mesh,count,&info);
            for(int p=0;p<count;p++) mesh[p].xyz=fb_affine_point(m,mesh[p].xyz);
            if(!cvar("ca_brush_view")) DrawQCPatchTextured(mesh,info,'1 1 1',0.65);
            DrawQCPatchWireframe(mesh,info,"chop",'1 0.5 0',1);memfree(mesh);
        } else {
            if(!cvar("ca_brush_view") && cvar("ftebroom_show_models"))
                fb_entity_draw_instance(item->id,m->translation,m->x,m->y,m->z);
            static vector corners[8];
            for(int c=0;c<8;c++) {
                vector point='0 0 0';for(int a=0;a<3;a++) point[a]=(c&(1i<<a))?item->high[a]:item->low[a];
                corners[c]=fb_affine_point(m,point);
            }
            for(int c=0;c<8;c++) for(int a=0;a<3;a++) if(!(c&(1i<<a))) {
                R_BeginPolygon("chop");R_PolygonVertex(corners[c],'0 0 0','1 0.5 0',1);
                R_PolygonVertex(corners[c|(1i<<a)],'0 0 0','1 0.5 0',1);R_EndPolygon();
            }
        }
    }
}
