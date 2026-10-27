// SPDX-License-Identifier: GPL-2.0-or-later
void fb_document_remap(int model,int oldid,int newid);
void fb_lines_copy(int model,int oldid,int newid);
void fb_tx_remap(int model,int oldid,int newid);
void fb_tool_refresh();
void fb_visibility_set(int model,int id,int hidden);
void fb_rotate_restore_center(vector center);
void editor_ents_reload();
void editor_ents_refresh(int index);
void fb_tx_begin(string name);
void fb_tx_commit();
void fb_history_labels();
int fb_tx_depth;
float fb_tx_lasttime;
string fb_tx_name;
int history_revision_serial,history_base_revision;
enum : int { HIST_GEOMETRY, HIST_SELECTION, HIST_ENTITY, HIST_NODEFLAGS, HIST_TREE, HIST_VISIBILITY, HIST_PIVOT, HIST_TERRAIN };
typedef struct {
    float timestamp;
    int revision,kind,selectionop,selectioncount,aftercount;
    selbrush_t *selection,*afterselection;
    int entitybefore,entityafter;
    string statebefore,stateafter;
    string label,beforefields,afterfields;
    int entityindex;
    int terrainhandle;
    int brushmodel,wasdelete,id;
    brushface_t *brushdata;
    patchvert_t *patchdata;
    union {
        struct {int numfaces,contents;} brushinfo;
        patchinfo_t patchinfo;
    };
} history_t;
history_t *historyring;
int historyring_length,history_min,history,history_max;
float history_transaction;
void history_release(history_t *h)
{
    if(h->kind==HIST_TERRAIN)terrain_edit(203,(float)h->terrainhandle);
    if(h->brushdata) for(int f=0;f<h->brushinfo.numfaces;f++) strunzone(h->brushdata[f].shadername);
    if(h->patchdata) strunzone(h->patchinfo.shadername);
    memfree(h->brushdata);memfree(h->patchdata);memfree(h->selection);memfree(h->afterselection);
    strunzone(h->statebefore);strunzone(h->stateafter);
    strunzone(h->label);strunzone(h->beforefields);strunzone(h->afterfields);
    memfill8((__variant*)h,0,sizeof(history_t));
}
void history_reserve()
{
    if(historyring_length && history_max-history_min<historyring_length) return;
    int length=max(512,historyring_length*2);
    history_t *larger=memalloc(sizeof(history_t)*length);
    for(int i=history_min;i<history_max;i++) larger[i&(length-1)]=historyring[i&(historyring_length-1)];
    memfree(historyring);historyring=larger;historyring_length=length;
}
void history_remap_record(history_t *h,int model,int oldid,int newid)
{
    for(int j=0;j<h->selectioncount;j++) if(h->selection[j].model==model) if(h->selection[j].id==oldid) h->selection[j].id=newid;
    for(int j=0;j<h->aftercount;j++) if(h->afterselection[j].model==model) if(h->afterselection[j].id==oldid) h->afterselection[j].id=newid;
    if(h->brushmodel==model) if(h->id==oldid) h->id=newid;
}
void history_remap(int model,int oldid,int newid)
{
    fb_document_remap(model,oldid,newid);
    for(int i=history_min;i<history_max;i++) history_remap_record(&historyring[i&(historyring_length-1)],model,oldid,newid);
    fb_tx_remap(model,oldid,newid);
    int selected=brush_isselected(model,oldid);
    if(selected) {
        brush_selected(model,oldid,-1,FALSE);brush_selected(model,newid,-1,TRUE);
        selectedbrushes[selected-1].id=newid;brush_selection_invalidate();
    }
}
string fb_selection_state();
void fb_selection_restore_state(string state);
int fb_selected_entity();
void fb_restore_entity_selection(int index);
void history_restore_list(selbrush_t *items,int count,int ent)
{
    brush_deselectall();
    for(int i=0;i<count;i++) {
        selbrush_t item=items[i];
        if(brush_get(item.model,item.id,__NULL__,0,__NULL__)<=0)
            if(patch_getcp(item.model,item.id,__NULL__,0,__NULL__)<=0) continue;
        brush_select(item.model,item.id);
        memcpy(&selectedbrushes[selectedbrushcount-1].facemask,&item.facemask,sizeof(item.facemask));
        selectedbrushes[selectedbrushcount-1].faceonly=item.faceonly;
    }
    fb_restore_entity_selection(ent);
}
void history_apply(history_t *h,int redo)
{
    if(h->kind==HIST_TERRAIN){terrain_edit(202,(float)h->terrainhandle,(float)redo);return;}
    if(h->kind==HIST_PIVOT){fb_rotate_restore_center(stov(redo?h->afterfields:h->beforefields));return;}
    if(h->kind==HIST_VISIBILITY) {fb_visibility_set(h->brushmodel,h->id,stoi(redo?h->afterfields:h->beforefields));return;}
    if(h->kind==HIST_SELECTION) {
        if(!redo) {history_restore_list(h->selection,h->selectioncount,h->entitybefore);fb_selection_restore_state(h->statebefore);}
        return;
    }
    if(h->kind>=HIST_ENTITY) {
        string fields=redo?h->afterfields:h->beforefields;
        if(fields=="") terrain_edit(TEREDIT_ENT_SET,h->entityindex,__NULL__);
        else terrain_edit(TEREDIT_ENT_SET,h->entityindex,fields);
        editor_ents_refresh(h->entityindex);return;
    }
    int create=h->wasdelete;
    if(redo) create=!create;
    if(create) {
        int id;
        if(h->patchdata) id=patch_create(h->brushmodel,0,h->patchdata,h->patchinfo);
        else id=brush_create(h->brushmodel,h->brushdata,h->brushinfo.numfaces,h->brushinfo.contents);
        history_remap(h->brushmodel,h->id,id);h->id=id;
        brush_select(h->brushmodel,id);
    } else {brush_deselect(h->brushmodel,h->id);brush_delete(h->brushmodel,h->id);}
}
void fb_terrain_finish();
void brush_undo()
{
    fb_terrain_finish();fb_tool_refresh();fb_tx_lasttime=-100;
    if(history<=history_min) return;
    float stamp=historyring[(history-1)&(historyring_length-1)].timestamp;
    do {history--;history_apply(&historyring[history&(historyring_length-1)],FALSE);}
    while(history>history_min && historyring[(history-1)&(historyring_length-1)].timestamp==stamp);
    fb_history_labels();
}
void brush_redo()
{
    fb_terrain_finish();fb_tool_refresh();fb_tx_lasttime=-100;
    if(history>=history_max) return;
    float stamp=historyring[history&(historyring_length-1)].timestamp;
    int selection=-1;
    do {
        history_t *h=&historyring[history&(historyring_length-1)];
        if(h->kind==HIST_SELECTION) selection=history;
        history_apply(h,TRUE);history++;
    } while(history<history_max && historyring[history&(historyring_length-1)].timestamp==stamp);
    if(selection>=0) {
        history_t *selected=&historyring[selection&(historyring_length-1)];
        history_restore_list(selected->afterselection,selected->aftercount,selected->entityafter);
        fb_selection_restore_state(selected->stateafter);
    }
    fb_history_labels();
}
history_t *history_allocate()
{
    for(int i=history;i<history_max;i++) history_release(&historyring[i&(historyring_length-1)]);
    history_max=history;history_reserve();
    history_t *h=&historyring[history&(historyring_length-1)];
    history++;history_max=history;
    memfill8((__variant*)h,0,sizeof(history_t));
    h->revision=++history_revision_serial;
    h->timestamp=history_transaction?history_transaction:gettime(0);
    h->label=strzone(fb_tx_depth?fb_tx_name:"Edit");
    return h;
}
void history_deallocate(history_t *h)
{
    if(h==&historyring[(history-1)&(historyring_length-1)]) {
        history_release(h);history--;history_max=history;
    }
}
//create and journal.
int(int mod, brushface_t *faces, int numfaces, int contents, float selectnow) brush_history_create =
{
	history_t *h = history_allocate();

	h->brushdata = memalloc(sizeof(brushface_t) * numfaces);
	memcpy(h->brushdata, faces, sizeof(brushface_t) * numfaces);
	for (int f=0; f<numfaces; f++) h->brushdata[f].shadername = strzone(faces[f].shadername);
	h->brushinfo.numfaces = numfaces;
	h->brushinfo.contents = contents;
	h->brushmodel = mod;
	h->wasdelete = FALSE;

	h->id = brush_create(h->brushmodel, h->brushdata, h->brushinfo.numfaces, h->brushinfo.contents);

	if (!h->id)
		history_deallocate(h);
	else if (selectnow)
		brush_select(mod, h->id);
	return h->id;
};
//create and journal.
int(int mod, patchvert_t *vert, patchinfo_t info, float selectnow) patch_history_create =
{
    if(cvar("ftebroom_quake1")){print("ReyondBroom: patches are unavailable in Quake 1.\n");return 0;}
	int totalcp = info.cpwidth * info.cpheight;
	history_t *h = history_allocate();

	h->patchdata = memalloc(sizeof(patchvert_t) * totalcp);
	memcpy(h->patchdata, vert, sizeof(patchvert_t) * totalcp);
	h->patchinfo = info;
    h->patchinfo.shadername=strzone(info.shadername);
	h->brushmodel = mod;
	h->wasdelete = FALSE;

	h->id = patch_create(h->brushmodel, 0, vert, info);

	if (!h->id)
		history_deallocate(h);
	else if (selectnow)
		brush_select(mod, h->id);
	return h->id;
};
//delete and journal.
void(int mod, int id) brush_history_delete =
{
	int numfaces = brush_get(mod, id, __NULL__, 0, __NULL__);
	int numcp = patch_getcp(mod, id, __NULL__, 0, __NULL__);
	brush_deselect(mod, id);

	if (!numfaces && !numcp)
		return;	//doesn't exist or something, some kind of error. we can't log a delete if it didn't delete anything, if only because we can't recreate it if its undone.
		
	history_t *h = history_allocate();

	h->brushmodel = mod;
	h->id = id;
	h->wasdelete = TRUE;

	if (numcp)
	{
		h->patchdata = memalloc(sizeof(patchvert_t) * numcp);
		patch_getcp(mod, id, h->patchdata, numcp, &h->patchinfo);
        h->patchinfo.shadername=strzone(h->patchinfo.shadername);
	}
	else
	{
		h->brushdata = memalloc(sizeof(brushface_t) * numfaces);
		h->brushinfo.numfaces = brush_get(mod, id, h->brushdata, numfaces, &h->brushinfo.contents);
        for (int f=0; f<h->brushinfo.numfaces; f++) h->brushdata[f].shadername = strzone(h->brushdata[f].shadername);
	}
	brush_delete(mod, id);
};

int(int mod, int id, brushface_t *faces, int numfaces, int contents) brush_history_edit =
{
    int sourceid=id;
    // The delete snapshot also calls brush_get; preserve incoming temporary strings.
    brushface_t *saved = memalloc(sizeof(brushface_t)*numfaces);
    memcpy(saved, faces, sizeof(brushface_t)*numfaces);
    for (int f=0; f<numfaces; f++) saved[f].shadername = strzone(faces[f].shadername);
	int wasselected = brush_isselected(mod, id);
	if (wasselected) {
        // Keep the selection slot while replacing the underlying brush.
        selectedbrushes[wasselected-1].id = -id;
        brush_selection_invalidate();
    }
	brush_history_delete(mod, id);
	id = brush_history_create(mod, saved, numfaces, contents, FALSE);
    fb_lines_copy(mod,sourceid,id);
    for (int f=0; f<numfaces; f++) strunzone(saved[f].shadername);
    memfree(saved);
	if (wasselected)
	{
		selectedbrushes[wasselected-1].id = id;brush_selection_invalidate();
		brush_selected(mod, id, -1, TRUE);
	}
	return id;
};
int(int mod, int id, patchvert_t *vert, patchinfo_t info) patch_history_edit =
{
    int sourceid=id;
	int wasselected = brush_isselected(mod, id);
	if (wasselected) {
        // Keep the selection slot while replacing the underlying brush.
        selectedbrushes[wasselected-1].id = -id;
        brush_selection_invalidate();
    }
	brush_history_delete(mod, id);
	id = patch_history_create(mod, vert, info, FALSE);
    fb_lines_copy(mod,sourceid,id);
	if (wasselected)
	{
		selectedbrushes[wasselected-1].id = id;brush_selection_invalidate();
		brush_selected(mod, id, -1, TRUE);
	}
	return id;
};


void brush_history_deselect()
{
    if(!selectedbrushcount && fb_selected_entity()<=0) return;
    fb_tx_begin("Deselect");brush_deselectall();fb_restore_entity_selection(-1);fb_tx_commit();
}
