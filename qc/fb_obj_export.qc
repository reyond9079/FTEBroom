// SPDX-License-Identifier: GPL-2.0-or-later
int fb_obj_vertex,fb_obj_unity,fb_obj_material_count;
filestream fb_obj_manifest;
void fb_obj_point(filestream file,vector p,float u,float v)
{
    fputs(file,sprintf("v %.9g %.9g %.9g\n",p_x,p_y,p_z));
    fputs(file,sprintf("vt %.9g %.9g\n",u,1-v));fb_obj_vertex++;
}
void fb_obj_material(filestream file,filestream mtl,hashtable materials,string shader)
{
    if(fb_obj_unity){
        string cached=materials[shader];
        if(cached==""){cached=strcat("material_",itos(fb_obj_material_count++));materials[shader]=cached;fputs(fb_obj_manifest,strcat(cached," ",fb_map_quote(shader),"\n"));fputs(mtl,strcat("newmtl ",cached,"\nKd 1 1 1\nd 1\n\n"));}
        fputs(file,strcat("usemtl ",cached,"\n"));return;
    }
    string name=strreplace(" ","_",strreplace("\t","_",strreplace("\n","_",strreplace("\r","_",shader))));
    if(name=="")name="untextured";
    if(materials[name]==""){materials[name]="1";fputs(mtl,strcat("newmtl ",name,"\nKd 1 1 1\nd 1\n\n"));}
    fputs(file,strcat("usemtl ",name,"\n"));
}
int fb_obj_export()
{
    fb_obj_unity=argv(0)=="ftebroom_export_unity";fb_obj_material_count=0;
    string status=fb_obj_unity?"ftebroom_unity_status":"ftebroom_obj_status";cvar_set(status,"-1");fb_document_tick();
    filestream file=fopen(fb_obj_unity?"unity.obj":"export.obj",FILE_WRITE);if((float)file<0)return FALSE;
    filestream mtl=fopen(fb_obj_unity?"unity.mtl":"export.mtl",FILE_WRITE);if((float)mtl<0){fclose(file);return FALSE;}
    if(fb_obj_unity){fb_obj_manifest=fopen("unity-materials.txt",FILE_WRITE);if((float)fb_obj_manifest<0){fclose(file);fclose(mtl);return FALSE;}}
    fputs(file,"# ReyondBroom OBJ: world units, Z up, UV V flipped for OBJ\nmtllib export.mtl\n");
    hashtable materials=hash_createtab(128,EV_STRING);fb_obj_vertex=0;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind>=FB_NODE_BRUSH && fb_export_included(n)) {
        int model=fb_nodes[n].model,id=fb_nodes[n].id;fputs(file,sprintf("o object_%i_%i\n",model,id));
        patchinfo_t info;int cpcount=patch_getcp(model,id,__NULL__,0,&info);
        if(cpcount) {
            patchvert_t *cp=memalloc(sizeof(patchvert_t)*cpcount);patch_getcp(model,id,cp,cpcount,&info);
            fb_obj_material(file,mtl,materials,info.shadername);
            int width=(info.cpwidth-1)*4+1,height=(info.cpheight-1)*4+1;
            patchvert_t *rows=memalloc(sizeof(patchvert_t)*width*info.cpheight);
            for(int row=0;row<info.cpheight;row++)for(int col=0;col<width;col++)
                fb_patch_curve(&cp[row*info.cpwidth],info.cpwidth,1,(float)col/(float)(width-1),&rows[row*width+col]);
            int base=fb_obj_vertex+1;
            for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
                patchvert_t point;fb_patch_curve(&rows[x],info.cpheight,width,(float)y/(float)(height-1),&point);
                fb_obj_point(file,point.xyz,point.s,point.t);
            }
            for(int y=0;y<height-1;y++)for(int x=0;x<width-1;x++) {
                int a=base+y*width+x,b=a+1,c=b+width,d=a+width;
                fputs(file,sprintf("f %i/%i %i/%i %i/%i\n",a,a,c,c,b,b));
                fputs(file,sprintf("f %i/%i %i/%i %i/%i\n",a,a,d,d,c,c));
            }
            memfree(rows);memfree(cp);
        }else {
            int count=brush_get(model,id,__NULL__,0,__NULL__);if(!count)continue;
            brushface_t *faces=memalloc(sizeof(brushface_t)*count);brush_get(model,id,faces,count,__NULL__);
            for(int f=0;f<count;f++) {
                static vector points[64];int vertices=brush_getfacepoints(model,id,f+1,points,64);if(vertices<3)continue;
                brushface_t face=faces[f];fb_obj_material(file,mtl,materials,face.shadername);
                vector size=fb_material_image_size(face.shadername);if(size_x<=0)size_x=64;if(size_y<=0)size_y=64;
                int first=fb_obj_vertex+1;
                for(int p=0;p<vertices;p++)fb_obj_point(file,points[p],(face.sdir*points[p]+face.sbias)/size_x,(face.tdir*points[p]+face.tbias)/size_y);
                fputs(file,"f");for(int p=vertices-1;p>=0;p--)fputs(file,sprintf(" %i/%i",first+p,first+p));fputs(file,"\n");
            }
            memfree(faces);
        }
    }
    hash_destroytab(materials);fclose(file);fclose(mtl);if(fb_obj_unity)fclose(fb_obj_manifest);cvar_set(status,"1");return TRUE;
}
