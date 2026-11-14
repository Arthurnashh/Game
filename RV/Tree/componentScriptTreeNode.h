#ifndef _componentScriptTreeNode_h_
#define _componentScriptTreeNode_h_

#include "primitiveTreeNode.h"
#include "componentScript.h"


namespace TreeEngine
{
    class CCScriptTreeNode : public CScriptedComponent {
  protected:
    //! Tree primitive associated with the component
    Ref<CPTreeNode> _primitiveTreeNode;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptTreeNode(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
    //! Virtual method
    virtual void Init(
      RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names);
    //! Virtual method
    virtual CPrimitive *GetPrimitive();

    virtual void InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names);

  };

 
}
#endif
