#pragma once

#ifndef USE_SIMPLE3D_STUB
#   include "Simple3DOSG.h"
    typedef     Simple3DOSG    Simple3D;
#else
#   include "Simple3DStub.h"
    typedef     Simple3DStub   Simple3D;
#endif

#include "Simple3D/AnimationRoutines.h"
#include "Simple3D/VisualizationRoutines.h"
#include "Simple3D/InfoRetrievalRoutines.h"
#include "Simple3D/TerrainCallbacks.h"


