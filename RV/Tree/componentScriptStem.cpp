#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptStem.h"


namespace TreeEngine
{
    int CCScriptStem::Execute(const SActualParams &ap,
                            const SConstantParams &cp,
                            CComponentChildBuffer &ccb) {

    // Set default values
    _primitiveStem->SetOrigin(ap._origin);
    _primitiveStem->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveStem->SetTexture("aa.bmp", 1);
    _primitiveStem->SetStemParams(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    
    return ExecuteScript(ap,cp,&ccb,_primitiveStem);
  }

  void CCScriptStem::Init(RString folder,
                          const ParamEntry &treeEntry,
                          const ParamEntry &subEntry,
                          const AutoArray<RString> &names) {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveStem = new CPStem();
  }


  void CCScriptStem::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveStem = new CPStem();
  }

  CPrimitive *CCScriptStem::GetPrimitive() {
    return _primitiveStem;
  }

}