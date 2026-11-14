#ifndef _componentTest_h_
#define _componentTest_h_

#include "primitiveTest.h"
#include "component.h"


namespace TreeEngine
{
    class CCTest : public CComponent {
  protected:
    float _arrowWidth;
    float _arrowHeight;
    int _childIndex;
    //! Primitive associated with the component
    Ref<CPTest> _primitiveTest;
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

    virtual void InitWithDb(IPlantControlDB *paramDB,const Array<RString> &names);

  };

}
#endif
