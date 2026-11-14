#include <El/elementpch.hpp>

#include "paramFile.hpp"
#include <El/Evaluator/evaluatorExpress.hpp>

static GameStateEvaluatorFunctions GGameStateEvaluatorFunctions;
EvaluatorFunctions *ParamFile::_defaultEvalFunctions = &GGameStateEvaluatorFunctions;
