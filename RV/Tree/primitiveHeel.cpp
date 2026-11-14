#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveHeel.h"

namespace TreeEngine
{
  void CPHeel::SetHeelParams(float width, float textureUTiling, float textureVOffsetTrunk, float textureVOffsetRoot)
  {
    _width = width;
    _textureUTiling = textureUTiling;
    _textureVOffsetTrunk = textureVOffsetTrunk;
    _textureVOffsetRoot = textureVOffsetRoot;
  }

  SPrimitiveChild CPHeel::GetTrunkChild()
  {
    SPrimitiveChild result;
    result._origin = _origin;
    result._slotIndex = 0;
    result._textureVOffset = 0.0f;
    result._geometrySpin = 0.0f;
    return result;
  }

  SPrimitiveChild CPHeel::GetRootChild()
  {
    SPrimitiveChild result;
    result._origin.SetDirectionAndAside(-_origin.Direction(), _origin.DirectionAside());
    result._origin.SetPosition(_origin.Position());
    result._slotIndex = 1;
    result._textureVOffset = 0.0f;
    result._geometrySpin = 0.0f;
    return result;
  }

  int CPHeel::Draw(
    CPrimitiveStream &ps,
    int sharedVertexIndex,
    int sharedVertexCount,
    bool isRoot,
    float detail,
    SSlot *pSlot)
  {
    // Count the number of sides
    int coneSidesNum = CONE_SIDES_NUM(detail, _width);

    // Create vertices for the trunk child
    int originalVertexIndex = DrawHeel(ps, _textureUTiling, D3DCOLOR_XRGB(255, 255, 255), _origin, _width, _textureVOffsetTrunk, coneSidesNum);
    pSlot[0]._sharedVertexIndex = originalVertexIndex;
    pSlot[0]._sharedVertexCount = coneSidesNum + 1;

    // Create vertices for the root child (by copying and reverting the trunk vertices)
    // Note only position is reverted
    unsigned long vbBIndex = ps.GetNewVertexIndex();
    for (int i = 0; i < coneSidesNum + 1; i++)
    {
      SPrimitiveVertex pvRev;
      ps.GetVertex(originalVertexIndex + coneSidesNum - i, &pvRev);
      pvRev._v0 = _textureVOffsetRoot;
      ps.AddVertex(&pvRev);
    }
    pSlot[1]._sharedVertexIndex = vbBIndex;
    pSlot[1]._sharedVertexCount = coneSidesNum + 1;
    pSlot[1]._isRoot = true;

    // No triangle was drawn
    return 0;
  }

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveHeel_SetHeelParams(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2)
  {

    CPHeel *primitiveHeel = dynamic_cast<CPHeel*>(GetPrimitive(oper1));

    if (!primitiveHeel)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 4)
    {
      if (state) state->SetError(EvalDim, array.Size(), 4);
      return GameValue();
    }

    if (array[0].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    if (array[2].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return GameValue();
    }

    if (array[3].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      return GameValue();
    }

    primitiveHeel->SetHeelParams(array[0], array[1], array[2], array[3]);

    return GameValue();
  }

  GameValue PrimitiveHeel_GetTrunkChild(const GameState *state,
                                        GameValuePar oper1)
  {

    CPHeel *primitiveHeel = dynamic_cast<CPHeel*>(GetPrimitive(oper1));

    if (!primitiveHeel)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    SPrimitiveChild child = primitiveHeel->GetTrunkChild();

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveHeel_GetRootChild(const GameState *state,
                                       GameValuePar oper1)
  {

    CPHeel *primitiveHeel = dynamic_cast<CPHeel*>(GetPrimitive(oper1));

    if (!primitiveHeel)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    SPrimitiveChild child = primitiveHeel->GetRootChild();

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  void CPHeel::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setHeelParams",  function, PrimitiveHeel_SetHeelParams,  GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameArray,   "getTrunkChild", PrimitiveHeel_GetTrunkChild, GamePrimitive TODO_FUNCTION_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameArray,   "getRootChild", PrimitiveHeel_GetRootChild, GamePrimitive TODO_FUNCTION_DOCUMENTATION));
  }

  // ------------------------------------------
}