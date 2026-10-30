// SPDX-License-Identifier: GPL-2.0-or-later
void fb_patch_convert();
// Opposite polygon chains form symmetric quad strips, with one collapsed quad for odd polygons.
int fb_patch_quad(int model,vector a,vector b,vector c,vector d,brushface_t *face)
{
    patchinfo_t info={0};info.cpwidth=info.cpheight=3;info.shadername=face->shadername;
    info.tesswidth=info.tessheight=-1;
    info.contents=CONTENTBIT_SOLID;info.texinfo='1 1 0';
    vector size=fb_material_image_size(face->shadername);if(size_x<=0)size_x=64;if(size_y<=0)size_y=64;
    patchvert_t points[9];
    for(int y=0;y<3;y++)for(int x=0;x<3;x++) {
        float u=(float)x*0.5,v=(float)y*0.5;int index=y*3+x;
        vector p=(a*(1-u)+b*u)*(1-v)+(d*(1-u)+c*u)*v;
        points[index].xyz=p;points[index].rgb='1 1 1';points[index].a=1;
        points[index].s=(face->sdir*p+face->sbias)/size_x;
        points[index].t=(face->tdir*p+face->tbias)/size_y;
    }
    return patch_history_create(model,points,info,TRUE)!=0;
}
void fb_patch_mix(patchvert_t *a,patchvert_t *b,float t,patchvert_t *out)
{
    out->xyz=a->xyz*(1-t)+b->xyz*t;out->rgb=a->rgb*(1-t)+b->rgb*t;
    out->a=a->a*(1-t)+b->a*t;out->s=a->s*(1-t)+b->s*t;out->t=a->t*(1-t)+b->t*t;
}
void fb_patch_curve(patchvert_t *points,int count,int stride,float t,patchvert_t *out)
{
    float parameter=bound(0,t,1)*(float)((count-1)/2);int segment=min((count-3)/2,floor(parameter));
    float fraction=parameter-(float)segment;int first=segment*2*stride;
    patchvert_t a,b;fb_patch_mix(&points[first],&points[first+stride],fraction,&a);
    fb_patch_mix(&points[first+stride],&points[first+2*stride],fraction,&b);fb_patch_mix(&a,&b,fraction,out);
}
patchvert_t *fb_patch_dimension(patchvert_t *source,int width,int height,int axis,int target)
{
    int oldCount=width,lines=height,step=1;if(axis){oldCount=height;lines=width;step=width;}
    int newWidth=width,newHeight=height;if(axis)newHeight=target;else newWidth=target;
    patchvert_t *output=memalloc(newWidth*newHeight*sizeof(patchvert_t));
    for(int line=0;line<lines;line++) {
        int start=line*width,base=line*newWidth,destStep=1;if(axis){start=base=line;destStep=newWidth;}
        if(target==oldCount+2) {
            int split=oldCount-3;patchvert_t left,right,middle;
            fb_patch_mix(&source[start+split*step],&source[start+(split+1)*step],0.5,&left);
            fb_patch_mix(&source[start+(split+1)*step],&source[start+(split+2)*step],0.5,&right);
            fb_patch_mix(&left,&right,0.5,&middle);
            for(int p=0;p<=split;p++)output[base+p*destStep]=source[start+p*step];
            output[base+(split+1)*destStep]=left;output[base+(split+2)*destStep]=middle;
            output[base+(split+3)*destStep]=right;output[base+(split+4)*destStep]=source[start+(split+2)*step];
        } else {
            for(int p=0;p<target;p++) {
                fb_patch_curve(&source[start],oldCount,step,(float)p/(float)(target-1),&output[base+p*destStep]);
            }
            for(int p=1;p<target;p+=2) {
                patchvert_t ends,control;fb_patch_mix(&output[base+(p-1)*destStep],&output[base+(p+1)*destStep],0.5,&ends);
                fb_patch_mix(&ends,&output[base+p*destStep],2,&control);output[base+p*destStep]=control;
            }
        }
    }
    return output;
}
void fb_patch_resize(int axis,int target)
{
    if(target<3 || target>31 || !(target&1) || (axis!=0 && axis!=1))return;
    fb_vertex_cancel();fb_tx_begin("Resize patch control grid");int valid=TRUE;
    for(int b=0;b<selectedbrushcount && valid;b++) {
        int model=selectedbrushes[b].model,id=selectedbrushes[b].id;patchinfo_t info;
        int count=patch_getcp(model,id,__NULL__,0,&info);if(!count)continue;
        patchvert_t *points=memalloc(count*sizeof(patchvert_t));patch_getcp(model,id,points,count,&info);info.shadername=strzone(info.shadername);
        int current=info.cpwidth;if(axis)current=info.cpheight;
        while(current!=target) {
            int next=target;if(current<target)next=current+2;
            patchvert_t *resized=fb_patch_dimension(points,info.cpwidth,info.cpheight,axis,next);memfree(points);points=resized;
            current=next;if(axis)info.cpheight=current;else info.cpwidth=current;
        }
        for(int p=0;p<info.cpwidth*info.cpheight;p++)if(!fb_geom_point(points[p].xyz))valid=FALSE;
        if(valid)valid=patch_history_edit(model,id,points,info)!=0;
        memfree(points);strunzone(info.shadername);
    }
    if(valid)fb_tx_commit();else fb_tx_cancel();fb_vertex_rebuild();
}
patchvert_t *fb_patch_preview_mesh(patchvert_t *points,patchinfo_t *info)
{
    int width=(info->cpwidth-1)*4+1,height=(info->cpheight-1)*4+1;
    patchvert_t *rows=memalloc(width*info->cpheight*sizeof(patchvert_t));
    patchvert_t *mesh=memalloc(width*height*sizeof(patchvert_t));
    for(int y=0;y<info->cpheight;y++)for(int x=0;x<width;x++)
        fb_patch_curve(&points[y*info->cpwidth],info->cpwidth,1,(float)x/(float)(width-1),&rows[y*width+x]);
    for(int x=0;x<width;x++)for(int y=0;y<height;y++)
        fb_patch_curve(&rows[x],info->cpheight,width,(float)y/(float)(height-1),&mesh[y*width+x]);
    info->cpwidth=width;info->cpheight=height;memfree(rows);return mesh;
}
void fb_patch_preview(patchvert_t *points,patchinfo_t info)
{
    patchvert_t *mesh=fb_patch_preview_mesh(points,&info);
    DrawQCPatchTextured(mesh,info,'1 1 1',1);memfree(mesh);
}
int fb_patch_command()
{
    if(argv(0)=="ftebroom_patch_convert"){fb_patch_convert();return TRUE;}
    if(argv(0)=="ftebroom_patch_resize"){fb_patch_resize(stoi(argv(1)),stoi(argv(2)));return TRUE;}
    return FALSE;
}
void fb_patch_convert()
{
    fb_tool_refresh();if(!selectedbrushcount)return;
    int count=selectedbrushcount;selbrush_t *selection=memalloc(count*sizeof(selbrush_t));memcpy(selection,selectedbrushes,count*sizeof(selbrush_t));
    fb_tx_begin("Convert to patches");fb_selection_clear();int valid=TRUE;
    for(int i=0;i<count && valid;i++) {
        fb_poly_t poly={0};int model=selection[i].model,id=selection[i].id;
        if(!fb_poly_from_brush(model,id,&poly)){fb_poly_free(&poly);continue;}
        int made=FALSE;
        for(int f=0;f<poly.nf && valid;f++) {
            if(!selbrush_whole(&selection[i]) && !selbrush_face(&selection[i],f))continue;
            fb_poly_face_t face=poly.faces[f];int left=0,right=face.count-1;
            while(right-left>=2 && valid) {
                int next=left+1,last=right-1;if(right-left==2)last=right;
                valid=fb_patch_quad(model,poly.vertices[poly.indices[face.start+left]],poly.vertices[poly.indices[face.start+next]],
                    poly.vertices[poly.indices[face.start+last]],poly.vertices[poly.indices[face.start+right]],&face.attr);
                made=TRUE;left++;right--;
            }
        }
        if(made && valid)brush_history_delete(model,id);fb_poly_free(&poly);
    }
    if(valid)fb_tx_commit();else fb_tx_cancel();memfree(selection);
    if(valid)fb_tool_activate(FB_TOOL_CONTROLPOINT);
}
