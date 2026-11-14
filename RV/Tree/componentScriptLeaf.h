#ifndef _componentScriptLeaf_h_
#define _componentScriptLeaf_h_

#include "primitiveLeaf.h"
#include "componentScript.h"


namespace TreeEngine
{
    class CCScriptLeaf : public CScriptedComponent {
  protected:
   //! Tree primitive associated with the component
    Ref<CPLeaf> _primitiveLeaf;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
    CCScriptLeaf(GameState &gState, const RString &script):CScriptedComponent(gState,script) {}
    //! Virtual method
    virtual void Init(
      RString folder,
      const ParamEntry &treeEntry,
      const ParamEntry &subEntry,
      const AutoArray<RString> &names);
    //! Virtual method
    virtual CPrimitive *GetPrimitive();

    virtual void InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names);

    virtual int EstimateFacesCount() const {return 4;}
  };

    class CCScriptSlice: public CCScriptLeaf
    {
    public:
      CCScriptSlice(GameState &gState, const RString &script):CCScriptLeaf (gState,script) {}

      virtual void Init(
        RString folder,
        const ParamEntry &treeEntry,
        const ParamEntry &subEntry,
        const AutoArray<RString> &names);
      virtual void InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names);
      virtual int EstimateFacesCount() const {return 2;}
    };

}
#endif
