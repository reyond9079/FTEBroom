// SPDX-License-Identifier: GPL-2.0-or-later
string fb_align_last;
int fb_align_history,fb_align_fraction,fb_align_cycle;
void fb_texture_align(string operation,int direction,int fraction)
{
    if(operation!="left" && operation!="right" && operation!="top" && operation!="bottom" && operation!="edge" && operation!="fit_h" && operation!="fit_v" && operation!="auto") return;
    int cycle=1;
    if(history==fb_align_history && operation==fb_align_last && fraction==fb_align_fraction) cycle=max(1,fb_align_cycle+direction);
    static brushface_t faces[128];static vector points[MAX_FACEPOINTS];
    fb_tx_begin("Align Texture");int changed=FALSE;
    for(int b=0;b<selectedbrushcount;b++) {
        int contents;int count=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,faces,128,&contents);
        if(count>128) {fb_tx_cancel();return;}
        int any=FALSE;
        for(int f=0;f<count;f++) if(selbrush_face(&selectedbrushes[b],f)) {
            int vertices=brush_getfacepoints(selectedbrushes[b].model,selectedbrushes[b].id,f+1,points,MAX_FACEPOINTS);
            if(vertices>MAX_FACEPOINTS) {fb_tx_cancel();return;}
            if(vertices<3) continue;
            brushface_t *face=&faces[f];
            vector image=cvar("ftebroom_game_profile")==3?fb_material_image_size(face->shadername):drawgetimagesize(strcat("textures/",face->shadername));
            if(image_x<=0 || image_y<=0) image=drawgetimagesize(face->shadername);
            if(image_x<=0 || image_y<=0) {fb_tx_cancel();cvar_set("ftebroom_selection_status","Texture dimensions are unavailable");return;}
            if(operation=="edge" || operation=="auto") {
                int edge=(cycle-1i)%vertices,next=edge+1;if(next==vertices) next=0;
                vector edgeU=normalize(points[next]-points[edge]),edgeV=normalize(crossproduct(face->planenormal,edgeU));
                if(edgeV*face->tdir<0) edgeV=-edgeV;
                face->sdir=edgeU*vlen(face->sdir);face->tdir=edgeV*vlen(face->tdir);
            }
            float umin=1000000000,umax=-1000000000,vmin=1000000000,vmax=-1000000000;
            for(int p=0;p<vertices;p++) {
                float u=face->sdir*points[p]+face->sbias,v=face->tdir*points[p]+face->tbias;
                umin=min(umin,u);umax=max(umax,u);vmin=min(vmin,v);vmax=max(vmax,v);
            }
            float repeats=cycle;if(fraction) repeats=1.0/(cycle+1);
            if(operation=="fit_h" || operation=="auto") {
                if(umax-umin<0.000001) {fb_tx_cancel();return;}
                float scale=image_x*repeats/(umax-umin);
                face->sdir*=scale;face->sbias=(face->sbias-umin)*scale+umin;
                if(operation=="auto") face->sbias-=umin;
            }
            if(operation=="fit_v" || operation=="auto") {
                if(vmax-vmin<0.000001) {fb_tx_cancel();return;}
                float vScale=image_y*repeats/(vmax-vmin);
                face->tdir*=vScale;face->tbias=(face->tbias-vmin)*vScale+vmin;
                if(operation=="auto") face->tbias-=vmin;
            }
            float uslot=(cycle-1)*(umax-umin),vslot=(cycle-1)*(vmax-vmin);
            uslot-=floor(uslot/image_x)*image_x;vslot-=floor(vslot/image_y)*image_y;
            if(operation=="left") face->sbias+=uslot-umin;
            if(operation=="right") face->sbias+=image_x-uslot-umax;
            if(operation=="top") face->tbias+=vslot-vmin;
            if(operation=="bottom") face->tbias+=image_y-vslot-vmax;
            any=TRUE;changed=TRUE;
        }
        if(any) brush_history_edit(selectedbrushes[b].model,selectedbrushes[b].id,faces,count,contents);
    }
    if(changed) fb_tx_commit();else fb_tx_cancel();
    strunzone(fb_align_last);fb_align_last=strzone(operation);fb_align_history=history;fb_align_fraction=fraction;fb_align_cycle=cycle;
}
