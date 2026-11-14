//-----------------------------------------------------------------------------
// File: enf_scriptapivars.h
//
// Desc: Script-engine API 
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTAPIVARS_H
#define ENF_SCRIPTAPIVARS_H

#include "enf_config.h"
#include "script/enf_scriptvariable.h"

namespace enf
{
/** @addtogroup EnforceScriptAPI
@{*/

extern Variable* g_pvFTDelta;
extern Variable* g_pvLMB;
extern Variable* g_pvRMB;
extern Variable* g_pvMMB;
extern Variable* g_pvMouseDeltaX;
extern Variable* g_pvMouseDeltaY;
extern Variable* g_pvMouseWheel;

extern Variable* g_pvJoyAxis;
extern Variable* g_pvJoyRAxis;
extern Variable* g_pvJoySlider;
extern Variable* g_pvJoyButtons;
extern Variable* g_pvJoyPOV;

extern Variable* g_pvWidth;
extern Variable* g_pvHeight;
extern Variable* g_pvRenderNormal;
extern Variable* g_pvRenderRTWidgets;
extern Variable* g_pvDepthComplex;
extern Variable* g_pvRenderParallax;
extern Variable* g_pvRenderShape;

extern Variable* g_pvViewFrame;
extern Variable* g_pvTraceShader;
extern Variable* g_pvTraceSurfparm;
extern Variable* g_pvTraceContent;
extern Variable* g_pvTraceBrush;
extern Variable* g_pvTraceSurfaceType;
extern Variable* g_pvTraceBone;

extern Variable* g_pvIsOutdoor;
extern Variable* g_pvShowFilter;

extern Variable* g_pvMaxPhysIterations;
extern Variable* g_pvPhysFixedTick;

//@}
}
#endif //ENF_SCRIPTAPIVARS_H