#include <El/elementpch.hpp>
#include "evaluatorExpress.hpp"
#include <Es/ErrorProp/errorProp.hpp>
#include <El/Evaluator/express.hpp>

void GameStateEvaluator::LoadVariables(SerializeBinStream &f)
{
	_vars._vars.Clear();
	int n;
	f.Transfer(n);
	for (int i=0; i<n; i++)
	{
		RString name;
		int value;
		f.Transfer(name);
		f.Transfer(value);
		GameVariable var(name, (float)value, true);
		_vars._vars.Add(var);
	}
}

void GameStateEvaluator::SaveVariables(SerializeBinStream &f)
{
	int n = _vars._vars.NItems();
	f.Transfer(n);
	if (n > 0)
		for (int i=0; i<_vars._vars.NTables(); i++)
		{
			AutoArray<GameVariable> &table = _vars._vars.GetTable(i);
			for (int j=0; j<table.Size(); j++)
			{
				RString name = table[j]._name;
				int value = toInt((float)table[j]._value);
				f.Transfer(name);
				f.Transfer(value);
			}
		}
}

void GameStateEvaluator::DeleteVariables()
{
	_vars._vars.Clear();
}


float GameStateEvaluator::EvaluateFloat(const char *expr)
{
	int gshandle = GGameState.BeginContext(&_vars);
	GameValue result = GGameState.Evaluate(expr, GameState::EvalContext::_default, _globals);
	if (result.GetNil())
	{
	  LogF("Warning: Cannot evaluate '%s'",expr);
	  ERROR_THROW(ErrorInfoText,"Cannot evaluate value");
	}
	GGameState.EndContext(gshandle);
	return result;
}

RString GameStateEvaluator::EvaluateStringInternal(const char *expr)
{
  GameValue value = GGameState.Evaluate(expr, GameState::EvalContext::_default, _globals);
  if (value.GetType() == GameString)
    return value.GetData()->GetString();
  else
    return value.GetText();
}

