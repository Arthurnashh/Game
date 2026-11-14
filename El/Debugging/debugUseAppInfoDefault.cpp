#include <El/elementpch.hpp>

#include "imexhnd.h"
#include <El/Interfaces/iAppInfo.hpp>

static ApplicationInfoFunctions GApplicationInfoFunctions INIT_PRIORITY_URGENT;
ApplicationInfoFunctions *CurrentAppInfoFunctions = &GApplicationInfoFunctions;
