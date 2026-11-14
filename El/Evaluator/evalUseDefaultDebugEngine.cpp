#include <El/Interfaces/iDebugEngine.hpp>

static DebugEngineFunctions GDebugEngineFunctions;
extern DebugEngineFunctions *CurrentDebugEngineFunctions = &GDebugEngineFunctions;
