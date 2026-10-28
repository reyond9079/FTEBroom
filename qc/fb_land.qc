// Copyright (C) 2026 Kristian Duske
// SPDX-License-Identifier: GPL-3.0-or-later
// Procedural terrain adapted from ReyondBroom's TerrainGeneratorDialog.
float fb_land_hash(int x,int y,int seed,int salt)
{
    int h=seed^salt;h^=x+0x9e3779b9i+(h<<6)+(h>>2);h^=y+0x85ebca6bi+(h<<6)+(h>>2);
    h^=h>>16;h*=0x7feb352di;h^=h>>15;h*=0x846ca68bi;h^=h>>16;
    return (float)(h&0x00ffffffi)/16777216;
}
float fb_land_noise(float u,float v)
{
    float frequency=bound(0.01,cvar("ftebroom_land_frequency"),64);
    float x=u*frequency,y=v*frequency,nearest=1000000,second=1000000;
    int cx=floor(x),cy=floor(y),seed=cvar("ftebroom_land_seed");
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++) {
        float px=cx+dx+fb_land_hash(cx+dx,cy+dy,seed,0x1234i),py=cy+dy+fb_land_hash(cx+dx,cy+dy,seed,0x9abci);
        float distance=sqrt((x-px)*(x-px)+(y-py)*(y-py));
        if(distance<nearest){second=nearest;nearest=distance;}else second=min(second,distance);
    }
    float roughness=bound(0,cvar("ftebroom_land_roughness"),2)*0.5;
    return (1-bound(0,nearest/sqrt(2),1))*(1-roughness)+bound(0,(second-nearest)*1.35,1)*roughness;
}
void fb_land_attributes(fb_poly_t *poly,brushface_t *source,int count)
{
    for(int f=0;f<poly->nf;f++) {
        brushface_t *face=&poly->faces[f].attr;int best=0;float alignment=-2;
        for(int a=0;a<count;a++)if(face->planenormal*source[a].planenormal>alignment){alignment=face->planenormal*source[a].planenormal;best=a;}
        vector normal=face->planenormal;float distance=face->planedist;strunzone(face->shadername);
        *face=source[best];face->shadername=strzone(source[best].shadername);face->planenormal=normal;face->planedist=distance;
    }
}
int fb_land_apply()
{
    int nx=bound(1,cvar("ftebroom_land_u"),32),ny=bound(1,cvar("ftebroom_land_v"),32);
    int side=bound(0,cvar("ftebroom_land_side"),5),axis=2,u=0,v=1;float sign=1;
    if(side&1)sign=-1;if(side>=2&&side<4){axis=0;u=1;v=2;}if(side>=4){axis=1;u=0;v=2;}
    float height=bound(0,cvar("ftebroom_land_height"),8192)*bound(0,cvar("ftebroom_land_intensity"),4);
    int total=selectedbrushcount;if(!total){fb_geom_error="Select a brush or patch first";return FALSE;}
    selbrush_t *selection=memalloc(total*sizeof(selbrush_t));memcpy(selection,selectedbrushes,total*sizeof(selbrush_t));
    int valid=TRUE,made=0;brush_deselectall();
    for(int b=0;b<total&&valid;b++) {
        int model=selection[b].model,id=selection[b].id;patchinfo_t info;
        int cp=patch_getcp(model,id,__NULL__,0,&info);
        if(cp) {
            if(cvar("ftebroom_classic_map")){valid=FALSE;fb_geom_error="Quake 1 does not support patches";break;}
            patchvert_t *points=memalloc(cp*sizeof(patchvert_t));patch_getcp(model,id,points,cp,&info);info.shadername=strzone(info.shadername);
            int width=bound(3,cvar("ftebroom_land_patch_u"),31)|1,heightPoints=bound(3,cvar("ftebroom_land_patch_v"),31)|1;
            for(int dim=0;dim<2;dim++) {
                int target=dim?heightPoints:width,current=dim?info.cpheight:info.cpwidth;
                while(current!=target) {
                    int next=current<target?current+2:target;
                    patchvert_t *output=fb_patch_dimension(points,info.cpwidth,info.cpheight,dim,next);memfree(points);points=output;
                    current=next;if(dim)info.cpheight=current;else info.cpwidth=current;
                }
            }
            for(int y=0;y<info.cpheight;y++)for(int x=0;x<info.cpwidth;x++) {
                int at=y*info.cpwidth+x;points[at].xyz[axis]+=sign*height*fb_land_noise((float)x/(info.cpwidth-1),(float)y/(info.cpheight-1));
                if(!fb_geom_point(points[at].xyz))valid=FALSE;
            }
            if(valid){brush_select(model,id);valid=patch_history_edit(model,id,points,info)!=0;}
            memfree(points);strunzone(info.shadername);made++;continue;
        }
        static brushface_t attrs[64];int contents,count=brush_get(model,id,attrs,64,&contents);vector bounds[2];
        if(count<4||count>64||brush_getfacepoints(model,id,0,bounds,2)!=2){valid=FALSE;fb_geom_error="Terrain requires a valid brush or patch";break;}
        vector low=bounds[0],high=bounds[1];float bottom=low[axis],top=high[axis];if(sign<0){bottom=high[axis];top=low[axis];}
        for(int y=0;y<ny&&valid;y++)for(int x=0;x<nx&&valid;x++) {
            vector vertices[8];
            for(int corner=0;corner<4;corner++) {
                int xx=x+(corner==1||corner==2),yy=y+(corner>=2);
                vector point=low;point[u]=low[u]+(high[u]-low[u])*xx/nx;point[v]=low[v]+(high[v]-low[v])*yy/ny;point[axis]=bottom;vertices[corner]=point;
                float noise=fb_land_noise((float)xx/nx,(float)yy/ny);
                if(!cvar("ftebroom_land_triangles"))noise=fb_land_noise((x+0.5)/nx,(y+0.5)/ny);
                point[axis]=top+sign*height*noise;vertices[corner+4]=point;
            }
            int triangles=cvar("ftebroom_land_triangles")!=0;
            for(int part=0;part<(triangles?2:1)&&valid;part++) {
                vector prism[8];int n=8;
                if(triangles) {
                    int a=0,bb=1,c=2;if(part){bb=2;c=3;}
                    if((x+y)&1){a=part?1:0;bb=part?2:1;c=3;}
                    prism[0]=vertices[a];prism[1]=vertices[bb];prism[2]=vertices[c];prism[3]=vertices[a+4];prism[4]=vertices[bb+4];prism[5]=vertices[c+4];n=6;
                }else memcpy(prism,vertices,sizeof(vertices));
                fb_poly_t poly={0};valid=fb_hull(prism,n,&poly);
                if(valid){fb_land_attributes(&poly,attrs,count);static brushface_t faces[64];for(int f=0;f<poly.nf;f++)faces[f]=poly.faces[f].attr;valid=brush_history_create(model,faces,poly.nf,contents,TRUE)!=0;made++;}
                fb_poly_free(&poly);if(made>2048){valid=FALSE;fb_geom_error="Terrain exceeds 2048 brushes";}
            }
        }
        if(valid)brush_history_delete(model,id);
    }
    memfree(selection);return valid&&made>0;
}
int fb_land_command()
{
    if(argv(0)!="ftebroom_land")return FALSE;
    fb_tool_refresh();fb_tx_begin("Generate terrain");
    if(fb_land_apply())fb_tx_commit();else {fb_tx_cancel();cvar_set("ftebroom_selection_status",fb_geom_error);}
    fb_document_tick();return TRUE;
}
