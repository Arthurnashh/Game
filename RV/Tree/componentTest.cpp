#include "RVTreeCommon.h"
#include "componentTest.h"


namespace TreeEngine
{
    int CCTest::Execute(const SActualParams &ap,
                      const SConstantParams &cp,
                      CComponentChildBuffer &ccb) {
    _primitiveTest->SetOrigin(ap._origin);
    _primitiveTest->SetPolyplaneType(PT_BLOCKSIMPLE2);
    _primitiveTest->SetTexture(_textureName, _textureIndex);
    _primitiveTest->SetParams(_arrowWidth, _arrowHeight);
    SActualParams cap;
    cap._origin = ap._origin;
    cap._seed = ap._seed;
    cap._age = ap._age;
    cap._counter = ap._counter;
    cap._diaOrigin = ap._diaOrigin;
    cap._textureVOffset = ap._textureVOffset;
    ccb.AddChild(_childIndex, cap._age, -1, cap);
    return 1;
  }

  void CCTest::Init(RString folder,
                    const ParamEntry &treeEntry,
                    const ParamEntry &subEntry,
                    const AutoArray<RString> &names) {

    CComponent::Init(folder, treeEntry, subEntry, names);

    _primitiveTest = new CPTest();
    _arrowWidth    = subEntry >> "arrowWidth";
    _arrowHeight   = subEntry >> "arrowHeight";
    _childIndex    = GetIndexOfName(subEntry >> "child");
  }

  CPrimitive *CCTest::GetPrimitive() {
    return _primitiveTest;
  }

  void CCTest::InitWithDb(IPlantControlDB *paramDB,const Array<RString> &names)
  {
    CComponent::InitWithDb(paramDB,names);
    _primitiveTest = new CPTest();
    _arrowWidth    = paramDB->GetDBItemScript("arrowWidth");
    _arrowHeight   = paramDB->GetDBItemScript("arrowHeight");
    _childIndex    = GetIndexOfName(paramDB->GetDBItemScript("child"));
  }

}