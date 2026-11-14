#include <El/elementpch.hpp>

#include "stringtable.hpp"
#include <El/Interfaces/iPreproc.hpp>

static PreprocessorFunctions GPreprocessorFunctions;
PreprocessorFunctions *GStringtablePreprocFunctions = &GPreprocessorFunctions;
