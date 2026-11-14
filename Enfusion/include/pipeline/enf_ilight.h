//-----------------------------------------------------------------------------
// File: enf_ilight.h
//
// Desc: Base ILight
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ILIGHT_H
#define ENF_ILIGHT_H

#include "enf_config.h"
#include "common/enf_mathlib.h"
#include "common/enf_color.h"

namespace enf
{
/** @addtogroup Pipeline
@{*/

class Texture;
struct MLeaf;

class SerializeOutputAdapter;
class SerializeInputAdapter;

//-----------------------------------------------------------------------
enum LightType
{
	LT_NONE = 0,
	LT_AMBIENT,
	LT_POINT,
	LT_SPOT,
	LT_DIRECTIONAL
};

enum LightFlags
{
	//!Cheap hint for explosion, muzzles etc.
	LFL_CHEAP			= BITF(0), 
	
	//!Casts shadows
	LFL_CASTSHADOW		= BITF(1),

	//!Light does lookup to cubic texture
	LFL_LOOKUP			= BITF(3),

	//!Notify player's flashlight on creatures by Entity::EOnFlashlight
	LFL_FLASHLIGHT		= BITF(4),
	
	/*!
	Special type of light from sky. It's used as
	directional+ambient light for ambient pass
	*/
	LFL_SKY				= BITF(5),

	//!Is it DynamicLight, instead of StaticLight
	LFL_DYNAMIC			= BITF(27)
};

struct PUBLIC_API LightParams
{
	LightType		m_Type;
	uint				m_iFlags;
	//!Light is visible only in cameras that have set bit here
	uint				m_iCameraMask;
	Vector3			m_Position;
	float				m_fConePhi;
	Vector3			m_SpotVec;
	float				m_fRange;
	FColor			m_Diffuse;
	FColor			m_Specular;
	FColor			m_Ambient;
	
	LightParams():
		m_Type(LT_POINT),
		m_iFlags(0),
		m_iCameraMask(0xffffffff),
		m_fRange(0.0f),
		m_fConePhi(0.0f),
		m_SpotVec(0.0f, 0.0f, -1.0f)
	{
		m_Position.Zero();
		m_Diffuse.Zero();
		m_Specular.Zero();
		m_Ambient.Zero();
	}
};

struct PUBLIC_API ILight: public LightParams
{
	//!Lookup cubic texture TODO: replace it by some generic light-shader
	Texture*			m_pLUPTexture;
	//!Transposed matrix for shader usage
	Matrix34			m_Matrix;
	
	/*!
	When matches to BaseWorld::GetLightCount(), this light
	has been	selected into visible scene.
	*/
	mutable uint	m_iCount;

	//!When m_iCount == BaseWorld::GetLightCount(), this is order number.
	mutable uint	m_iNum;
	uint				m_iLightingLevel;

	void Serialize(SerializeOutputAdapter* adapter);
	void Serialize(SerializeInputAdapter* adapter);

	ENF_INLINE bool IsVisibleInCamera(uint cam) const
	{
		return ((m_iCameraMask & (1 << cam)) != 0);
	}

	ENF_INLINE bool IsDynamic() const
	{
		return (m_iFlags & LFL_DYNAMIC) != 0;
	}

	ENF_INLINE uint SetFlags(uint flags)
	{
		uint newFlags = m_iFlags | flags;
		m_iFlags = newFlags;
		return newFlags;
	}

	ENF_INLINE uint ClearFlags(uint flags)
	{
		uint oldFlags = m_iFlags;
		uint res = oldFlags & flags;
		m_iFlags = oldFlags & ~flags;
		return res;
	}

	ENF_INLINE bool IsFlagSet(uint flag) const
	{
		return (m_iFlags & flag) != 0;
	}

	ILight(): LightParams(),
		m_Matrix(true),
		m_pLUPTexture(NULL),
		m_iCount(0),
		m_iNum(0),
		m_iLightingLevel(0)
	{
	}
};

//-----------------------------------------------------------------------
/*!
Environment cubic texture. Used for reflections on objects.
Objects are selecting the best one visible cube. See
VObjectInstance::m_pEnvMap
*/
struct MEnvMap
{
/*!
Origin of this cubic texture
*/
	Vector3				m_vOrigin;
/*!
Pointer to cubic texture
*/
	Texture*				m_pTexture;
	uint					m_iTextureID;

/*!
Pointer to scene-tree leaf. TODO: remove this dependency!
*/
	MLeaf*				m_pLeaf;

/*!
When matches to BaseWorld::m_iCubeCount, this cubemap has been
selected into visible scene.
*/
	uint					m_iCount;

/*!
When cubemap is visible, this is sort number.
*/
	uint					m_iNum;

	MEnvMap():
		m_pTexture(NULL),
		m_iTextureID(0),
		m_pLeaf(NULL),
		m_iCount(0),
		m_iNum(0)
	{
	}
};

//!maximum number of lights on one object
const int MAX_MODEL_LIGHTS	= 64;
typedef SArray<const ILight*, MAX_MODEL_LIGHTS>	TModelLights;

//@}
}
#endif //ENF_ILIGHT_H
