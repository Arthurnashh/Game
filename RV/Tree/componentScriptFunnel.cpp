#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptFunnel.h"

namespace TreeEngine
{

  int CCScriptFunnel::Execute(
    const SActualParams &ap,
    const SConstantParams &cp,
    CComponentChildBuffer &ccb)
  {

    // Set default values
    _primitiveFunnel->SetOrigin(ap._origin);
    _primitiveFunnel->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveFunnel->SetTexture("aaa.bmp", 0);
    _primitiveFunnel->SetParams(1.0f, 0.0f, 3);

    return ExecuteScript(ap,cp,0,_primitiveFunnel);
  }

  void CCScriptFunnel::Init(
    RString folder,
    const ParamEntry &treeEntry,
    const ParamEntry &subEntry,
    const AutoArray<RString> &names)
  {

      CScriptedComponent::Init(folder, treeEntry, subEntry, names);
      _adressU = MIRROR;
      _adressV = MIRROR;

      _primitiveFunnel = new CPFunnel();
    }

  
  void CCScriptFunnel::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = MIRROR;
    _adressV = MIRROR;
    _primitiveFunnel = new CPFunnel();
    
  }
  

  CPrimitive *CCScriptFunnel::GetPrimitive()
  {
    return _primitiveFunnel;
  }

}