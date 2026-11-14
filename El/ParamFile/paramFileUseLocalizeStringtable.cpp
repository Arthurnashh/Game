#include <El/elementpch.hpp>

#include "paramFile.hpp"
#include <El/Stringtable/localizeStringtable.hpp>

static StringtableFunctions GStringtableFunctions;
LocalizeStringFunctions *ParamFile::_defaultLocalizeStringFunctions = &GStringtableFunctions;
