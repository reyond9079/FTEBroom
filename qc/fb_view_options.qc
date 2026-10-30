// SPDX-License-Identifier: GPL-2.0-or-later
var float autocvar_ftebroom_visibility_workers=1;
var float autocvar_ftebroom_opaque_batches=1;
var float autocvar_ftebroom_show_brushes=1,autocvar_ftebroom_show_points=1,autocvar_ftebroom_show_models=1;
var float autocvar_ftebroom_show_entity_names=1,autocvar_ftebroom_show_entity_bounds=1,autocvar_ftebroom_show_group_bounds=1,autocvar_ftebroom_show_brush_entity_bounds=1;
var float autocvar_ftebroom_render_faces=0,autocvar_ftebroom_shade_faces=1,autocvar_ftebroom_show_edges=1;
var float autocvar_ftebroom_tool_opacity=.35;
float fb_surface_shade(vector normal){return autocvar_ftebroom_shade_faces?0.6+0.4*fabs(normal*normalize('0.3 -0.5 1')):1;}
var float autocvar_ftebroom_bounds_visible=0;
var float autocvar_ftebroom_hidden_tags=0,autocvar_ftebroom_entity_links=1;
var string autocvar_ftebroom_hidden_classes="";
int *fb_filter_tags;int fb_filter_tag_count;
hashtable fb_shader_tags,fb_shader_previews,fb_shader_loose;
string fb_material_preview_key(string source)
{
    string key=fb_material_key(source);
    if(cvar("ftebroom_game_profile")!=3 || !fb_shader_previews)return key;
    string image=fb_shader_previews[key];
    if(image!="" && substring(image,0,27)!="textures/_ftebroom_missing/")return key;
    int start=0,slash;while((slash=strstrofs(key,"/",start))>=0)start=slash+1;
    string leaf=substring(key,start,-1);
    if(fb_shader_loose)if((int)fb_shader_loose[leaf])return leaf;
    return key;
}
int fb_sky_material(string material)
{
    string name=fb_material_preview_key(material);
    if(fb_shader_tags)if((int)fb_shader_tags[name]&512)return TRUE;
    if(cvar("ftebroom_quake1") && substring(name,0,3)=="sky")return TRUE;
    return substring(name,0,4)=="sky/" || substring(name,0,6)=="skies/";
}
enum : int {FB_TAG_TRIGGER=1,FB_TAG_CLIP=2,FB_TAG_CAULK=4,FB_TAG_HINT=8,FB_TAG_DETAIL=16,FB_TAG_LIQUID=32,FB_TAG_TRANSLUCENT=64,FB_TAG_AREAPORTAL=128,FB_TAG_CLUSTERPORTAL=256};
hashtable fb_shader_opacity,fb_shader_masked,fb_shader_blend,fb_shader_alpha_test;
void fb_filter_materials_load()
{
    if(fb_shader_tags)hash_destroytab(fb_shader_tags);if(fb_shader_previews)hash_destroytab(fb_shader_previews);
    if(fb_shader_loose)hash_destroytab(fb_shader_loose);fb_shader_loose=hash_createtab(512,EV_INTEGER);
    fb_shader_tags=hash_createtab(512,EV_INTEGER);fb_shader_previews=hash_createtab(512,EV_STRING);
    if(fb_shader_opacity)hash_destroytab(fb_shader_opacity);if(fb_shader_masked)hash_destroytab(fb_shader_masked);if(fb_shader_blend)hash_destroytab(fb_shader_blend);if(fb_shader_alpha_test)hash_destroytab(fb_shader_alpha_test);
    fb_shader_opacity=hash_createtab(512,EV_FLOAT);fb_shader_masked=hash_createtab(512,EV_INTEGER);fb_shader_blend=hash_createtab(512,EV_STRING);fb_shader_alpha_test=hash_createtab(512,EV_STRING);
    filestream file=fopen("ftebroom-material-tags.txt",FILE_READ);if((float)file<0)return;
    string line;while((line=fgets(file))){int count=tokenize(line);if(count>=2){fb_shader_tags[argv(0)]=stoi(argv(1));fb_shader_previews[argv(0)]=argv(2);fb_shader_opacity[argv(0)]=count>=4?stof(argv(3)):1.0;fb_shader_masked[argv(0)]=stoi(argv(4));fb_shader_blend[argv(0)]=argv(5);fb_shader_alpha_test[argv(0)]=argv(6);fb_shader_loose[argv(0)]=count>=8?stoi(argv(7)):0i;}}
    fclose(file);fb_filter_epoch++;
}
int fb_filter_face_tags(brushface_t *face,int contents)
{
    string name=fb_material_preview_key(face->shadername);int tags=0;
    if(fb_shader_tags)tags=fb_shader_tags[name];
    if(substring(name,0,7)=="common/") {
        if(strstrofs(name,"clip",7)>=0)tags|=FB_TAG_CLIP;
        if(strstrofs(name,"caulk",7)>=0)tags|=FB_TAG_CAULK;
        if(substring(name,7,4)=="hint")tags|=FB_TAG_HINT;
    }
    if(cvar("ftebroom_quake1")||cvar("ftebroom_game_profile")==3){
        if(name=="clip"||name=="playerclip"||name=="monsterclip"||name=="botclip")tags|=FB_TAG_CLIP;
        if(name=="skip"||name=="nodraw"||name=="caulk"||name=="null"||name=="origin"||name=="bevel"||name=="bevel2")tags|=FB_TAG_CAULK;
        if(name=="hint"||name=="hintskip")tags|=FB_TAG_HINT;
        if(name=="trigger")tags|=FB_TAG_TRIGGER;
        if(substring(name,0,1)=="*")tags|=FB_TAG_LIQUID;
    }
    if(cvar("ftebroom_game_profile")==3){
        if(face->surfaceflags&640i)tags|=FB_TAG_CAULK;
        if(face->surfaceflags&256i)tags|=FB_TAG_HINT;
    }
    if(contents&134217728i)tags|=FB_TAG_DETAIL;
    if(contents&536870912i)tags|=FB_TAG_TRANSLUCENT;
    return tags;
}
int fb_face_occluder(brushface_t *face,int contents,int node,int model)
{
    if(!cvar("ftebroom_quake1"))return 0;
    string name=fb_material_key(face->shadername);if(substring(name,0,1)=="{" || substring(name,0,1)=="*")return 0;
    int tags=fb_filter_face_tags(face,contents);if(tags&(FB_TAG_TRIGGER|FB_TAG_CLIP|FB_TAG_CAULK|FB_TAG_HINT|FB_TAG_TRANSLUCENT|FB_TAG_AREAPORTAL|FB_TAG_CLUSTERPORTAL))return 0;
    if(node<fb_filter_tag_count)if(fb_filter_tags[node]&FB_TAG_TRIGGER)return 0;
    if(model==1 && ((face->surfaceflags&4)||fb_sky_material(name)))return 2;
    return 3;
}
void fb_filter_prepare()
{
    float profile81Time=gettime(1);
    static int epoch=-1,materials=-1;static string previous="";
    string signature=sprintf("%g %g %g %g %g %g",autocvar_ftebroom_show_brushes,autocvar_ftebroom_show_points,autocvar_ftebroom_show_models,autocvar_ftebroom_render_faces,autocvar_ftebroom_shade_faces,autocvar_ftebroom_show_edges);
    signature=strcat(signature,sprintf(" %g %g %g %g %g %g %i",autocvar_ftebroom_hidden_tags,autocvar_ftebroom_entity_links,autocvar_ftebroom_show_entity_names,autocvar_ftebroom_show_entity_bounds,autocvar_ftebroom_show_group_bounds,autocvar_ftebroom_show_brush_entity_bounds,fb_open_group)," ",autocvar_ftebroom_hidden_classes,ftos(autocvar_ftebroom_bounds_visible));
    signature=strcat(signature," fog=",cvar_string("r_fog_enabled")," water=",cvar_string("r_wateralpha")," tools=",cvar_string("ftebroom_tool_opacity"));
    signature=strcat(signature," ",cvar_string("r_lavaalpha")," ",cvar_string("r_slimealpha")," ",cvar_string("r_telealpha"));
    if(signature!=previous){strunzone(previous);previous=strzone(signature);fb_filter_epoch++;}
    int materialEpoch=cvar("ftebroom_material_tag_epoch");
    if(materialEpoch!=materials){materials=materialEpoch;fb_filter_materials_load();epoch=-1;}
    if(epoch==fb_grid_geometry_epoch)return;epoch=fb_grid_geometry_epoch;
    memfree(fb_filter_tags);fb_filter_tag_count=fb_node_count;fb_filter_tags=memalloc(fb_node_capacity*sizeof(int));
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
    fb_commit_profile("fb_filter_prepare",profile81Time);
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
    // Classic MAPs use one material/shading path for world, group and selected
    // brushes. Native lightmap shaders must not brighten only some surfaces.
    if(cvar("ftebroom_classic_map"))return TRUE;
    // Map-edit submodels have no server entity submitting their surfaces.
    // Draw them explicitly, independently of selection and point-model visibility.
    if(fb_nodes[node].kind>=FB_NODE_BRUSH && fb_nodes[node].model!=1)return TRUE;
    return autocvar_ftebroom_render_faces!=0 || !autocvar_ftebroom_shade_faces || (fb_filter_tags[node] & (FB_TAG_TRIGGER|FB_TAG_CLIP|FB_TAG_CAULK|FB_TAG_HINT|FB_TAG_LIQUID|FB_TAG_TRANSLUCENT|FB_TAG_AREAPORTAL|FB_TAG_CLUSTERPORTAL));
}
int *fb_render_access;int fb_render_access_count;
int fb_render_allowed(int node,int flags)
{
    static int geometry=-1,filters=-1,visibility=-1;
    if(geometry!=fb_grid_geometry_epoch || filters!=fb_filter_epoch || visibility!=fb_visibility_epoch) {
        geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;visibility=fb_visibility_epoch;
        fb_occlusion_revision++;memfree(fb_render_access);fb_render_access_count=fb_node_count;
        fb_render_access=memalloc(fb_node_capacity*sizeof(int));
        for(int n=0;n<fb_node_count;n++) {
            if(fb_pick_allowed(n,FB_PICK_LOCKED))fb_render_access[n]|=1;
            if(n>=fb_filter_tag_count || !(fb_filter_tags[n]&FB_TAG_TRIGGER))fb_render_access[n]|=4;
            if(fb_pick_allowed(n,0))fb_render_access[n]|=2;
            if(fb_nodes[n].kind==FB_NODE_BRUSH && (fb_filter_custom(n)||brush_isselected(fb_nodes[n].model,fb_nodes[n].id)))fb_render_access[n]|=16;
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
// Keep the static map's native passes during a large-map gesture. The live
// access table remains untouched for selection/UI and dynamic preview drawing.
void fb_static_preview_prepare()
{
    fb_render_allowed(0,FB_PICK_LOCKED);fb_incremental_access();
    fb_static_preview_supported=fb_grid_face_count>=(cvar("ftebroom_classic_map")?4096:30000) && cvar("ftebroom_cached_meshes");
    if(!fb_static_preview_supported)return;
    static int capacity;static string signature="",previousMask="";
    string key=sprintf("%i %i %i %i %i %g",fb_grid_geometry_epoch,fb_filter_epoch,fb_visibility_epoch,fb_selection_epoch,fb_node_count,fb_active);key=strcat(key," ",itos(fb_incremental_epoch));
    key=strcat(key," ",itos(fb_vertex_preview_epoch));
    key=strcat(key,sprintf(" %i %i %i %i %i",fb_transform_preview,fb_transform_duplicate,fb_vertex_preview_valid,fb_generator_count,fb_generator_hide_original));
    key=strcat(key,sprintf(" %i %v %g",fb_clip_count,fb_clip_normal,fb_clip_dist));
    key=strcat(key," ",cvar_string("ftebroom_tool"));
    if(key==signature)return;strunzone(signature);signature=strzone(key);
    if(capacity<fb_render_access_count){capacity=max(fb_render_access_count,fb_node_capacity);memfree(fb_static_access);fb_static_access=memalloc(capacity*sizeof(int));}
    fb_static_access_count=fb_render_access_count;memcpy(fb_static_access,fb_render_access,fb_static_access_count*sizeof(int));
    string mask="";
    for(int pass=0;pass<2;pass++) {
        int count=pass?fb_generator_original_count:selectedbrushcount;
        if(pass && !fb_generator_count)continue;
        for(int b=0;b<count;b++) {
            int model,id;if(pass){model=fb_generator_original[b].model;id=fb_generator_original[b].id;}else{model=selectedbrushes[b].model;id=selectedbrushes[b].id;}
            int n=fb_document_brush_node(model,id);if(n<2 || n>=fb_static_access_count)continue;
            if(!(fb_static_access[n]&1))continue;
            if(!fb_preview_replaces(model,id))continue;
            fb_static_access[n]&=~1i;mask=strcat(mask,itos(n),",");
        }
    }
    if(mask!=previousMask){strunzone(previousMask);previousMask=strzone(mask);fb_occlusion_revision++;}
}
// FTE returns a registered shader without replacing its generator script.
// A new Q1 resource generation must not reuse a previous missing/old image.
string fb_surface_material_shader(string material,float alpha)
{
    string name=strcat(alpha<1?"ftebroom/filtered/":"ftebroom/opaque/",material);
    name=strcat(name,"#assets_",cvar_string("ftebroom_material_tag_epoch"));
    return name;
}
float fb_surface_opacity(string source,int tags)
{
    if(tags&(FB_TAG_TRIGGER|FB_TAG_CLIP|FB_TAG_CAULK|FB_TAG_HINT|FB_TAG_AREAPORTAL|FB_TAG_CLUSTERPORTAL))return bound(0,autocvar_ftebroom_tool_opacity,1);
    string key=fb_material_preview_key(source);float opacity=1;if(fb_shader_opacity)opacity=(float)hash_get(fb_shader_opacity,key,1.0);
    if(opacity<1)return opacity;
    if(cvar("ftebroom_quake1")&&(tags&FB_TAG_LIQUID)){
        string alphaVar="r_wateralpha";
        if(substring(key,0,5)=="*lava")alphaVar="r_lavaalpha";
        else if(substring(key,0,6)=="*slime")alphaVar="r_slimealpha";
        else if(substring(key,0,5)=="*tele")alphaVar="r_telealpha";
        if(cvar_string(alphaVar)=="")alphaVar="r_wateralpha";return bound(0,cvar(alphaVar),1);
    }
    return 1;
}
// Every editable preview keeps masked WAD texels, including selection/drag layers.
string fb_surface_preview_shader(string source,float alpha)
{
    string material=fb_material_key(source),preview="";
    string lookup=fb_material_preview_key(material);
    if(fb_shader_previews)preview=fb_shader_previews[lookup];
    if(preview=="")preview=cvar("ftebroom_classic_map")?"textures/ftebroom/missing.tga":strcat("textures/",material);
    string shader=fb_surface_material_shader(material,alpha);
    int masked=cvar("ftebroom_quake1") && substring(material,0,1)=="{";if(fb_shader_masked)if((int)fb_shader_masked[material])masked=TRUE;
    string blend="";if(fb_shader_blend)blend=fb_shader_blend[material];
    string depth="depthwrite\n";if(alpha<1 || blend!="")depth=strcat("blendfunc ",blend!=""?blend:"blend","\n");
    string program="";if(masked){string test="ge128";if(fb_shader_alpha_test)if(fb_shader_alpha_test[material]!="")test=fb_shader_alpha_test[material];if(alpha<1)program=strcat("program ftebroom_mask#",test=="lt128"?"LT128":test=="gt0"?"GT0":"GE128","\n");else depth=strcat("alphafunc ",test,"\n",depth);}
    shaderforname(shader,strcat("{\ncull none\n",program,(alpha<1 || blend!="")?"sort blend\n":"sort opaque\n","{\nmap ",fb_map_quote(preview),"\nrgbgen vertex\nalphagen vertex\n",depth,"}\n}"));
    return shader;
}
vector fb_material_image_size(string source)
{
    // Raw shader paths may already refer to a missing-image shader in FTE.
    return drawgetimagesize(cvar("ftebroom_game_profile")==3?fb_surface_preview_shader(source,1):source);
}
void fb_filter_patch(int model,int id)
{
    patchinfo_t info;int total=patch_getmesh(model,id,__NULL__,0,&info);
    if(total<1 || total>65536)return;
    if(info.cpwidth<2 || info.cpheight<2 || info.cpwidth*info.cpheight!=total)return;
    patchvert_t *mesh=memalloc(total*sizeof(patchvert_t));
    if(patch_getmesh(model,id,mesh,total,&info)!=total){memfree(mesh);return;}
    brushface_t patchFace={0};patchFace.shadername=info.shadername;float opacity=fb_surface_opacity(info.shadername,fb_filter_face_tags(&patchFace,0));
    string shader=opacity<1?"ftebroom/flattransparent":"ftebroom/flatface";
    if(!autocvar_ftebroom_render_faces) {
        shader=fb_surface_preview_shader(info.shadername,opacity);
    }
    for(int y=0;y<info.cpheight-1;y++)for(int x=0;x<info.cpwidth-1;x++) {
        int at=y*info.cpwidth+x;int corners[4];corners[0]=at;corners[1]=at+1;corners[2]=at+info.cpwidth+1;corners[3]=at+info.cpwidth;
        vector normal=normalize(crossproduct(mesh[corners[1]].xyz-mesh[at].xyz,mesh[corners[3]].xyz-mesh[at].xyz));
        float shade=1;if(autocvar_ftebroom_shade_faces)shade=0.6+0.4*fabs(normal*normalize('0.3 -0.5 1'));
        vector color='0.65 0.68 0.72'*shade;if(!autocvar_ftebroom_render_faces)color='1 1 1'*shade;
        R_BeginPolygon(shader,3);
        for(int c=0;c<4;c++){patchvert_t v=mesh[corners[c]];R_PolygonVertex(v.xyz,[v.s,v.t,0],color,opacity);}
        R_EndPolygon();
    }
    memfree(mesh);
}
typedef struct {
    int node,count,brushEnd;string shader;float alpha,radius,distance;
    vector color,center,normal,brushCenter;float brushRadius;
    vector *points;vector *uv;
    trisoup_simple_vert_t *vertices;int *triangles;int enabled;
} fb_render_face_t;
int(fb_render_face_t *faces,int count,int *access,int accessCount,int stride,...) ftebroom_draw_faces = #0:ftebroom_draw_faces;
int(fb_render_face_t *faces,int count,int *access,int accessCount,int stride,int revision,...) ftebroom_draw_faces_cached = #0:ftebroom_draw_faces_cached;
int(vector *source,int count,fb_render_face_t *face,vector *params) ftebroom_build_face=#0:ftebroom_build_face;
int fb_face_upload_epoch;
// Large-map GPU visibility keeps source geometry and sampling unchanged.
// Engines without the required capabilities retain the conservative CPU path.
var float autocvar_ftebroom_bindless_materials=1;
var float autocvar_ftebroom_gpu_face_cull=1,autocvar_ftebroom_gpu_grid_cull=1;
var float autocvar_ftebroom_gpu_visibility=1,autocvar_ftebroom_global_order=1;
var float autocvar_ftebroom_native_faces=1;
var float autocvar_ftebroom_merge_missing_materials=1;
fb_render_face_t *fb_render_faces;int fb_render_face_count,fb_render_face_capacity;
vector *fb_render_points;
vector *fb_render_uv;
trisoup_simple_vert_t *fb_render_vertices;
int *fb_render_triangles;int fb_render_vertex_capacity;
typedef struct fb_render_shader_s {string name;struct fb_render_shader_s *next;} fb_render_shader_t;
fb_render_shader_t *fb_render_shader_names;
hashtable fb_render_shader_pool;
int *fb_render_first;
int fb_render_append_brush(int n,int vertexOffset)
{
    static brushface_t faces[64];static vector points[64],brushBounds[2];
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
            float alpha=fb_surface_opacity(face.shadername,tags);
            float shade=1;if(autocvar_ftebroom_shade_faces)shade=0.6+0.4*fabs(face.planenormal*normalize('0.3 -0.5 1'));
            vector color='0.65 0.68 0.72'*shade,size='1 1 1';string shader="ftebroom/flatface";
            if(alpha<1)shader="ftebroom/flattransparent";
            if(!autocvar_ftebroom_render_faces) {
                string material=fb_material_key(face.shadername);
                shader=fb_surface_preview_shader(material,alpha);
                size=drawgetimagesize(shader);size_x=max(1,size_x);size_y=max(1,size_y);color='1 1 1'*shade;
                if(material=="ftebroom/grid")color*=0.8;
            }
            item.points=&fb_render_points[vertexOffset];item.uv=&fb_render_uv[vertexOffset];
            item.vertices=&fb_render_vertices[vertexOffset];
            item.triangles=&fb_render_triangles[vertexOffset*3];vertexOffset+=vertices;
            item.brushCenter=brushCenter;item.brushRadius=brushRadius;
            item.enabled=fb_incremental_enabled()?2i:(fb_filter_custom(n) || brush_isselected(fb_nodes[n].model,fb_nodes[n].id)!=0);
            fb_render_shader_t *shared=fb_render_shader_pool[shader];
            if(!shared){shared=memalloc(sizeof(fb_render_shader_t));shared.name=strzone(shader);shared.next=fb_render_shader_names;fb_render_shader_names=shared;fb_render_shader_pool[shader]=shared;}
            item.node=n;item.count=vertices;item.shader=shared.name;item.alpha=alpha;item.color=color;
            item.normal=face.planenormal;item.distance=face.planedist;item.center='0 0 0';item.radius=0;
            if(fb_node_count>=10000 && checkbuiltin(ftebroom_build_face)){
                static vector params[5];params[0]=face.sdir;params[1]=face.tdir;params[2]=[face.sbias,face.tbias,0];params[3]=[size_x,size_y,alpha];params[4]=color;
                if(ftebroom_build_face(points,vertices,item,params))continue;
            }
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

    return vertexOffset;
}
void fb_render_faces_prepare()
{
    float profile81Time=gettime(1);
    static int geometry=-1,filters=-1,selection=-1;
    static string style;string nextStyle=strcat(cvar_string("ftebroom_render_faces")," ",cvar_string("ftebroom_shade_faces")," ",cvar_string("ftebroom_material_tag_epoch")," ",cvar_string("r_wateralpha"));
    nextStyle=strcat(nextStyle," ",cvar_string("r_lavaalpha")," ",cvar_string("r_slimealpha")," ",cvar_string("r_telealpha"));
    nextStyle=strcat(nextStyle," ",cvar_string("ftebroom_tool_opacity"));
    if(geometry==fb_grid_geometry_epoch && ((fb_incremental_enabled()||cvar("ftebroom_classic_map"))?nextStyle==style:(filters==fb_filter_epoch&&selection==fb_selection_epoch)))return;
    strunzone(style);style=strzone(nextStyle);
    geometry=fb_grid_geometry_epoch;filters=fb_filter_epoch;selection=fb_selection_epoch;fb_face_upload_epoch++;
    while(fb_render_shader_names){fb_render_shader_t *entry=fb_render_shader_names;fb_render_shader_names=entry.next;strunzone(entry.name);memfree(entry);}
    if(fb_render_shader_pool)hash_destroytab(fb_render_shader_pool);fb_render_shader_pool=hash_createtab(1024,EV_POINTER);
    fb_render_face_count=0;memfree(fb_render_first);fb_render_first=memalloc(fb_node_capacity*sizeof(int));
    shaderforname("ftebroom/flatface","{\ncull none\nsort opaque\n{\nmap $whiteimage\nrgbgen vertex\ndepthwrite\n}\n}");
    shaderforname("ftebroom/flattransparent","{\ncull none\n{\nmap $whiteimage\nrgbgen vertex\nalphagen vertex\nblendfunc blend\n}\n}");
    if(autocvar_ftebroom_render_faces==2)return;
    static vector points[64];
    // Reuse four packed buffers rather than fragmenting the QC heap per face.
    int required=0,vertexOffset=0;
    for(int n=2;n<fb_node_count;n++) {
        if(fb_nodes[n].kind!=FB_NODE_BRUSH)continue;
        if(!fb_incremental_enabled() && !fb_filter_custom(n)&&!brush_isselected(fb_nodes[n].model,fb_nodes[n].id))continue;
        int count=brush_get(fb_nodes[n].model,fb_nodes[n].id,__NULL__,0,__NULL__);
        for(int f=0;f<min(count,64);f++) {
            int vertices=brush_getfacepoints(fb_nodes[n].model,fb_nodes[n].id,f+1,points,64);
            if(vertices>=3 && vertices<=64)required+=vertices;
        }
    }
    if(required>fb_render_vertex_capacity) {
        // Reserve modest edit space rather than doubling all textured-face
        // buffers when the first brush is added to an already large document.
        if(required>65536)fb_render_vertex_capacity=required+max(4096,ceil(required/16.0));
        else fb_render_vertex_capacity=max(required,max(1024,fb_render_vertex_capacity*2));
        memfree(fb_render_points);memfree(fb_render_uv);
        memfree(fb_render_vertices);memfree(fb_render_triangles);
        fb_render_points=memalloc(fb_render_vertex_capacity*sizeof(vector));
        fb_render_uv=memalloc(fb_render_vertex_capacity*sizeof(vector));
        fb_render_vertices=memalloc(fb_render_vertex_capacity*sizeof(trisoup_simple_vert_t));
        fb_render_triangles=memalloc(fb_render_vertex_capacity*3*sizeof(int));
    }
    for(int n=2;n<fb_node_count;n++) {
        if(fb_nodes[n].kind!=FB_NODE_BRUSH)continue;
        if(!fb_incremental_enabled() && !fb_filter_custom(n)&&!brush_isselected(fb_nodes[n].model,fb_nodes[n].id))continue;
        int first=fb_render_face_count;vertexOffset=fb_render_append_brush(n,vertexOffset);
        if(fb_render_face_count>first)fb_render_first[n]=first+1;
    }
    fb_commit_profile("fb_render_faces_prepare",profile81Time);
}
void fb_filter_draw()
{
    float profile81Time=gettime(1);
    if(cvar("ca_brush_view"))return;
    fb_render_faces_prepare();
    vector eye=getproperty(VF_ORIGIN),fov=getproperty(VF_FOV);makevectors(getproperty(VF_ANGLES));
    vector forward=v_forward,right=v_right,up=v_up;
    float tx=tan(fov_x*M_PI/360),ty=tan(fov_y*M_PI/360);
    int previous=-1,visible=FALSE;
    int nativePass=FALSE;
    if(autocvar_ftebroom_native_faces && checkbuiltin(ftebroom_draw_faces) && (fb_static_preview_supported || (!fb_active && !fb_transform_preview && !fb_vertex_preview_valid && !fb_generator_count && cvar("ftebroom_tool")!=FB_TOOL_CLIP))){
        fb_render_allowed(0,FB_PICK_LOCKED);
        int *access=fb_static_preview_supported?fb_static_access:fb_render_access;int accessCount=fb_static_preview_supported?fb_static_access_count:fb_render_access_count;
        int drawn=-1;
        if(autocvar_ftebroom_cached_meshes && fb_render_face_count>=(cvar("ftebroom_classic_map")?4096:30000) && checkbuiltin(ftebroom_draw_faces_cached))drawn=ftebroom_draw_faces_cached(fb_render_faces,fb_render_face_count,access,accessCount,sizeof(fb_render_face_t),fb_face_upload_epoch,fb_incremental_config());
        if(drawn<0)drawn=ftebroom_draw_faces(fb_render_faces,fb_render_face_count,access,accessCount,sizeof(fb_render_face_t),fb_incremental_config());
        nativePass=drawn>=0;cvar_set("ftebroom_native_faces_drawn",itos(drawn));
    }
    if(!nativePass)for(int i=0;i<fb_render_face_count;i++) {
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
    fb_commit_profile("fb_filter_draw",profile81Time);
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

int fb_point_preview_cache_allowed(){return fb_static_preview_supported && !fb_selected_entity_count;}
