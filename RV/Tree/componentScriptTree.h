#ifndef _componentScriptTree_h_
#define _componentScriptTree_h_

#include "primitiveTree.h"
#include "componentScript.h"


namespace TreeEngine
{
    class CCScriptTree : public CScriptedComponent {
  protected:
    //! Tree primitive associated with the component
    Ref<CPTree> _primitiveTree;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptTree(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
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
