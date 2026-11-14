#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveStar.h"


namespace TreeEngine
{

  void CPStar::SetTexture(RString name, int index)
  {
    _textureIndex = index;
  }

  void CPStar::SetParams(float width,
                        float height,
                        int segmentsCount,
                        float textureVOffset,
                        float textureVTiling)
  {
    _width = width;
    _height = height;
    _segmentsCount = segmentsCount;
    _textureVOffset = textureVOffset;
    _textureVTiling = textureVTiling;
  }

  SPrimitiveChild CPStar::GetFrontChild()
  {
    SPrimitiveChild result;
    result._origin.SetPosition(_origin.Position() + _origin.Direction() * _height);
    result._origin.SetOrientation(_origin.Orientation());
    result._slotIndex = 0;
    result._textureVOffset = _textureVOffset + _textureVTiling;
    result._geometrySpin = 0.0f;
    return result;
  }

  SPrimitiveChild CPStar::GetSideChild(float directionShift,
                                      float sideShift,
                                      float slopeAngle,
                                      float twistAngle,
                                      float spinAngle)
  {
    Matrix3 RotSpin;
    RotSpin.SetRotationZ(spinAngle);
    Matrix3 RotAside;
    RotAside.SetRotationY(slopeAngle);
    Matrix3 RotDirection;
    RotDirection.SetRotationZ(twistAngle);
    Matrix3 SideChildOriginOrientation = _origin.Orientation() * RotDirection * RotAside * RotSpin;

    SPrimitiveChild result;
    result._origin.SetPosition(
      _origin.Position() + _origin.Direction() * directionShift
      + _origin.DirectionAside() * sideShift * cos(twistAngle)
      + _origin.DirectionUp() * sideShift * sin(twistAngle));
    result._origin.SetOrientation(SideChildOriginOrientation);
    result._slotIndex = -1;
    result._textureVOffset = 0.0f;
    result._geometrySpin = 0.0f;

    return result;
  }

  SPrimitiveChild CPStar::GetSideChildFA(float directionShift,
                                        float forwardShift,
                                        const AxisRotation *ar,
                                        int arCount)
  {

    // Add all axis rotations
    Matrix3 sideChildOriginOrientation = GetSideChildOriginOrientation(_origin.Orientation(),ar,arCount);

    SPrimitiveChild result;
    result._origin.SetPosition(
      _origin.Position() + _origin.Direction() * directionShift
      + sideChildOriginOrientation.Direction() * forwardShift);
    result._origin.SetOrientation(sideChildOriginOrientation);
    result._slotIndex = -1;
    result._textureVOffset = 0.0f;
    result._geometrySpin = 0.0f;

    return result;
  }

  int CPStar::Draw(CPrimitiveStream &ps,
                  int sharedVertexIndex,
                  int sharedVertexCount,
                  bool isRoot,
                  float detail,
                  SSlot *pSlot)
  {

    int tAccu = 0;

    // Get the front origin
    Matrix4 FrontOrigin;
    FrontOrigin.SetPosition(_origin.Position() + _origin.Direction() * _height);
    FrontOrigin.SetOrientation(_origin.Orientation());

    // Draw the star and save the index of shared vertices
    if (sharedVertexCount == _segmentsCount * 4)
    {
      pSlot[0]._sharedVertexIndex = DrawStarShared(
        ps,
        _textureIndex,
        sharedVertexIndex,
        sharedVertexCount,
        FrontOrigin,
        _width,
        _textureVOffset + _textureVTiling,
        _segmentsCount,
        &tAccu);
    }
    else
    {
      pSlot[0]._sharedVertexIndex = DrawStar(
        ps,
        _textureIndex,
        _origin,
        _width,
        _textureVOffset,
        FrontOrigin,
        _width,
        _textureVOffset + _textureVTiling,
        _segmentsCount,
        &tAccu);
    }

    // Save the number of shared vertices
    pSlot[0]._sharedVertexCount = _segmentsCount * 4;

    return tAccu;
  }

  void CPStar::UpdatePointList(CPointList &pointList)
  {
    pointList.AddPoint(_origin.Position() + _origin.DirectionUp() * _width * SquareCoef + _origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(_origin.Position() + _origin.DirectionUp() * _width * SquareCoef - _origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(_origin.Position() - _origin.DirectionUp() * _width * SquareCoef + _origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(_origin.Position() - _origin.DirectionUp() * _width * SquareCoef - _origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(_origin.Position() + _origin.DirectionUp() * _width * SquareCoef + _origin.DirectionAside() * _width * SquareCoef + _origin.Direction() * _height);
    pointList.AddPoint(_origin.Position() + _origin.DirectionUp() * _width * SquareCoef - _origin.DirectionAside() * _width * SquareCoef + _origin.Direction() * _height);
    pointList.AddPoint(_origin.Position() - _origin.DirectionUp() * _width * SquareCoef + _origin.DirectionAside() * _width * SquareCoef + _origin.Direction() * _height);
    pointList.AddPoint(_origin.Position() - _origin.DirectionUp() * _width * SquareCoef - _origin.DirectionAside() * _width * SquareCoef + _origin.Direction() * _height);
  }

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveStar_SetTexture(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2)
  {
    CPStar *primitiveStar = dynamic_cast<CPStar*>(GetPrimitive(oper1));

    if (!primitiveStar) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 2) {
      if (state) state->SetError(EvalDim, array.Size(), 2);
      return GameValue();
    }

    if (array[0].GetType() != GameString) {
      if (state) state->TypeError(GameString, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    primitiveStar->SetTexture(array[0], toInt(array[1]));

    return GameValue();
  }

  GameValue PrimitiveStar_SetParams(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2)
  {
    CPStar *primitiveStar = dynamic_cast<CPStar*>(GetPrimitive(oper1));

    if (!primitiveStar)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 5) {
      if (state) state->SetError(EvalDim, array.Size(), 5);
      return GameValue();
    }

    if (array[0].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    if (array[2].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return GameValue();
    }

    if (array[3].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      return GameValue();
    }

    if (array[4].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[4].GetType());
      return GameValue();
    }

    primitiveStar->SetParams(array[0], array[1], toInt(array[2]), array[3], array[4]);

    return GameValue();
  }

  GameValue PrimitiveStar_GetFrontChild(const GameState *state,
                                        GameValuePar oper1)
  {
    CPStar *primitiveStar = dynamic_cast<CPStar*>(GetPrimitive(oper1));

    if (!primitiveStar)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    SPrimitiveChild child = primitiveStar->GetFrontChild();

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveStar_GetSideChild(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2)
  {
    CPStar *primitiveStar = dynamic_cast<CPStar*>(GetPrimitive(oper1));

    if (!primitiveStar)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 5) {
      if (state) state->SetError(EvalDim, array.Size(), 5);
      return GameValue();
    }

    if (array[0].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    if (array[2].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return GameValue();
    }

    if (array[3].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      return GameValue();
    }

    if (array[4].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[4].GetType());
      return GameValue();
    }

    SPrimitiveChild child = primitiveStar->GetSideChild(array[0], array[1], array[2], array[3], array[4]);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveStar_GetSideChildFA(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2)
  {

    CPStar *primitiveStar = dynamic_cast<CPStar*>(GetPrimitive(oper1));

    if (!primitiveStar)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 3)
    {
      if (state) state->SetError(EvalDim, array.Size(), 3);
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

    // Array of transformations
    if (array[2].GetType() != GameArray)
    {
      if (state) state->TypeError(GameArray, array[2].GetType());
      return GameValue();
    }

    const GameArrayType &tArray = array[2];
    int tArraySize = tArray.Size();

    AxisRotation axisRotation[AXIS_ROTATION_COUNT_MAX];
    DoAssert(tArraySize <= AXIS_ROTATION_COUNT_MAX);

    for (int i = 0; i < tArraySize; i++)
    {
      if (tArray[i].GetType() != GameArray)
      {
        if (state) state->TypeError(GameArray, tArray[i].GetType());
        return GameValue();
      }

      const GameArrayType &pair = tArray[i];
      if (pair.Size() != 2)
      {
        if (state) state->SetError(EvalDim, pair.Size(), 2);
        return GameValue();
      }

      if (pair[0].GetType() != GameScalar)
      {
        if (state) state->TypeError(GameScalar, pair[0].GetType());
        return GameValue();
      }

      if (pair[1].GetType() != GameScalar)
      {
        if (state) state->TypeError(GameScalar, pair[1].GetType());
        return GameValue();
      }

      axisRotation[i]._at = (AxisType)toInt(pair[0]);
      axisRotation[i]._angle = pair[1];
    }

    SPrimitiveChild child = primitiveStar->GetSideChildFA(array[0], array[1], axisRotation, tArraySize);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  void CPStar::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setStarTexture",     function, PrimitiveStar_SetTexture,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setStarParams",      function, PrimitiveStar_SetParams,    GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameArray,   "getStarFrontChild",  PrimitiveStar_GetFrontChild, GamePrimitive TODO_FUNCTION_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getStarSideChild",   function, PrimitiveStar_GetSideChild, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getStarSideChildFA", function, PrimitiveStar_GetSideChildFA, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
  };

  // ------------------------------------------

}