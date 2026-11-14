#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CAM_EFFECTS_HPP
#define _CAM_EFFECTS_HPP

#include "world.hpp"

// predefined static camera positions

DEFINE_ENUM_BEG(CamEffectPosition)
	CamEffectTop,
	CamEffectLeft,CamEffectRight,
	CamEffectFront,CamEffectBack,

	CamEffectLeftFront,CamEffectRightFront,
	CamEffectLeftBack,CamEffectRightBack,

	CamEffectLeftTop,CamEffectRightTop,
	CamEffectFrontTop,CamEffectBackTop,

	CamEffectBottom,
	
	NCamEffectPositions
DEFINE_ENUM_END(CamEffectPosition)

class CameraEffect;

ConstParamEntryPtr FindCameraEffect(RString name);

CameraEffect *CreateCameraEffect
(
	Object *obj, const char *name, CamEffectPosition pos
);

CameraEffect *CreateCameraEffect
(
	Object *obj, const char *name, CamEffectPosition pos, bool infinite
);


#endif
