#ifndef _componentScriptGutter_h_
#define _componentScriptGutter_h_

#include "primitiveGutter.h"
#include "componentScript.h"


namespace TreeEngine
{
    class CCScriptGutter : public CScriptedComponent {
  protected:
    //! Tree primitive associated with the component
    Ref<CPGutter> _primitiveGutter;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptGutter(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
    //! Virtual method
    virtual void Init(
      RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names);
    //! Virtual method
    virtual CPrimitive *GetPrimitive();

    virtual void InitWithDb(IPlantControlDB *paramDB,  const Array<RString> &names);

  };

}
#endif
