#ifndef _componentScriptFunnel_h_
#define _componentScriptFunnel_h_

#include "primitiveFunnel.h"
#include "componentScript.h"


namespace TreeEngine
{
    class CCScriptFunnel : public CScriptedComponent {
  protected:
    //! Tree primitive associated with the component
    Ref<CPFunnel> _primitiveFunnel;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptFunnel(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
    //! Virtual method
    virtual void Init(
      RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names);
    //! Virtual method
    virtual CPrimitive *GetPrimitive();

    virtual void InitWithDb(IPlantControlDB *paramDB,  const Array<RString> &names);

    virtual int EstimateFacesCount() {return 12;}

  };

}
#endif
