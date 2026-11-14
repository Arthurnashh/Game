#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "randomFlyman.h"
#include "bbox.h"
#include "component.h"

namespace TreeEngine
{
  void CComponentChildBuffer::AddChild(int componentIndex,
    float importancy,
    int slotIndex,
    const SActualParams &ap) 
  {

    if (componentIndex < 0)
    {
      LogF("Index of a child component is not valid.");
      return;
    }
    SComponentChild &cc = _children.Append();
    Assert(componentIndex >= 0);
    cc._componentIndex = componentIndex;
    cc._importancy = importancy;
    cc._slotIndex = slotIndex;
    cc._actualParams = ap;
  }

  SComponentChild *CComponentChildBuffer::GetChild(int i) {
    return &_children[i];
  }

  int CComponentChildBuffer::GetChildCount()
  {
    return _children.Size();
  }

  void CComponentChildBuffer::BeginChildUpdate()
  {
    _newIntervalIndex = GetChildCount();
  }

  void CComponentChildBuffer::EndChildUpdate(const SInternalParams &ip)
  {
    for (int i = _newIntervalIndex; i < GetChildCount(); i++)
    {
      _children[i]._internalParams = ip;
    }
    _newIntervalIndex = -1;
  }

  void CComponent::Init(RString folder,
    const ParamEntry &treeEntry,
    const ParamEntry &subEntry,
    const AutoArray<RString> &names) 
  {

    _componentNames = names;
    _textureName  = subEntry >> "textureName";
    if ((_textureName[1] != ':') && (folder.GetLength() > 0)) _textureName = folder + RString("\\") + _textureName;
    _textureSaveName  = subEntry >> "textureSaveName";
    _textureIndex = subEntry >> "textureIndex";

    // Clear previousely added parameters
    _parameters.Clear();

    // Load the external parameters from the component defined by subEntry
    ConstParamEntryPtr parameters = subEntry.FindEntry("ScriptVariables");
    if (parameters)
    {
      int n = parameters->GetEntryCount();
      _parameters.Realloc(n);
      _parameters.Resize(n);
      for (int i=0; i<n; i++)
      {
        ParamEntryVal entry = parameters->GetEntry(i);
        _parameters[i]._name = entry.GetName();
        _parameters[i]._value = entry;
      }
    }

    // Load the external parameters from the included component
    ConstParamEntryPtr includedComponents = subEntry.FindEntry("IncludedComponents");
    if (includedComponents)
    {
      int icSize = includedComponents->GetSize();
      // For all included components...
      for (int i = 0; i < icSize; i++)
      {
        const ParamEntryVal &includedEntry = treeEntry >> (*includedComponents)[i].GetValue();
        ConstParamEntryPtr parameters = includedEntry.FindEntry("ScriptVariables");
        if (parameters)
        {
          int n = parameters->GetEntryCount();
          int offset = _parameters.Size();
          _parameters.Resize(offset + n);
          // For all parameters in the included component
          for (int j = 0; j < n; j++)
          {
            ParamEntryVal entry = parameters->GetEntry(j);
            _parameters[offset + j]._name = entry.GetName();
            _parameters[offset + j]._value = entry;
          }
        }
      }
    }
  }

  void CComponent::InitWithDb(IPlantControlDB *paramDB, const Array<RString> &names)
  {
    _componentNames = AutoArray<RString>(names);
    _parameters.Clear();
    _paramDB=paramDB;
  }

  void CComponent::GetTextureIndexAndName(int &index, RString &name, RString &saveName) {
    index = _textureIndex;
    name = _textureName;
    saveName = _textureSaveName;
  }

  void CComponent::GetTextureAdress(ETextureAdress &adressU, ETextureAdress &adressV) {
    adressU = _adressU;
    adressV = _adressV;
  }

  RString CComponent::GetComponentName(int componentIndex) {
    return _componentNames[componentIndex];
  }

  int CComponent::GetChildren(const SActualParams &ap,
                              const SInternalParams &ip,
                              const SConstantParams &cp,
                              CComponentChildBuffer &ccb) 
  {
    ccb.BeginChildUpdate();
    int result = Execute(ap, cp, ccb);
    ccb.EndChildUpdate(ip);
    return result;
  }

  void CComponent::CloseBranchTip(CPrimitiveStream &psBranch,
    int sharedVertexIndex,
    int sharedVertexCount) 
  {

    if (sharedVertexCount <= 0) return;

    unsigned long nvi = psBranch.GetNewVertexIndex();
    Assert(sharedVertexCount >= 3);

    // Calculate the branch tip normal
    Vector3 normal;
    {
      const AutoArray<BYTE> &vertices = psBranch.GetVertices();
      SPrimitiveVertex *pBV = (SPrimitiveVertex*)vertices.Data();
      normal = (pBV[sharedVertexIndex + 1]._position - pBV[sharedVertexIndex]._position).CrossProduct(pBV[sharedVertexIndex + 2]._position - pBV[sharedVertexIndex]._position).Normalized();
    }

    // Add branch tip vertices
    for (int i = 0; i < sharedVertexCount; i++)
    {
      const AutoArray<BYTE> &vertices = psBranch.GetVertices();
      SPrimitiveVertex *pBV = (SPrimitiveVertex*)vertices.Data();
      SPrimitiveVertex pv;
      pv._position = pBV[sharedVertexIndex + i]._position;
      pv._normal = normal;
      pv._diffuse = pBV[sharedVertexIndex + i]._diffuse;
      pv._u0 = (cos((2.0f * H_PI * i) / sharedVertexCount) + 1.0f) * 0.5f;
      pv._v0 = (sin((2.0f * H_PI * i) / sharedVertexCount) + 1.0f) * 0.5f;
      psBranch.AddVertex(&pv);
    }

    for (int i = 1; i < (sharedVertexCount - 1); i++) {
      psBranch.AddIndex(_textureIndex, nvi + 0);
      psBranch.AddIndex(_textureIndex, nvi + i);
      psBranch.AddIndex(_textureIndex, nvi + i + 1);
    }

    /*
    for (int i = 0; i < sharedVertexCount - 2; i++) {
    psBranch.AddIndex(_textureIndex, sharedVertexIndex);
    psBranch.AddIndex(_textureIndex, sharedVertexIndex + i + 1);
    psBranch.AddIndex(_textureIndex, sharedVertexIndex + i + 2);
    }
    */
  }

//   int CComponent::DrawPrimitive(CPrimitiveStream &psBranch,
//     int sharedVertexIndex,
//     int sharedVertexCount,
//     float detail,
//     SSlot *pSlot) 
//   {
// 
//     // Get the primitive
//     CPrimitive *pPrimitive = GetPrimitive();
// 
//     // Draw it, fill the slots
//     if (pPrimitive) 
//     {
//       return pPrimitive->Draw(psBranch, sharedVertexIndex, sharedVertexCount, detail, pSlot);
//     }
//     else 
//     {
//       return 0;
//     }
//   }

  int CComponent::DrawSubtree(const RefArray<CComponent> &components,
    const SActualParams &ap,
    const SConstantParams &cp,
    CPrimitiveStream &psBranch,
    int &walkthroughCount,
    int walkthroughDepth,
    int sharedVertexIndex,
    int sharedVertexCount,
    int *pPolyplaneType,
    CBBoxCounter *pBBoxCounter) 
  {
    /*
    walkthroughCount++;

    // Check the limits of component count
    if (walkthroughCount >= MAX_WALKTHROUGH_COUNT) {
    LogF("Too many components.");
    return 0;
    }

    // Check the limits of depth
    if (walkthroughDepth >= MAX_WALKTHROUGH_DEPTH) {
    LogF("Too big depth of walkthrough.");
    return 0;
    }

    // Create buffer of children
    CComponentChildBuffer ccb;

    // Set primitive parameters, create children
    if (!Execute(ap, cp, ccb)) {
    LogF("Error in component execution.");
    return 0;
    };

    // Draw the primitive, fill the slots, update BBox counter
    SSlot pSlot[MAX_SLOT_COUNT];
    CPrimitive *pPrimitive = GetPrimitive();
    if (pPrimitive) {
    pPrimitive->Draw(psBranch, sharedVertexIndex, sharedVertexCount, cp._detail, pSlot);
    if (pBBoxCounter) {
    pPrimitive->UpdateBBox(*pBBoxCounter);
    }
    if (pPolyplaneType) {
    *pPolyplaneType = pPrimitive->GetPolyplaneType();
    }
    }

    // Cycle through the children
    int childCount = ccb.GetChildCount();
    for (int i = 0; i < childCount; i++) {

    // Get the child
    SComponentChild *pChild = ccb.GetChild(i);

    // Optimalization - remember the index of slot
    int slotIndex = pChild->_slotIndex;

    // Retrieve proper shared vertex index and count from the slot and draw the child
    if (pChild->_componentIndex == -1) {
    LogF("Child exists, but the index is not set.");
    return 0;
    }
    else {
    if (slotIndex > -1) {
    if (!components[pChild->_componentIndex]->DrawSubtree(
    components,
    pChild->_actualParams,
    cp,
    psBranch,
    walkthroughCount,
    walkthroughDepth + 1,
    pSlot[slotIndex]._sharedVertexIndex,
    pSlot[slotIndex]._sharedVertexCount,
    NULL,
    pBBoxCounter)) {
    return 0;
    };
    }
    else {
    if (!components[pChild->_componentIndex]->DrawSubtree(
    components,
    pChild->_actualParams,
    cp,
    psBranch,
    walkthroughCount,
    walkthroughDepth + 1,
    0,
    0,
    NULL,
    pBBoxCounter)) {
    return 0;
    };
    }
    }
    }
    */
    // Succeeded
    return 1;
  }

  void CComponent::Walkthrough(const RefArray<CComponent> &Components,
    const SActualParams &ap,
    const SConstantParams &cp) 
  {
    CComponentChildBuffer ccb;
    Execute(ap, cp, ccb);
    for (int i = 0; i < ccb.GetChildCount(); i++) {
      SComponentChild *pChild = ccb.GetChild(i);
      Components[pChild->_componentIndex]->Walkthrough(Components, pChild->_actualParams, cp);
    }
  }

  int CComponent::Draw(PDirect3DDevice &pD3DDevice,
    DWORD hVSColor,
    DWORD hPSColor,
    DWORD hVSNormal,
    DWORD hPSNormal,
    const RefArray<CComponent> &components,
    TVectorSpace<CPolyplane> &polyplaneSpace,
    const SActualParams &ap,
    const SConstantParams &cp,
    CPrimitiveStream &psBranch,
    CPrimitiveStream &psPolyplane,
    int sharedVertexIndex,
    int sharedVertexCount,
    int walkthroughDepth,
    int walkthroughCount,
    int disablePolyplanes,
    CBBoxCounter *pBBoxCounter) 
  {

    /*
    // Check the limits of depth
    if (walkthroughDepth >= MAX_WALKTHROUGH_DEPTH) {
    LogF("Too big depth of walkthrough.");
    return walkthroughCount;
    }

    // Create buffer of children
    CComponentChildBuffer ccb;

    // Set primitive parameters, create children
    Execute(ap, cp, ccb);

    // Get the primitive
    CPrimitive *pPrimitive = GetPrimitive();

    // Set drawOption status
    int drawOption;
    if (pPrimitive) {
    drawOption = pPrimitive->GetPolyplaneType();
    }
    else {
    drawOption = 0;
    }

    // Determine wheter we will render primitives into polyplane
    if ((drawOption) && (!disablePolyplanes)) {

    // Close graphics
    CloseBranchTip(psBranch, sharedVertexIndex, sharedVertexCount);

    // Get origin of the polyplane. Direction will be identical with the input origin,
    // Up vector will points in the Y direction. This is done because of deformations
    // in Y direction like gravitropism and phototropism.
    Matrix4 polyplaneOrigin;
    polyplaneOrigin = ap._origin;

    // Get the polyplane if it was already created
    float polyplaneVector[2];
    polyplaneVector[0] = ap._age;
    polyplaneVector[1] = ap._counter;
    Ref<CPolyplane> polyplane = polyplaneSpace.GetVector(polyplaneVector);

    // If the polyplane doesn't exist, then generate it
    if (polyplane.IsNull()) {

    // Create BBox
    CBBoxCounter BBoxCounter;
    BBoxCounter.Init(polyplaneOrigin);

    // Create temporary primitive stream
    CPrimitiveStream tempPS(psBranch);

    // Draw the rest of the tree and start with this branch
    walkthroughCount = Draw(pD3DDevice,
    hVSColor, hPSColor, hVSNormal, hPSNormal,
    components, polyplaneSpace, ap, cp, tempPS, psPolyplane,
    0, 0, walkthroughDepth, walkthroughCount, 1, &BBoxCounter);

    // Create new polyplane
    switch (drawOption) {
    case PT_BUNCH:
    polyplane = new CPolyplaneBunch();
    break;
    default:
    polyplane = new CPolyplaneBlock();
    break;
    }
    polyplane->Init(pD3DDevice, 128);

    // Render it to polyplane
    SBBox BBox = BBoxCounter.GetBBox();
    //polyplane->Render(tempPS, hVSColor, hPSColor, hVSNormal, hPSNormal, polyplaneOrigin, BBox.GetFocus(), BBox.GetDimension(), Vector3(0, 0, BBoxCounter.GetAvgDir()), ap._age);

    // Save the polyplane
    polyplaneSpace.AddVector(polyplane, polyplaneVector);
    }

    // Draw polyplane
    polyplane->Draw(polyplane->_id * 3, psPolyplane, polyplaneOrigin, ap._age, RString(""));
    }
    else {
    // Draw the primitive, fill the slots, update BBox counter
    SSlot pSlot[MAX_SLOT_COUNT];
    if (pPrimitive) {
    pPrimitive->Draw(psBranch, sharedVertexIndex, sharedVertexCount, cp._detail, pSlot);
    if (pBBoxCounter) {
    pPrimitive->UpdateBBox(*pBBoxCounter);
    }
    }

    // Cycle through the children
    int childCount = ccb.GetChildCount();
    for (int i = 0; i < childCount; i++) {

    // Check the limits of component count
    if (walkthroughCount >= MAX_WALKTHROUGH_COUNT) {
    LogF("Too many components.");
    return walkthroughCount;
    }

    // Get the child
    SComponentChild *pChild = ccb.GetChild(i);

    // Optimalization - remember the index of slot
    int slotIndex = pChild->_slotIndex;

    // Retrieve proper shared vertex index and count from the slot and draw the child
    if (slotIndex > -1) {
    if (pChild->_componentIndex == -1) {
    LogF("Childern exists, but the index is not set.");
    }
    else {
    walkthroughCount = components[pChild->_componentIndex]->Draw(
    pD3DDevice,
    hVSColor, hPSColor, hVSNormal, hPSNormal,
    components,
    polyplaneSpace,
    pChild->_actualParams,
    cp, psBranch, psPolyplane,
    pSlot[slotIndex]._sharedVertexIndex,
    pSlot[slotIndex]._sharedVertexCount,
    walkthroughDepth + 1,
    walkthroughCount + 1,
    disablePolyplanes,
    pBBoxCounter);
    }
    }
    else {
    walkthroughCount = components[pChild->_componentIndex]->Draw(
    pD3DDevice,
    hVSColor, hPSColor, hVSNormal, hPSNormal,
    components,
    polyplaneSpace,
    pChild->_actualParams,
    cp, psBranch, psPolyplane, 0, 0,
    walkthroughDepth + 1,
    walkthroughCount + 1,
    disablePolyplanes,
    pBBoxCounter);
    }
    }
    }
    */






    return walkthroughCount;
  }

  float CComponent::Rand(int seed) 
  {
    CRandomFlyman rf;
    return rf.Random(seed);
  }

  int CComponent::RandIndex(int seed) 
  {
    CRandomFlyman rf;
    return rf.RandomIndex(seed);
  }

  float CComponent::RandShake(int seed,
    float value,
    float interval) 
  {
    return value + (Rand(seed) - 0.5f) * interval * 2.0f;
  }

  float CComponent::RandGauss(int seed, float exp, float var, float interval) {
    float r0 = Rand(seed);
    float r1 = Rand(RandIndex(seed));
    return max(-interval + exp, min(interval + exp, (sqrt(-2.0f * log(r0)) * (cos(2.0f * H_PI * r1) + sin(2.0f * H_PI * r1)) * 0.5f) * var + exp));
  }

  float CComponent::GetLinearCurveValue(AutoArray<float> &aa, float index) 
  {
    saturate(index, 0.0f, 1.0f);
    double IntegerPart;
    float FractionPart = modf((float)(aa.Size() - 1)*(index), &IntegerPart);
    int i = toInt(IntegerPart);
    if (i < (aa.Size() - 1)) {
      return aa[i] + (aa[i + 1] - aa[i]) * FractionPart;
    }
    else {
      return aa[i];
    }
  }
  /*
  float CComponent::GetLinearNonUniformCurveValue(AutoArray<float> &aa, float index) {
  saturate(index, 0.0f, 1.0f);

  return 0.0;
  }
  */

  float CComponent::GetQuadraticCurveValue(AutoArray<float> &aa, float index) 
  {
    saturate(index, 0.0f, 1.0f);
    int nodeCount = aa.Size() / 2;
    double integerPart;
    float fractionPart = modf((nodeCount - 1) * index, &integerPart);
    int i = toInt(integerPart);
    if (i == nodeCount - 1) {
      i--;
      fractionPart += 1.0;
    }

    float dY = aa[(i + 1) * 2] - aa[i * 2];
    float dX = 1.0f / (nodeCount - 1);
    float t = aa[i * 2 + 1];
    float a = (dY - t * dX) / (dX * dX);

    return a * fractionPart * fractionPart + t * fractionPart + aa[i * 2];


    /*
    if (i < (aa.Size()/2 - 1)) {
    float dY = aa[(i + 1) * 2] - aa[i * 2];
    float dX = 1.0f / (aa.Size()/2 - 1);
    float a = (dY - aa[i + 1] * dX) / (dX * dX);


    return aa[i] + (aa[i + 1] - aa[i]) * FractionPart;
    }
    else {
    return aa[i];
    }
    */
  }

  float CComponent::GetCurveValue(AutoArray<float> &aa, float index) 
  {

    if (aa.Size() > 0)
    {
      // PolyLineEx
      if (aa[0] == 3)
      {
        AutoArray<float> x, y;
        int size = (aa.Size() - 1) / 3;
        x.Realloc(size); x.Resize(size);
        y.Realloc(size); y.Resize(size);
        for (int i = 0; i < size; i++)
        {
          x[i] = aa[i * 3 + 1];
          y[i] = aa[i * 3 + 2];
        }
        return Lint(x.Data(), y.Data(), size, index);
      }
    }
    // Default value
    return 1;
  }

  float CComponent::Round(float number) 
  {
    return (float)toInt(number);
  }

  int CComponent::GetIndexOfName(const RString &name) 
  {    
    RString fullname=name;
    int size = _componentNames.Size();
    if (_paramDB) 
    {
      fullname=_paramDB->GetFullComponentName(name);
    }
    for (int i = 0; i < size; i++)
    {
      if (fullname == _componentNames[i]) return i;
    }
    return -1;
  }

  void CComponent::PrintNumber(const char *text, float num) 
  {

    LogF(text, num);

    //vaLogF(text, (va_list)&num);
    /*
    va_list arglist;
    va_start(arglist, format);

    LogF(format, arglist);

    va_end(arglist);
    */
  }

  // ------------------------------------------
  // Following code is here for script purposes

  DEFINE_FAST_ALLOCATOR(GameDataComponentChildBuffer)

    RString GameDataComponentChildBuffer::GetText() const
  {
    if( _value==NULL ) return "<NULL-object>";
    return "ComponentChildBuffer";
  }

  bool GameDataComponentChildBuffer::IsEqualTo(const GameData *data) const
  {
    const GameComponentChildBufferType val1 = GetComponentChildBuffer();
    const GameComponentChildBufferType val2 = static_cast<const GameDataComponentChildBuffer *>(data)->GetComponentChildBuffer();
    return val1==val2;
  }

#ifndef CHECK
#define CHECK(command) {LSError err = command; if (err != 0) return err;}
#endif

  LSError GameDataComponentChildBuffer::Serialize(ParamArchive &ar)
  {
    CHECK(base::Serialize(ar));
    return (LSError)0;
  }

  GameData *CreateGameDataComponentChildBuffer() {return new GameDataComponentChildBuffer();}

  CComponentChildBuffer *GetComponentChildBuffer(GameValuePar oper) 
  {
    if (oper.GetType() == GameComponentChildBuffer) {
      return static_cast<GameDataComponentChildBuffer*> (oper.GetData())->GetComponentChildBuffer();
    }
    return NULL;
  }

  GameValue ComponentChildBuffer_AddChild(const GameState *state,
    GameValuePar oper1,
    GameValuePar oper2) 
  {

    CComponentChildBuffer *componentChildBuffer = GetComponentChildBuffer(oper1);
    if (!componentChildBuffer) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;

    // Array of 4 elements
    if (array.Size() != 4)
    {
      if (state) state->SetError(EvalDim, array.Size(), 4);
      return GameValue();
    }

    for (int i = 0; i < array.Size(); i++)
        if (array[i].GetNil()) {
          if (state) state->SetError(EvalBadVar);
          LogF("#Error: addChild parameter %d cannot be NIL", i);
          return GameValue();
        }

    for (int i = 0; i < 3; i++) {
        if (array[i].GetType() != GameScalar)
        {
          if (state) state->TypeError(GameScalar, array[i].GetType());
          LogF("#Error: addChild parameter %d must be SCALAR", i);
          return GameValue();
        }
    }
    // importancy
    if (array[3].GetType() != GameArray)
    {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      LogF("#Error: addChild parameter 3 must be ARRAY");
      return GameValue();
    }

    // Actual parameters
    const GameArrayType &actualParams = array[3];
    int actualParamsCount = actualParams.Size();

    // Array of 8 or 9 elements
    if (actualParamsCount < 8 || actualParamsCount>11)
    {
      if (state) state->SetError(EvalDim, actualParams.Size(), 10);
      return GameValue();
    }

    for (int i = 0; i < actualParams.Size(); i++)
        if (actualParams[i].GetNil()) {
          if (state) state->SetError(EvalBadVar);
          LogF("#Error: addChild position %d of actual parameters cannot be NIL.", i);
          return GameValue();
        }

    // origin
    if (!IsTypeMatrix4(actualParams[0])) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }
    // seed
    if (actualParams[1].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, actualParams[1].GetType());
      return GameValue();
    }
    // age
    if (actualParams[2].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, actualParams[2].GetType());
      return GameValue();
    }
    // counter
    if (actualParams[3].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, actualParams[3].GetType());
      return GameValue();
    }

    // trunk
    if (actualParams[4].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, actualParams[4].GetType());
      return GameValue();
    }

    // diaOrigin
    if (!IsTypeVector3(actualParams[5])) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    // textureLength
    if (actualParams[6].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, actualParams[6].GetType());
      return GameValue();
    }

    // geometrySpin
    if (actualParams[7].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, actualParams[7].GetType());
      return GameValue();
    }

    SActualParams ap;
    ap._origin = ConvertGameValueToMatrix4(actualParams[0]);
    ap._seed = toInt(actualParams[1]);
    ap._age = actualParams[2];
    ap._counter = toInt(actualParams[3]);
    ap._trunk = actualParams[4];
    ap._diaOrigin = ConvertGameValueToVector3(actualParams[5]);
    ap._textureVOffset = actualParams[6];
    ap._geometrySpin = actualParams[7];
    if (actualParamsCount >= 9) ap._optional = actualParams[8];
    if (actualParamsCount >= 10) ap._branchTypeId=toLargeInt(actualParams[9]);
    else ap._branchTypeId=0;
    if (actualParamsCount >= 11) {
        ap._polyplaneType=abs(toLargeInt(actualParams[10]));
        ap._forcePolyplane =  toLargeInt(actualParams[10]) < 0;
    }
    else 
    {
        ap._polyplaneType=PT_NOTDEFINED;
        ap._forcePolyplane=false;
    }

    componentChildBuffer->AddChild(toInt(array[0]), array[1], toInt(array[2]), ap);

    return GameValue();
  }

  GameValue ComponentChildBuffer_ForcePolyplane(const GameState *state, GameValuePar oper1) {
        int type = toInt(oper1);
        if (type > 0) type = -type;
        return (float)type;
  }
  GameData *CreateGameComponentChildBuffer(ParamArchive *ar) {
      return 0;
  }



    TYPES_GAMECOMPONENTCHILDBUFFER(DEFINE_TYPE,"")

  void CComponentChildBuffer::RegisterCommands(GameState &gState)
  {
    GameState& state = gState;
    TYPES_GAMECOMPONENTCHILDBUFFER(REGISTER_TYPE,Category)
    //gState.NewType("COMPONENT_CHILD_BUFFER", GameComponentChildBuffer, CreateGameDataComponentChildBuffer, "ComponentChildBuffer");
    gState.NewOperator(GameOperator(GameNothing, "addChild", function, ComponentChildBuffer_AddChild, GameComponentChildBuffer, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameNothing, "forcePolyplane", ComponentChildBuffer_ForcePolyplane, GameScalar TODO_FUNCTION_DOCUMENTATION));
  };


  DEFINE_FAST_ALLOCATOR(GameDataComponent)

    RString GameDataComponent::GetText() const
  {
    if( _value==NULL ) return "<NULL-object>";
    return "Component";
  }

  bool GameDataComponent::IsEqualTo(const GameData *data) const
  {
    const GameComponentType val1 = GetComponent();
    const GameComponentType val2 = static_cast<const GameDataComponent *>(data)->GetComponent();
    return val1==val2;
  }

#ifndef CHECK
#define CHECK(command) {LSError err = command; if (err != 0) return err;}
#endif

  LSError GameDataComponent::Serialize(ParamArchive &ar)
  {
    CHECK(base::Serialize(ar));
    //	CHECK(ar.SerializeRef("value", _value, 1))
    return (LSError)0;
  }

  GameData *CreateGameDataComponent() {return new GameDataComponent();}

  CComponent *GetComponent(GameValuePar oper) {
    if (oper.GetType() == GameComponent) {
      return static_cast<GameDataComponent*> (oper.GetData())->GetComponent();
    }
    return NULL;
  }

  GameValue Component_Rand(const GameState *state,      GameValuePar oper) 
  {
    return CComponent::Rand(toInt(oper));
  }

  GameValue Component_RandIndex(const GameState *state,        GameValuePar oper) 
  {
    return (float)CComponent::RandIndex(toLargeInt(oper));
  }


  GameValue Component_RandShake(const GameState *state,          GameValuePar oper) 
  {

    const GameArrayType &array = oper;
    if (array.Size() != 3)
    {
      if (state) state->SetError(EvalDim, array.Size(), 3);
      return 0.0f;
    }
    if (array[0].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return 0.0f;
    }
    if (array[1].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return 0.0f;
    }
    if (array[2].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return 0.0f;
    }

    return CComponent::RandShake(toLargeInt(array[0]), array[1], array[2]);
  }

  GameValue Component_RandGauss(const GameState *state,            GameValuePar oper) 
  {

    const GameArrayType &array = oper;
    if (array.Size() != 4)
    {
      if (state) state->SetError(EvalDim, array.Size(), 4);
      return 0.0f;
    }
    if (array[0].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return 0.0f;
    }
    if (array[1].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return 0.0f;
    }
    if (array[2].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return 0.0f;
    }
    if (array[3].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      return 0.0f;
    }

    return CComponent::RandGauss(toLargeInt(array[0]), array[1], array[2], array[3]);
  }

  GameValue Component_GetLinearCurveValue(const GameState *state,              GameValuePar oper) 
  {

    const GameArrayType &array = oper;
    if (array.Size() != 2) {
      if (state) state->SetError(EvalDim, array.Size(), 2);
      return 0.0f;
    }

    // Curve
    if (array[0].GetType() != GameArray) {
      if (state) state->TypeError(GameArray, array[0].GetType());
      return 0.0f;
    }
    const GameArrayType &curve = array[0];
    for (int i = 0; i < curve.Size(); i++) {
      if (curve[i].GetType() != GameScalar) {
        if (state) state->TypeError(GameScalar, curve[i].GetType());
        return 0.0f;
      }
    }

    // Index
    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return 0.0f;
    }

    // Create an autoarray which represents a curve
    AutoArray<float> aa; aa.Realloc(curve.Size()); aa.Resize(curve.Size());
    for (int i = 0; i < aa.Size(); i++) {
      aa[i] = curve[i];
    }

    return CComponent::GetLinearCurveValue(aa, array[1]);
  }

  GameValue Component_GetQuadraticCurveValue(const GameState *state,    GameValuePar oper) 
  {

    const GameArrayType &array = oper;
    if (array.Size() != 2) {
      if (state) state->SetError(EvalDim, array.Size(), 2);
      return 0.0f;
    }

    // Curve
    if (array[0].GetType() != GameArray) {
      if (state) state->TypeError(GameArray, array[0].GetType());
      return 0.0f;
    }
    const GameArrayType &curve = array[0];
    for (int i = 0; i < curve.Size(); i++) {
      if (curve[i].GetType() != GameScalar) {
        if (state) state->TypeError(GameScalar, curve[i].GetType());
        return 0.0f;
      }
    }

    // Index
    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return 0.0f;
    }

    // Create an autoarray which represents a curve
    AutoArray<float> aa; aa.Realloc(curve.Size()); aa.Resize(curve.Size());
    for (int i = 0; i < aa.Size(); i++) {
      aa[i] = curve[i];
    }

    return CComponent::GetQuadraticCurveValue(aa, array[1]);
  }

  GameValue Component_GetCurveValue(const GameState *state,    GameValuePar oper) 
  {

    const GameArrayType &array = oper;
    if (array.Size() != 2) {
      if (state) state->SetError(EvalDim, array.Size(), 2);
      return 0.0f;
    }

    // Curve
    if (array[0].GetType() != GameArray) {
      if (state) state->TypeError(GameArray, array[0].GetType());
      return 0.0f;
    }
    const GameArrayType &curve = array[0];
    for (int i = 0; i < curve.Size(); i++) {
      if (curve[i].GetType() != GameScalar) {
        if (state) state->TypeError(GameScalar, curve[i].GetType());
        return 0.0f;
      }
    }

    // Index
    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return 0.0f;
    }

    // Create an autoarray which represents a curve
    AutoArray<float> aa; aa.Realloc(curve.Size()); aa.Resize(curve.Size());
    for (int i = 0; i < aa.Size(); i++) {
      aa[i] = curve[i];
    }

    return CComponent::GetCurveValue(aa, array[1]);
  }

  GameValue Component_Get3DGraphValue(const GameState *state, GameValuePar oper)
  {
    const GameArrayType &array = oper;
    if (array.Size() < 2 || array.Size()>3 ) {
      if (state) state->SetError(EvalDim, array.Size(), 2);
      return 0.0f;
    }
    // Curve
    if (array[0].GetType() != GameArray) 
    {
      if (state) state->TypeError(GameArray, array[0].GetType());
      return 0.0f;
    }
    const GameArrayType &curve = array[0];
    if (curve.Size()!=3) 
    {
      if (state) state->SetError(EvalDim, curve.Size(),3);
      return 0.0f;
    }
    const GameArrayType &xaxis=curve[0];
    const GameArrayType &yaxis=curve[1];
    const GameArrayType &zaxis=curve[2];
    if (xaxis.Size()==0 || yaxis.Size()==0 || zaxis.Size()!=xaxis.Size()*yaxis.Size())
    {
      if (state) state->SetError(EvalForeignError,"Count of third doesn't match count of first multiply count of second");
        return 0.0f;      
    }

    float x,y;
    if (array.Size()==2 && array[1].GetType()==GameArray)
    {
      const GameArrayType &data=array[1];
      if (data.Size()>=2 && data[0].GetType()==GameScalar && data[1].GetType()==GameScalar)
      {
        x=data[0];
        y=data[1];
      }
      else
      {
        if (state) state->SetError(EvalForeignError,"Excepted vector as second parameter (array of two scalar numbers)");
        return 0.0f;
      }
    }
    else if (array.Size()==3 && array[1].GetType()==GameScalar && array[2].GetType()==GameScalar)
    {
      x=array[1];
      y=array[2];
    }
    else 
    {
      if (state) state->SetError(EvalForeignError,"Coordinates in unknown format");
      return 0.0f;
    }

    int xpos=xaxis.Size();
    int ypos=yaxis.Size();
    for (int i=0;i<xaxis.Size();i++) if ((float)xaxis[i]>x) {xpos=i;break;}
    for (int i=0;i<yaxis.Size();i++) if ((float)yaxis[i]>y) {ypos=i;break;}
    int xmin=(xpos-1)<0?0:xpos-1;
    int xmax=xpos>=xaxis.Size()?xaxis.Size()-1:xpos;
    int ymin=(ypos-1)<0?0:ypos-1;
    int ymax=ypos>=yaxis.Size()?yaxis.Size()-1:ypos;
    
    class GetValueAt
    {
      const GameArrayType &_data;
      int _xlen;
    public:
      GetValueAt(const GameArrayType &ddata, int xxlen):_xlen(xxlen),_data(ddata) {}
      float operator()(int x,int y) const
      {
        return _data[y*_xlen+x];
      }
    };
    GetValueAt getValue(zaxis,xaxis.Size());

    float resx1,resx2,res;
    if (xmin==xmax)
    {
      resx1=getValue(xmin,ymin);
      resx2=getValue(xmin,ymax);
    }
    else
    {
      float pos=(x-(float)xaxis[xmin])/((float)xaxis[xmax]-(float)xaxis[xmin]);
      float v1,v2;
      v1=getValue(xmin,ymin);
      v2=getValue(xmax,ymin);
      resx1=v1*(1-pos)+v2*(pos);
      v1=getValue(xmin,ymax);
      v2=getValue(xmax,ymax);
      resx2=v1*(1-pos)+v2*(pos);
    }
    if (ymin==ymax)
    {
      res=resx1;
    }
    else
    {
      float pos=(y-(float)yaxis[ymin])/((float)yaxis[ymax]-(float)yaxis[ymin]);
      res=resx1*(1-pos)+resx2*pos;
    }
    return res;
  }

    GameValue Component_Round(const GameState *state,GameValuePar oper) 
    {
      return CComponent::Round(oper);
    }

    GameValue Component_GetIndexOfName(const GameState *state,
      GameValuePar oper1,
      GameValuePar oper2) {

        CComponent *component = GetComponent(oper1);
        if (!component) {
          if (state) state->SetError(EvalGen);
          LogF("#Function getIndexOfName failed! Not component: ?? getIndexOfName %s",
              ((RString)oper2).Data());
          return GameValue();
        }

        RString name = oper2;
        int cmpidx =  component->GetIndexOfName(name);
        if (cmpidx < 0)
            LogF("#Function getIndexOfName failed! Object '%s' not fouund.\n\tComponent: %s",
                component->GetComponentName().Data(),
                name.Data());
            
        return (float)cmpidx;
      }

      GameValue Component_PrintNumber(const GameState *state,
        GameValuePar oper) {

          const GameArrayType &array = oper;
          if (array.Size() != 2)
          {
            if (state) state->SetError(EvalDim, array.Size(), 2);
            return GameValue();
          }
          if (array[0].GetType() != GameString)
          {
            if (state) state->TypeError(GameString, array[0].GetType());
            return GameValue();
          }
          if (array[1].GetType() != GameScalar)
          {
            if (state) state->TypeError(GameScalar, array[1].GetType());
            return GameValue();
          }

          CComponent::PrintNumber(RString(array[0]), array[1]);

          return GameValue();
        }

        GameValue Component_Max(const GameState *state,
          GameValuePar oper1,
          GameValuePar oper2) {

            if (oper1.GetType() != GameScalar)
            {
              if (state) state->TypeError(GameScalar, oper1.GetType());
              return GameValue();
            }

            if (oper2.GetType() != GameScalar)
            {
              if (state) state->TypeError(GameScalar, oper2.GetType());
              return GameValue();
            }

            float a = oper1;
            float b = oper2;

            return max(a, b);
          }

          GameValue Component_Min(const GameState *state,
            GameValuePar oper1,
            GameValuePar oper2) {

              if (oper1.GetType() != GameScalar)
              {
                if (state) state->TypeError(GameScalar, oper1.GetType());
                return GameValue();
              }

              if (oper2.GetType() != GameScalar)
              {
                if (state) state->TypeError(GameScalar, oper2.GetType());
                return GameValue();
              }

              float a = oper1;
              float b = oper2;

              return min(a, b);
            }

            GameData *CreateGameComponent(ParamArchive *ar) {
                return 0;
            }
            TYPES_GAMECOMPONENT(DEFINE_TYPE,"");  
   
            void CComponent::RegisterCommands(GameState &gState)
            {
              GameState& state = gState;
                TYPES_GAMECOMPONENT(REGISTER_TYPE,Category);  
   //           gState.NewType("COMPONENT", GameComponent, CreateGameDataComponent, "Component");
              gState.NewOperator(GameOperator(GameScalar,   "getIndexOfName",       function, Component_GetIndexOfName,       GameComponent, GameString TODO_OPERATOR_DOCUMENTATION));
              gState.NewOperator(GameOperator(GameScalar,   "max",                  function, Component_Max,                  GameScalar, GameScalar TODO_OPERATOR_DOCUMENTATION));
              gState.NewOperator(GameOperator(GameScalar,   "min",                  function, Component_Min,                  GameScalar, GameScalar TODO_OPERATOR_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "rand",                   Component_Rand,                   GameScalar TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "randIndex",              Component_RandIndex,              GameScalar TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "randShake",              Component_RandShake,              GameArray TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "randGauss",              Component_RandGauss,              GameArray TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "getLinearCurveValue",    Component_GetLinearCurveValue,    GameArray TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "getQuadraticCurveValue", Component_GetQuadraticCurveValue, GameArray TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "getCurveValue",          Component_GetCurveValue,          GameArray TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "get3DGraphValue",        Component_Get3DGraphValue,        GameArray TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameScalar,   "round",                  Component_Round,                  GameScalar TODO_FUNCTION_DOCUMENTATION));
              gState.NewFunction(GameFunction(GameNothing,  "printNumber",            Component_PrintNumber,            GameArray TODO_FUNCTION_DOCUMENTATION));
            };

            // ------------------------------------------
  }