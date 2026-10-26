// SPDX-License-Identifier: GPL-2.0-or-later
var float autocvar_ftebroom_show_brushes=1,autocvar_ftebroom_show_points=1,autocvar_ftebroom_show_models=1;
var float autocvar_ftebroom_show_entity_names=1,autocvar_ftebroom_show_entity_bounds=1,autocvar_ftebroom_show_group_bounds=1,autocvar_ftebroom_show_brush_entity_bounds=1;
var float autocvar_ftebroom_render_faces=0,autocvar_ftebroom_shade_faces=1,autocvar_ftebroom_show_edges=1;
var float autocvar_ftebroom_bounds_visible=0;
var float autocvar_ftebroom_hidden_tags=0,autocvar_ftebroom_entity_links=1;
var string autocvar_ftebroom_hidden_classes="";
int *fb_filter_tags;int fb_filter_tag_count;
hashtable fb_shader_tags,fb_shader_previews;
int fb_sky_material(string material)
{
    string name=fb_material_key(material);
    if(fb_shader_tags)if((int)fb_shader_tags[name]&512)return TRUE;
    if(cvar("ftebroom_quake1") && substring(name,0,3)=="sky")return TRUE;
    return substring(name,0,4)=="sky/" || substring(name,0,6)=="skies/";
}
enum : int {FB_TAG_TRIGGER=1,FB_TAG_CLIP=2,FB_TAG_CAULK=4,FB_TAG_HINT=8,FB_TAG_DETAIL=16,FB_TAG_LIQUID=32,FB_TAG_TRANSLUCENT=64,FB_TAG_AREAPORTAL=128,FB_TAG_CLUSTERPORTAL=256};
void fb_filter_materials_load()
{
    if(fb_shader_tags)hash_destroytab(fb_shader_tags);if(fb_shader_previews)hash_destroytab(fb_shader_previews);
    fb_shader_tags=hash_createtab(512,EV_INTEGER);fb_shader_previews=hash_createtab(512,EV_STRING);
    filestream file=fopen("ftebroom-material-tags.txt",FILE_READ);if((float)file<0)return;
    string line;while((line=fgets(file)))if(tokenize(line)>=2){fb_shader_tags[argv(0)]=stoi(argv(1));fb_shader_previews[argv(0)]=argv(2);}
    fclose(file);fb_filter_epoch++;
}
int fb_filter_face_tags(brushface_t *face,int contents)
{
    string name=fb_material_key(face->shadername);int tags=0;
    if(fb_shader_tags)tags=fb_shader_tags[name];
    if(substring(name,0,7)=="common/") {
        if(strstrofs(name,"clip",7)>=0)tags|=FB_TAG_CLIP;
        if(strstrofs(name,"caulk",7)>=0)tags|=FB_TAG_CAULK;
        if(substring(name,7,4)=="hint")tags|=FB_TAG_HINT;
    }
    if(contents&134217728i)tags|=FB_TAG_DETAIL;
    if(contents&536870912i)tags|=FB_TAG_TRANSLUCENT;
    return tags;
}
void fb_filter_prepare()
{
    static int epoch=-1,materials=-1;static string previous="";
    string signature=sprintf("%g %g %g %g %g %g",autocvar_ftebroom_show_brushes,autocvar_ftebroom_show_points,autocvar_ftebroom_show_models,autocvar_ftebroom_render_faces,autocvar_ftebroom_shade_faces,autocvar_ftebroom_show_edges);
    signature=strcat(signature,sprintf(" %g %g %g %g %g %g %i",autocvar_ftebroom_hidden_tags,autocvar_ftebroom_entity_links,autocvar_ftebroom_show_entity_names,autocvar_ftebroom_show_entity_bounds,autocvar_ftebroom_show_group_bounds,autocvar_ftebroom_show_brush_entity_bounds,fb_open_group)," ",autocvar_ftebroom_hidden_classes,ftos(autocvar_ftebroom_bounds_visible));
    signature=strcat(signature," fog=",cvar_string("r_fog_enabled"));
    if(signature!=previous){strunzone(previous);previous=strzone(signature);fb_filter_epoch++;}
    int materialEpoch=cvar("ftebroom_material_tag_epoch");
    if(materialEpoch!=materials){materials=materialEpoch;fb_filter_materials_load();epoch=-1;}
    if(epoch==fb_grid_geometry_epoch)return;epoch=fb_grid_geometry_epoch;
    memfree(fb_filter_tags);fb_filter_tag_count=fb_node_count;fb_filter_tags=memalloc(fb_node_count*sizeof(int));
    static brushface_t faces[64];
    for(int n=2;n<fb_node_count;n++) {
        int tags=0;
        if(fb_nodes[n].kind==FB_NODE_BRUSH) {
            int contents,count=brush_get(fb_nodes[n].model,fb_nodes[n].id,faces,64,&contents);

            for(int f=0;f<min(count,64);f++)tags|=fb_filter_face_tags(&faces[f],contents);
        }
        for(int parent=n;parent>=2;parent=fb_nodes[parent].parent)if(fb_nodes[parent].kind==FB_NODE_ENTITY)
            if(substring(fb_nodes[parent].fields["classname"],0,8)=="trigger_")tags|=FB_TAG_TRIGGER;
        fb_filter_tags[n]=tags;
    }
    fb_filter_epoch++;
}
int fb_filter_allowed(int node)
{
    if(node<0 || node>=fb_node_count)return TRUE;
    if(fb_nodes[node].kind>=FB_NODE_BRUSH && !autocvar_ftebroom_show_brushes)return FALSE;
    if(fb_nodes[node].kind==FB_NODE_ENTITY) {
        if(strstrofs(autocvar_ftebroom_hidden_classes,strcat("|",fb_nodes[node].fields["classname"],"|"),0)>=0)return FALSE;
        if(substring(fb_nodes[node].fields["model"],0,1)!="*" && !autocvar_ftebroom_show_points)return FALSE;
    }
    if(node<fb_filter_tag_count)if(fb_filter_tags[node]&(int)autocvar_ftebroom_hidden_tags)return FALSE;
    return TRUE;
}
int fb_filter_custom(int node)
{
    if(node<0 || node>=fb_filter_tag_count)return FALSE;
    // Map-edit submodels have no server entity submitting their surfaces.
    // Draw them explicitly, independently of selection and point-model visibility.
    if(fb_nodes[node].kind>=FB_NODE_BRUSH && fb_nodes[node].model!=1)return TRUE;
    return autocvar_ftebroom_render_faces!=0 || !autocvar_ftebroom_shade_faces || (fb_filter_tags[node] & (FB_TAG_TRIGGER|FB_TAG_CLIP|FB_TAG_CAULK|FB_TAG_HINT|FB_TAG_TRANSLUCENT|FB_TAG_AREAPORTAL|FB_TAG_CLUSTERPORTAL));
}
int *fb_render_access;int fb_render_access_count;
int fb_render_allowed(int node,int flags)
{
    static int geometry=-1,filters=-1,visibility=-1;
    if(geometry!=fb_grid_geometry_epoch || filters!=fb_filter_epoch || visibility!=fb_visibility_epoch) {
        geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;visibility=fb_visibility_epoch;
        memfree(fb_render_access);fb_render_access_count=fb_node_count;
        fb_render_access=memalloc(fb_node_count*sizeof(int));
        for(int n=0;n<fb_node_count;n++) {
            if(fb_pick_allowed(n,FB_PICK_LOCKED))fb_render_access[n]|=1;
            if(fb_pick_allowed(n,0))fb_render_access[n]|=2;
        }
    }
    if(node<0 || node>=fb_render_access_count)return FALSE;
    if(flags&FB_PICK_LOCKED)return (fb_render_access[node]&1)!=0;
    return (fb_render_access[node]&2)!=0;
}
int fb_render_visible(int model,int id)
{
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].model==model && fb_nodes[n].id==id)return fb_pick_allowed(n,FB_PICK_LOCKED);
    return FALSE;
}
void fb_filter_patch(int model,int id)
{
    patchinfo_t info;int total=patch_getmesh(model,id,__NULL__,0,&info);
    if(total<1 || total>65536)return;
    if(info.cpwidth<2 || info.cpheight<2 || info.cpwidth*info.cpheight!=total)return;
    patchvert_t *mesh=memalloc(total*sizeof(patchvert_t));
    if(patch_getmesh(model,id,mesh,total,&info)!=total){memfree(mesh);return;}
    string shader="ftebroom/flatface";
    if(!autocvar_ftebroom_render_faces) {
        string material=fb_material_key(info.shadername),preview="";
        if(fb_shader_previews)preview=fb_shader_previews[material];if(preview=="")preview=strcat("textures/",material);
        shader=strcat("ftebroom/opaque/",material);
        shaderforname(shader,strcat("{\ncull none\nsort opaque\n{\nmap ",fb_map_quote(preview),"\nrgbgen vertex\ndepthwrite\n}\n}"));
    }
    for(int y=0;y<info.cpheight-1;y++)for(int x=0;x<info.cpwidth-1;x++) {
        int at=y*info.cpwidth+x;int corners[4];corners[0]=at;corners[1]=at+1;corners[2]=at+info.cpwidth+1;corners[3]=at+info.cpwidth;
        vector normal=normalize(crossproduct(mesh[corners[1]].xyz-mesh[at].xyz,mesh[corners[3]].xyz-mesh[at].xyz));
        float shade=1;if(autocvar_ftebroom_shade_faces)shade=0.6+0.4*fabs(normal*normalize('0.3 -0.5 1'));
        vector color='0.65 0.68 0.72'*shade;if(!autocvar_ftebroom_render_faces)color='1 1 1'*shade;
        R_BeginPolygon(shader,3);
        for(int c=0;c<4;c++){patchvert_t v=mesh[corners[c]];R_PolygonVertex(v.xyz,[v.s,v.t,0],color,1);}
        R_EndPolygon();
    }
    memfree(mesh);
}
typedef struct {
    int node,count,brushEnd;string shader;float alpha,radius,distance;
    vector color,center,normal,brushCenter;float brushRadius;
    vector *points;vector *uv;
    trisoup_simple_vert_t *vertices;int *triangles;
} fb_render_face_t;
fb_render_face_t *fb_render_faces;int fb_render_face_count,fb_render_face_capacity;
vector *fb_render_points;
vector *fb_render_uv;
trisoup_simple_vert_t *fb_render_vertices;
int *fb_render_triangles;int fb_render_vertex_capacity;
void fb_render_faces_prepare()
{
    static int geometry=-1,filters=-1,selection=-1;
    if(geometry==fb_grid_geometry_epoch && filters==fb_filter_epoch&&selection==fb_selection_epoch)return;
    geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;selection=fb_selection_epoch;
    for(int i=0;i<fb_render_face_count;i++) {
        strunzone(fb_render_faces[i].shader);
    }
    fb_render_face_count=0;
    shaderforname("ftebroom/flatface","{\ncull none\nsort opaque\n{\nmap $whiteimage\nrgbgen vertex\ndepthwrite\n}\n}");
    shaderforname("ftebroom/flattransparent","{\ncull none\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc blend\n}\n}");
    if(autocvar_ftebroom_render_faces==2)return;
    static brushface_t faces[64];static vector points[64],brushBounds[2];
    // Reuse four packed buffers rather than fragmenting the QC heap per face.
    int required=0,vertexOffset=0;
    for(int n=2;n<fb_node_count;n++) {
        if(fb_nodes[n].kind!=FB_NODE_BRUSH)continue;
        if(!fb_filter_custom(n)&&!brush_isselected(fb_nodes[n].model,fb_nodes[n].id))continue;
        int count=brush_get(fb_nodes[n].model,fb_nodes[n].id,__NULL__,0,__NULL__);
        for(int f=0;f<min(count,64);f++) {
            int vertices=brush_getfacepoints(fb_nodes[n].model,fb_nodes[n].id,f+1,points,64);
            if(vertices>=3 && vertices<=64)required+=vertices;
        }
    }
    if(required>fb_render_vertex_capacity) {
        fb_render_vertex_capacity=max(required,max(1024,fb_render_vertex_capacity*2));
        memfree(fb_render_points);memfree(fb_render_uv);
        memfree(fb_render_vertices);memfree(fb_render_triangles);
        fb_render_points=memalloc(fb_render_vertex_capacity*sizeof(vector));
        fb_render_uv=memalloc(fb_render_vertex_capacity*sizeof(vector));
        fb_render_vertices=memalloc(fb_render_vertex_capacity*sizeof(trisoup_simple_vert_t));
        fb_render_triangles=memalloc(fb_render_vertex_capacity*3*sizeof(int));
    }
    for(int n=2;n<fb_node_count;n++) {
        if(fb_nodes[n].kind!=FB_NODE_BRUSH)continue;
        if(!fb_filter_custom(n)&&!brush_isselected(fb_nodes[n].model,fb_nodes[n].id))continue;
        int contents,count=brush_get(fb_nodes[n].model,fb_nodes[n].id,faces,64,&contents);
        int hasBounds=brush_getfacepoints(fb_nodes[n].model,fb_nodes[n].id,0,brushBounds,2);
        vector brushCenter=(brushBounds[0]+brushBounds[1])*0.5;
        float brushRadius=hasBounds==2?vlen(brushBounds[1]-brushBounds[0])*0.5+0.1:1000000000.0;
        int firstFace=fb_render_face_count;
        for(int f=0;f<min(count,64);f++) {
            int vertices=brush_getfacepoints(fb_nodes[n].model,fb_nodes[n].id,f+1,points,64);
            if(vertices<3 || vertices>64)continue;
            if(fb_render_face_count==fb_render_face_capacity) {
                int capacity=max(256,fb_render_face_capacity*2);
                fb_render_face_t *items=memalloc(capacity*sizeof(fb_render_face_t));
                if(fb_render_face_count)memcpy(items,fb_render_faces,fb_render_face_count*sizeof(fb_render_face_t));
                memfree(fb_render_faces);fb_render_faces=items;fb_render_face_capacity=capacity;
            }
            fb_render_face_t *item=&fb_render_faces[fb_render_face_count++];
            brushface_t face=faces[f];int tags=fb_filter_face_tags(&face,contents);
            if(fb_filter_tags[n]&FB_TAG_TRIGGER)tags|=FB_TAG_TRIGGER;
            float alpha=1;if(tags&(FB_TAG_TRIGGER|FB_TAG_CLIP|FB_TAG_CAULK|FB_TAG_HINT|FB_TAG_TRANSLUCENT|FB_TAG_AREAPORTAL|FB_TAG_CLUSTERPORTAL))alpha=0.35;
            float shade=1;if(autocvar_ftebroom_shade_faces)shade=0.6+0.4*fabs(face.planenormal*normalize('0.3 -0.5 1'));
            vector color='0.65 0.68 0.72'*shade,size='1 1 1';string shader="ftebroom/flatface";
            if(alpha<1)shader="ftebroom/flattransparent";
            if(!autocvar_ftebroom_render_faces) {
                string material=fb_material_key(face.shadername),preview="";
                if(fb_shader_previews)preview=fb_shader_previews[material];if(preview=="")preview=strcat("textures/",material);
                shader=strcat(alpha<1?"ftebroom/filtered/":"ftebroom/opaque/",material);
                shaderforname(shader,strcat("{\ncull none\n",alpha<1?"":"sort opaque\n","{\nmap ",fb_map_quote(preview),"\nrgbgen vertex\nalphagen vertex\n",alpha<1?"blendfunc blend\n":"depthwrite\n","}\n}"));
                size=drawgetimagesize(shader);size_x=max(1,size_x);size_y=max(1,size_y);color='1 1 1'*shade;
                if(material=="ftebroom/grid")color*=0.8;
            }
            item.points=&fb_render_points[vertexOffset];item.uv=&fb_render_uv[vertexOffset];
            item.vertices=&fb_render_vertices[vertexOffset];
            item.triangles=&fb_render_triangles[vertexOffset*3];vertexOffset+=vertices;
            item.brushCenter=brushCenter;item.brushRadius=brushRadius;
            item.node=n;item.count=vertices;item.shader=strzone(shader);item.alpha=alpha;item.color=color;
            item.normal=face.planenormal;item.distance=face.planedist;item.center='0 0 0';item.radius=0;
            for(int p=0;p<vertices;p++) {
                item.points[p]=points[p];item.center+=points[p];
                item.uv[p]=[(points[p]*face.sdir+face.sbias)/size_x,(points[p]*face.tdir+face.tbias)/size_y,0];
                for(int axis=0;axis<3;axis++){item.vertices[p].xyz[axis]=points[p][axis];item.vertices[p].rgba[axis]=color[axis];}
                item.vertices[p].rgba[3]=alpha;item.vertices[p].st[0]=item.uv[p][0];item.vertices[p].st[1]=item.uv[p][1];
                if(p>=2){item.triangles[(p-2)*3]=0;item.triangles[(p-2)*3+1]=p-1;item.triangles[(p-2)*3+2]=p;}
            }
            item.center/=(float)vertices;
            for(int p=0;p<vertices;p++)item.radius=max(item.radius,vlen(points[p]-item.center));
        }
        for(int i=firstFace;i<fb_render_face_count;i++)fb_render_faces[i].brushEnd=fb_render_face_count;
    }
}
void fb_filter_draw()
{
    if(cvar("ca_brush_view"))return;
    fb_render_faces_prepare();
    vector eye=getproperty(VF_ORIGIN),fov=getproperty(VF_FOV);makevectors(getproperty(VF_ANGLES));
    vector forward=v_forward,right=v_right,up=v_up;
    float tx=tan(fov_x*M_PI/360),ty=tan(fov_y*M_PI/360);
    int previous=-1,visible=FALSE;
    for(int i=0;i<fb_render_face_count;i++) {
        fb_render_face_t *face=&fb_render_faces[i];
        if(face.node!=previous) {
            previous=face.node;fb_node_t node=fb_nodes[previous];
            visible=fb_render_allowed(previous,FB_PICK_LOCKED);
            if(fb_preview_replaces(node.model,node.id))visible=FALSE;
            if(visible && !fb_filter_custom(previous))visible=brush_isselected(node.model,node.id)!=0;
            // Reject an entire off-screen brush before testing each of its faces.
            vector brushDelta=face.brushCenter-eye;float depth=brushDelta*forward,extent=face.brushRadius;
            if(depth+extent<0)visible=FALSE;
            if(fabs(brushDelta*right)>depth*tx+extent*(1+tx)||fabs(brushDelta*up)>depth*ty+extent*(1+ty))visible=FALSE;
        }
        // Skip the cached face range of a rejected brush in one step.
        if(!visible){i=face.brushEnd-1;continue;}
        if(face.alpha==1 && eye*face.normal<face.distance-0.01)continue;
        vector delta=face.center-eye;float z=delta*forward,radius=face.radius;
        if(z+radius<0)continue;
        if(fabs(delta*right)>z*tx+radius*(1+tx) || fabs(delta*up)>z*ty+radius*(1+ty))continue;
        addtrisoup_simple(face.shader,3,face.vertices,face.triangles,(face.count-2)*3);
    }
    fb_document_lists_prepare();
    int wire=autocvar_ftebroom_render_faces==2;
    for(int entry=0;entry<(wire?fb_node_count:fb_patch_node_count);entry++) {
        int n=wire?entry:fb_patch_nodes[entry];
        if(fb_nodes[n].kind<FB_NODE_BRUSH)continue;
        if(!fb_render_allowed(n,FB_PICK_LOCKED))continue;
        fb_node_t node=fb_nodes[n];if(fb_preview_replaces(node.model,node.id))continue;
        if(node.kind==FB_NODE_PATCH && (fb_filter_custom(n) || brush_isselected(node.model,node.id)) && autocvar_ftebroom_render_faces!=2)fb_filter_patch(node.model,node.id);
        if(autocvar_ftebroom_render_faces==2)DrawEngineBrushWireframe(node.model,node.id);
    }
}
string fb_structural_material(string material)
{
    brushface_t face={0};face.shadername=material;int tags=fb_filter_face_tags(&face,0);
    string key=fb_material_key(material);
    if(!(tags&(FB_TAG_CLIP|FB_TAG_CAULK|FB_TAG_HINT)) && key!="common/trigger")return material;
    string replacement=cvar_string("ca_newbrushtexture");face.shadername=replacement;tags=fb_filter_face_tags(&face,0);
    if(tags&(FB_TAG_CLIP|FB_TAG_CAULK|FB_TAG_HINT) || fb_material_key(replacement)=="common/trigger")replacement="ftebroom/grid";
    return replacement;
}
void fb_filter_tag(string type,int enabled)
{
    if(type!="trigger" && type!="clip" && type!="caulk" && type!="hint" && type!="detail")return;
    string tag=strzone(type);fb_tool_refresh();fb_document_tick();
    if(tag=="trigger") {
        if(enabled){fb_tx_begin("Create trigger");fb_entity_create("trigger_multiple",TRUE,'0 0 0');cvar_set("ca_newbrushtexture","common/trigger");fb_material_edit("material",0);fb_tx_commit();}
        else fb_organize_action("structural",0,"");strunzone(tag);return;
    }
    fb_tx_begin("Change brush tag");static brushface_t faces[64];
    for(int b=0;b<selectedbrushcount;b++) {
        selbrush_t item=selectedbrushes[b];int contents,count=brush_get(item.model,item.id,faces,64,&contents);if(count<4||count>64)continue;
        if(tag=="detail") {if(enabled)contents|=134217728i;else contents&=~134217728i;}
        else for(int f=0;f<count;f++)if(selbrush_whole(&item)||selbrush_face(&item,f)) {
            if(enabled)faces[f].shadername=strcat("common/",tag);
            else {int bits=fb_filter_face_tags(&faces[f],contents),bit=tag=="clip"?FB_TAG_CLIP:(tag=="caulk"?FB_TAG_CAULK:FB_TAG_HINT);if(bits&bit)faces[f].shadername=fb_structural_material(faces[f].shadername);}
        }
        brush_history_edit(item.model,item.id,faces,count,contents);
    }
    fb_tx_commit();strunzone(tag);
}
