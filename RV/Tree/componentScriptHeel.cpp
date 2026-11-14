#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptHeel.h"

namespace TreeEngine
{
  int CCScriptHeel::Execute(const SActualParams &ap,
                            const SConstantParams &cp,
                            CComponentChildBuffer &ccb)
  {

    // Set default values
    _primitiveHeel->SetOrigin(ap._origin);
    _primitiveHeel->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveHeel->SetHeelParams(1.0f, 1.0f, 0.0f, 0.0f);

    return ExecuteScript(ap,cp,&ccb,_primitiveHeel);
  }

  void CCScriptHeel::Init(RString folder,
                          const ParamEntry &treeEntry,
                          const ParamEntry &subEntry,
                          const AutoArray<RString> &names)
  {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveHeel = new CPHeel();
  }

  void CCScriptHeel::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveHeel = new CPHeel();
  }

  CPrimitive *CCScriptHeel::GetPrimitive()
  {
    return _primitiveHeel;
  }
}