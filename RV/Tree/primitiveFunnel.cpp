#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveFunnel.h"


namespace TreeEngine
{
    void CPFunnel::SetTexture(RString name, int index)
    {
//        _textureName  = name;
        _textureIndex = index;
    }

    void CPFunnel::SetParams(float width, float angle, int sideCount)
    {
        _width = width;
        _angle = angle;
        _sideCount = sideCount;
    }

    int CPFunnel::Draw(
        CPrimitiveStream &ps,
        int sharedVertexIndex,
        int sharedVertexCount,
        bool isRoot,
        float detail,
        SSlot *pSlot)
    {

        int tAccu = 0;

        // Calculate the depth from the angle
        float depth = (_width * 0.5f) / tan(_angle * 0.5f);

        DrawFunnel(
            ps,
            _textureIndex,
            _origin,
            _width,
            depth,
            _sideCount,
            &tAccu,_twotextures);

        return tAccu;
    }

    void CPFunnel::UpdatePointList(CPointList &pointList)
    {

        // Calculate the depth from the angle
        float depth = (_width * 0.5f) / tan(_angle * 0.5f);

        pointList.AddPoint(_origin.Position());
        pointList.AddPoint(_origin.Position() + _origin.Direction() * depth + _origin.DirectionAside() * _width * SquareCoef);
        pointList.AddPoint(_origin.Position() + _origin.Direction() * depth - _origin.DirectionAside() * _width * SquareCoef);
        pointList.AddPoint(_origin.Position() + _origin.Direction() * depth + _origin.DirectionUp() * _width * SquareCoef);
        pointList.AddPoint(_origin.Position() + _origin.Direction() * depth - _origin.DirectionUp() * _width * SquareCoef);
    }

    // ------------------------------------------
    // Following code is here for script purposes

    GameValue PrimitiveFunnel_SetTexture(
        const GameState *state,
        GameValuePar oper1,
        GameValuePar oper2)
    {

        CPFunnel *primitiveFunnel = dynamic_cast<CPFunnel*>(GetPrimitive(oper1));

        if (!primitiveFunnel) {
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

        primitiveFunnel->SetTexture(array[0], toInt(array[1]));

        return GameValue();
    }

    GameValue PrimitiveFunnel_SetParams(
        const GameState *state,
        GameValuePar oper1,
        GameValuePar oper2)
    {

        CPFunnel *primitiveFunnel = dynamic_cast<CPFunnel*>(GetPrimitive(oper1));

        if (!primitiveFunnel) {
            if (state) state->SetError(EvalGen);
            return GameValue();
        }

        const GameArrayType &array = oper2;
        if (array.Size() != 3) {
            if (state) state->SetError(EvalDim, array.Size(), 3);
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

        primitiveFunnel->SetParams(array[0], array[1], toInt(array[2]));

        return GameValue();
    }

    GameValue PrimitiveFunnel_TwoTex(
        const GameState *state,
        GameValuePar oper1,
        GameValuePar oper2)
    {

        CPFunnel *primitiveFunnel = dynamic_cast<CPFunnel*>(GetPrimitive(oper1));

        if (!primitiveFunnel) {
            if (state) state->SetError(EvalGen);
            return GameValue();
        }

        primitiveFunnel->EnableTwoTextures(oper2);
        return GameValue();
    }


    void CPFunnel::RegisterCommands(GameState &gState)
    {
        gState.NewOperator(GameOperator(GameNothing, "setFunnelTexture", function, PrimitiveFunnel_SetTexture,  GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
        gState.NewOperator(GameOperator(GameNothing, "setFunnelParams",  function, PrimitiveFunnel_SetParams,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
        gState.NewOperator(GameOperator(GameNothing, "setFunnelTwoTextures",  function, PrimitiveFunnel_TwoTex,   GamePrimitive, GameBool TODO_OPERATOR_DOCUMENTATION));
    };

    // ------------------------------------------

}