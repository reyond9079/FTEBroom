// SPDX-License-Identifier: GPL-2.0-or-later
typedef struct {int line,model,id,entity,mixed,next;} fb_line_t;
fb_line_t *fb_lines;
int fb_line_count;
int fb_line_capacity;hashtable fb_line_lookup;var int fb_line_lookup_count=-1;
void fb_lines_lookup_prepare()
{
    if(fb_line_lookup_count==fb_line_count)return;
    if(fb_line_lookup)hash_destroytab(fb_line_lookup);fb_line_lookup=hash_createtab(max(64,fb_line_count*2),EV_INTEGER);
    for(int i=0;i<fb_line_count;i++)if(fb_lines[i].model){
        string key=sprintf("%i:%i",fb_lines[i].model,fb_lines[i].id);
        fb_lines[i].next=(int)fb_line_lookup[key];fb_line_lookup[key]=i+1i;
    }
    fb_line_lookup_count=fb_line_count;
}
void fb_lines_copy(int model,int oldid,int newid)
{
    if(!newid || oldid==newid)return;fb_lines_lookup_prepare();
    int first=(int)fb_line_lookup[sprintf("%i:%i",model,oldid)];string key=sprintf("%i:%i",model,newid);
    for(int row=first;row;){
        fb_line_t item=fb_lines[row-1];row=item.next;
        if(fb_line_count>=fb_line_capacity){
            fb_line_capacity=max(fb_line_count+1,max(64,fb_line_capacity*2));
            fb_line_t *grown=memalloc(fb_line_capacity*sizeof(fb_line_t));memcpy(grown,fb_lines,fb_line_count*sizeof(fb_line_t));memfree(fb_lines);fb_lines=grown;
        }
        item.id=newid;item.next=(int)fb_line_lookup[key];fb_lines[fb_line_count]=item;fb_line_lookup[key]=++fb_line_count;
    }
    fb_line_lookup_count=fb_line_count;
}
void fb_lines_remap(int model,int oldid,int newid)
{
    if(oldid==newid)return;fb_lines_lookup_prepare();
    string oldKey=sprintf("%i:%i",model,oldid),newKey=sprintf("%i:%i",model,newid);
    int first=(int)fb_line_lookup[oldKey];hash_delete(fb_line_lookup,oldKey);
    for(int row=first;row;){int next=fb_lines[row-1].next;fb_lines[row-1].id=newid;fb_lines[row-1].mixed=FALSE;fb_lines[row-1].next=(int)fb_line_lookup[newKey];fb_line_lookup[newKey]=row;row=next;}
}
void fb_lines_index()
{
    fb_document_tick();memfree(fb_lines);fb_line_count=0;fb_line_lookup_count=-1;fb_line_capacity=fb_node_count;
    fb_lines=memalloc(sizeof(fb_line_t)*fb_node_count);
    filestream file=fopen("map-lines.txt",FILE_READ);
    if((float)file<0) return;
    string row;int entityCount=terrain_edit(TEREDIT_ENT_COUNT),ordinal=0;
    int *entities=memalloc(max(1,entityCount)*sizeof(int));
    int *parents=memalloc(max(1,entityCount)*sizeof(int));
    hashtable brushes=hash_createtab(max(64,fb_node_count*2),EV_STRING),mixed=hash_createtab(64,EV_STRING);
    for(int e=0;e<entityCount;e++){parents[e]=-1;if(terrain_edit(TEREDIT_ENT_GET,e)!="")entities[ordinal++]=e;}
    if(entityCount)parents[0]=1;
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind<FB_NODE_BRUSH&&fb_nodes[n].id>=0&&fb_nodes[n].id<entityCount)parents[fb_nodes[n].id]=n;
    int *ordinals=memalloc(max(1,fb_node_count)*sizeof(int));
    for(int n=2;n<fb_node_count;n++)if(fb_nodes[n].kind>=FB_NODE_BRUSH){int parent=fb_nodes[n].parent;brushes[sprintf("%i:%i",parent,ordinals[parent]++)]=itos(n+1);}
    while((row=fgets(file))) {
        int words=tokenize(row);
        if(words==2 && argv(0)=="mixed") {mixed[argv(1)]="1";continue;}
        if(words!=3) continue;
        int line=stoi(argv(0)),entityOrdinal=stoi(argv(1)),brushOrdinal=stoi(argv(2));
        if(entityOrdinal<0||entityOrdinal>=ordinal)continue;
        int entityIndex=entities[entityOrdinal],parent=parents[entityIndex];
        if(parent<0 || fb_line_count>=fb_node_count) continue;
        fb_line_t item={0};item.line=line;item.entity=entityIndex;
        if(brushOrdinal>=0) {
            int node=stoi(brushes[sprintf("%i:%i",parent,brushOrdinal)])-1;
            if(node<0||node>=fb_node_count)continue;item.model=fb_nodes[node].model;item.id=fb_nodes[node].id;
        }
        fb_lines[fb_line_count++]=item;
    }
    fclose(file);
    for(int i=0;i<fb_line_count;i++)fb_lines[i].mixed=mixed[itos(fb_lines[i].line)]=="1";
    memfree(entities);memfree(parents);memfree(ordinals);hash_destroytab(brushes);hash_destroytab(mixed);
    if(cvar("ftebroom_debug_lines")) print(sprintf("FTEBROOM_LINE_INDEX %i\n",fb_line_count));
}
void fb_select_line(int line)
{
    fb_tool_refresh();fb_document_tick();
    for(int i=0;i<fb_line_count;i++) if(fb_lines[i].line==line) {
        for(int n=0;n<fb_node_count;n++) {
            int match=FALSE;
            if(fb_lines[i].model) match=fb_nodes[n].kind>=FB_NODE_BRUSH && fb_nodes[n].model==fb_lines[i].model && fb_nodes[n].id==fb_lines[i].id;
            else match=fb_nodes[n].kind<FB_NODE_BRUSH && fb_nodes[n].kind!=FB_NODE_WORLD && fb_nodes[n].id==fb_lines[i].entity;
            if(match && fb_select_in_scope(n)) {
                fb_selection_click(fb_selection_target(n),FALSE);
                cvar_set("ftebroom_selection_status","");return;
            }
        }
    }
    cvar_set("ftebroom_selection_status","No selectable object starts on this MAP line.");
}
