#include "RVTreeCommon.h"
#include "componentTree.h"


namespace TreeEngine
{
    int CCTree::Execute(const SActualParams &ap,
                      const SConstantParams &cp,
                      CComponentChildBuffer &ccb) {
    SActualParams cap;
    SPrimitiveChild pc;
    _primitiveTree->SetOrigin(ap._origin);
    _primitiveTree->SetPolyplaneType(PT_BLOCKSIMPLE);
    _primitiveTree->SetTexture(_textureName, _textureIndex);
    //_primitiveTree->ApplyGravitropism(H_PI*0.1f);
    _primitiveTree->SetTreeParams(
      _width, _height, _balance, 0.0f, _twist, _spinA, _spinB, _spread, _segmentCount, 3, ap._textureVOffset, 0.0f, 1.0f, 1.0f,false);
  /*
    if (ap._age > 0.0f) {
      pc = _primitiveTree->GetChildA();
      cap._origin = pc._origin;
      cap._seed = ap._seed + 1;
      cap._age = ap._age - 0.1f;
      cap._counter = 0;
      cap._diaOrigin = ap._diaOrigin;
      cap._textureVOffset = pc._textureVOffset;
      ccb.AddChild(1, pc._slotIndex, cap);
      pc = _primitiveTree->GetChildB();
      cap._origin = pc._origin;
      cap._seed = ap._seed + 2;
      cap._textureVOffset = pc._textureVOffset;
      ccb.AddChild(1, pc._slotIndex, cap);
    }
  */
    return 1;
  }

  void CCTree::Init(RString folder,
                    const ParamEntry &treeEntry,
                    const ParamEntry &subEntry,
                    const AutoArray<RString> &names) {

    CComponent::Init(folder, treeEntry, subEntry, names);

    _primitiveTree = new CPTree();
    _width        = subEntry >> "width";
    _height       = subEntry >> "height";
    _balance      = subEntry >> "balance";
    _twist        = subEntry >> "twist";
    _spinA        = subEntry >> "spinA";
    _spinB        = subEntry >> "spinB";
    _spread       = subEntry >> "spread";
    _segmentCount = subEntry >> "segmentCount";
    _childIndexA = GetIndexOfName(subEntry >> "childA");
    _childIndexB = GetIndexOfName(subEntry >> "childB");
  }

  CPrimitive *CCTree::GetPrimitive() {
    return _primitiveTree;
  }

  void CCTree::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CComponent::InitWithDb(paramDB,names);

    _primitiveTree = new CPTree();
    _width        = paramDB->GetDBItemScript("width");
    _height       = paramDB->GetDBItemScript("height");
    _balance      = paramDB->GetDBItemScript("balance");
    _twist        = paramDB->GetDBItemScript("twist");
    _spinA        = paramDB->GetDBItemScript("spinA");
    _spinB        = paramDB->GetDBItemScript("spinB");
    _spread       = paramDB->GetDBItemScript("spread");
    _segmentCount = paramDB->GetDBItemScript("segmentCount");
    _childIndexA = GetIndexOfName(paramDB->GetDBItemScript("childA"));
    _childIndexB = GetIndexOfName(paramDB->GetDBItemScript("childB"));
  }
}