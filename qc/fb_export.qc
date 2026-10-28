// SPDX-License-Identifier: GPL-2.0-or-later
int fb_export_included(int node)
{
    int layer=fb_nodes[node].layer;
    if(layer==1)return fb_nodes[0].fields["_tb_layer_omit_from_export"]!="1";
    return fb_nodes[layer].fields["_tb_layer_omit_from_export"]!="1";
}
int fb_export_owner(int node)
{
    int parent=fb_nodes[node].parent;return parent<=1?0:parent;
}
int fb_export_map()
{
    cvar_set("ftebroom_export_status","-1");fb_document_tick();filestream file=fopen("export-map.map",FILE_WRITE);if((float)file<0)return FALSE;
    fb_clip_q3_export=TRUE;int success=TRUE;
    // Build owner buckets once, preserving the original geometry order.
    int *heads=memalloc(sizeof(int)*fb_node_count);
    int *next=memalloc(sizeof(int)*fb_node_count);
    for(int h=0;h<fb_node_count;h++)heads[h]=0;
    for(int b=fb_node_count-1;b>=2;b--)if(fb_nodes[b].kind>=FB_NODE_BRUSH && fb_export_included(b)) {
        int owner=fb_export_owner(b);
        if(owner>=0 && owner<fb_node_count){next[b]=heads[owner];heads[owner]=b;}
    }
    for(int e=0;e<fb_node_count;e++) {
        if(fb_nodes[e].kind<0 || e==1 || (e && (fb_nodes[e].kind>=FB_NODE_BRUSH || !fb_export_included(e))))continue;
        fputs(file,"{\n");hashtable fields=(hashtable)fb_nodes[e].fields;
        for(int k=0;;k++) {
            string key=hash_getkey(fields,k);if isnull(key)break;
            if(substring(key,0,10)=="_ftebroom_")continue;
            if(key=="model" && substring(fields[key],0,1)=="*")continue;
            fputs(file,strcat(fb_map_quote(key)," ",fb_map_quote(fields[key]),"\n"));
        }
        for(int n=heads[e];n;n=next[n])
            if(!fb_clip_write_geometry(file,fb_nodes[n].model,fb_nodes[n].id))success=FALSE;
        fputs(file,"}\n");
    }
    memfree(heads);memfree(next);fb_clip_q3_export=FALSE;fclose(file);cvar_set("ftebroom_export_status",success?"1":"-1");return success;
}

// Export a brush-only world, without changing selection, history or entity data.
int fb_export_selected_bsp()
{
    cvar_set("ftebroom_export_status","-1");fb_document_tick();
    if(!selectedbrushcount)return FALSE;
    vector minimum='0 0 0';int haveBounds=FALSE;
    // Validate the whole selection before writing: patches are not brush models.
    for(int i=0;i<selectedbrushcount;i++){
        int contents;int count=brush_get(selectedbrushes[i].model,selectedbrushes[i].id,__NULL__,0,&contents);
        if(count<4 || count>64)return FALSE;
        vector bounds[2];if(brush_getfacepoints(selectedbrushes[i].model,selectedbrushes[i].id,0,bounds,2)!=2)return FALSE;
        if(!haveBounds){minimum=bounds[0];haveBounds=TRUE;}else for(int axis=0;axis<3;axis++)minimum[axis]=min(minimum[axis],bounds[0][axis]);
    }
    filestream file=fopen("export-map.map",FILE_WRITE);if((float)file<0)return FALSE;
    fputs(file,"{\n\"classname\" \"worldspawn\"\n");
    string wad=fb_nodes[0].fields["wad"];if(wad!="")fputs(file,strcat("\"wad\" ",fb_map_quote(wad),"\n"));
    fb_clip_q3_export=TRUE;int success=TRUE;
    for(int b=0;b<selectedbrushcount;b++)if(!fb_clip_write_geometry(file,selectedbrushes[b].model,selectedbrushes[b].id))success=FALSE;
    fb_clip_q3_export=FALSE;fputs(file,"}\n");
    // q3map2 needs an exterior flood seed to retain surfaces of an open model.
    // The native publisher removes this synthetic entity from the output BSP.
    if(!cvar("ftebroom_quake1"))fputs(file,strcat("{\n\"classname\" \"info_player_start\"\n\"origin\" ",fb_map_quote(fb_clip_vec(minimum-'32 32 32')),"\n}\n"));
    fclose(file);
    cvar_set("ftebroom_export_status",success?"1":"-1");return success;
}
