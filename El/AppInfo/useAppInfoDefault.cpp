#include <El/elementpch.hpp>
#include <El/Interfaces/iAppInfo.hpp>

static ApplicationInfoFunctions GApplicationInfoFunctions INIT_PRIORITY_URGENT;
ApplicationInfoFunctions *CurrentAppInfoFunctions = &GApplicationInfoFunctions;
