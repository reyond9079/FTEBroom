// SPDX-License-Identifier: GPL-2.0-or-later
// A transaction owns its redo branch until commit, so cancellation can restore it.
int fb_tx_start,fb_tx_redocount,fb_tx_serial,fb_tx_lastmarker;
int fb_tx_empty_cursor,fb_tx_empty_serial;
int fb_linked_before_commit();
void fb_autosnap_before_commit();
void fb_repeat_selection(history_t *marker);
history_t *fb_tx_redo;
var float autocvar_ftebroom_undo_levels=512;
var float autocvar_ftebroom_undo_collate_time=1;
void fb_tx_remap(int model,int oldid,int newid)
{
    for(int i=0;i<fb_tx_redocount;i++) history_remap_record(&fb_tx_redo[i],model,oldid,newid);
}
void fb_tx_remap_bulk()
{
    for(int i=0;i<fb_tx_redocount;i++)history_remap_bulk_record(&fb_tx_redo[i]);
}
void fb_history_labels()
{
    cvar_set("ftebroom_undo_name",history>history_min?historyring[(history-1)&(historyring_length-1)].label:"");
    cvar_set("ftebroom_redo_name",history<history_max?historyring[history&(historyring_length-1)].label:"");
}
int fb_selection_equal(selbrush_t *a,int na,selbrush_t *b,int nb)
{
    if(na!=nb) return FALSE;
    for(int i=0;i<na;i++) {
        if(a[i].model!=b[i].model) return FALSE;
        if(a[i].id!=b[i].id) return FALSE;
        if(!selbrush_same_mask(&a[i],&b[i])) return FALSE;
    }
    return TRUE;
}
void fb_tx_begin(string name)
{
    if(fb_tx_depth++) return;
    fb_tx_start=history;
    fb_tx_name=strzone(name);
    fb_tx_redocount=history_max-history;
    fb_tx_redo=memalloc(sizeof(history_t)*fb_tx_redocount);
    for(int i=0;i<fb_tx_redocount;i++) {
        history_t *h=&historyring[(history+i)&(historyring_length-1)];
        fb_tx_redo[i]=*h;memfill8((__variant*)h,0,sizeof(history_t));
    }
    history_max=history;
    history_transaction=-(++fb_tx_serial);
    history_t *h=history_allocate();
    h->kind=HIST_SELECTION;h->selectionop=TRUE;
    h->selectioncount=selectedbrushcount;
    h->selection=memalloc(sizeof(selbrush_t)*selectedbrushcount);
    memcpy(h->selection,selectedbrushes,sizeof(selbrush_t)*selectedbrushcount);
    h->entitybefore=fb_selected_entity();h->statebefore=strzone(fb_selection_state());
}
void fb_tx_restore_redo()
{
    for(int i=0;i<fb_tx_redocount;i++) {
        history_reserve();
        historyring[history_max&(historyring_length-1)]=fb_tx_redo[i];history_max++;
    }
    memfree(fb_tx_redo);fb_tx_redo=__NULL__;fb_tx_redocount=0;
}
void fb_tx_prune()
{
    int limit=bound(1,autocvar_ftebroom_undo_levels,8192),count=0;
    float stamp=0;
    for(int i=history_min;i<history_max;i++) {
        float next=historyring[i&(historyring_length-1)].timestamp;
        if(i==history_min || next!=stamp) count++;
        stamp=next;
    }
    while(count>limit && history_min<history) {
        stamp=historyring[history_min&(historyring_length-1)].timestamp;
        do {
            history_t *h=&historyring[history_min&(historyring_length-1)];
            history_base_revision=h->revision;history_release(h);history_min++;
        } while(history_min<history && historyring[history_min&(historyring_length-1)].timestamp==stamp);
        count--;
    }
}
void fb_tx_cancel()
{
    if(!fb_tx_depth) return;
    // Collapse nesting: cancel always rolls back the complete outer transaction.
    if(history>fb_tx_start) brush_undo();
    for(int i=fb_tx_start;i<history_max;i++) history_release(&historyring[i&(historyring_length-1)]);
    history=history_max=fb_tx_start;
    fb_tx_restore_redo();
    fb_tx_depth=0;history_transaction=0;strunzone(fb_tx_name);fb_tx_name="";
    fb_tx_lasttime=-100;
    fb_history_labels();
}
void fb_tx_commit()
{
    if(!fb_tx_depth) return;
    if(fb_tx_depth==1)fb_autosnap_before_commit();
    if(fb_tx_depth==1)if(!fb_linked_before_commit()){fb_tx_cancel();return;}
    if(--fb_tx_depth) return;
    history_t *marker=&historyring[fb_tx_start&(historyring_length-1)];
    marker->aftercount=selectedbrushcount;
    marker->afterselection=memalloc(sizeof(selbrush_t)*selectedbrushcount);
    memcpy(marker->afterselection,selectedbrushes,sizeof(selbrush_t)*selectedbrushcount);
    marker->entityafter=fb_selected_entity();marker->stateafter=strzone(fb_selection_state());
    fb_repeat_selection(marker);
    if(history==fb_tx_start+1)
        if(marker->statebefore==marker->stateafter)
            if(fb_selection_equal(marker->selection,marker->selectioncount,marker->afterselection,marker->aftercount)) {
                history_release(marker);history=history_max=fb_tx_start;
                fb_tx_restore_redo();history_transaction=0;strunzone(fb_tx_name);fb_tx_name="";
                fb_history_labels();return;
            }
    int repeat=fb_tx_name=="Nudge" || fb_tx_name=="Nudge texture" || fb_tx_name=="Rotate";
    if(repeat && !fb_tx_redocount && fb_tx_start>history_min && fb_tx_lastmarker>=history_min)
        if(gettime(0)-fb_tx_lasttime<autocvar_ftebroom_undo_collate_time)
            if(historyring[(fb_tx_start-1)&(historyring_length-1)].revision!=fb_saved_revision) {
                history_t *previous=&historyring[fb_tx_lastmarker&(historyring_length-1)];
                if(previous->label==marker->label)
                    if(previous->stateafter==marker->statebefore)
                        if(fb_selection_equal(previous->afterselection,previous->aftercount,marker->selection,marker->selectioncount))
                            for(int i=fb_tx_start;i<history;i++) historyring[i&(historyring_length-1)].timestamp=previous->timestamp;
            }
    for(int i=0;i<fb_tx_redocount;i++) history_release(&fb_tx_redo[i]);
    memfree(fb_tx_redo);fb_tx_redo=__NULL__;fb_tx_redocount=0;
    fb_tx_lastmarker=fb_tx_start;fb_tx_lasttime=gettime(0);
    history_transaction=0;strunzone(fb_tx_name);fb_tx_name="";
    fb_tx_prune();fb_history_labels();
}
void fb_tx_entity_set(int index,string data,int kind)
{
    int entityCount=terrain_edit(TEREDIT_ENT_COUNT);
    if(index<0 || index>entityCount) return;
    string replacement=strzone(data);
    string before=strzone(terrain_edit(TEREDIT_ENT_GET,index));
    if(before==replacement) {strunzone(before);strunzone(replacement);return;}
    history_t *h=history_allocate();h->kind=kind;h->entityindex=index;
    h->beforefields=before;h->afterfields=replacement;
    if(replacement=="") terrain_edit(TEREDIT_ENT_SET,index,__NULL__);
    else terrain_edit(TEREDIT_ENT_SET,index,replacement);
    editor_ents_refresh(index);
}
int fb_tx_empty_entity()
{
    // FTE grows entity storage in blocks. Reuse empty slots instead of leaking
    // a whole block per plug, especially across cancelled seed attempts.
    if(!fb_tx_depth || fb_tx_empty_serial!=fb_tx_serial){fb_tx_empty_cursor=1;fb_tx_empty_serial=fb_tx_serial;}
    int count=terrain_edit(TEREDIT_ENT_COUNT);
    for(;fb_tx_empty_cursor<count;fb_tx_empty_cursor++)
        if(terrain_edit(TEREDIT_ENT_GET,fb_tx_empty_cursor)=="")return fb_tx_empty_cursor++;
    return count;

}
void fb_tx_set_key(int index,string key,string value,int kind)
{
    int entityCount=terrain_edit(TEREDIT_ENT_COUNT);
    if(index<0 || index>=entityCount) return;
    if(kind<HIST_ENTITY || kind>HIST_TREE) return;
    hashtable fields=hash_createtab(32,EV_STRING);
    int count=fb_entity_tokenize(terrain_edit(TEREDIT_ENT_GET,index));
    for(int i=0;i<count;i+=2) fields[argv(i)]=argv(i+1);
    if(fields[key]==value) {hash_destroytab(fields);return;}
    fields[key]=value;
    string data="";
    for(int i=0;;i++) {
        string name=hash_getkey(fields,i);
        if isnull(name) break;
        string escaped=fields[name];
        escaped=strreplace("\\","\\\\",escaped);
        escaped=strreplace("\"","\\\"",escaped);
        escaped=strreplace("\n","\\n",escaped);
        data=strcat(data,"\"",name,"\" \"",escaped,"\"\n");
    }
    fb_tx_begin(kind==HIST_ENTITY?"Entity property":(kind==HIST_NODEFLAGS?"Layer flags":"Reparent"));
    fb_tx_entity_set(index,data,kind);fb_tx_commit();
    hash_destroytab(fields);
}
