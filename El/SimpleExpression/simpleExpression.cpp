#include <El/elementpch.hpp>

#include "simpleExpression.hpp"
#include <El/Evaluator/express.hpp>
#include <El/Common/randomGen.hpp>

//////////////////////////////////////////////////////////////////////////
//
// Evaluation
//

/// context of compiled expression evaluation
struct EvaluateExpressionCodeContext
{
  const float *varValues;
  int varCount;
  AutoArray<float, MemAllocLocal<float,32> > stack;

  EvaluateExpressionCodeContext(const float *values, int count)
  {
    varValues = values;
    varCount = count;
  }
};

bool ExpressionCode::Evaluate(float &result, const float *varValues, int varCount) const
{
  result = 0;
  if (varCount < _variablesCount) return false;
  EvaluateExpressionCodeContext ctx(varValues, varCount);
  for (int i=0; i<Size(); i++)
  {
    (Get(i).func)(Get(i).arg, ctx);
  }
  if (ctx.stack.Size() != 1) return false;
  result = ctx.stack[0];
  return _finite(result) ? true : false;
}

static void EvalConst(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  ctx.stack.Add(arg.value);
}

static void EvalVariable(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  ctx.stack.Add(ctx.varValues[arg.varIndex]);
}

static void EvalNegative(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 1;
  Assert(i >= 0);
  ctx.stack[i] = -ctx.stack[i];
}

static void EvalRandom(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 1;
  Assert(i >= 0);
  ctx.stack[i] = ctx.stack[i] * GRandGen.RandomValue();
}

static void EvalMul(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 2;
  Assert(i >= 0);
  ctx.stack[i] = ctx.stack[i] * ctx.stack[i + 1];
  ctx.stack.Resize(i + 1);
}

static void EvalDiv(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 2;
  Assert(i >= 0);
  ctx.stack[i] = ctx.stack[i] / ctx.stack[i + 1];
  ctx.stack.Resize(i + 1);
}

static void EvalAdd(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 2;
  Assert(i >= 0);
  ctx.stack[i] = ctx.stack[i] + ctx.stack[i + 1];
  ctx.stack.Resize(i + 1);
}

static void EvalSub(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 2;
  Assert(i >= 0);
  ctx.stack[i] = ctx.stack[i] - ctx.stack[i + 1];
  ctx.stack.Resize(i + 1);
}

static void EvalMin(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 2;
  Assert(i >= 0);
  ctx.stack[i] = floatMin(ctx.stack[i],ctx.stack[i + 1]);
  ctx.stack.Resize(i + 1);
}

static void EvalMax(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 2;
  Assert(i >= 0);
  ctx.stack[i] = floatMax(ctx.stack[i],ctx.stack[i + 1]);
  ctx.stack.Resize(i + 1);
}

static void EvalFactor(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 3;
  Assert(i >= 0);
  float c = ctx.stack[i];
  float cMin = ctx.stack[i+1];
  float cMax = ctx.stack[i+2];
  if (cMin<cMax)
  {
    ctx.stack[i] = InterpolativC(c,cMin,cMax,0,1);
  }
  else
  {
    ctx.stack[i] = 1-InterpolativC(c,cMax,cMin,0,1);
  }
  ctx.stack.Resize(i + 1);
}

static void EvalInterpolate(ExpressionCodeArg arg, EvaluateExpressionCodeContext &ctx)
{
  int i = ctx.stack.Size() - 5;
  Assert(i >= 0);
  float c = ctx.stack[i];
  float cMin = ctx.stack[i+1];
  float cMax = ctx.stack[i+2];
  float vMin = ctx.stack[i+3];
  float vMax = ctx.stack[i+4];
  if (cMin<cMax)
  {
    ctx.stack[i] = InterpolativC(c,cMin,cMax,vMin,vMax);
  }
  else
  {
    ctx.stack[i] = 1-InterpolativC(c,cMax,cMin,vMax,vMin);
  }
  ctx.stack.Resize(i + 1);
}

#define OPERATORS(YY, XX, Category) \
  YY("/", Div, soucin, XX, Category) \
  YY("*", Mul, soucin, XX, Category) \
  YY("+", Add, soucet, XX, Category) \
  YY("-", Sub, soucet, XX, Category) \
  YY("min", Min, soucet, XX, Category) \
  YY("max", Max, soucet, XX, Category) \

#define OPERATORS_ARRAY(YY, XX, Category) \
  YY("factor", Factor, function, 2, XX, Category) \
  YY("interpolate", Interpolate, function, 4, XX, Category) \

//////////////////////////////////////////////////////////////////////////
//
// Compilation
//

#define TYPES_SE(XX, Category) \
  XX("EXPRESSION", GameExpression, CreateGameDataExpression, "Expression", "Expression", "Compiled simple expression.", Category, "GameExpression", "Lcom/bistudio/JNIScripting/RVEngine$GameExpression;")

TYPES_SE(DECLARE_TYPE, "Simple expression")

template<> CachedJavaClass ConvertTypes<&GTypeGameExpression>::_cache("com/bistudio/JNIScripting/NativeObject");

#include <Es/Memory/normalNew.hpp>

class GameDataExpression : public GameData
{
  typedef GameData base;

  ExpressionCodeBase _code;

public:
  GameDataExpression() {}
  GameDataExpression(int varIndex);
  GameDataExpression(const ExpressionCodeBase &code) : _code(code) {}
  ~GameDataExpression(){}

  const GameType &GetType() const {return GameExpression;}
  GameStringType GetString() const {return RString("Simple expression code");}
  const ExpressionCodeBase &GetCode() const {return _code;}
  ExpressionCodeBase &GetCode() {return _code;}

  RString GetText() const {return RString("Simple expression code");}
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "expression";}
  GameData *Clone() const {return new GameDataExpression(*this);}

  jobject ToJObject(JNIEnv *env, GameValuePar from) const;

#ifndef ACCESS_ONLY
  LSError Serialize(ParamArchive &ar);
#endif

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(GameDataExpression)

GameDataExpression::GameDataExpression(int varIndex)
{
  _code.Realloc(1);
  _code.Resize(1);
  _code[0].func = &EvalVariable;
  _code[0].arg.varIndex = varIndex;
}

bool GameDataExpression::IsEqualTo(const GameData *data) const
{
  // now equal only to itself
  return data == this;
}

jobject GameDataExpression::ToJObject(JNIEnv *env, GameValuePar from) const
{
  return ConvertTypes<&GTypeGameExpression>::FromGameValue(env, from);
}

#ifndef ACCESS_ONLY
LSError GameDataExpression::Serialize(ParamArchive &ar)
{
  Fail("Not implemented");
  return LSError(0);
}
#endif

static SRef<GameState> ExpressionCompiler;

bool ExpressionCode::Compile(RString expression, const RString *varNames, int varCount)
{
  #if _PROFILE
  _source = expression;
  #endif
  GameDataNamespace globals(NULL, RString(), false); // no global variables
  GameVarSpace local(false);
  int gshandle = ExpressionCompiler->BeginContext(&local);
  for (int i=0; i<varCount; i++)
    ExpressionCompiler->VarSetLocal(varNames[i], GameValue(new GameDataExpression(i)), true, true);
  GameValue value = ExpressionCompiler->Evaluate(expression, GameState::EvalContext::_reportUndefined, &globals);
  ExpressionCompiler->EndContext(gshandle);
  _usedVariables.Clear();
  _variablesCount = 0;
  if (value.GetNil())
  {
    Clear();
    return false;
  }
  else if (value.GetType() == GameExpression)
  {
    GameDataExpression *e = static_cast<GameDataExpression *>(value.GetData());
    ExpressionCodeBase::operator =(e->GetCode());
    // scan for used variables
    int maxVar = -1;
    for (int i=0; i<Size(); i++)
    {
      if (Get(i).func == &EvalVariable)
      {
        int var = Get(i).arg.varIndex;
        _usedVariables.AddUnique(var);
        if (var > maxVar) maxVar = var;
      }
    }
    _variablesCount = maxVar + 1;
    return true;
  }
  else if (value.GetType() == GameScalar)
  {
    // constant expression
    Realloc(1);
    Resize(1);
    Set(0).func = &EvalConst;
    Set(0).arg.value = value;
    return true;
  }
  Clear();
  return false;
}

void ExpressionCode::Init(float value)
{
  Realloc(1);
  Resize(1);
  Set(0).func = &EvalConst;
  Set(0).arg.value = value;
}

//////////////////////////////////////////////////////////////////////////
//
// Compiler registration
//

GameData *CreateGameDataExpression(ParamArchive *ar) {return new GameDataExpression();}

TYPES_SE(DEFINE_TYPE, "Simple expression")

inline const ExpressionCodeBase &GetCode(GameValuePar oper)
{
  return static_cast<GameDataExpression *>(oper.GetData())->GetCode();
}

static GameValue ExpPositive(const GameState *state, GameValuePar oper1)
{
  // make identical copy of argument
  return oper1;
}

static GameValue ExpNegative(const GameState *state, GameValuePar oper1)
{
  const ExpressionCodeBase &exp = GetCode(oper1);
  GameDataExpression *data = new GameDataExpression();
  ExpressionCodeBase &result = data->GetCode();

  int n = exp.Size();
  result.Realloc(n + 1);
  result.Resize(n + 1);
  for (int i=0; i<n; i++) result[i] = exp[i];
  result[n].func = &EvalNegative;

  return GameValue(data);
}

static GameValue ValRandom(const GameState *state, GameValuePar oper1)
{
  GameDataExpression *data = new GameDataExpression();
  ExpressionCodeBase &result = data->GetCode();

  result.Realloc(2);
  result.Resize(2);
  result[0].func = &EvalConst;
  result[0].arg.value = oper1;
  result[1].func = &EvalRandom;

  return GameValue(data);
}

static GameValue ExpRandom(const GameState *state, GameValuePar oper1)
{
  const ExpressionCodeBase &exp = GetCode(oper1);
  GameDataExpression *data = new GameDataExpression();
  ExpressionCodeBase &result = data->GetCode();

  int n = exp.Size();
  result.Realloc(n + 1);
  result.Resize(n + 1);
  for (int i=0; i<n; i++) result[i] = exp[i];
  result[n].func = &EvalRandom;

  return GameValue(data);
}

static GameValue ExpFuncExp(const GameState *state, GameValuePar oper1, GameValuePar oper2, ExpressionFunc *func)
{
  const ExpressionCodeBase &exp1 = GetCode(oper1);
  const ExpressionCodeBase &exp2 = GetCode(oper2);
  GameDataExpression *data = new GameDataExpression();
  ExpressionCodeBase &result = data->GetCode();

  int n = exp1.Size();
  int m = exp2.Size();
  result.Realloc(n + m + 1);
  result.Resize(n + m + 1);
  for (int i=0; i<n; i++) result[i] = exp1[i];
  for (int i=0; i<m; i++) result[n + i] = exp2[i];
  result[n + m].func = func;

  return GameValue(data);
}

static GameValue ExpFuncVal(const GameState *state, GameValuePar oper1, GameValuePar oper2, ExpressionFunc *func)
{
  const ExpressionCodeBase &exp = GetCode(oper1);
  GameDataExpression *data = new GameDataExpression();
  ExpressionCodeBase &result = data->GetCode();

  int n = exp.Size();
  result.Realloc(n + 2);
  result.Resize(n + 2);
  for (int i=0; i<n; i++) result[i] = exp[i];
  result[n].func = &EvalConst;
  result[n].arg.value = oper2;
  result[n + 1].func = func;

  return GameValue(data);
}

static GameValue ExpFuncValArray(const GameState *state, GameValuePar oper1, GameValuePar oper2, ExpressionFunc *func, int count)
{
  // check arguments
  const GameArrayType &array = oper2;
  if (array.Size()!=count)
  {
    state->SetError(EvalDim,array.Size(),count);
    return GameValue();
  }
  for (int i=0; i<count; i++)
  {
    if (array[i].GetType()!=GameScalar)
  {
      state->TypeError(GameScalar,array[i].GetType());
    return GameValue();
  }
  }
  
  // compile expression
  const ExpressionCodeBase &exp = GetCode(oper1);
  GameDataExpression *data = new GameDataExpression();
  ExpressionCodeBase &result = data->GetCode();
  int n = exp.Size();
  result.Realloc(n + count+1);
  result.Resize(n + count+1);
  for (int i=0; i<n; i++) result[i] = exp[i];
  for (int i=0; i<count; i++)
  {
    result[n+i].func = &EvalConst;
    result[n+i].arg.value = array[i];
  }
  result[n + count].func = func;

  return GameValue(data);
}

static GameValue ExpFuncValArray2(const GameState *state, GameValuePar oper1, GameValuePar oper2, ExpressionFunc *func)
{
  return ExpFuncValArray(state,oper1,oper2,func,2);
}
static GameValue ExpFuncValArray4(const GameState *state, GameValuePar oper1, GameValuePar oper2, ExpressionFunc *func)
{
  return ExpFuncValArray(state,oper1,oper2,func,4);
}

static GameValue ValFuncExp(const GameState *state, GameValuePar oper1, GameValuePar oper2, ExpressionFunc *func)
{
  const ExpressionCodeBase &exp = GetCode(oper2);
  GameDataExpression *data = new GameDataExpression();
  ExpressionCodeBase &result = data->GetCode();

  int n = exp.Size();
  result.Realloc(n + 2);
  result.Resize(n + 2);
  result[0].func = &EvalConst;
  result[0].arg.value = oper1;
  for (int i=0; i<n; i++) result[i + 1] = exp[i];
  result[n + 1].func = func;

  return GameValue(data);
}

#define DEFINE_OP(name, func, priority, XX, Category) \
static GameValue Exp##func##Exp(const GameState *state, GameValuePar oper1, GameValuePar oper2) \
{ \
  return ExpFuncExp(state, oper1, oper2, &Eval##func); \
} \
static GameValue Exp##func##Val(const GameState *state, GameValuePar oper1, GameValuePar oper2) \
{ \
  return ExpFuncVal(state, oper1, oper2, &Eval##func); \
} \
static GameValue Val##func##Exp(const GameState *state, GameValuePar oper1, GameValuePar oper2) \
{ \
  return ValFuncExp(state, oper1, oper2, &Eval##func); \
}

#define DEFINE_OP_ARRAY(name, func, priority, argCount, XX, Category) \
static GameValue Exp##func##ValArray(const GameState *state, GameValuePar oper1, GameValuePar oper2) \
{ \
  return ExpFuncValArray##argCount(state, oper1, oper2, &Eval##func); \
}


OPERATORS(DEFINE_OP, dummy, dummy)
OPERATORS_ARRAY(DEFINE_OP_ARRAY, dummy, dummy)

#define FUNCTIONS_SE(XX, Category) \
  XX(GameExpression, "+", ExpPositive, GameExpression, "expr", "Extend expression", "+daylight", "", "", "", Category) \
  XX(GameExpression, "-", ExpNegative, GameExpression, "expr", "Extend expression", "-daylight", "", "", "", Category) \
  XX(GameExpression, "randomGen", ValRandom, GameScalar, "multiplier", "Create expression which will generate random number from interval &lt;0, multiplier)", "randomGen 5", "", "", "", Category) \
  XX(GameExpression, "randomGen", ExpRandom, GameExpression, "multiplier", "Create expression which will generate random number from interval &lt;0, multiplier)", "randomGen daylight", "", "", "", Category) \

FUNCTIONS_SE(JNI_FUNCTION, "Simple expression")

static const GameFunction SEUnary[]=
{
  FUNCTIONS_SE(REGISTER_FUNCTION, "Simple expression")
};

#define REGISTER_OP(name, func, priority, XX, Category) \
  XX(GameExpression, name, priority, Exp##func##Exp, GameExpression, GameExpression, "expr1", "expr2", "Extend expression", "", "", "", "", Category) \
  XX(GameExpression, name, priority, Exp##func##Val, GameExpression, GameScalar, "expr", "val", "Extend expression", "", "", "", "", Category) \
  XX(GameExpression, name, priority, Val##func##Exp, GameScalar, GameExpression, "val", "expr", "Extend expression", "", "", "", "", Category) \

#define REGISTER_OP_ARRAY(name, func, priority, argCount, XX, Category) \
  XX(GameExpression, name, priority, Exp##func##ValArray, GameExpression, GameArray, "expr", "val", "Extend expression", "", "", "", "", Category) \

#define OPERATORS_SE(XX, Category) \
  OPERATORS(REGISTER_OP, XX, Category)  

#define OPERATORS_SE_ARRAY(XX, Category) \
  OPERATORS_ARRAY(REGISTER_OP_ARRAY, XX, Category)  

OPERATORS_SE(JNI_OPERATOR, "Simple expression")
OPERATORS_SE_ARRAY(JNI_OPERATOR, "Simple expression")

static const GameOperator SEBinary[]=
{
  OPERATORS_SE(REGISTER_OPERATOR, "Simple expression")
  OPERATORS_SE_ARRAY(REGISTER_OPERATOR, "Simple expression")
};

#include <El/Modules/modules.hpp>

INIT_MODULE(ExpressionCode, 2)
{
  ExpressionCompiler = new GameState;
  GameState &state = *ExpressionCompiler; // helper to make macro works
  TYPES_SE(REGISTER_TYPE, "Simple expression")

  for (int i=0; i<sizeof(SEUnary)/sizeof(*SEUnary); i++)
  {
    ExpressionCompiler->NewFunction(SEUnary[i]);
  }
  for (int i=0; i<sizeof(SEBinary)/sizeof(*SEBinary); i++)
  {
    ExpressionCompiler->NewOperator(SEBinary[i]);
  }
/*
  for (int i=0; i<sizeof(SENular)/sizeof(*SENular); i++)
  {
    ExpressionCompiler->NewNularOp(SENular[i]);
  }
*/

/*
  // Testing code
  static RString varNames[] = {"Test12", "Test15"};
  static float varValues[] = {12, 15};
  RString expression = "(Test12 + 1) * (13 - Test15)";

  ExpressionCode code;
  bool okc = CompileExpressionCode(code, expression, varNames, lenof(varNames));
  float result;
  bool oke = EvaluateExpressionCode(result, code, varValues, lenof(varValues));
  LogF("Result: %g", result);
*/
}
