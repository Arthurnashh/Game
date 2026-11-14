#ifndef _componentFern_h_
#define _componentFern_h_

#include "primitiveStem.h"
#include "component.h"

namespace TreeEngine
{

  class CCFern : public CComponent {
  protected:
    float _width;
    float _height;
    int _frontChildIndex;
    float _frontChildTreshold;
    AutoArray<float> _aWidth;

    //! Primitive associated with the component
    Ref<CPStem> _primitiveStem;
    //! Virtual method
    virtual int Execute(
      const SActualParams &ap,
      const SConstantParams &cp,
      CComponentChildBuffer &ccb);
  public:
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