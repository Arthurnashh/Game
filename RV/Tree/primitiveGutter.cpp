#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveGutter.h"


namespace TreeEngine
{

  void CPGutter::SetTexture(RString name, int index)
  {
//    _textureName  = name;
    _textureIndex = index;
  }

  void CPGutter::SetGutterParams(float width,
                                float height,
                                float angle,
                                int segmentsCount,
                                float textureVOffset,
                                float textureUTiling,
                                float textureVTiling)
  {
    _width = width;
    _height = height;
    _angle = angle;
    _segmentsCount = segmentsCount;
    _textureVOffset = textureVOffset;
    _textureUTiling = textureUTiling;
    _textureVTiling = textureVTiling;
  }

  SPrimitiveChild CPGutter::GetFrontChild()
  {
    SPrimitiveChild result;
    result._origin.SetPosition(_origin.Position() + _origin.Direction() * _height);
    result._origin.SetOrientation(_origin.Orientation());
    result._slotIndex = 0;
    result._textureVOffset = _textureVOffset + _textureVTiling;
    result._geometrySpin = 0.0f;
    return result;
  }

  SPrimitiveChild CPGutter::GetSideChild(float directionShift,
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

  int CPGutter::Draw(CPrimitiveStream &ps,
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

    // Calculate the depth and new width from the angle
    float depth = cos(_angle * 0.5f) * _width;
    float width = sin(_angle * 0.5f) * _width;

    // Draw the cone and save the index of shared vertices
    if (sharedVertexCount > 0)
    {
      pSlot[0]._sharedVertexIndex = DrawGutterShared(
        ps,
        _textureIndex,
        _textureUTiling,
        sharedVertexIndex,
        (sharedVertexCount / 2) - 1/* + (sharedVertexCount & 1)*/,
        FrontOrigin,
        width,
        depth,
        _textureVOffset + _textureVTiling,
        _segmentsCount,
        (sharedVertexCount & 1) == 1/*false*/,
        &tAccu);
    }
    else
    {
      pSlot[0]._sharedVertexIndex = DrawGutter(
        ps,
        _textureIndex,
        _textureUTiling,
        _origin,
        width,
        depth,
        _textureVOffset,
        _segmentsCount,
        FrontOrigin,
        width,
        depth,
        _textureVOffset + _textureVTiling,
        _segmentsCount,
        &tAccu);
    }

    // Save the number of shared vertices
    pSlot[0]._sharedVertexCount = (_segmentsCount + 1) * 2;

    return tAccu;
  }

  void CPGutter::UpdatePointList(CPointList &pointList)
  {
    pointList.AddPoint(_origin.Position());
    pointList.AddPoint(_origin.Position() + _origin.Direction() * _height);
  }

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveGutter_SetTexture(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2)
  {
    CPGutter *primitiveGutter = dynamic_cast<CPGutter*>(GetPrimitive(oper1));

    if (!primitiveGutter) {
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

    primitiveGutter->SetTexture(array[0], toInt(array[1]));

    return GameValue();
  }

  GameValue PrimitiveGutter_SetGutterParams(const GameState *state,
                                            GameValuePar oper1,
                                            GameValuePar oper2)
  {
    CPGutter *primitiveGutter = dynamic_cast<CPGutter*>(GetPrimitive(oper1));

    if (!primitiveGutter)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 7) {
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

    if (array[6].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[6].GetType());
      return GameValue();
    }

    primitiveGutter->SetGutterParams(array[0], array[1], array[2], toInt(array[3]), array[4], array[5], array[6]);

    return GameValue();
  }

  GameValue PrimitiveGutter_GetFrontChild(const GameState *state,
                                          GameValuePar oper1)
  {
    CPGutter *primitiveGutter = dynamic_cast<CPGutter*>(GetPrimitive(oper1));

    if (!primitiveGutter)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    SPrimitiveChild child = primitiveGutter->GetFrontChild();

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveGutter_GetSideChild(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2)
  {
    CPGutter *primitiveGutter = dynamic_cast<CPGutter*>(GetPrimitive(oper1));

    if (!primitiveGutter)
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

    SPrimitiveChild child = primitiveGutter->GetSideChild(array[0], array[1], array[2], array[3], array[4]);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }


  void CPGutter::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setGutterTexture",    function, PrimitiveGutter_SetTexture,      GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setGutterParams",     function, PrimitiveGutter_SetGutterParams, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameArray,   "getGutterFrontChild", PrimitiveGutter_GetFrontChild, GamePrimitive TODO_FUNCTION_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getGutterSideChild",  function, PrimitiveGutter_GetSideChild,    GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
  };

  // ------------------------------------------

}