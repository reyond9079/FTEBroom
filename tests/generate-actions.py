from pathlib import Path
import json
p=Path(__file__).resolve().parents[1]
rows=[]
def add(group,label,key="",context="CtxAny",command=None,gesture=""):
    rows.append(dict(id=f"Menu/{group}/{label}",label=label,keys=key,context=context,command=command,gesture=gesture))
for label,key in [
("New","Ctrl+n"),("Open...","Ctrl+o"),("Save","Ctrl+s"),("Save as...","Ctrl+Shift+s"),("Close","Ctrl+w"),
("Reload Material Collections",""),("Reload Entity Definitions",""),("Preferences...",""),
("Export Map...",""),("Export OBJ...",""),("Export selected BSP model...",""),("Load Point File...",""),("Reload Point File",""),("Unload Point File",""),
("Load Portal File...",""),("Reload Portal File",""),("Unload Portal File",""),("Revert",""),("Game / resources...",""),("Exit","")]:
    cmd={"New":"ftebroom_ui new","Close":"ftebroom_ui close","Exit":"ftebroom_ui exit","Save as...":"ftebroom_ui saveas","Open...":"ftebroom_ui open","Save":"ftebroom_ui save","Game / resources...":"ftebroom_ui resources","Reload Material Collections":"ftebroom_assets_ready","Preferences...":"ftebroom_ui preferences"}.get(label)
    add("Edit" if label=="Preferences..." else "File",label,key,command=cmd)
for label,key in [
("Undo","Ctrl+z"),("Redo","Ctrl+Shift+z;Ctrl+y"),("Repeat Last Commands","Ctrl+r"),("Clear Repeatable Commands","Ctrl+Shift+r"),
("Cut","Ctrl+x"),("Copy","Ctrl+c"),("Paste","Ctrl+v"),("Paste at Original Position","Ctrl+Alt+v"),
("Duplicate","Ctrl+d"),("Delete","del;backspace"),("Move objects...","Ctrl+Alt+m"),("Select All","Ctrl+a"),
("Deselect All","Ctrl+Shift+a"),("Invert Selection","Ctrl+Alt+a"),("Select Siblings","Ctrl+b"),
("Select by Line Number...",""),("Select Touching","Ctrl+t"),("Select Inside","Ctrl+e"),("Select Tall","Ctrl+Shift+e"),
("Group","Ctrl+g"),("Ungroup","Ctrl+Shift+g"),("Rename Groups","Ctrl+Alt+g"),("Create Linked Duplicate","Ctrl+Shift+d"),
("CSG Convex Merge","Ctrl+j"),("CSG Subtract","Ctrl+k"),("CSG Hollow","Ctrl+Shift+k"),("CSG Intersect","Ctrl+l"),
("Snap Vertices to Integer","Ctrl+Shift+v"),("Snap Vertices to Grid","Ctrl+Alt+Shift+v"),("Convert Selection to Patches","Ctrl+p"),
("UV Lock","u"),("Texture Lock",""),("Make Structural","Alt+s")]:
    ctx="CtxAny"
    if label=="Delete":ctx="CtxNodeSelection|CtxNoTool|CtxBrushTool|CtxRotateTool|CtxScaleTool|CtxShearTool|CtxSweepTool"
    if label=="Select Tall":ctx="CtxView2D"
    if label=="Make Structural" or label=="Move objects...":ctx="CtxNodeSelection"
    add("Edit",label,key,ctx,{"Cut":"ftebroom_ui cut","Copy":"ftebroom_ui copy","Paste":"ftebroom_ui paste","Paste at Original Position":"ftebroom_ui pasteoriginal","Duplicate":"ftebroom_duplicate","Move objects...":"ftebroom_ui moveobjects","Select by Line Number...":"ftebroom_ui selectline","Select All":"ftebroom_select all","Invert Selection":"ftebroom_select invert","Select Siblings":"ftebroom_select siblings","Select Touching":"ftebroom_select touching","Select Inside":"ftebroom_select inside","Select Tall":"ftebroom_select tall","Undo":"ftebroom_undo","Redo":"ftebroom_redo","Delete":"ftebroom_delete","Texture Lock":"ftebroom_ui texturelock","UV Lock":"ftebroom_ui uvlock"}.get(label))
for label,key in [("Brush","b"),("Clip","c"),("Rotate","r"),("Sweep","y"),("Scale","t"),("Shear","g"),("Vertex","v"),("Edge","e"),("Face","f"),("Control Point","p"),("Sculpt","")]:
    add("Tools",label,key,"CtxAnyView|CtxAnyOrNoSelection|CtxAnyOrNoTool" if label=="Sculpt" else "CtxAnyView","ftebroom_tool_toggle "+label.lower().replace(" ",""))
add("View/Options","Auto Integer Snap","",command="toggle ftebroom_auto_integer_snap")
add("Edit","Convert Triangulated Brushes","","CtxNodeSelection","ftebroom_generator apply convert")
add("Edit","CSG Subtract 2","","CtxNodeSelection","ftebroom_csg subtract2")
add("Tools","Bezier Bend","","CtxAnyView|CtxNodeSelection","ftebroom_tool_toggle bend")
add("Tools","Polygon Cut","","CtxAnyView|CtxAnySelection","ftebroom_tool_toggle polygoncut")
add("Tools","Polygon Spawn","","CtxAnyView","ftebroom_tool_toggle polygonspawn")
add("Tools","Apply polygon","enter","CtxPolygonCutTool|CtxPolygonSpawnTool","ftebroom_polygon apply")
add("Tools","Remove polygon point","del;backspace","CtxPolygonCutTool|CtxPolygonSpawnTool","ftebroom_polygon back")
add("Tools","Unity Sync","","CtxAny","ftebroom_ui unity")
add("Tools","Spawn Master","","CtxNodeSelection","ftebroom_ui spawnmaster")
add("Tools","Spin","","CtxAnyView|CtxAnySelection","ftebroom_tool_toggle spin")
add("Tools","Apply Spin","enter","CtxSpinTool","ftebroom_spin apply")
add("Tools","Apply Bend","enter","CtxBendTool","ftebroom_bend apply")
add("Tools","Cancel","escape",command="ftebroom_ui cancel")
add("Tools","Deactivate current tool","Shift+escape",command="ftebroom_tool_deactivate")
for label,key,ctx,cmd in [
("Create brush","enter","CtxView3D|CtxBrushTool",None),("Perform clip","enter","CtxClipTool","ftebroom_clip apply"),
("Toggle clip side","Ctrl+enter","CtxClipTool","ftebroom_tool_key side"),("Perform sweep","enter","CtxSweepTool",None),
("Increase sweep scale","]","CtxSweepTool",None),("Decrease sweep scale","[","CtxSweepTool",None),
("Delete last clip point","del","CtxClipTool","ftebroom_tool_key back"),("Delete vertices","del","CtxAnyHandleTool",None)]:
    add("Tools",label,key,ctx,cmd)
add("Edit","Replace Material...",command="ftebroom_ui replacematerial")
directions=[("Forward","uparrow"),("Back","downarrow"),("Left","leftarrow"),("Right","rightarrow"),("Up","pgup"),("Down","pgdn")]
for label,key in directions:
    add("Transform","Move "+label,key,"CtxAnyView|CtxNodeSelection",command="ftebroom_nudge "+{"Forward":"1 1","Back":"1 -1","Left":"0 -1","Right":"0 1","Up":"2 1","Down":"2 -1"}[label])
    add("Transform","Duplicate and move "+label,"Ctrl+"+key,"CtxAnyView|CtxNodeSelection",command="ftebroom_duplicate_nudge "+{"Forward":"1 1","Back":"1 -1","Left":"0 -1","Right":"0 1","Up":"2 1","Down":"2 -1"}[label])
    add("Transform","Rotate "+label,"Alt+"+key,"CtxAnyView|CtxNodeSelection",command="ftebroom_transform_key "+{"Forward":"roll 1","Back":"roll -1","Left":"yaw 1","Right":"yaw -1","Up":"pitch 1","Down":"pitch -1"}[label])
for label,key in [("Flip horizontal","Ctrl+f"),("Flip vertical","Ctrl+Alt+f")]:
    add("Transform",label,key,"CtxAnyView|CtxNodeSelection",command="ftebroom_transform_key "+("fliph" if "horizontal" in label else "flipv")+" 1")
for label,key in directions:
    for suffix,mod in [("",""),(" coarse","Shift+"),(" fine","Ctrl+")]:
        add("Textures",("Rotate " if key in ("pgup","pgdn") else "Move ")+label+suffix,mod+key,"CtxView3D|CtxFaceSelection",command="ftebroom_texture_key "+{"Forward":"vertical 1","Back":"vertical -1","Left":"horizontal -1","Right":"horizontal 1","Up":"rotate 1","Down":"rotate -1"}[label]+" "+str({"":0," coarse":1," fine":2}[suffix]))
for label,key in [("Flip horizontal","Ctrl+f"),("Flip vertical","Ctrl+Alt+f")]:
    add("Textures",label,key,"CtxView3D|CtxFaceSelection",command="ftebroom_texture_key "+("fliph" if "horizontal" in label else "flipv")+" 1 0")
add("Textures","Reset alignment","Shift+r","CtxAnyView|CtxAnySelection",command="ftebroom_texture_key reset 1 0")
add("Textures","Reset to world aligned","Alt+Shift+r","CtxAnyView|CtxAnySelection",command="ftebroom_texture_key world 1 0")
for label,key,ctx,cmd in [
("Show Grid","0","CtxAny","ftebroom_grid visible"),("Snap to Grid","Alt+0","CtxAny","ftebroom_grid snap"),
("Increase Grid Size","+;=;Shift+=;kp_plus","CtxAnyView","ftebroom_grid step 1"),("Decrease Grid Size","-;kp_minus","CtxAnyView","ftebroom_grid step -1"),
("Next Point",".","CtxAnyView",None),("Previous Point",",","CtxAnyView",None),("Reset 2D Cameras","Ctrl+Shift+u","CtxAny","ftebroom_view_reset"),
("Focus Camera on Selection","Ctrl+u","CtxAny","ftebroom_camera_focus"),
("Move Camera to...","","CtxAny","ftebroom_ui cameraposition"),("Reset camera zoom","Ctrl+Alt+z","CtxView3D","ftebroom_camera_input resetzoom 0 0 0 0"),
("Cycle map view","space","CtxAnyView","ftebroom_ui cycle"),("Maximize current view","Ctrl+space","CtxAnyView","ftebroom_ui maximize"),
("Isolate","Ctrl+i","CtxAny",None),("Hide","Ctrl+Alt+i","CtxAny",None),("Show All","Ctrl+Shift+i","CtxAny",None),
("Map Inspector","Ctrl+1","CtxAny","ftebroom_ui map"),("Entity Inspector","Ctrl+2","CtxAny",None),
("Face Inspector","Ctrl+3","CtxAny","ftebroom_ui material"),("Toggle Info Panel","Ctrl+4","CtxAny",None),
("Toggle Inspector","Ctrl+5","CtxAny",None),("Toggle Toolbar","Ctrl+Alt+t","CtxAny",None),
("Open Inspector","","CtxAny","ftebroom_ui inspector")]:
    add("View/Grid" if "Grid" in label else "View",label,key,ctx,cmd)
for n in range(9):add("View/Grid",f"Grid {2**n}",str(n+1),"CtxAnyView",f"ftebroom_grid set {2**n}")
for n in (0.125,0.25,0.5):add("View/Grid",f"Grid {n}",command=f"ftebroom_grid set {n}")
for label,key in [("Forward","w"),("Back","s"),("Left","a"),("Right","d"),("Up","q"),("Down","x")]:
    add("Camera",label,key,"CtxView3D","ftebroom_hold "+key)
for label in ["Compile...","Launch...","Launch in FTE","Rerun"]:add("Run",label)
add("Run","Build","Ctrl+Shift+b",command="ftebroom_ui buildnow")
add("Run","Toggle BSP preview","Ctrl+Shift+p",command="ftebroom_ui bsptoggle")
add("Run","Build map...",command="ftebroom_ui build")
add("Help","Action reference",command="ftebroom_ui actions")
gestures=[
("Look","RMB drag","CtxView3D"),("Orbit","Alt+RMB drag","CtxView3D"),("Flight speed","RMB+wheel","CtxView3D"),
("Pan","MMB drag","CtxView3D"),("Move camera","wheel","CtxView3D"),("Temporary zoom","Shift+wheel","CtxView3D"),
("Drill selection","Ctrl+wheel","CtxAnyView"),("Grid wheel","Ctrl+Alt+wheel","CtxAnyView"),
("Select object","LMB click","CtxAnyView|CtxNoTool"),("Toggle object","Ctrl+LMB click","CtxAnyView|CtxNoTool"),
("Paint selection","Ctrl+LMB drag (unselected)","CtxAnyView|CtxNoTool"),("Select face","Shift+LMB click","CtxAnyView|CtxNoTool"),
("Toggle face","Ctrl+Shift+LMB click","CtxAnyView|CtxNoTool"),("Paint faces","Ctrl+Shift+LMB drag","CtxAnyView|CtxNoTool"),
("Select siblings or open group","LMB double click","CtxAnyView|CtxNoTool"),("All brush faces","Shift+LMB double click","CtxAnyView|CtxNoTool"),
("Add all brush faces","Ctrl+Shift+LMB double click","CtxAnyView|CtxNoTool"),("Coplanar flood","Alt+Shift+LMB double click","CtxAnyView|CtxNoTool"),
("Add coplanar flood","Ctrl+Alt+Shift+LMB double click","CtxAnyView|CtxNoTool"),
("Move XY","LMB drag (selected)","CtxAnyView|CtxNoTool"),("Move Z","Alt+LMB drag (selected)","CtxAnyView|CtxNoTool"),
("Move axis locked","Shift+LMB drag (selected)","CtxAnyView|CtxNoTool"),("Duplicate and move","Ctrl+LMB drag (selected)","CtxAnyView|CtxNoTool"),
("Draw shape","LMB drag (empty or unselected)","CtxAnyView|CtxNoTool"),
("Extrude","Shift+LMB drag (face hover)","CtxAnyView|CtxNoTool"),("Split","Ctrl+Shift+LMB drag (face hover)","CtxAnyView|CtxNoTool"),
("Stamp","Ctrl+Alt+Shift+LMB drag (face hover)","CtxAnyView|CtxNoTool"),("Move face","Alt+Shift+LMB drag (face hover)","CtxAnyView|CtxNoTool"),
("Transfer material projection","Alt+LMB click","CtxAnyView|CtxNoTool"),("Transfer material rotation","Alt+Shift+LMB click","CtxAnyView|CtxNoTool"),
("Transfer material only","Ctrl+Alt+LMB click","CtxAnyView|CtxNoTool"),
("Pan 2D","RMB or MMB drag","CtxView2D"),("Zoom 2D","wheel","CtxView2D"),("Select smallest face area","LMB click","CtxView2D|CtxNoTool")]
for index,(label,gesture,ctx) in enumerate(gestures):
    if label in ("Select face","Toggle face","Paint faces","All brush faces","Add all brush faces","Coplanar flood","Add coplanar flood"):
        gestures[index]=(label,gesture,"CtxView3D|CtxNoTool")
for row in rows:
    organize={"Export Map...":"ftebroom_ui exportmap","Group":"ftebroom_ui group","Rename Groups":"ftebroom_ui renamegroups","Ungroup":"ftebroom_organize ungroup","Make Structural":"ftebroom_organize structural","Hide":"ftebroom_visibility hide","Isolate":"ftebroom_visibility isolate","Show All":"ftebroom_visibility showall","Entity Inspector":"ftebroom_ui entity","Reload Entity Definitions":"ftebroom_definitions"}
    if row["label"] in organize: row["command"]=organize[row["label"]]
    if row["id"] == "Menu/Edit/Group": row["context"] = "CtxAnyView|CtxNoTool|CtxNodeSelection"
    if row["id"]=="Menu/Edit/Deselect All": row["command"]="ftebroom_deselect"
    if row["id"]=="Menu/Tools/Delete vertices": row["command"]="ftebroom_vertex_delete"
    if row["id"]=="Menu/Tools/Create brush": row["command"]="ftebroom_tool_key apply"
    if row["label"]=="Snap Vertices to Integer": row["command"]="ftebroom_vertex_snap integer"
    if row["label"]=="Snap Vertices to Grid": row["command"]="ftebroom_vertex_snap grid"
    csg = {"CSG Convex Merge": "merge", "CSG Subtract": "subtract", "CSG Hollow": "hollow", "CSG Intersect": "intersect"}
    if row["label"] in csg:
        row["command"] = "ftebroom_csg " + csg[row["label"]]
        row["context"] = "CtxNodeSelection"
for label, name in [("Show brushes", "brushes"), ("Show point entities", "points"), ("Show entity models", "models"), ("Show entity names", "entity_names"), ("Show point bounds", "entity_bounds"), ("Show group bounds", "group_bounds"), ("Show brush entity bounds", "brush_entity_bounds"), ("Show edges", "edges")]:
    add("View/Options", label, command="toggle ftebroom_show_" + name)
for label, cmd in [("Shade faces", "toggle ftebroom_shade_faces"), ("Show soft map bounds", "toggle ftebroom_bounds_visible"), ("Textured faces", "set ftebroom_render_faces 0"), ("Flat faces", "set ftebroom_render_faces 1"), ("Wireframe", "set ftebroom_render_faces 2")]:
    add("View/Options", label, command=cmd)
for index, label in enumerate(["No entity links", "Direct entity links", "Transitive entity links", "All entity links"]):
    add("View/Options", label, command=f"set ftebroom_entity_links {index}")
for tag in ["trigger", "clip", "caulk", "hint", "detail"]:
    for enabled, verb in [(1, "Set"), (0, "Remove")]:
        add("Edit/Tags", f"{verb} {tag}", context="CtxNodeSelection", command=f"ftebroom_tag {tag} {enabled}")
add("View/Options", "Use fog", command="toggle r_fog_enabled")
for sides in (2, 4, 8):
    for rotate, label in ((0, "Mirror"), (1, "Rotate")):
        add("Edit/Symmetry", f"{sides} sides ({label})", context="CtxNodeSelection", command=f"ftebroom_symmetry {sides} {rotate}")
for label,gesture,ctx in gestures:add("Mouse reference",label,context=ctx,gesture=gesture)
advanced_commands = {
    "Revert": "ftebroom_ui revert",
    "Compile...": "ftebroom_ui compile", "Launch...": "ftebroom_ui launch",
    "Launch in FTE": "ftebroom_ui launchfte", "Rerun": "ftebroom_ui rerun",
    "Export OBJ...": "ftebroom_ui exportobj",
    "Export selected BSP model...": "ftebroom_ui exportbspmodel",
    "Load Point File...": "ftebroom_ui loadpoints", "Reload Point File": "ftebroom_ui reloadpoints",
    "Unload Point File": "ftebroom_trace unloadpoints", "Load Portal File...": "ftebroom_ui loadportals",
    "Reload Portal File": "ftebroom_ui reloadportals", "Unload Portal File": "ftebroom_trace unloadportals",
    "Next Point": "ftebroom_trace next", "Previous Point": "ftebroom_trace previous",
    "Repeat Last Commands": "ftebroom_repeat run", "Clear Repeatable Commands": "ftebroom_repeat clear",
    "Create Linked Duplicate": "ftebroom_linked duplicate", "Convert Selection to Patches": "ftebroom_patch_convert",
    "Perform sweep": "ftebroom_sweep apply", "Increase sweep scale": "ftebroom_sweep grow 1",
    "Decrease sweep scale": "ftebroom_sweep grow -1",
}
add("View", "Issues", command="ftebroom_ui issues")
for row in rows:
    if row["label"] in advanced_commands: row["command"] = advanced_commands[row["label"]]
    if row["id"].startswith(("Menu/Textures/", "Menu/Transform/")): row["context"] += "|(CtxAnyOrNoTool & ~CtxSweepTool)"
    if row["label"] == "Create Linked Duplicate": row["context"] = "CtxNodeSelection"
    if row["label"] == "Convert Selection to Patches": row["context"] = "CtxAnySelection"
for label, cmd in [("Select Linked Groups", "ftebroom_linked select"), ("Separate Linked Groups", "ftebroom_linked separate"),
                   ("Extract Linked Group", "ftebroom_linked extract"), ("Clear Protected Properties", "ftebroom_protect clear")]:
    add("Edit", label, context="CtxNodeSelection", command=cmd)
for label, key, axis, direction in [("forward","uparrow",1,1),("back","downarrow",1,-1),
                                   ("left","leftarrow",0,-1),("right","rightarrow",0,1),
                                   ("up","pgup",2,1),("down","pgdn",2,-1)]:
    add("Tools", "Sweep " + label, key, "CtxSweepTool", f"ftebroom_nudge {axis} {direction}")
for label, key, axis, direction in [("roll forward","uparrow","roll",1),("roll back","downarrow","roll",-1),
                                   ("yaw left","leftarrow","yaw",1),("yaw right","rightarrow","yaw",-1),
                                   ("pitch up","pgup","pitch",1),("pitch down","pgdn","pitch",-1)]:
    add("Tools", "Sweep " + label, "Alt+" + key, "CtxSweepTool", f"ftebroom_transform_key {axis} {direction}")
for index, label in enumerate(["Trigger", "Clip", "Caulk", "Hint", "Detail", "Liquid", "Translucent", "Areaportal", "Clusterportal"]):
    add("View/Filters", label, command=f"ftebroom_filter_tag {1 << index}")
(p/"native/ftebroom_actions.inc").write_text("// Generated by tests/generate-actions.py; edit the catalogue there.\n"+"\n".join("add("+", ".join([json.dumps(r["id"]),json.dumps(r["label"]),r["context"],json.dumps(r["keys"]),json.dumps(r["command"]) if r["command"] else "nullptr",json.dumps(r["gesture"])]) +");" for r in rows)+"\n",encoding="utf8")
(p/"tests/actions-catalog.json").write_text(json.dumps(rows,indent=2)+"\n",encoding="utf8")
print(len(rows),"actions")
