#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptEmpty.h"

namespace TreeEngine
{

  int CCScriptEmpty::Execute(const SActualParams &ap,
                            const SConstantParams &cp,
                            CComponentChildBuffer &ccb) {

     return ExecuteScript(ap,cp,&ccb);
    }

  void CCScriptEmpty::Init(RString folder,
                          const ParamEntry &treeEntry,
                          const ParamEntry &subEntry,
                          const AutoArray<RString> &names) {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

  }

  CPrimitive *CCScriptEmpty::GetPrimitive() {
    return NULL;
  }

  void CCScriptEmpty::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;
  }

}