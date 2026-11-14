#include "RVTreeCommon.h"
#include "componentFern.h"

namespace TreeEngine
{

  enum {
    RandomBase_Height = 0,
    RandomBase_FrontChildSeed
  };

  int CCFern::Execute(const SActualParams &ap,
    const SConstantParams &cp,
    CComponentChildBuffer &ccb) 
  {
    _primitiveStem->SetOrigin(ap._origin);
    _primitiveStem->SetPolyplaneType(PT_BLOCKSIMPLE);
    _primitiveStem->SetTexture(_textureName, _textureIndex);
    _primitiveStem->SetStemParams(
      GetLinearCurveValue(_aWidth, ap._age),
      RandShake(ap._seed + RandomBase_Height, _height, 0.1f),
      ap._textureVOffset, 0.0f,
      1.0f, 1.0f);
    if (ap._age > _frontChildTreshold) {
      SPrimitiveChild pc;
      pc = _primitiveStem->GetFrontChild();
      SActualParams cap;
      cap._origin = pc._origin;
      cap._seed = RandIndex(ap._seed + RandomBase_FrontChildSeed);
      cap._age = ap._age - 0.1f;
      cap._counter = 0;
      cap._diaOrigin = ap._diaOrigin;
      cap._textureVOffset = pc._textureVOffset;
      ccb.AddChild(_frontChildIndex, cap._age, pc._slotIndex, cap);
    }
    return 1;
  }

  void CCFern::Init(RString folder,
    const ParamEntry &treeEntry,
    const ParamEntry &subEntry,
    const AutoArray<RString> &names) 
  {

    CComponent::Init(folder, treeEntry, subEntry, names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveStem      = new CPStem();
    _width              = subEntry >> "width";
    _height             = subEntry >> "height";
    _frontChildIndex    = GetIndexOfName(subEntry >> "frontChild");
    _frontChildTreshold = subEntry >> "frontChildTreshold";

    // Load aWidth curve
    const ParamEntryVal &array = subEntry >> "aWidth";
    int n = array.GetSize();
    _aWidth.Realloc(n);
    _aWidth.Resize(n);
    for (int i = 0; i < n; i++) {
      _aWidth[i] = array[i];
    }
  }

  CPrimitive *CCFern::GetPrimitive() {
    return _primitiveStem;
  }


  void CCFern::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    CComponent::InitWithDb(paramDB,names);
    _adressU = WRAP;
    _adressV = WRAP;

    _primitiveStem      = new CPStem();
    
    _width =paramDB->GetDBItemScript("width");
    _height=paramDB->GetDBItemScript("height");
    _frontChildIndex=paramDB->GetDBItemScript("frontChild");
    _frontChildTreshold=paramDB->GetDBItemScript("frontChildTreshold");

    GameValue gv=paramDB->GetDBItemScript("aWidth");
    if (gv.GetType()==GameArray)
    {
      const GameArrayType &arr=gv;
      _aWidth.Resize(arr.Size());
      for (int i=0;i<arr.Size();i++)
      {
        _aWidth[i]=arr[i];
      }
    }
  }


}