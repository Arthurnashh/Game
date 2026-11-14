#ifndef _SIMPLE_EXPRESSION_HPP
#define _SIMPLE_EXPRESSION_HPP

#include <Es/Containers/array.hpp>
#include <Es/Strings/rString.hpp>

struct EvaluateExpressionCodeContext; 
union ExpressionCodeArg
{
  float value;
  int varIndex;
};

typedef void ExpressionFunc(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx);

struct ExpressionCodeItem
{
  ExpressionFunc *func;
  ExpressionCodeArg arg;
};
TypeIsSimple(ExpressionCodeItem)

typedef AutoArray<ExpressionCodeItem> ExpressionCodeBase;

/// list of variables used in given expression
typedef FindArrayKey<int> ExpressionCodeVars;

/// representation of compiled simple expression
class ExpressionCode : public ExpressionCodeBase
{
protected:
  ExpressionCodeVars _usedVariables;
  int _variablesCount;
  #if _PROFILE
  RString _source;
  #endif

public:
  ExpressionCode() {_variablesCount = 0;}
  const ExpressionCodeVars &GetUsedVariables() const {return _usedVariables;}
  
  /// compile text expression
  bool Compile(RString expression, const RString *varNames, int varCount);
  /// evaluate compiled expression 
  bool Evaluate(float &result, const float *varValues, int varCount) const;
  /// initialize with the constant expression
  void Init(float value);
};
TypeIsMovable(ExpressionCode)

#endif
