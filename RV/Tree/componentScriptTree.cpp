#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptTree.h"


namespace TreeEngine
{
    int CCScriptTree::Execute(const SActualParams &ap,
                            const SConstantParams &cp,
                            CComponentChildBuffer &ccb) {

    // Set default values
    _primitiveTree->SetOrigin(ap._origin);
    _primitiveTree->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveTree->SetTexture("aa.bmp", 1);
    _primitiveTree->SetTreeParams(1.0f, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 5.0f, 3, 0.0f, 0.0f, 1.0f, 1.0f,false);
    
    return ExecuteScript(ap,cp,&ccb,_primitiveTree);
  }

  void CCScriptTree::Init(RString folder,
                          const ParamEntry &treeEntry,
                          const ParamEntry &subEntry,
                          const AutoArray<RString> &names) {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveTree = new CPTree();
  }

  void CCScriptTree::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveTree = new CPTree();

  }

  CPrimitive *CCScriptTree::GetPrimitive() {
    return _primitiveTree;
  }

}