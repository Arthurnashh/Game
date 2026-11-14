#include <El/elementpch.hpp>

#include "expressImpl.hpp"
#include <El/Stringtable/localizeStringtable.hpp>

static StringtableFunctions GStringtableFunctions;
LocalizeStringFunctions *GameState::_defaultLocalizeFunctions = &GStringtableFunctions;
