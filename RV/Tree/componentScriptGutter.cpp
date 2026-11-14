#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptGutter.h"


namespace TreeEngine
{
    int CCScriptGutter::Execute(const SActualParams &ap,
                              const SConstantParams &cp,
                              CComponentChildBuffer &ccb)
  {

    // Set default values
    _primitiveGutter->SetOrigin(ap._origin);
    _primitiveGutter->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveGutter->SetTexture("aa.bmp", 1);
    _primitiveGutter->SetGutterParams(1.0f, 1.0f, 0.0f, 1, 0.0f, 1.0f, 1.0f);

    return ExecuteScript(ap,cp,&ccb,_primitiveGutter);
  }

  void CCScriptGutter::Init(RString folder,
                            const ParamEntry &treeEntry,
                            const ParamEntry &subEntry,
                            const AutoArray<RString> &names)
  {
    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveGutter = new CPGutter();
  }

  void CCScriptGutter::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveGutter = new CPGutter();

  }

  CPrimitive *CCScriptGutter::GetPrimitive()
  {
    return _primitiveGutter;
  }

}