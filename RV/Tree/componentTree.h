#ifndef _componentTree_h_
#define _componentTree_h_

#include "primitiveTree.h"
#include "component.h"


namespace TreeEngine
{
    class CCTree : public CComponent {
  protected:
    float _width;
    float _height;
    float _balance;
    float _twist;
    float _spinA;
    float _spinB;
    float _spread;
    int _segmentCount;
    int _childIndexA;
    int _childIndexB;
    //! Tree primitive associated with the component
    Ref<CPTree> _primitiveTree;
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
