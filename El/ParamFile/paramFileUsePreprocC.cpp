#include <El/elementpch.hpp>

#include "paramFile.hpp"
#include <El/PreprocC/preprocC.hpp>

static CPreprocessorFunctions GCPreprocessorFunctions;
PreprocessorFunctions *ParamFile::_defaultPreprocFunctions = &GCPreprocessorFunctions;
