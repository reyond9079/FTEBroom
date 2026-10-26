// SPDX-License-Identifier: GPL-3.0-or-later
// Polygonal prisms are subtracted through one local world axis.
int fb_plugin_layout_active;
int fb_perforate_apply(int preview)
{
    if(selectedbrushcount!=1){fb_geom_error="Select exactly one convex brush";return FALSE;}
    int model=selectedbrushes[0].model,id=selectedbrushes[0].id,contents;
    fb_poly_t source={0};if(!fb_poly_from_brush(model,id,&source))return FALSE;
    brush_get(model,id,__NULL__,0,&contents);
    int parent=fb_generator_source_parent(model,id);
    int count=bound(1,cvar("ftebroom_hole_count"),64),sides=bound(3,cvar("ftebroom_hole_sides"),32),axis=bound(0,cvar("ftebroom_hole_axis"),2),seed=cvar("ftebroom_hole_seed");
    // Keep a single QC invocation below the VM runaway budget. Reject before mutation.
    if(count*sides>128){fb_poly_free(&source);fb_geom_error="Perforate complexity limit: holes times sides must not exceed 128. Use fewer holes or sides.";return FALSE;}
    int u=(axis+1)%3,v=(axis+2)%3;
    float minimum=cvar("ftebroom_hole_min"),maximum=cvar("ftebroom_hole_max"),spacing=max(.05,cvar("ftebroom_hole_spacing"));
    if(minimum<=.1||maximum<minimum){fb_poly_free(&source);fb_geom_error="Set a positive minimum diameter no larger than the maximum";return FALSE;}
    vector low=source.vertices[0],high=low;
    for(int p=0;p<source.nv;p++)for(int a=0;a<3;a++){low[a]=min(low[a],source.vertices[p][a]);high[a]=max(high[a],source.vertices[p][a]);}
    vector centers[64];float radii[64];fb_poly_t *cutters=memalloc(count*sizeof(fb_poly_t));fb_poly_t *plugs=memalloc(count*sizeof(fb_poly_t));
    vector layout[64];int fixedLayout=FALSE;
    if(fb_plugin_layout_active&&cvar_string("ftebroom_plugin_hole_layout")!=""){
        int tokens=tokenize(cvar_string("ftebroom_plugin_hole_layout"));fixedLayout=tokens==count*3;
        if(!fixedLayout){memfree(cutters);memfree(plugs);fb_poly_free(&source);fb_geom_error="Invalid certified hole layout.";return FALSE;}
        for(int item=0;item<count;item++){layout[item]=[stof(argv(item*3)),stof(argv(item*3+1)),stof(argv(item*3+2))];if(!fb_geom_point(layout[item])||layout[item][2]<minimum*.5||layout[item][2]>maximum*.5)fixedLayout=FALSE;}
        if(!fixedLayout){memfree(cutters);memfree(plugs);fb_poly_free(&source);fb_geom_error="Invalid certified hole radius.";return FALSE;}
    }
    int placed=0,valid=TRUE;
    for(int attempt=0;attempt<(fixedLayout?count:2048)&&placed<count;attempt++) {
        float radius=(minimum+(maximum-minimum)*fb_land_hash(attempt,0,seed,0x311i))*.5;
        vector center=(low+high)*.5;center[u]=low[u]+(high[u]-low[u])*fb_land_hash(attempt,1,seed,0x311i);center[v]=low[v]+(high[v]-low[v])*fb_land_hash(attempt,2,seed,0x311i);
        if(fixedLayout){center[u]=layout[attempt][0];center[v]=layout[attempt][1];radius=layout[attempt][2];}
        int inside=TRUE;for(int p=0;p<placed;p++)if(vlen(center-centers[p])<radius+radii[p]+spacing)inside=FALSE;
        if(!inside)continue;
        vector points[64];
        for(int p=0;p<sides;p++) {
            float angle=(p+fb_land_hash(attempt,p,seed,0x821i)*.6)*2*M_PI/(float)sides;
            vector point=center;point[u]+=cos(angle)*radius;point[v]+=sin(angle)*radius;
            float bottom=low[axis],top=high[axis];
            for(int f=0;f<source.nf;f++) {
                brushface_t face=source.faces[f].attr;float n=face.planenormal[axis];
                float remaining=face.planedist-face.planenormal[u]*point[u]-face.planenormal[v]*point[v]-.025;
                if(fabs(n)<.00001){if(remaining<0)inside=FALSE;}else if(n>0)top=min(top,remaining/n);else bottom=max(bottom,remaining/n);
            }
            if(top-bottom<.05)inside=FALSE;
            point[axis]=low[axis]-1;points[p]=point;point[axis]=high[axis]+1;points[p+sides]=point;
        }
        if(!inside)continue;
        // Construct the prism directly instead of the cubic point-hull search.
        static brushface_t planes[34];
        for(int p=0;p<sides;p++) {
            int next=p+1;if(next>=sides)next=0;vector edge=points[next]-points[p],normal='0 0 0';normal[u]=edge[v];normal[v]=-edge[u];
            planes[p]=source.faces[0].attr;planes[p].planenormal=normalize(normal);planes[p].planedist=planes[p].planenormal*points[p];
        }
        planes[sides]=source.faces[0].attr;planes[sides].planenormal='0 0 0';planes[sides].planenormal[axis]=1;planes[sides].planedist=high[axis]+1;
        planes[sides+1]=planes[sides];planes[sides+1].planenormal[axis]=-1;planes[sides+1].planedist=1-low[axis];
        valid=fb_poly_from_faces(planes,sides+2,&cutters[placed]);if(!valid)break;
        centers[placed]=center;radii[placed]=radius;placed++;
    }
    if(placed!=count){valid=FALSE;fb_geom_error="Holes do not fit without overlapping; reduce size, count or spacing";}
    fb_csg_clear();fb_csg_results=memalloc(2048*sizeof(fb_csg_item_t));
    if(valid){fb_poly_t copy={0};valid=fb_csg_copy(&source,&copy);if(valid)valid=fb_csg_append(&copy,model,contents);fb_poly_free(&copy);}
    for(int hole=0;hole<count&&valid;hole++) {
        valid=fb_csg_copy(&source,&plugs[hole]);
        for(int f=0;f<cutters[hole].nf&&valid;f++) {
            fb_poly_t front={0},back={0};valid=fb_csg_split(&plugs[hole],cutters[hole].faces[f].attr,&front,&back);fb_poly_free(&front);fb_poly_free(&plugs[hole]);plugs[hole]=back;if(!back.nf)valid=FALSE;
        }
        fb_csg_item_t *previous=fb_csg_results;int previousCount=fb_csg_count;
        fb_csg_results=memalloc(2048*sizeof(fb_csg_item_t));fb_csg_count=0;
        for(int i=0;i<previousCount;i++){if(valid)valid=fb_csg_difference(&previous[i].poly,&cutters[hole],model,contents);fb_poly_free(&previous[i].poly);}memfree(previous);
    }
    if(valid) {
        brush_deselectall();static brushface_t attrs[64],faces[64];for(int f=0;f<source.nf;f++)attrs[f]=source.faces[f].attr;
        for(int i=0;i<fb_csg_count&&valid;i++) {
            fb_poly_t *poly=&fb_csg_results[i].poly;fb_land_attributes(poly,attrs,source.nf);fb_csg_materials(poly,attrs,source.nf);
            for(int f=0;f<poly->nf;f++)faces[f]=poly->faces[f].attr;
            valid=brush_history_create(model,faces,poly->nf,contents,preview)!=0;
        }
        for(int i=0;i<count&&valid&&cvar("ftebroom_hole_keep");i++) {
            int entityIndex=fb_tx_empty_entity();string name=strzone(brush_model_create());
            // Initialize the reserved empty entity before individual key edits.
            // Empty slots are outside ENT_COUNT until the first full field set.
            fb_tx_entity_set(entityIndex,"\"classname\" \"func_wall\"\n",HIST_ENTITY);fb_organize_parent(entityIndex,parent);fb_tx_set_key(entityIndex,"model",name,HIST_ENTITY);
            precache_model(name);if(!fb_document_probe)fb_document_probe=spawn();setmodel(fb_document_probe,name);int destination=fb_document_probe.modelindex;strunzone(name);
            fb_poly_t *poly=&plugs[i];fb_land_attributes(poly,attrs,source.nf);fb_csg_materials(poly,attrs,source.nf);for(int f=0;f<poly->nf;f++)faces[f]=poly->faces[f].attr;
            valid=destination&&brush_history_create(destination,faces,poly->nf,contents,TRUE)!=0;
        }
        if(valid)brush_history_delete(model,id);
    }
    fb_csg_clear();for(int i=0;i<count;i++){fb_poly_free(&cutters[i]);fb_poly_free(&plugs[i]);}memfree(cutters);memfree(plugs);fb_poly_free(&source);return valid;
}
