#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "componentScriptTreeNode.h"

                                    
namespace TreeEngine
{
    int CCScriptTreeNode::Execute(const SActualParams &ap,
                                const SConstantParams &cp,
                                CComponentChildBuffer &ccb) {

    // Set default values
    _primitiveTreeNode->SetOrigin(ap._origin);
    _primitiveTreeNode->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveTreeNode->SetTexture("aa.bmp", 1);
    _primitiveTreeNode->SetTreeNodeParams(1.0f, 0.0f, 0.0f, 0);
    for (int i = 0; i < TREENODE_MAXCHILDCOUNT; i++)
    {
      _primitiveTreeNode->SetTreeNodeChildParams(i, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1, false);
    }

    return ExecuteScript(ap,cp,&ccb,_primitiveTreeNode);
  }

  void CCScriptTreeNode::Init(RString folder,
                              const ParamEntry &treeEntry,
                              const ParamEntry &subEntry,
                              const AutoArray<RString> &names) {

    CScriptedComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveTreeNode = new CPTreeNode();
  }

  void CCScriptTreeNode::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CScriptedComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveTreeNode = new CPTreeNode();
  }

  CPrimitive *CCScriptTreeNode::GetPrimitive() {
    return _primitiveTreeNode;
  }

}