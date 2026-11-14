//-----------------------------------------------------------------------------
// File: enf_materialsystemenums.h
//
// Desc: External class for material system
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MATERIALSYSTEMENUMS_H
#define ENF_MATERIALSYSTEMENUMS_H

#include "enf_config.h"

namespace enf
{
/** @addtogroup MaterialSystem
@{*/

//-----------------------------------------------------------------------
enum RenderType
{
	//!No rendering
	RT_NONE = 0,
	//!Rendering as normal
	RT_NORMAL,
	//!Rendering as image from external camera
	RT_CAMERA,
	//!Rendering mirrored scene
	RT_INMIRROR,
	//!Rendering environment cubemap
	RT_CAPTURE_CUBEMAP,
	//!Temporally -> rendering environment cubemap when user press shift f7 (or from dev menu)
	RT_USER_CAPTURE_CUBEMAP,
	//!Just marker. Under this value are render types taken as "normal"
	RT_NORMAL_TYPES,
	//!Just marker. Above this value are render types taken as "debug"
	RT_DEBUG					= 100,
	//!Rendering with fillmode=wireframe. Just for debug purpose
	RT_WIREFRAME,
};

/*!
Creation flags. Tells what features we demand
for MaterialSystem::GetMaterial
*/
enum MaterialCreationFlags
{
	//! Material know skinning
	MCF_SKINNING		= BITF(2),

	//!Material know to render terrain geometry
	MCF_TERRAIN			= BITF(3),

	//! Material know to render particles
	MCF_PARTICLE_SPRITE	= BITF(4),
	//! Material know to render particles
	MCF_PARTICLE_POINT	= BITF(5),
	//! Material know to render particles
	MCF_PARTICLE_STREAK	= BITF(6),
	
	MCF_PARTICLE			= MCF_PARTICLE_SPRITE|MCF_PARTICLE_POINT|MCF_PARTICLE_STREAK,

	//!Material know to render decal geometry
	MCF_DECAL				= BITF(7),

	//!Material know to handle 2.uv set
	MCF_SECOND_TEXCOORD	= BITF(8),

	//!Material know to render tree-animated geometry
	MCF_TREE					= BITF(10),

	MCF_MESHOBJECT			= BITF(11),
	MCF_2D					= BITF(12)
};

/*!
Various flags which tells how and for what
we are using the TextureMap
*/
enum TextureMapFlags
{
	/*!
	Tells to ResourceManager that he has to
	preserve alpha channel at any cost
	*/
	TMAP_ALPHA			= BITF(0),

	/*!
	Tells to ResourceManager that texture
	map is cube
	*/
	TMAP_CUBE			= BITF(5),

	/*!
	Tells to ResourceManager that texture
	do not need mipmaps
	*/
	TMAP_NOMIPMAP		= BITF(7),

	/*!
	Tells to ResourceManager that texture
	mipmaps needs to be renormalized if they
	are generated on the fly (DDS with mipmaps
	are not affected as mipmaps of normalmaps are
	supposed to be normalized.
	*/
	TMAP_NORMALIZE		= BITF(8),

	/*!
	Tells to material system, that texture has to be clamped
	and not wrapped in U-axis
	*/
	TMAP_CLAMPU			= BITF(9),

	/*!
	Tells to material system, that texture has to be clamped
	and not wrapped in V-axis
	*/
	TMAP_CLAMPV			= BITF(10),

	/*!
	Tells to material system, that texture has not to be
	filtered in any way.
	*/
	TMAP_NOFILTER		= BITF(11),
	
	//!Yiels to TEXTR_NORESIZE
	TMAP_NORESIZE		= BITF(12),

	/*!
	Texture map is reference to current renderview
	(just copy of actually rendered image)
	*/
	TMAP_RENDERVIEW	= BITF(20),

	/*!
	Texture map is reference to enf::RTTextureWidget
	attached to current enf::ModelContext -> enf::VModelInstance
	*/
	TMAP_GUI				= BITF(21),

	/*!
	Texture map is reference to nearest cube map, supplied
	by current enf::ModelContext -> enf::VModelInstance
	*/
	TMAP_NEARESTCUBE	= BITF(22),

	/*!
	Texture map is reference to current cube map, supplied
	by current enf::ModelContext -> enf::VModelInstance.
	Not implemented yet.
	*/
	TMAP_CURRENTCUBE	= BITF(23),

};

//-----------------------------------------------------------------------
//@}
}
#endif //ENF_MATERIALSYSTEMENUMS_H