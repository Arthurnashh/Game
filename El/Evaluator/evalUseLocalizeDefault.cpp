#include <El/elementpch.hpp>

#include "express.hpp"
#include <El/Interfaces/iLocalize.hpp>

static LocalizeStringFunctions GLocalizeStringFunctions;
LocalizeStringFunctions *GameState::_defaultLocalizeFunctions = &GLocalizeStringFunctions;
