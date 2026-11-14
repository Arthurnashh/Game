#ifndef _componentScriptStar_h_
#define _componentScriptStar_h_

#include "primitiveStar.h"
#include "componentScript.h"


namespace TreeEngine
{
    class CCScriptStar : public CScriptedComponent {
  protected:
    //! Tree primitive associated with the component
    Ref<CPStar> _primitiveStar;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptStar(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
    //! Virtual method
    virtual void Init(
      RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names);
    //! Virtual method
    virtual CPrimitive *GetPrimitive();

    virtual void InitWithDb(IPlantControlDB *paramDB,const Array<RString> &names);

  };

}
#endif
