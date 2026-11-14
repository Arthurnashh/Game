#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptLeaf.h"


namespace TreeEngine
{
    int CCScriptLeaf::Execute(const SActualParams &ap,
                            const SConstantParams &cp,
                            CComponentChildBuffer &ccb) {

    // Set default values
    _primitiveLeaf->SetOrigin(ap._origin);
    _primitiveLeaf->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveLeaf->SetTexture("aaa.bmp", 0);
    _primitiveLeaf->SetLeafParams(1.0f, 1.0f);
    
    return ExecuteScript(ap,cp,0,_primitiveLeaf);

  }

  void CCScriptLeaf::Init(RString folder,
                          const ParamEntry &treeEntry,
                          const ParamEntry &subEntry,
                          const AutoArray<RString> &names) {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = MIRROR;
    _adressV = MIRROR;

    _primitiveLeaf      = new CPLeaf();
  }

  void CCScriptSlice::Init(RString folder,
                          const ParamEntry &treeEntry,
                          const ParamEntry &subEntry,
                          const AutoArray<RString> &names) {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = MIRROR;
    _adressV = MIRROR;

    _primitiveLeaf      = new CPSlice();
  }

  void CCScriptLeaf::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = MIRROR;
    _adressV = MIRROR;

    _primitiveLeaf      = new CPLeaf();
  }

  void CCScriptSlice::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = MIRROR;
    _adressV = MIRROR;

    _primitiveLeaf      = new CPSlice();
  }

  CPrimitive *CCScriptLeaf::GetPrimitive() {
    return _primitiveLeaf;
  }

}