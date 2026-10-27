// SPDX-License-Identifier: GPL-2.0-or-later
typedef struct {int node,type;string key,message,fix;} fb_issue_t;
fb_issue_t *fb_issues;int fb_issue_count,fb_issue_capacity,fb_issue_epoch;var int fb_issue_seen=-1;
void fb_issue_add(int node,int type,string key,string message,string fix)
{
    if(fb_issue_count>=16384)return;
    if(fb_issue_count==fb_issue_capacity){int capacity=max(64,fb_issue_capacity*2);fb_issue_t *items=memalloc(capacity*sizeof(fb_issue_t));
        if(fb_issue_count)memcpy(items,fb_issues,fb_issue_count*sizeof(fb_issue_t));memfree(fb_issues);fb_issues=items;fb_issue_capacity=capacity;}
    fb_issue_t *issue=&fb_issues[fb_issue_count++];issue->node=node;issue->type=type;
    issue->key=strzone(key);issue->message=strzone(message);issue->fix=strzone(fix);
}
void fb_issues_scan()
{
    fb_document_tick();
    for(int i=0;i<fb_issue_count;i++){strunzone(fb_issues[i].key);strunzone(fb_issues[i].message);strunzone(fb_issues[i].fix);}fb_issue_count=0;
    hashtable targets=hash_createtab(256,EV_INTEGER),sources=hash_createtab(256,EV_INTEGER);
    hashtable lines=hash_createtab(max(64,fb_line_count),EV_INTEGER),mixed=hash_createtab(64,EV_INTEGER);
    for(int l=0;l<fb_line_count;l++) {
        string identity=fb_lines[l].model?sprintf("b%i/%i",fb_lines[l].model,fb_lines[l].id):sprintf("e%i",fb_lines[l].entity);
        lines[identity]=fb_lines[l].line;if(fb_lines[l].mixed)mixed[identity]=1;
    }
    vector softLow='0 0 0',softHigh='0 0 0';int soft=tokenize(cvar_string("ftebroom_map_bounds"))==6;
    if(soft){softLow=[stof(argv(0)),stof(argv(1)),stof(argv(2))];softHigh=[stof(argv(3)),stof(argv(4)),stof(argv(5))];}
    int *children=memalloc(fb_node_count*sizeof(int));
    for(int n=2;n<fb_node_count;n++) {
        int parent=fb_nodes[n].parent;if(parent>=0)children[parent]++;
        if(fb_nodes[n].kind<FB_NODE_BRUSH){string name=fb_nodes[n].fields["targetname"],target=fb_nodes[n].fields["target"];
            if(name!="")targets[name]=1;if(target!="")sources[target]=1;}
    }
    for(int n=0;n<fb_node_count;n++) {
        fb_node_t node=fb_nodes[n];if(n==1)continue;
        if(node.kind<FB_NODE_BRUSH) {
            string classname=node.fields["classname"];int definition=0;
            if(fb_entity_kinds)definition=(int)fb_entity_kinds[classname];
            if(node.kind==FB_NODE_GROUP && !children[n])fb_issue_add(n,0,"","Empty group","Delete");
            if(node.kind==FB_NODE_ENTITY) {
                if(classname=="")fb_issue_add(n,1,"","Missing classname","Delete");
                else if((int)fb_entity_kinds && !definition)fb_issue_add(n,2,"","Missing entity definition","Delete");
                if(definition==2 && !children[n])fb_issue_add(n,3,"","Empty brush entity","Delete");
                if(definition==1 && children[n])fb_issue_add(n,4,"","Point entity contains brushes","Move brushes to world");
            }
            for(int k=0;;k++) {
                string key=hash_getkey(node.fields,k);if isnull(key)break;string value=node.fields[key];
                if(key=="")fb_issue_add(n,5,key,"Empty property key","Delete property");
                if(value=="")fb_issue_add(n,6,key,strcat("Empty property value: ",key),"Delete property");
                if(strlen(key)>1023)fb_issue_add(n,7,key,"Property key exceeds 1023 bytes","Delete property");
                if(strlen(value)>1023)fb_issue_add(n,8,key,strcat("Property value exceeds 1023 bytes: ",key),"Truncate value");
                if(strstrofs(key,"\"",0)>=0 || strstrofs(value,"\"",0)>=0)fb_issue_add(n,9,key,strcat("Double quotation marks: ",key),"Replace quotation marks");
                if(strstrofs(value,"\\",0)>=0 && (key=="model" || key=="noise" || key=="_tb_def" || key=="music"))fb_issue_add(n,10,key,strcat("Backslashes in path: ",key),"Replace backslashes");
                if(value!="" && key=="target" && !(int)targets[value])fb_issue_add(n,11,key,"Missing entity link target","Delete property");
                if(value!="" && key=="targetname" && !(int)sources[value])fb_issue_add(n,12,key,"Missing entity link source","Delete property");
            }
        }else if(node.kind==FB_NODE_BRUSH) {
            if((int)mixed[sprintf("b%i/%i",node.model,node.id)])fb_issue_add(n,17,"","Mixed source brush content flags","");
            int count=brush_get(node.model,node.id,__NULL__,0,__NULL__);
            brushface_t *faces=memalloc(count*sizeof(brushface_t));brush_get(node.model,node.id,faces,count,__NULL__);
            int invalid=FALSE,fractional=FALSE;
            for(int f=0;f<count;f++) {
                if(vlen(faces[f].sdir)<0.000001 || vlen(faces[f].tdir)<0.000001 || !fb_geom_finite(vlen(faces[f].sdir)) || !fb_geom_finite(vlen(faces[f].tdir)))invalid=TRUE;
                static vector points[64];int vertices=brush_getfacepoints(node.model,node.id,f+1,points,64);
                for(int v=0;v<vertices;v++)for(int axis=0;axis<3;axis++)if(fabs(points[v][axis]-rint(points[v][axis]))>0.001)fractional=TRUE;
            }
            if(invalid)fb_issue_add(n,13,"","Invalid UV scale","Reset UV scale");
            if(fractional)fb_issue_add(n,14,"","Non-integer vertices","Snap vertices to integer");memfree(faces);
        }
        vector bounds[2];int bounded=FALSE;
        if(node.kind>=FB_NODE_BRUSH)bounded=brush_getfacepoints(node.model,node.id,0,bounds,2)==2;
        else if(node.kind==FB_NODE_ENTITY && !children[n])bounded=fb_entity_bounds(node.id,bounds);
        if(bounded){int outside=FALSE;for(int a=0;a<3;a++)if(bounds[0][a]<-autocvar_ftebroom_world_bound || bounds[1][a]>autocvar_ftebroom_world_bound)outside=TRUE;
            if(outside)fb_issue_add(n,15,"","Object outside world bounds","Delete");
            else if(soft){int softOutside=FALSE;for(int a=0;a<3;a++)if(bounds[0][a]<softLow[a] || bounds[1][a]>softHigh[a])softOutside=TRUE;
                if(softOutside)fb_issue_add(n,16,"","Object outside soft map bounds","Delete");}}
    }
    memfree(children);hash_destroytab(targets);hash_destroytab(sources);
    fb_issue_seen=(fb_grid_geometry_epoch+fb_incremental_epoch);fb_issue_epoch++;
    filestream file=fopen("ftebroom-issues.txt",FILE_WRITE);
    if((float)file>=0) {
        fputs(file,strcat(itos(fb_issue_epoch)," ",fb_map_quote(fb_nodes[0].fields["_tb_mod"]),"\n"));
        for(int i=0;i<fb_issue_count;i++) {
            fb_issue_t issue=fb_issues[i];fb_node_t node=fb_nodes[issue.node];
            int line=(int)lines[node.kind>=FB_NODE_BRUSH?sprintf("b%i/%i",node.model,node.id):sprintf("e%i",node.id)];
            fputs(file,sprintf("%i %i %i %i %i ",i,issue.type,node.model,node.id,line));
            fputs(file,strcat(fb_map_quote(issue.message)," ",fb_map_quote(issue.fix)," ",fb_map_quote(issue.key),"\n"));
        }
        fclose(file);
    }
    hash_destroytab(lines);hash_destroytab(mixed);
    cvar_set("ftebroom_issues_epoch",itos(fb_issue_epoch));
}
void fb_issue_property(fb_issue_t *issue)
{
    fb_node_t node=fb_nodes[issue->node];string data="";
    for(int k=0;;k++) {
        string key=hash_getkey(node.fields,k);if isnull(key)break;string value=node.fields[key];
        if(key==issue->key) {
            if(issue->type==5 || issue->type==6 || issue->type==7 || issue->type==11 || issue->type==12)continue;
            if(issue->type==8)value=substring(value,0,1023);
            if(issue->type==9){key=strreplace("\"","'",key);value=strreplace("\"","'",value);}
            if(issue->type==10)value=strreplace("\\","/",value);
        }
        data=strcat(data,fb_map_quote(key)," ",fb_map_quote(value),"\n");
    }
    fb_tx_entity_set(node.id,data,HIST_ENTITY);
}
int fb_issues_command()
{
    if(argv(0)!="ftebroom_issue")return FALSE;
    string operation=strzone(argv(1));int index=stoi(argv(2)),epoch=stoi(argv(3));
    if(operation=="scan"){strunzone(operation);fb_issues_scan();return TRUE;}
    if(operation=="removemod") {
        string remove=strzone(argv(2));
        if(epoch==fb_issue_epoch && fb_issue_seen==(fb_grid_geometry_epoch+fb_incremental_epoch)) {
            string mods=fb_nodes[0].fields["_tb_mod"],kept="";int start=0;
            for(int p=0;p<=strlen(mods);p++)if(p==strlen(mods) || substring(mods,p,1)==";") {
                string part=substring(mods,start,p-start);start=p+1;
                if(part!="" && part!=remove)kept=strcat(kept,kept!=""?";":"",part);
            }
            fb_tx_begin("Remove missing mod");
            if(kept!="")fb_tx_set_key(0,"_tb_mod",kept,HIST_ENTITY);
            else {fb_issue_t property={0};property.type=6;property.key="_tb_mod";fb_issue_property(&property);}
            fb_tx_commit();
        }
        strunzone(remove);strunzone(operation);return TRUE;
    }
    if(epoch!=fb_issue_epoch || fb_issue_seen!=(fb_grid_geometry_epoch+fb_incremental_epoch) || index<0 || index>=fb_issue_count){strunzone(operation);return TRUE;}
    fb_issue_t issue=fb_issues[index];fb_node_t node=fb_nodes[issue.node];fb_tool_refresh();
    fb_tx_begin(operation=="fix"?"Fix map issue":"Select map issue");fb_selection_clear();
    fb_selection_set(issue.node,TRUE);
    if(operation=="fix") {
        if(issue.type==0 || issue.type==1 || issue.type==2 || issue.type==3 || issue.type==15 || issue.type==16) {
            if(node.kind<FB_NODE_BRUSH && !selectedbrushcount && !fb_selected_entity_count)fb_tx_entity_set(node.id,"",HIST_ENTITY);
            else fb_delete_selection();
        }else if(issue.type==4) {
            for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].parent==issue.node && fb_nodes[n].kind>=FB_NODE_BRUSH)
                fb_organize_move_brush(fb_nodes[n].model,fb_nodes[n].id,1,FALSE);
        }else if(issue.type<=12)fb_issue_property(&issue);
        else if(issue.type==13) {
            int count=brush_get(node.model,node.id,__NULL__,0,__NULL__),contents;
            brushface_t *faces=memalloc(count*sizeof(brushface_t));brush_get(node.model,node.id,faces,count,&contents);
            for(int f=0;f<count;f++)reset_texturecoords(&faces[f]);brush_history_edit(node.model,node.id,faces,count,contents);memfree(faces);
        }else if(issue.type==14)fb_vertex_snap(1);
    }
    fb_tx_commit();strunzone(operation);return TRUE;
}
void fb_issues_tick()
{
    if(cvar("ftebroom_issues_visible") && fb_issue_seen!=(fb_grid_geometry_epoch+fb_incremental_epoch) && !fb_tx_depth && !fb_active)fb_issues_scan();
}
