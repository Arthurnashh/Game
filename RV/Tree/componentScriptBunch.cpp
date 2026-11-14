#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptBunch.h"

namespace TreeEngine
{


  int CCScriptBunch::Execute(const SActualParams &ap,
    const SConstantParams &cp,
    CComponentChildBuffer &ccb) 
  {

    // Set default values
    _primitiveBunch->SetOrigin(ap._origin);
    _primitiveBunch->SetPolyplaneType(PT_BLOCKSIMPLE2);

    return ExecuteScript(ap,cp,&ccb,_primitiveBunch);
  }

  void CCScriptBunch::Init(RString folder,
    const ParamEntry &treeEntry,
    const ParamEntry &subEntry,
    const AutoArray<RString> &names) 
  {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;
    _primitiveBunch=new CPBunch();

  }

  void CCScriptBunch::InitWithDb(IPlantControlDB *paramDB,  const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;
    _primitiveBunch=new CPBunch();

  }

  CPrimitive *CCScriptBunch::GetPrimitive() {
    return _primitiveBunch;
  }

}