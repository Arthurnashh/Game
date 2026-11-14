#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EVALUATOR_EXPRESS_HPP
#define _EVALUATOR_EXPRESS_HPP

#include <El/Interfaces/iEval.hpp>
#include "expressImpl.hpp"

//! evaluator using GameState
class GameStateEvaluator : public Evaluator
{
protected:
  Ref<GameDataNamespace> _globals;
	GameVarSpace _vars;

public:
	GameStateEvaluator(IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables, bool enableSerialization)
	: _vars(static_cast<GameVarSpace *>(parentVariables), enableSerialization), _globals(static_cast<GameDataNamespace *>(globalVariables))
	{
	}

	virtual int  BeginContext() {return GGameState.BeginContext(&_vars);}
	virtual void EndContext(int handle) {GGameState.EndContext(handle);}
	virtual void LoadVariables(SerializeBinStream &f);
	virtual void SaveVariables(SerializeBinStream &f);
	virtual void DeleteVariables();

	// External functions (used outside BeginContext ... EndContext)
	virtual float EvaluateFloat(const char *expr);

	// Internal functions (used inside BeginContext ... EndContext)
	virtual float EvaluateFloatInternal(const char *expr) {return GGameState.Evaluate(expr, GameState::EvalContext::_default, _globals);}
	virtual RString EvaluateStringInternal(const char *expr);
	virtual void ExecuteInternal(const char *expr) {GGameState.Execute(expr, GameState::EvalContext::_default, _globals);}
	virtual void VarSetFloatInternal(const char *name, float value, bool readOnly, bool forceLocal)
	{GGameState.VarSet(name, GameValue(value), readOnly, forceLocal, _globals);}
};

//! class of callback functions
class GameStateEvaluatorFunctions : public EvaluatorFunctions
{
public:
	virtual Evaluator *CreateEvaluator(IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables, bool enableSerialization) {return new GameStateEvaluator(parentVariables, globalVariables, enableSerialization);}
};

#endif
