// SPDX-License-Identifier: GPL-2.0-or-later
int fb_uv_editing,fb_uv_model,fb_uv_id,fb_uv_face,fb_uv_count,fb_uv_contents,fb_uv_epoch;
brushface_t fb_uv_original[64];
void fb_repeat_uv(vector row0,vector row1);
vector fb_uv_row0,fb_uv_row1;
int fb_uv_changed;
void fb_uv_finish(int commit)
{
    if(!fb_uv_editing)return;fb_uv_editing=FALSE;
    if(commit){fb_tx_commit();if(fb_uv_changed)fb_repeat_uv(fb_uv_row0,fb_uv_row1);}else fb_tx_cancel();
    for(int f=0;f<fb_uv_count;f++)strunzone(fb_uv_original[f].shadername);fb_uv_count=0;
}
int fb_uv_begin()
{
    if(fb_uv_editing)return TRUE;
    if(fb_faces_count()!=1 || selectedbrushcount!=1 || cvar("ftebroom_tool")==FB_TOOL_SWEEP)return FALSE;
    fb_tool_refresh();fb_uv_model=selectedbrushes[0].model;fb_uv_id=selectedbrushes[0].id;fb_uv_face=-1;
    fb_uv_count=brush_get(fb_uv_model,fb_uv_id,fb_uv_original,64,&fb_uv_contents);if(fb_uv_count<4 || fb_uv_count>64)return FALSE;
    for(int f=0;f<fb_uv_count;f++) {
        fb_uv_original[f].shadername=strzone(fb_uv_original[f].shadername);
        if(selbrush_face(&selectedbrushes[0],f))fb_uv_face=f;
    }
    fb_tx_begin("UV editor");fb_uv_changed=FALSE;fb_uv_editing=TRUE;return TRUE;
}
void fb_uv_matrix(vector row0,vector row1)
{
    if(!fb_uv_editing)return;
    if(selectedbrushcount!=1 || selectedbrushes[0].model!=fb_uv_model || selectedbrushes[0].id!=fb_uv_id || !selbrush_face(&selectedbrushes[0],fb_uv_face)) {fb_uv_finish(FALSE);return;}
    for(int a=0;a<3;a++)if(!fb_geom_finite(row0[a]) || !fb_geom_finite(row1[a]))return;
    if(fabs(row0_x*row1_y-row0_y*row1_x)<0.000001)return;
    brushface_t faces[64];memcpy(faces,fb_uv_original,fb_uv_count*sizeof(brushface_t));
    brushface_t original=fb_uv_original[fb_uv_face];brushface_t *out=&faces[fb_uv_face];
    out->sdir=original.sdir*row0_x+original.tdir*row0_y;
    out->tdir=original.sdir*row1_x+original.tdir*row1_y;
    out->sbias=original.sbias*row0_x+original.tbias*row0_y+row0_z;
    out->tbias=original.sbias*row1_x+original.tbias*row1_y+row1_z;
    int id=brush_history_edit(fb_uv_model,fb_uv_id,faces,fb_uv_count,fb_uv_contents);
    if(id){fb_uv_id=id;fb_uv_row0=row0;fb_uv_row1=row1;fb_uv_changed=TRUE;}else fb_uv_finish(FALSE);
}
void fb_uv_publish()
{
    static string previous="";int face=-1,model=0,id=0;
    if(selectedbrushcount==1 && !selbrush_whole(&selectedbrushes[0]) && fb_faces_count()==1) {
        model=selectedbrushes[0].model;id=selectedbrushes[0].id;
        for(int f=0;f<64;f++)if(selbrush_face(&selectedbrushes[0],f))face=f;
    }
    string signature=sprintf("%i %i %i %i",fb_document_revision(),model,id,face);
    if(signature==previous)return;strunzone(previous);previous=strzone(signature);
    cvar_set("ftebroom_uv_face",itos(face>=0));if(face<0)return;
    brushface_t faces[64];int count=brush_get(model,id,faces,64,__NULL__);if(face>=count){cvar_set("ftebroom_uv_face","0");return;}
    brushface_t attr=faces[face],base=attr;fb_texture_basecoords(&base);
    vector u=normalize(base.sdir-attr.planenormal*(base.sdir*attr.planenormal)),v=normalize(crossproduct(attr.planenormal,u));
    if(v*base.tdir<0)v=-v;
    vector origin=attr.planenormal*attr.planedist,size=fb_material_image_size(attr.shadername);if(size_x<=0)size_x=64;if(size_y<=0)size_y=64;
    static vector points[64];int n=brush_getfacepoints(model,id,face+1,points,64);
    filestream stream=fopen("ftebroom-uv.txt",FILE_WRITE);if((float)stream<0)return;
    fputs(stream,strcat(fb_map_quote(attr.shadername),"\n"));
    fputs(stream,sprintf("%i %i %i %i %g %g\n",model,id,face,n,size_x,size_y));
    fputs(stream,sprintf("%.9g %.9g %.9g ",attr.sdir*u,attr.sdir*v,attr.sbias+attr.sdir*origin));
    fputs(stream,sprintf("%.9g %.9g %.9g\n",attr.tdir*u,attr.tdir*v,attr.tbias+attr.tdir*origin));
    for(int p=0;p<n;p++)fputs(stream,sprintf("%.9g %.9g\n",points[p]*u,points[p]*v));
    fclose(stream);cvar_set("ftebroom_uv_epoch",itos(++fb_uv_epoch));
}
int fb_uv_command()
{
    if(argv(0)!="ftebroom_uv")return FALSE;
    string op=argv(1);
    if(op=="begin"){fb_uv_begin();return TRUE;}
    if(op=="commit"){fb_uv_finish(TRUE);return TRUE;}
    if(op=="cancel"){fb_uv_finish(FALSE);return TRUE;}
    if(op=="matrix") {
        vector row0=[stof(argv(2)),stof(argv(3)),stof(argv(6))],row1=[stof(argv(4)),stof(argv(5)),stof(argv(7))];
        fb_uv_matrix(row0,row1);return TRUE;
    }
    return FALSE;
}
