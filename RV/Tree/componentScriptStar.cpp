#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptStar.h"


namespace TreeEngine
{
    int CCScriptStar::Execute(const SActualParams &ap,
                            const SConstantParams &cp,
                            CComponentChildBuffer &ccb)
  {

    // Set default values
    _primitiveStar->SetOrigin(ap._origin);
    _primitiveStar->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveStar->SetTexture("aa.bmp", 1);
    _primitiveStar->SetParams(1.0f, 1.0f, 1, 0.0f, 1.0f);
    return ExecuteScript(ap,cp,&ccb,_primitiveStar);
  }

  void CCScriptStar::Init(RString folder,
                          const ParamEntry &treeEntry,
                          const ParamEntry &subEntry,
                          const AutoArray<RString> &names)
  {
    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveStar = new CPStar();
  }

  void CCScriptStar::InitWithDb(IPlantControlDB *paramDB,const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveStar = new CPStar();

  }

  CPrimitive *CCScriptStar::GetPrimitive()
  {
    return _primitiveStar;
  }

}