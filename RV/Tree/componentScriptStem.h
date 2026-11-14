#ifndef _componentScriptStem_h_
#define _componentScriptStem_h_

#include "primitiveStem.h"
#include "componentScript.h"


namespace TreeEngine
{
    class CCScriptStem : public CScriptedComponent {
  protected:
    //! Tree primitive associated with the component
    Ref<CPStem> _primitiveStem;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptStem(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
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
