#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveBunch.h"


namespace TreeEngine
{
    SPrimitiveChild CPBunch::GetChild(float sideShift,
                                    float slopeAngle,
                                    float twistAngle,
                                    float spinAngle) {

    Matrix4 RotSpin;
    RotSpin.SetRotationZ(spinAngle);
    Matrix4 RotAside;
    RotAside.SetRotationY(slopeAngle);
    Matrix4 Translation;
    Translation.SetTranslation(Vector3(sideShift, 0.0f, 0.0f));
    Matrix4 RotDirection;
    RotDirection.SetRotationZ(twistAngle);


    SPrimitiveChild result;
    result._origin = _origin * RotDirection * Translation * RotAside * RotSpin;
    result._slotIndex = -1.0f;
    result._textureVOffset = 0.0f;
    result._geometrySpin = 0.0f;

    return result;
  }

  int CPBunch::Draw(CPrimitiveStream &ps,
                    int sharedVertexIndex,
                    int sharedVertexCount,
                    bool isRoot,
                    float detail,
                    SSlot *pSlot) {
    return 0;
  }

  void CPBunch::UpdatePointList(CPointList &pointList) {
  }

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveBunch_GetChild(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPBunch *primitiveBunch = dynamic_cast<CPBunch*>(GetPrimitive(oper1));

    if (!primitiveBunch) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 4) {
		  if (state) state->SetError(EvalDim, array.Size(), 4);
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

    SPrimitiveChild child = primitiveBunch->GetChild(array[0], array[1], array[2], array[3]);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  void CPBunch::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameArray, "getBunchChild", function, PrimitiveBunch_GetChild, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
  };

  // ------------------------------------------

}