// SPDX-License-Identifier: GPL-2.0-or-later
void fb_repeat_texture(string mode,int direction,int speed);
string fb_material_key(string name)
{
    // Quake WAD names are case-insensitive; keep the MAP's stored spelling.
    if(cvar("ftebroom_quake1"))name=strtolower(name);
    if(substring(name,0,9)=="textures/") return substring(name,9,-1);
    return name;
}
hashtable fb_material_counts;
void fb_material_usage()
{
    float profile81Time=gettime(1);
    static int valid,geometry=-1,edits=-1;
    int rebuild=!valid || geometry!=fb_grid_geometry_epoch;
    if(!rebuild && edits==fb_incremental_epoch)return;
    valid=TRUE;geometry=fb_grid_geometry_epoch;edits=fb_incremental_epoch;
    hashtable counts;
    if(rebuild || !fb_material_counts){
    if(fb_material_counts)hash_destroytab(fb_material_counts);
    counts=fb_material_counts=hash_createtab(512,EV_INTEGER);
    static brushface_t faces[128];
    for(int n=0;n<fb_node_count;n++) {
        if(fb_nodes[n].kind==FB_NODE_BRUSH) {
            int total=brush_get(fb_nodes[n].model,fb_nodes[n].id,faces,128,__NULL__);
            for(int f=0;f<min(total,128);f++) {
                string name=fb_material_key(faces[f].shadername);int count=counts[name];counts[name]=count+1i;
            }
        } else if(fb_nodes[n].kind==FB_NODE_PATCH) {
            patchinfo_t patch;patch_getcp(fb_nodes[n].model,fb_nodes[n].id,__NULL__,0,&patch);
            string name=fb_material_key(patch.shadername);int count=counts[name];counts[name]=count+1i;
        }
    }
    }
    counts=fb_material_counts;
    filestream file=fopen("material-usage.txt",FILE_WRITE);
    if((float)file>=0) {
        for(int i=0;;i++) {
            string key=hash_getkey(counts,i);if isnull(key) break;
            int count=counts[key];fputs(file,strcat(itos(count)," ",key,"\n"));
        }
        fclose(file);
    }
    fb_commit_profile("fb_material_usage",profile81Time);
}
void fb_material_select(string mode)
{
    string material=strzone(fb_material_key(cvar_string("ftebroom_material_query")));
    fb_document_tick();fb_tx_begin("Select Material");fb_selection_clear();
    static brushface_t faces[128];
    for(int n=0;n<fb_node_count;n++) if(fb_select_in_scope(n)) {
        if(fb_nodes[n].kind==FB_NODE_BRUSH) {
            int count=brush_get(fb_nodes[n].model,fb_nodes[n].id,faces,128,__NULL__);
            for(int f=0;f<min(count,128);f++) if(fb_material_key(faces[f].shadername)==material) {
                if(mode=="brushes") {fb_selection_set(fb_selection_target(n),TRUE);break;}
                if(f<64) brushface_select(fb_nodes[n].model,fb_nodes[n].id,f+1);
            }
        } else if(mode=="brushes" && fb_nodes[n].kind==FB_NODE_PATCH) {
            patchinfo_t patch;patch_getcp(fb_nodes[n].model,fb_nodes[n].id,__NULL__,0,&patch);
            if(fb_material_key(patch.shadername)==material) fb_selection_set(fb_selection_target(n),TRUE);
        }
    }
    fb_tx_commit();strunzone(material);
}
void fb_material_replace()
{
    string from=strzone(fb_material_key(cvar_string("ftebroom_replace_from")));
    string to=strzone(fb_material_key(cvar_string("ftebroom_replace_to")));
    if(from==to || to=="") {strunzone(from);strunzone(to);return;}
    int count=selectedbrushcount,restricted=count>0 || fb_selected_entity_count>0;
    selbrush_t *old=memalloc(sizeof(selbrush_t)*count);memcpy(old,selectedbrushes,sizeof(selbrush_t)*count);
    fb_document_tick();fb_tx_begin("Replace Material");fb_selection_clear();int changed=0;
    static brushface_t faces[128];static int matches[128];
    for(int n=0;n<fb_node_count;n++) if(fb_nodes[n].kind>=FB_NODE_BRUSH) {
        int selected=-1;
        for(int b=0;b<count;b++) if(old[b].model==fb_nodes[n].model && old[b].id==fb_nodes[n].id) selected=b;
        if(restricted && selected<0) continue;
        if(fb_nodes[n].kind==FB_NODE_PATCH) {
            patchinfo_t info;int patchTotal=patch_getcp(fb_nodes[n].model,fb_nodes[n].id,__NULL__,0,&info);
            if(fb_material_key(info.shadername)!=from) continue;
            patchvert_t *points=memalloc(sizeof(patchvert_t)*patchTotal);patch_getcp(fb_nodes[n].model,fb_nodes[n].id,points,patchTotal,&info);
            info.shadername=to;int patchId=patch_history_edit(fb_nodes[n].model,fb_nodes[n].id,points,info);
            brush_select(fb_nodes[n].model,patchId);memfree(points);changed++;
        } else {
            int contents;int total=brush_get(fb_nodes[n].model,fb_nodes[n].id,faces,128,&contents);
            if(total>128) {fb_tx_cancel();memfree(old);strunzone(from);strunzone(to);return;}
            int any=FALSE;
            for(int f=0;f<total;f++) {
                matches[f]=FALSE;
                if(restricted) if(!selbrush_face(&old[selected],f)) continue;
                if(fb_material_key(faces[f].shadername)==from) {faces[f].shadername=to;matches[f]=TRUE;any=TRUE;}
            }
            if(!any) continue;
            int id=brush_history_edit(fb_nodes[n].model,fb_nodes[n].id,faces,total,contents);
            for(int f=0;f<min(total,64);f++) if(matches[f]) {brushface_select(fb_nodes[n].model,id,f+1);changed++;}
        }
    }
    if(changed) fb_tx_commit();else fb_tx_cancel();
    memfree(old);strunzone(from);strunzone(to);
}
void fb_texture_key(string mode,int direction,int speed)
{
    string repeatMode=strzone(mode);
    float step=fb_grid();if(speed==1) step*=2;if(speed==2) step=1;
    if(mode=="rotate") {step=15;if(speed==1) step=90;if(speed==2) step=1;}
    makevectors(getproperty(VF_ANGLES));vector right=v_right,up=v_up,eye=getproperty(VF_ORIGIN);
    static brushface_t faces[128];int contents;
    fb_tx_begin("Texture Alignment");
    for(int b=0;b<selectedbrushcount;b++) {
        int count=brush_get(selectedbrushes[b].model,selectedbrushes[b].id,faces,128,&contents);
        if(count>128) {fb_tx_cancel();strunzone(repeatMode);return;}
        for(int f=0;f<count;f++) if(selbrush_face(&selectedbrushes[b],f)) {
            brushface_t *face=&faces[f];
            if(mode=="reset" || mode=="world") {
                reset_texturecoords(face);
                if(mode=="reset") {
                    face->sdir=normalize(face->sdir-face->planenormal*(face->sdir*face->planenormal));
                    vector projected=face->tdir-face->planenormal*(face->tdir*face->planenormal);
                    face->tdir=normalize(crossproduct(face->planenormal,face->sdir));
                    if(face->tdir*projected<0) face->tdir=-face->tdir;
                }
            } else if(mode=="rotate") {
                float sign=1;if(eye*face->planenormal-face->planedist<0) sign=-1;
                float angle=direction*step*M_PI/180*sign;
                face->sdir=fb_uv_rotate(face->sdir,face->planenormal,angle);face->tdir=fb_uv_rotate(face->tdir,face->planenormal,angle);
            } else {
                vector screen=right;if(mode=="vertical" || mode=="flipv") screen=up;
                screen=screen-face->planenormal*(screen*face->planenormal);
                float u=normalize(face->sdir)*screen,v=normalize(face->tdir)*screen;
                if(mode=="fliph" || mode=="flipv") {
                    if(fabs(u)>=fabs(v)) {face->sdir=-face->sdir;face->sbias=-face->sbias;}
                    else {face->tdir=-face->tdir;face->tbias=-face->tbias;}
                } else if(fabs(u)>=fabs(v)) face->sbias-=direction*step*vlen(face->sdir)*(u<0?-1:1);
                else face->tbias-=direction*step*vlen(face->tdir)*(v<0?-1:1);
            }
        }
        brush_history_edit(selectedbrushes[b].model,selectedbrushes[b].id,faces,count,contents);
    }
    fb_tx_commit();fb_repeat_texture(repeatMode,direction,speed);strunzone(repeatMode);
}

// Paint only the surface under the drop, independently of the current selection.
void fb_material_drop(string material,vector pixel)
{
    string owned=strzone(material);
    if(!fb_pick_query(unproject(pixel),unproject(pixel+'0 0 8192'),pixel,0,FB_HIT_BRUSH|FB_HIT_PATCH,cvar("ca_brush_view")!=0)){strunzone(owned);return;}
    fb_hit_t hit=fb_hits[0];fb_tx_begin("Drop Material");int changed=FALSE;
    if(hit.kind==FB_HIT_PATCH) {
        patchinfo_t info;int patchCount=patch_getcp(hit.model,hit.id,__NULL__,0,&info);
        if(patchCount>0){patchvert_t *points=memalloc(sizeof(patchvert_t)*patchCount);patch_getcp(hit.model,hit.id,points,patchCount,&info);info.shadername=owned;changed=patch_history_edit(hit.model,hit.id,points,info)!=0;memfree(points);}
    } else {
        int count=brush_get(hit.model,hit.id,__NULL__,0,__NULL__),contents;
        if(count>0&&hit.face>0&&hit.face<=count){brushface_t *faces=memalloc(sizeof(brushface_t)*count);brush_get(hit.model,hit.id,faces,count,&contents);faces[hit.face-1].shadername=owned;changed=brush_history_edit(hit.model,hit.id,faces,count,contents)!=0;memfree(faces);}
    }
    if(changed)fb_tx_commit();else fb_tx_cancel();strunzone(owned);
}
