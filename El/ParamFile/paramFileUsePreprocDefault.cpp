#include <El/elementpch.hpp>

#include "paramFile.hpp"

static PreprocessorFunctions GPreprocessorFunctions;
PreprocessorFunctions *ParamFile::_defaultPreprocFunctions = &GPreprocessorFunctions;
