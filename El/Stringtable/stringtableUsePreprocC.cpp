#include <El/elementpch.hpp>

#include "stringtable.hpp"
#include <El/PreprocC/preprocC.hpp>

static CPreprocessorFunctions GCPreprocessorFunctions;
PreprocessorFunctions *GStringtablePreprocFunctions = &GCPreprocessorFunctions;
