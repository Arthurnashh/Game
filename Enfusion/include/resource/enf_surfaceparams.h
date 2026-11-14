//-----------------------------------------------------------------------------
// File: resource\enf_surfaceparams.h
//
// Desc: surface parameters
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SURFACEPARAMS_H
#define ENF_SURFACEPARAMS_H

#include "enf_config.h"

namespace enf
{

//Surface parameters. Game related sometimes :-(
enum SurfFlags
{
	SURF_NOIMPACT		= 0x10,		///< don't make missile explosions
	SURF_NOMARKS		= 0x20,		///< don't leave missile marks
	SURF_NODRAW			= 0x80,		///< don't generate a drawsurface
	SURF_SKIP			= 0x200,		///< completely ignore, allowing non-closed brushes
	SURF_NOSOLID		= 0x40000,

	//footsteps
	SURF_DUST			= 0x80000,
};

class SurfaceParms
{
private:
	uint	Parms;

public:
	static const uint SURF_MATERIAL_MASK	= 0xff000000; //MASK for 255 possible materials
	static const uint SURF_MATERIAL_SHIFT	= 24;

	SurfaceParms(): Parms(0) {}
	SurfaceParms(uint parms): Parms(parms) {}
	ENF_INLINE const SurfaceParms& operator=(const SurfaceParms& parms)
	{
		Parms = parms.Parms;
		return *this;
	}
	ENF_INLINE const SurfaceParms& operator=(uint parms)
	{
		Parms = parms;
		return *this;
	}
	ENF_INLINE bool operator!=(const SurfaceParms& parms) const	{ return Parms != parms.Parms;}
	ENF_INLINE bool operator==(const SurfaceParms& parms) const	{ return Parms == parms.Parms;}

	ENF_INLINE uint GetParms() const { return Parms; }
	ENF_INLINE bool IsFlagsSet(uint flags) const
	{
		return (Parms & flags) != 0;
	}
	ENF_INLINE void SetFlags(uint flags)
	{
		Parms |= flags;
	}

	ENF_INLINE void SetMaterialIndex(uint material)
	{
		Parms = (Parms & ~SURF_MATERIAL_MASK) | ((material << SURF_MATERIAL_SHIFT) & SURF_MATERIAL_MASK);
	}

	ENF_INLINE uint GetMaterialIndex() const
	{
		return (Parms & SURF_MATERIAL_MASK) >> SURF_MATERIAL_SHIFT;
	}
};

}

#endif