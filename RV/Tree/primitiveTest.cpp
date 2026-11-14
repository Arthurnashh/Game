#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveTest.h"


namespace TreeEngine
{
    void CPTest::SetTexture(RString name, int index) {
    _textureIndex = index;
  }

  void CPTest::SetParams(float arrowWidth, float arrowHeight) {
    _arrowWidth = arrowWidth;
    _arrowHeight = arrowHeight;
  }

  int CPTest::Draw(CPrimitiveStream &ps,
                  int sharedVertexIndex,
                  int sharedVertexCount,
                  bool isRoot,
                  float detail,
                  SSlot *pSlot) {

    int tAccu = 0;
    
    UVTransform trns[2];

    //Matrix4 
    DrawRhomb(
      ps,
      _textureIndex,
      D3DCOLOR_XRGB(255, 255, 255),
      _origin,
      _arrowWidth,
      _arrowHeight,Array<const UVTransform>(trns,2),true,
      &tAccu);
    
    return tAccu;
  }

  void CPTest::UpdatePointList(CPointList &pointList) {
    pointList.AddPoint(_origin.Position());
  }

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveTest_SetTexture(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPTest *primitiveTest = dynamic_cast<CPTest*>(GetPrimitive(oper1));

    if (!primitiveTest) {
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

    primitiveTest->SetTexture(array[0], toInt(array[1]));

    return GameValue();
  }

  GameValue PrimitiveTest_SetParams(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPTest *primitiveTest = dynamic_cast<CPTest*>(GetPrimitive(oper1));

    if (!primitiveTest) {
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

    primitiveTest->SetParams(array[0], array[1]);

    return GameValue();
  }

  void CPTest::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setTestTexture", function, PrimitiveTest_SetTexture,    GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setTestParams",  function, PrimitiveTest_SetParams,     GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
  };

  // ------------------------------------------

}