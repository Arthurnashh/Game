#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveStem.h"


namespace TreeEngine
{
  void CPStem::SetTexture(RString name, int index) {
    _textureIndex = index;
  }

  void CPStem::SetStemParams(float width,
                            float height,
                            float textureVOffset,
                            float geometrySpin,
                            float textureUTiling,
                            float textureVTiling) {
    _width = width;
    _height = height;
    _textureVOffset = textureVOffset;
    _geometrySpin = geometrySpin;
    _textureUTiling = textureUTiling;
    _textureVTiling = textureVTiling;
  }

  SPrimitiveChild CPStem::GetFrontChild() {
    SPrimitiveChild result;
    result._origin.SetPosition(_origin.Position() + _origin.Direction() * _height);
    result._origin.SetOrientation(_origin.Orientation());
    result._slotIndex = 0;
    result._textureVOffset = _textureVOffset + _textureVTiling;
    result._geometrySpin = _geometrySpin;
    return result;
  }

  SPrimitiveChild CPStem::GetSideChild(float directionShift,
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

  SPrimitiveChild CPStem::GetSideChildFA(float directionShift,
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

  int CPStem::Draw(CPrimitiveStream &ps,
                  int sharedVertexIndex,
                  int sharedVertexCount,
                  bool isRoot,
                  float detail,
                  SSlot *pSlot) {

    int tAccu = 0;

    // Get the front origin
    Matrix4 FrontOrigin;
    FrontOrigin.SetPosition(_origin.Position() + _origin.Direction() * _height);
    FrontOrigin.SetOrientation(_origin.Orientation());

    // Count the number of sides
    int coneSidesNum = CONE_SIDES_NUM(detail, _width);

    // Draw the cone and save the index of shared vertices
    if (sharedVertexCount > 0) {
      pSlot[0]._sharedVertexIndex = DrawConeShared(
        ps,
        _textureIndex,
        _textureUTiling,
        D3DCOLOR_XRGB(255, 255, 255),
        sharedVertexIndex,
        sharedVertexCount - 1,
        isRoot,
        FrontOrigin,
        _width,
        _textureVOffset + _textureVTiling,
        coneSidesNum,
        &tAccu);
    }
    else {
      pSlot[0]._sharedVertexIndex =  DrawCone(
        ps,
        _textureIndex,
        _textureUTiling,
        D3DCOLOR_XRGB(255, 255, 255),
        _origin,
        _width,
        _textureVOffset,
        coneSidesNum,
        FrontOrigin,
        _width,
        _textureVOffset + _textureVTiling,
        coneSidesNum,
        &tAccu);
    }

    // Save the number of shared vertices
    pSlot[0]._sharedVertexCount = coneSidesNum + 1;

    return tAccu;
  }

  void CPStem::UpdatePointList(CPointList &pointList)
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

  int CPStem::DrawBody(ObjectData *o, int &bodyId)
  {
    return CPrimitive::DrawBlock(o, bodyId, _origin, _width, _height);
  };

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveStem_SetTexture(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPStem *primitiveStem = dynamic_cast<CPStem*>(GetPrimitive(oper1));

    if (!primitiveStem) {
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

    primitiveStem->SetTexture(array[0], toInt(array[1]));

    return GameValue();
  }

  GameValue PrimitiveStem_SetStemParams(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2) {

    CPStem *primitiveStem = dynamic_cast<CPStem*>(GetPrimitive(oper1));

    if (!primitiveStem) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 6) {
		  if (state) state->SetError(EvalDim, array.Size(), 6);
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

	  if (array[5].GetType() != GameScalar) {
		  if (state) state->TypeError(GameScalar, array[5].GetType());
		  return GameValue();
    }

    primitiveStem->SetStemParams(array[0], array[1], array[2], array[3], array[4], array[5]);

    return GameValue();
  }

  GameValue PrimitiveStem_GetFrontChild(const GameState *state,
                                        GameValuePar oper1) {

    CPStem *primitiveStem = dynamic_cast<CPStem*>(GetPrimitive(oper1));

    if (!primitiveStem) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    SPrimitiveChild child = primitiveStem->GetFrontChild();

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveStem_GetSideChild(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2)
  {

    CPStem *primitiveStem = dynamic_cast<CPStem*>(GetPrimitive(oper1));

    if (!primitiveStem) {
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

    SPrimitiveChild child = primitiveStem->GetSideChild(array[0], array[1], array[2], array[3], array[4]);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveStem_GetSideChildFA(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2)
  {

    CPStem *primitiveStem = dynamic_cast<CPStem*>(GetPrimitive(oper1));

    if (!primitiveStem)
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

    SPrimitiveChild child = primitiveStem->GetSideChildFA(array[0], array[1], axisRotation, tArraySize);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  void CPStem::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setStemTexture", function, PrimitiveStem_SetTexture,     GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setStemParams",  function, PrimitiveStem_SetStemParams,  GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameArray,   "getFrontChild", PrimitiveStem_GetFrontChild, GamePrimitive TODO_FUNCTION_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getSideChild",   function, PrimitiveStem_GetSideChild,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getSideChildFA", function, PrimitiveStem_GetSideChildFA, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
  };

  // ------------------------------------------

}