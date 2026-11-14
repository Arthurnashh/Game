#ifndef _componentScriptHeel_h_
#define _componentScriptHeel_h_

#include "primitiveHeel.h"
#include "componentScript.h"

namespace TreeEngine
{
  class CCScriptHeel : public CScriptedComponent
  {
  protected:
    //! Tree primitive associated with the component
    Ref<CPHeel> _primitiveHeel;
    //! Virtual method
    virtual int Execute(const SActualParams &ap,
                        const SConstantParams &cp,
                        CComponentChildBuffer &ccb);
  public:
    //! Constructor
    CCScriptHeel(GameState &gState, const RString &script) : CScriptedComponent(gState,script) {}
    //! Virtual method
    virtual void Init(RString folder,
                      const ParamEntry &treeEntry,
                      const ParamEntry &subEntry,
                      const AutoArray<RString> &names);
    //! Virtual method
    virtual CPrimitive *GetPrimitive();
    //! Virtual method
    virtual void InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names);
  };
}

#endif