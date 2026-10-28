// SPDX-License-Identifier: GPL-2.0-or-later
// Main-thread, read-only CSQC queries. Never retain VM pointers or QC strings.
static bool qcQuery(unsigned operation, unsigned cursor, std::string& result)
{
  static bool active=false;
  if(active || modEmbedded)return false;
  auto vm=static_cast<pubprogfuncs_t*>(core->GetEngineInterface("CSQCVM",sizeof(pubprogfuncs_t)));
  if(!vm)return false;
  auto function=vm->FindFunction(vm,"ReyondBroom_UIQuery",PR_ANY);
  if(!function)return false;
  active=true;
  // Menu rendering runs on the engine thread. Preserve call registers even if
  // another engine callback has left values there; globals may move during QC.
  constexpr unsigned registers=OFS_PARM7+3;
  std::array<pint_t,registers> saved;
  auto globals=reinterpret_cast<pint_t*>(vm->globals(vm,PR_CURRENT));
  std::memcpy(saved.data(),globals,sizeof(saved));
  reinterpret_cast<pvec_t*>(globals)[OFS_PARM0]=float(operation);
  reinterpret_cast<pvec_t*>(globals)[OFS_PARM1]=float(cursor);
  vm->ExecuteProgram(vm,function);
  globals=reinterpret_cast<pint_t*>(vm->globals(vm,PR_CURRENT));
  const char* text=vm->StringToNative(vm,globals[OFS_RETURN]);
  result=text?text:"";
  std::memcpy(globals,saved.data(),sizeof(saved));
  active=false;
  return true;
}
