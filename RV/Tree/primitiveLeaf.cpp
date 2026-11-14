#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveLeaf.h"


namespace TreeEngine
{

  void CPLeaf::SetTexture(RString name, int index) {
    _textureIndex = index;
  }

  void CPLeaf::SetLeafParams(float width, float height) {
    _width = width;
    _height = height;
  }

  int CPLeaf::Draw(CPrimitiveStream &ps,
                  int sharedVertexIndex,
                  int sharedVertexCount,
                  bool isRoot,
                  float detail,
                  SSlot *pSlot) {

    int tAccu = 0;

    DrawRhomb(
      ps,
      _textureIndex,
      D3DCOLOR_XRGB(255, 255, 255),
      _origin,
      _width,
      _height,
      Array<const UVTransform>(_uvTransform,2),true,
      &tAccu);
    
    return tAccu;
  }

  int CPSlice::Draw(CPrimitiveStream &ps,
    int sharedVertexIndex,
    int sharedVertexCount,
    float detail,
    SSlot *pSlot) 
  {

      int tAccu = 0;

      DrawRhomb(
        ps,
        _textureIndex,
        D3DCOLOR_XRGB(255, 255, 255),
        _origin,
        _width,
        _height,
        Array<const UVTransform>(_uvTransform,1),false,
        &tAccu);

      return tAccu;
  }


  void CPLeaf::UpdatePointList(CPointList &pointList) {
    pointList.AddPoint(_origin.Position());
    pointList.AddPoint(_origin.Position() + _origin.Direction() * _height);
    pointList.AddPoint(_origin.Position() + _origin.Direction() * _height * 0.5f + _origin.DirectionAside() * _width);
    pointList.AddPoint(_origin.Position() + _origin.Direction() * _height * 0.5f - _origin.DirectionAside() * _width);
  }

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveLeaf_SetTexture(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPLeaf *primitiveLeaf = dynamic_cast<CPLeaf*>(GetPrimitive(oper1));

    if (!primitiveLeaf) {
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

    primitiveLeaf->SetTexture(array[0], toInt(array[1]));

    return GameValue();
  }

  GameValue PrimitiveLeaf_SetLeafParams(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2) {

    CPLeaf *primitiveLeaf = dynamic_cast<CPLeaf*>(GetPrimitive(oper1));

    if (!primitiveLeaf) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2) {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
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

    primitiveLeaf->SetLeafParams(array[0], array[1]);

    return GameValue();
  }
  
  GameValue PrimitiveLeaf_SetLeafUVTransform_Helper(const GameState *state, GameValuePar oper1, GameValuePar oper2,void (CPLeaf::*fn)(const UVTransform &trns))
  {
    CPLeaf *primitiveLeaf = dynamic_cast<CPLeaf*>(GetPrimitive(oper1));

    if (!primitiveLeaf) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size()==4)
    {
      (primitiveLeaf->*fn)(UVTransform(array[0],array[1],array[2],array[3]));
    }
    else if (array.Size()==6)
    {
      (primitiveLeaf->*fn)(UVTransform(array[0],array[1],array[2],array[3],array[4],array[5]));
    }
    else
      if (state) state->SetError(EvalForeignError,"Needs four or six parameters");


    return GameValue();

  }

  GameValue PrimitiveLeaf_SetLeafUVTransform(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
    return PrimitiveLeaf_SetLeafUVTransform_Helper(state,oper1,oper2,&CPLeaf::SetLeafUVTransform);
  }

  GameValue PrimitiveLeaf_SetLeafUVTransformFront(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
    return PrimitiveLeaf_SetLeafUVTransform_Helper(state,oper1,oper2,&CPLeaf::SetLeafUVTransformFront);
  }

  GameValue PrimitiveLeaf_SetLeafUVTransformBack(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
    return PrimitiveLeaf_SetLeafUVTransform_Helper(state,oper1,oper2,&CPLeaf::SetLeafUVTransformBack);
  }


  void CPLeaf ::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setLeafTexture", function, PrimitiveLeaf_SetTexture,    GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setLeafParams",  function, PrimitiveLeaf_SetLeafParams, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setLeafUVTransform",  function, PrimitiveLeaf_SetLeafUVTransform, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setLeafUVTransformFront",  function, PrimitiveLeaf_SetLeafUVTransformFront, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setLeafUVTransformBack",  function, PrimitiveLeaf_SetLeafUVTransformBack, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
  };

  // ------------------------------------------

}