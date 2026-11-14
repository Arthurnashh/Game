#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DIAG_MODES_HPP
#define _DIAG_MODES_HPP

#if _ENABLE_CHEATS  || _VBS3

#include <El/Enum/enumNames.hpp>

#define DIAG_ENABLE_ENUM(type,prefix,XX) \
	XX(type, prefix, CostMap) \
	XX(type, prefix, LockMap) \
	XX(type, prefix, Combat) \
	XX(type, prefix, Force) \
	XX(type, prefix, Animation) \
	XX(type, prefix, Dammage) \
	XX(type, prefix, Collision) \
	XX(type, prefix, Transparent) \
	XX(type, prefix, Sound) \
	XX(type, prefix, HDR) \
	XX(type, prefix, Path) \
  XX(type, prefix, Network) \
  XX(type, prefix, ManCollision) \
	XX(type, prefix, Wind) \
  XX(type, prefix, Rumble) \
  XX(type, prefix, AutoAction) \
  XX(type, prefix, Model) \
  XX(type, prefix, MapScale) \
  XX(type, prefix, ID) \
  XX(type, prefix, Ambient) \
  XX(type, prefix, Updates) \
  XX(type, prefix, EAX) \
  XX(type, prefix, Roads) \
  XX(type, prefix, Light) \
	XX(type, prefix, DynamicLight) \
  XX(type, prefix, ContactJoints) \
  XX(type, prefix, PermanentJoints) \
  XX(type, prefix, Phys) \
  XX(type, prefix, Resource) \
  XX(type, prefix, FSM) \
  XX(type, prefix, Prune) \
  XX(type, prefix, LogAnimPaths) \
  XX(type, prefix, Streaming) \
  XX(type, prefix, MouseSensitivity) \
  XX(type, prefix, UIControls) \
  XX(type, prefix, Terrain) \
  XX(type, prefix, Temperature) \
  XX(type, prefix, TIHistogram) \
  XX(type, prefix, PathFind) \
  XX(type, prefix, MinObjectDistance) /*_VBS3_NPFLT*/\
  XX(type, prefix, OperCache) /*age of fields in the Landscape::_operCache*/\
  XX(type, prefix, ShowGCollision) /*_VBS3_CRATERS_COLLGEOMETRYWORKING*/\
  XX(type, prefix, ShowGRoadway) /*_VBS3_CRATERS_COLLGEOMETRYWORKING*/\
  XX(type, prefix, LateInit) /*_VBS3_CRATERS_LATEINIT*/ \
  XX(type, prefix, Convoy) /* Moving in convoy */ \
  XX(type, prefix, ProfileScripts) /* Expressions profiling on / off */ \
  XX(type, prefix, CombatMode) /* Combat mode of units */ \
  XX(type, prefix, PruneX) \
  XX(type, prefix, Scene) \
  XX(type, prefix, VisualStates) \
  XX(type, prefix, Joystick) \
  XX(type, prefix, SimulWeather) \
  XX(type, prefix, Zombie) \
  XX(type, prefix, Surface) \

//VBS3 added Terrain to draw terrain and landgrid

#ifndef DECL_ENUM_DIAG_ENABLE
#define DECL_ENUM_DIAG_ENABLE
DECL_ENUM(DiagEnable)
#endif
DECLARE_ENUM(DiagEnable,DE,DIAG_ENABLE_ENUM)

extern int DiagMode;
extern int DiagMode2;

#define DIAG_DRAW_MODE_ENUM(type,prefix,XX) \
	XX(type, prefix, Normal) \
	XX(type, prefix, Roadway) \
	XX(type, prefix, Geometry) \
	XX(type, prefix, ViewGeometry) \
	XX(type, prefix, FireGeometry) \
	XX(type, prefix, Paths)

#ifndef DECL_ENUM_DIAG_DRAW_MODE
#define DECL_ENUM_DIAG_DRAW_MODE
DECL_ENUM(DiagDrawMode)
#endif
DECLARE_ENUM(DiagDrawMode,DDM,DIAG_DRAW_MODE_ENUM);
extern DiagDrawMode DiagDrawModeState;

#pragma warning(disable:4293)
#define CHECK_DIAG(x) ( x<32 ? ( ::DiagMode&(1U<<(x)) ) : ( ::DiagMode2&(1U<<(x-32)) ) )

#else 

#define CHECK_DIAG(x) (false)

#endif

#endif

