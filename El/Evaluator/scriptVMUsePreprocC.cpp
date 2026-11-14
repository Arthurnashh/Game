#include <El/elementpch.hpp>
#include <El/Evaluator/scriptVM.hpp>

#if USE_PRECOMPILATION

#include <El/PreprocC/preprocC.hpp>

static CPreprocessorFunctions GCPreprocessorFunctions;
PreprocessorFunctions *ScriptVM::_defaultPreprocFunctions = &GCPreprocessorFunctions;

#endif
