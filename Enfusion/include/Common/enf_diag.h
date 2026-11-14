//-----------------------------------------------------------------------------
// File: enf_diag.h
//
// Desc: 
//
// Copyright (c) 2000-2005 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_DIAG_H
#define ENF_DIAG_H

#include "enf_config.h"
#include "common/enf_inttypes.h"

namespace enf
{
	enum DefaultDiagModes
	{
		DM_STATS_SUBMENU,
		DM_STATS_FPS,
		DM_STATS_STATS,
		DM_STATS_COUNTER,
		DM_STATS_SLOWFRAMES,
		DM_STATS_SLOWSCRIPT,
		DM_STATS_SCRIPTPROFILER,
		DM_STATS_STRESSMEM,
		DM_STATS_FLUSHMEM,
		DM_DUMP_MEMORY,
		DM_MEMORY_SNAPSHOT,
		DM_MEMORY_COMPARE,
		DM_DUMP_RESOURCES,
		DM_DUMP_ACTIVE_BODIES,
		DM_DUMP_ACTIVE_SOUNDS,
		DM_DUMP_OBJECTS,
		DM_STATS_NET,
		DM_STATS_SLOWDOWN,

		DM_MEMORY_VALIDATION,
		#ifdef ENF_PS3
			DM_PS3_MEMORY_VALIDATION,
		#endif

		DM_REND_SUBMENU,

		DM_REND_OFFLINESHADERS,
		DM_REND_RELOADSHADERS,
		DM_REND_WIREFRAME,
		DM_REND_COMPLEX,
		DM_REND_SHOWSHADOWMAP,
		DM_REND_SHOWDEPTHMAP,
		DM_REND_SHOWTERRSHADOWMAP,
		DM_REND_SHOWREFL,
		DM_REND_MIPMAPBIAS,
		DM_REND_MAXMIPMAP,
		DM_REND_FORCE_LOD,
		DM_REND_NORENDER,
		DM_REND_DIRECT_DEPTH,
		DM_REND_PARTICLE_HISPEED,
		DM_REND_LOD_MUL,
		DM_REND_SHOWSHADOWFRUSTUM,

		#ifdef ENF_STEREO_VISION
			DM_REND_STEREO_INFO,
			DM_REND_STEREO_GUI_DEPTH,
		#endif

		DM_REND_LIGHT_SUBMENU,
		DM_REND_MRT,
		DM_REND_AMB,
		DM_REND_DIFF,
		DM_REND_SPEC,
		DM_REND_SPECBIAS,
		DM_REND_AMBBIAS,
		DM_REND_DIFFBIAS,
		DM_REND_LIGHTS,
		DM_REND_SHADOWS,
		DM_REND_REFLECTION,
		DM_REND_DECALS,

		DM_REND_CLEARRT,
		DM_REND_DEPTHPASS,
		DM_REND_CASCADESHADOW,
		DM_REND_CASCADESHADOW_CULL,
		DM_REND_MT,
		DM_REND_OCCLUDERS,
		DM_REND_OCCLUDERS_ENTS,
		DM_REND_OCCLUDERS_VOL,
		DM_REND_OCCLUDERS_ACTIVE,

		DM_REND_FILTER_SUBMENU,
		DM_REND_FILTER_MESH,
		DM_REND_FILTER_MMODEL,
		DM_REND_FILTER_SPRITE,
		DM_REND_FILTER_VOLLIGHT,
		DM_REND_FILTER_PARTICLE,
		DM_REND_FILTER_ENTITIES,

		DM_SKYOCEAN_MENU,
		DM_REND_SKY,
		DM_REND_SKY_FOG,
		DM_REND_SKY_PLANETS,
		DM_REND_SKY_STARS,
		DM_REND_SKY_STARS_DEBUG,
		DM_REND_SKY_FARLAYER,
		DM_REND_SKY_DYNLAYER,
		DM_REND_LENS,
		DM_REND_GODRAYS,
		DM_REND_OCEAN,
		DM_REND_OCEAN_FOAM,
		DM_REND_OCEAN_LHEIGHT,
		DM_REND_OCEAN_GRID,
		DM_REND_OCEAN_FFTRES,
		DM_REND_OCEAN_SHOWTEX,
		DM_REND_SHOWOCEANCUBE,

		DM_REND_POSTPROCESS,
		DM_REND_PP_SUBMENU,
		DM_REND_PP_SSAO,
		DM_REND_PP_SSAO_DEBUG,
		DM_REND_PP_DOF,
		DM_REND_PP_SMARTGAMMA,
		DM_REND_PP_SMARTGAMMA_MODE,
		DM_REND_PP_FILMGRAIN,
		DM_REND_PP_EDGEAA,
		DM_REND_PP_RAIN2,
		DM_REND_PP_APERTURE,
		DM_REND_PP_SPU,

		DM_TERR_TERRAIN,
		DM_TERR_MENU,
		DM_TERR_LAYERS,
		DM_TERR_LAYERS_MODE,
		DM_TERR_BBOX,
		DM_TERR_BBOXTILE,
		DM_TERR_STEXTURE,
		DM_TERR_GEOMDETAIL,
		DM_TERR_TEXDETAIL,
		DM_TERR_GRASS,
		DM_TERR_GRASSDETAIL,
		DM_TERR_GRASSFREEZE,
		DM_TERR_GRASSBBOX,
		DM_TERR_GRASSLOD,
		DM_TERR_RENDER_MODE,

		//to be removed...sometimes
		DM_TEST_GRASSMIP,
		
		DM_WORLD_SUBMENU,
		DM_SHOW_ACTIVE,
		DM_SHOW_AABB,
		DM_SHOW_BONES,
		DM_SHOW_BULLET,
		DM_FORCE_SLEEP,
		DM_SHOW_BODIES,
		DM_SHOW_OCCLUDERS,
		DM_SHOW_WIND,
		DM_SHOW_TREE,
		DM_DISABLE_WIND,
		DM_DISABLE_PARTSORT,
		DM_PHYSICS,
		DM_PROFILE_BULLET,
		DM_PHYSICS_RAGDOLLS,
		DM_GAMEUPDATE,
		DM_SHOW_CLIPS,
		DM_FORCE_UPDATE,
		DM_FORCE_VISIBLE,
		DM_SKY_DEBUG,
		DM_CAPTURE_CUBEMAP,
		DM_CAPTURE_CUBEMAP_SIZE,
		DM_WORLD_FOV,
		DM_WORLD_FARPLANE,

		DM_WATCHMAN_ENABLE,
		DM_WATCHMAN_BREAK,
		DM_FIDOKEY,
		DM_XBOX_HIGHPREC_ALPHA,


		//here goes user diag modes
		ENF_DIAG_USER,

	};

#ifdef ENF_ENABLE_DIAG
	class PUBLIC_API Diag
	{
	protected:
		friend class Engine;

		static const uint MAX_DIAG_VALUES = 512;

		struct Mode
		{
			const char*		Name;
			const char*		ParentName;
			const char*		ValueNames;
			const char*		KeyControl;
			uint				Color;
			uint				KeyCode;
			uint				KeyQualifier;
			uint				Parent;
			int				Min;
			int				Max;
			bool				Pressed;
			Mode():
				Name(NULL),
				ParentName(NULL), 
				ValueNames(NULL),
				KeyControl(NULL),
				KeyCode(0),
				KeyQualifier(0),
				Parent((unsigned)-1),
				Min(0),
				Max(0),
				Pressed(false)
			{
			}
		};

		static int	Values[MAX_DIAG_VALUES];
		static Mode	Modes[MAX_DIAG_VALUES];
		
		static void SetupValues(Diag::Mode& mode);

	public:
		static bool IsEnabled(uint index)
		{
			return Values[index] != 0;
		}
		static void SetValue(uint index, int val)
		{
			enf_assert(index < MAX_DIAG_VALUES);
			Values[index] = val;
		}
		static int GetValue(uint index)
		{
			enf_assert(index < MAX_DIAG_VALUES);
			return Values[index];
		}
		static void Register(uint index, const char* keycontrol, const char* name, const char* parent, const char* valuenames, uint color = 0xffffffff);
		static void Unregister(uint index);
	};

	#define ENF_DIAG_UNREGISTER(index) enf::Diag::Unregister(index)
	#define ENF_DIAG_REGISTER(index, keycontrol, name, parent, valuenames) enf::Diag::Register(index, keycontrol, name, parent, valuenames)
	#define ENF_DIAG_REGISTER_EX(index, keycontrol, name, parent, valuenames, color) enf::Diag::Register(index, keycontrol, name, parent, valuenames, color)
	#define ENF_DIAG_BOOL(a) enf::Diag::IsEnabled(a)
	#define ENF_DIAG_RBOOL(a) (!enf::Diag::IsEnabled(a))
	#define ENF_DIAG_VALUE(a) enf::Diag::GetValue(a)
	#define ENF_DIAG_SET(a, v) enf::Diag::SetValue(a, v)
#else
	#define ENF_DIAG_UNREGISTER(index) ((void)0)
	#define ENF_DIAG_REGISTER(index, keycontrol, name, parent, valuenames) ((void)0)
	#define ENF_DIAG_REGISTER_EX(index, keycontrol, name, parent, valuenames, color) ((void)0)
	#define ENF_DIAG_BOOL(a) (false)
	#define ENF_DIAG_RBOOL(a) (true)
	#define ENF_DIAG_VALUE(a) (0)
	#define ENF_DIAG_SET(a, v) (0)
#endif
	#define ENF_DIAG_MENU(index, name, parent) ENF_DIAG_REGISTER(index, NULL, name, parent, NULL)
	#define ENF_DIAG_REGISTER_BOOL(index, keycontrol, name, parent) ENF_DIAG_REGISTER(index, keycontrol, name, parent, "false,true")
	#define ENF_DIAG_REGISTER_RBOOL(index, keycontrol, name, parent) ENF_DIAG_REGISTER(index, keycontrol, name, parent, "true,false")
	#define ENF_DIAG_REGISTER_SWITCH(index, keycontrol, name, parent) ENF_DIAG_REGISTER(index, keycontrol, name, parent, "disabled,enabled")
	#define ENF_DIAG_REGISTER_RSWITCH(index, keycontrol, name, parent) ENF_DIAG_REGISTER(index, keycontrol, name, parent, "enabled,disabled")
}

#endif