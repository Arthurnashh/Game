#include <El/elementpch.hpp>

#include <El/Evaluator/scriptVM.hpp>

#if USE_PRECOMPILATION

static PreprocessorFunctions GPreprocessorFunctions;
PreprocessorFunctions *ScriptVM::_defaultPreprocFunctions = &GPreprocessorFunctions;

#endif