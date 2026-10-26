// SPDX-License-Identifier: GPL-3.0-or-later
// Seeded convex Voronoi cells. The caller owns the transaction and preview.
int fb_generator_source_parent(int model,int id)
{
    for(int n=0;n<fb_node_count;n++)if(fb_nodes[n].kind==FB_NODE_BRUSH&&fb_nodes[n].model==model&&fb_nodes[n].id==id) {
        int parent=fb_nodes[n].parent;
        if(parent>=0&&fb_nodes[parent].kind==FB_NODE_ENTITY)parent=fb_nodes[parent].parent;
        return parent;
    }
    return 1;
}
int fb_fracture_apply()
{
    if(selectedbrushcount!=1){fb_geom_error="Select exactly one convex brush";return FALSE;}
    int model=selectedbrushes[0].model,id=selectedbrushes[0].id,contents;
    fb_poly_t source={0};if(!fb_poly_from_brush(model,id,&source)){fb_geom_error=strcat("Source: ",fb_geom_error);return FALSE;}
    brush_get(model,id,__NULL__,0,&contents);
    int parent=fb_generator_source_parent(model,id);
    int count=bound(2,cvar("ftebroom_fracture_count"),128),seed=cvar("ftebroom_fracture_seed");
    vector low=source.vertices[0],high=low,center='0 0 0';
    for(int v=0;v<source.nv;v++){center+=source.vertices[v];for(int a=0;a<3;a++){low[a]=min(low[a],source.vertices[v][a]);high[a]=max(high[a],source.vertices[v][a]);}}
    center/=(float)source.nv;
    // Clip Voronoi cells near the origin. Large seed windings otherwise lose
    // matching edge endpoints on thin, elevated slabs.
    static brushface_t localFaces[64];fb_poly_t localSource={0};
    for(int f=0;f<source.nf;f++){localFaces[f]=source.faces[f].attr;localFaces[f].planedist-=localFaces[f].planenormal*center;}
    if(!fb_poly_from_faces(localFaces,source.nf,&localSource)){fb_poly_free(&source);return FALSE;}
    vector sites[128];int placed=0;
    for(int attempt=0;attempt<65536&&placed<count;attempt++) {
        vector point='0 0 0';for(int a=0;a<3;a++)point[a]=low[a]+(high[a]-low[a])*fb_land_hash(attempt,a,seed,0x51f2i);
        int inside=TRUE;
        for(int f=0;f<source.nf;f++)if(point*source.faces[f].attr.planenormal>source.faces[f].attr.planedist-.01)inside=FALSE;
        for(int p=0;p<placed;p++)if(vlen(point-sites[p])<.02)inside=FALSE;
        if(inside)sites[placed++]=point-center;
    }
    if(placed!=count){fb_poly_free(&source);fb_poly_free(&localSource);fb_geom_error="Cannot place fracture sites; reduce the count or enlarge the brush";return FALSE;}
    fb_poly_t *cells=memalloc(count*sizeof(fb_poly_t));int valid=TRUE;float volume=0;
    for(int i=0;i<count&&valid;i++) {
        fb_poly_t cell={0};valid=fb_csg_copy(&localSource,&cell);
        for(int j=0;j<count&&valid;j++)if(j!=i) {
            brushface_t plane=source.faces[0].attr;plane.planenormal=normalize(sites[j]-sites[i]);plane.planedist=((sites[i]+sites[j])*.5)*plane.planenormal;
            fb_poly_t front={0},back={0};valid=fb_csg_split(&cell,plane,&front,&back);fb_poly_free(&front);fb_poly_free(&cell);cell=back;
            if(!valid)fb_geom_error=strcat("Cell split: ",fb_geom_error);
            if(!cell.nf)valid=FALSE;
        }
        if(valid&&cell.volume<.001)valid=FALSE;
        if(valid){cells[i]=cell;volume+=cell.volume;}else fb_poly_free(&cell);
    }
    fb_poly_free(&localSource);
    for(int i=0;i<count&&valid;i++){
        for(int v=0;v<cells[i].nv;v++)cells[i].vertices[v]+=center;
        for(int f=0;f<cells[i].nf;f++)cells[i].faces[f].attr.planedist+=cells[i].faces[f].attr.planenormal*center;
    }
    if(valid&&fabs(volume-source.volume)>max(.01,source.volume*.001)){valid=FALSE;fb_geom_error="Fracture volume validation failed";}
    if(valid) {
        brush_deselectall();float separation=bound(0,cvar("ftebroom_fracture_separation"),1000)/100;
        for(int i=0;i<count&&valid;i++) {
            fb_poly_t *cell=&cells[i];vector midpoint='0 0 0';for(int v=0;v<cell->nv;v++)midpoint+=cell->vertices[v];midpoint/=(float)cell->nv;
            vector delta=(midpoint-center)*separation;static brushface_t faces[64];
            for(int v=0;v<cell->nv;v++)if(!fb_geom_point(cell->vertices[v]+delta))valid=FALSE;
            for(int f=0;f<cell->nf;f++) {
                faces[f]=cell->faces[f].attr;
                // Exterior planes keep the exact original material and UV projection.
                int exterior=FALSE;for(int a=0;a<source.nf;a++)if(fb_geom_sameplane(&faces[f],&source.faces[a].attr)){faces[f]=source.faces[a].attr;exterior=TRUE;break;}
                if(!exterior){vector normal=faces[f].planenormal;float distance=faces[f].planedist;faces[f]=source.faces[0].attr;faces[f].planenormal=normal;faces[f].planedist=distance;string material=cvar_string("ftebroom_fracture_material");if(material!="")faces[f].shadername=material;}
                faces[f].planedist+=faces[f].planenormal*delta;faces[f].sbias-=faces[f].sdir*delta;faces[f].tbias-=faces[f].tdir*delta;
            }
            int destination=model;
            if(valid&&cvar("ftebroom_fracture_entities")) {
                int entityIndex=fb_tx_empty_entity();string name=strzone(brush_model_create());
                fb_tx_entity_set(entityIndex,"\"classname\" \"func_wall\"\n",HIST_ENTITY);fb_organize_parent(entityIndex,parent);fb_tx_set_key(entityIndex,"model",name,HIST_ENTITY);
                precache_model(name);if(!fb_document_probe)fb_document_probe=spawn();setmodel(fb_document_probe,name);destination=fb_document_probe.modelindex;strunzone(name);
                if(!destination)valid=FALSE;
            }
            if(valid)valid=brush_history_create(destination,faces,cell->nf,contents,TRUE)!=0;
        }
        if(valid)brush_history_delete(model,id);
    }
    for(int i=0;i<count;i++)fb_poly_free(&cells[i]);memfree(cells);fb_poly_free(&source);
    if(!valid&&fb_geom_error=="")fb_geom_error="Fracture failed; try fewer fragments or another seed";
    return valid;
}
