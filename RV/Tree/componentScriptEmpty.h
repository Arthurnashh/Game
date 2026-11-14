#ifndef _componentScriptEmpty_h_
#define _componentScriptEmpty_h_

#include "componentScript.h"


namespace TreeEngine
{

  class CCScriptEmpty : public CScriptedComponent {
  protected:
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptEmpty(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
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
