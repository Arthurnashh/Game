#include <El/elementpch.hpp>

#include "paramFile.hpp"
#include <El/Interfaces/iLocalize.hpp>

static LocalizeStringFunctions GLocalizeStringFunctions;
LocalizeStringFunctions *ParamFile::_defaultLocalizeStringFunctions = &GLocalizeStringFunctions;
