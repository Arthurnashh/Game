#ifndef _componentScriptBunch_h_
#define _componentScriptBunch_h_

#include "primitiveBunch.h"
#include "componentScript.h"

namespace TreeEngine
{


  class CCScriptBunch : public CScriptedComponent {
  protected:
    //! Tree primitive associated with the component
    Ref<CPBunch> _primitiveBunch;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptBunch(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
    //! Virtual method
    virtual void Init(
      RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names);
    //! Virtual method
    virtual CPrimitive *GetPrimitive();
    //! Virtual method
    virtual bool ForceNewPolyplane() {return true;}
  
    virtual void InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names);

  };


}
#endif