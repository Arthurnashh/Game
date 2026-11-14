#include <El/elementpch.hpp>

#include "paramFile.hpp"
#include <El/Interfaces/iEval.hpp>

static EvaluatorFunctions GEvaluatorFunctions;
EvaluatorFunctions *ParamFile::_defaultEvalFunctions = &GEvaluatorFunctions;
