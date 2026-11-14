#include "wpch.hpp"
#include "appInfoOFP.hpp"

static OFPApplicationInfoFunctions GOFPApplicationInfoFunctions INIT_PRIORITY_URGENT;
ApplicationInfoFunctions *CurrentAppInfoFunctions = &GOFPApplicationInfoFunctions;
