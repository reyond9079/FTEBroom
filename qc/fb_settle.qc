// SPDX-License-Identifier: GPL-3.0-or-later
// Bounded, immutable simulation snapshots. The document is touched only by Apply.
selbrush_t *fb_settle_selected;
int fb_settle_count,fb_settle_history,fb_settle_selection,fb_settle_geometry;
void fb_settle_clear(){memfree(fb_settle_selected);fb_settle_selected=__NULL__;fb_settle_count=0;fb_generator_clear();cvar_set("ftebroom_settle_valid","0");}
int fb_settle_current(){return fb_settle_count&&history==fb_settle_history&&fb_selection_epoch==fb_settle_selection&&fb_grid_geometry_epoch==fb_settle_geometry;}
void fb_settle_point(filestream file,vector p){fputs(file,sprintf("%.9g %.9g %.9g\n",p_x,p_y,p_z));}
int fb_settle_body(filestream file,int model,int id,int dynamic)
{
    fb_poly_t poly={0};if(!fb_poly_from_brush(model,id,&poly)){fb_poly_free(&poly);return FALSE;}
    vector center='0 0 0';for(int v=0;v<poly.nv;v++)center+=poly.vertices[v];center/=(float)poly.nv;
    fputs(file,sprintf("B %i %.9g %i\n",dynamic,poly.volume,poly.nv));fb_settle_point(file,center);
    for(int v=0;v<poly.nv;v++)fb_settle_point(file,poly.vertices[v]);fb_poly_free(&poly);return TRUE;
}
int fb_settle_export()
{
    fb_settle_clear();fb_tool_refresh();fb_document_tick();
    if(fb_vertex_scope||selectedbrushcount<1||selectedbrushcount>128){fb_geom_error="Select 1..128 whole convex brushes.";return FALSE;}
    fb_settle_count=selectedbrushcount;fb_settle_selected=memalloc(fb_settle_count*sizeof(selbrush_t));memcpy(fb_settle_selected,selectedbrushes,fb_settle_count*sizeof(selbrush_t));
    filestream file=fopen("settle-input.txt",FILE_WRITE);if((float)file<0){fb_geom_error="Cannot write Settle snapshot.";return FALSE;}
    int valid=TRUE,obstacles=0,triangles=0;
    for(int b=0;b<fb_settle_count&&valid;b++)valid=fb_settle_body(file,fb_settle_selected[b].model,fb_settle_selected[b].id,TRUE);
    for(int n=2;n<fb_node_count&&valid;n++) {
        int model=fb_nodes[n].model,id=fb_nodes[n].id;
        if(fb_nodes[n].kind<FB_NODE_BRUSH||!fb_render_allowed(n,0)||brush_isselected(model,id))continue;
        if(fb_nodes[n].kind==FB_NODE_BRUSH){if(++obstacles>4096){valid=FALSE;break;}valid=fb_settle_body(file,model,id,FALSE);}
        else if(fb_nodes[n].kind==FB_NODE_PATCH){patchinfo_t info;int count=patch_getcp(model,id,__NULL__,0,&info);if(!count)continue;
            patchvert_t *points=memalloc(count*sizeof(patchvert_t));patch_getcp(model,id,points,count,&info);patchvert_t *mesh=fb_patch_preview_mesh(points,&info);memfree(points);
            int width=info.cpwidth,height=info.cpheight;
            for(int y=0;y<height-1&&valid;y++)for(int x=0;x<width-1&&valid;x++) {
                if((triangles+=2)>262144){valid=FALSE;break;}int a=y*width+x,b=a+1,c=a+width,d=c+1;
                fputs(file,"T\n");fb_settle_point(file,mesh[a].xyz);fb_settle_point(file,mesh[b].xyz);fb_settle_point(file,mesh[d].xyz);
                fputs(file,"T\n");fb_settle_point(file,mesh[a].xyz);fb_settle_point(file,mesh[d].xyz);fb_settle_point(file,mesh[c].xyz);
            }memfree(mesh);
        }
    }
    if(valid)fputs(file,"END\n");fclose(file);
    if(!valid){fb_geom_error="Settle snapshot is invalid or exceeds the scene budget.";return FALSE;}
    fb_settle_history=history;fb_settle_selection=fb_selection_epoch;fb_settle_geometry=fb_grid_geometry_epoch;return TRUE;
}
int fb_settle_preview()
{
    if(!fb_settle_current()){fb_geom_error="The document or selection changed. Run Settle again.";return FALSE;}
    filestream file=fopen("settle-result.txt",FILE_READ);if((float)file<0){fb_geom_error="Cannot read Settle result.";return FALSE;}
    int count=stoi(fgets(file)),valid=count==fb_settle_count;fb_generator_clear();
    if(valid){fb_generator_count=count;fb_generator_items=memalloc(count*sizeof(fb_generator_preview_t));}
    int moved=0,dropped=0;float drop=0;
    for(int b=0;b<count&&valid;b++) {
        fb_affine_t matrix={0};matrix.x=stov(fgets(file));matrix.y=stov(fgets(file));matrix.z=stov(fgets(file));matrix.translation=stov(fgets(file));
        valid=fb_affine_prepare(&matrix)&&fabs(matrix.determinant-1)<.001;
        static brushface_t faces[64];int contents,nf=brush_get(fb_settle_selected[b].model,fb_settle_selected[b].id,faces,64,&contents);
        if(nf<4||nf>64){valid=FALSE;break;}
        fb_poly_t original={0};vector before='0 0 0';
        valid=fb_poly_from_faces(faces,nf,&original);
        if(valid){for(int p=0;p<original.nv;p++)before+=original.vertices[p];before/=(float)original.nv;}fb_poly_free(&original);
        for(int f=0;f<nf&&valid;f++)fb_affine_face(&matrix,&faces[f],cvar("ftebroom_texture_lock"));
        if(valid)valid=fb_poly_from_faces(faces,nf,&fb_generator_items[b].poly);
        if(valid){fb_poly_t *result=&fb_generator_items[b].poly;vector after='0 0 0';for(int p=0;p<result->nv;p++)after+=result->vertices[p];after/=(float)result->nv;if(vlen(after-before)>.25)moved++;if(before_z-after_z>1)dropped++;drop+=before_z-after_z;}
    }
    if(fgets(file)!="END")valid=FALSE;fclose(file);
    if(!valid){fb_generator_clear();fb_geom_error="Settle result could not be represented as valid brushes.";return FALSE;}
    fb_generator_hide_original=TRUE;fb_generator_original_count=count;fb_generator_original=memalloc(count*sizeof(selbrush_t));memcpy(fb_generator_original,fb_settle_selected,count*sizeof(selbrush_t));
    fb_generator_history=history;fb_generator_selection=fb_selection_epoch;fb_generator_geometry=fb_grid_geometry_epoch;
    cvar_set("ftebroom_settle_moved",itos(moved));cvar_set("ftebroom_settle_dropped",itos(dropped));cvar_set("ftebroom_settle_drop",ftos(drop/max(1,count)));
    cvar_set("ftebroom_settle_valid","1");fb_visibility_epoch++;return TRUE;
}
int fb_settle_command()
{
    if(argv(0)!="ftebroom_settle")return FALSE;string mode=argv(1);fb_geom_error="";int valid=TRUE;
    if(mode=="export"){valid=fb_settle_export();cvar_set("ftebroom_settle_ready",valid?"1":"-1");}
    else if(mode=="preview")valid=fb_settle_preview();
    else if(mode=="apply"||mode=="apply_grouped") {
        valid=FALSE;
        if(fb_settle_current())if(fb_generator_count==fb_settle_count)if(cvar("ftebroom_settle_valid")!=0)if(fb_generator_items)valid=TRUE;
        if(valid){fb_tx_begin("Settle");for(int b=0;b<fb_settle_count;b++){int contents;brush_get(fb_settle_selected[b].model,fb_settle_selected[b].id,__NULL__,0,&contents);static brushface_t faces[64];fb_poly_t *poly=&fb_generator_items[b].poly;for(int f=0;f<poly->nf;f++)faces[f]=poly->faces[f].attr;brush_history_edit(fb_settle_selected[b].model,fb_settle_selected[b].id,faces,poly->nf,contents);}if(mode=="apply_grouped")valid=fb_plugin_group_fragments();if(valid)fb_tx_commit();else fb_tx_cancel();fb_vertex_rebuild();}
        else fb_geom_error="The document or selection changed. Run Settle again.";fb_settle_clear();cvar_set("ftebroom_settle_applied",valid?"1":"0");
    }else fb_settle_clear();
    if(!valid&&mode=="export")fb_settle_clear();cvar_set("ftebroom_settle_error",valid?"":fb_geom_error);return TRUE;
}
