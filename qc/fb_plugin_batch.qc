// SPDX-License-Identifier: GPL-3.0-or-later
// Plugins can compose existing geometry operations in one cancellable transaction.
int fb_plugin_fracture_selection()
{
    int count=selectedbrushcount,fragments=bound(2,cvar("ftebroom_fracture_count"),128);
    if(count<1||count*fragments>128){fb_geom_error="Plugin fracture budget: total fragments must be 1..128.";return FALSE;}
    selbrush_t *original=memalloc(count*sizeof(selbrush_t));selbrush_t *result=memalloc(128*sizeof(selbrush_t));
    memcpy(original,selectedbrushes,count*sizeof(selbrush_t));int total=0,valid=TRUE;
    for(int source=0;source<count&&valid;source++){
        brush_deselectall();brush_select(original[source].model,original[source].id);
        valid=fb_fracture_apply();
        if(valid){for(int b=0;b<selectedbrushcount;b++)result[total++]=selectedbrushes[b];}
    }
    if(valid){brush_deselectall();for(int b=0;b<total;b++)brush_select(result[b].model,result[b].id);}
    memfree(original);memfree(result);return valid;
}
int fb_plugin_release(vector sourceLow,vector sourceHigh)
{
    int axis=bound(0,cvar("ftebroom_plugin_release_axis"),2);float gap=cvar("ftebroom_plugin_release_gap");
    if(!fb_geom_finite(gap)||gap<0||gap>256||selectedbrushcount<1){fb_geom_error="Invalid release clearance.";return FALSE;}
    static vector bounds[2];float low=100000000,high=-100000000;
    for(int b=0;b<selectedbrushcount;b++){if(brush_getfacepoints(selectedbrushes[b].model,selectedbrushes[b].id,0,bounds,2)!=2)return FALSE;low=min(low,bounds[0][axis]);high=max(high,bounds[1][axis]);}
    vector delta='0 0 0';delta[axis]=sourceHigh[axis]+gap-low;
    if(cvar("ftebroom_plugin_release_negative"))delta[axis]=sourceLow[axis]-gap-high;
    for(int b=0;b<selectedbrushcount;b++){
        int model=selectedbrushes[b].model,id=selectedbrushes[b].id,contents;static brushface_t faces[64];int count=brush_get(model,id,faces,64,&contents);
        if(count<4||count>64)return FALSE;brush_getfacepoints(model,id,0,bounds,2);if(!fb_geom_point(bounds[0]+delta)||!fb_geom_point(bounds[1]+delta)){fb_geom_error="Released fragments exceed world bounds.";return FALSE;}
        for(int f=0;f<count;f++){faces[f].planedist+=faces[f].planenormal*delta;faces[f].sbias-=faces[f].sdir*delta;faces[f].tbias-=faces[f].tdir*delta;}
        if(!brush_history_edit(model,id,faces,count,contents))return FALSE;
    }
    return TRUE;
}
int fb_plugin_batch_command()
{
    if(argv(0)!="ftebroom_plugin_batch")return FALSE;
    string operations[16];int count=0;while(count<17&&argv(count+1)!="")count++;int valid=count>0&&count<=16;
    if(!valid){cvar_set("ftebroom_plugin_status","-1");cvar_set("ftebroom_plugin_error","Choose 1..16 geometry operations.");return TRUE;}
    for(int i=0;i<count;i++)operations[i]=strzone(argv(i+1));
    static vector sourceBounds[2];sourceBounds[0]=sourceBounds[1]='0 0 0';
    if(selectedbrushcount==1)brush_getfacepoints(selectedbrushes[0].model,selectedbrushes[0].id,0,sourceBounds,2);
    fb_tool_deactivate();fb_generator_clear();fb_geom_error="";fb_tx_begin("Plugin geometry batch");
    for(int step=0;step<count&&valid;step++){
        string operation=operations[step];
        if(operation=="perforate")valid=fb_perforate_apply(FALSE);
        else if(operation=="fracture")valid=fb_plugin_fracture_selection();
        else if(operation=="release")valid=fb_plugin_release(sourceBounds[0],sourceBounds[1]);
        else if(operation=="inset")valid=fb_inset_apply();
        else if(operation=="bevel")valid=fb_bevel_apply();
        else if(operation=="subdivision")valid=fb_subdivision_apply();
        else {fb_geom_error="Unsupported plugin batch operation.";valid=FALSE;}
    }
    for(int i=0;i<count;i++)strunzone(operations[i]);
    if(valid)fb_tx_commit();else fb_tx_cancel();fb_document_tick();fb_vertex_rebuild();
    if(valid){float center=0;for(int b=0;b<selectedbrushcount;b++){fb_poly_t part={0};if(fb_poly_from_brush(selectedbrushes[b].model,selectedbrushes[b].id,&part)){vector middle='0 0 0';for(int p=0;p<part.nv;p++)middle+=part.vertices[p];center+=middle_z/(float)part.nv;}fb_poly_free(&part);}cvar_set("ftebroom_plugin_result_center_z",ftos(center/max(1,selectedbrushcount)));}
    cvar_set("ftebroom_plugin_status",valid?"1":"-1");cvar_set("ftebroom_plugin_error",valid?"":fb_geom_error);return TRUE;
}
