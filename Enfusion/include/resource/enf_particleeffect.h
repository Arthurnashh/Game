//-----------------------------------------------------------------------------
// File: vobject/enf_particleeffect.h
//
// Desc: Particle effect & emitors
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PARTICLEEFFECT_H
#define ENF_PARTICLEEFFECT_H

#include "enf_config.h"
#include "engine/enf_module.h"
#include "common/enf_mathlib.h"
#include "common/enf_color.h"
#include "resource/enf_vobject.h"

namespace enf
{
/** @defgroup ParticleEffect Particle effect
    @ingroup VObjects
@{*/

class ParticleSystem;
class ParticleEffect;
class ParticleEffectInstance;
class EmitorDef;
struct PEmitor;
class Entity;
class Material;
struct Particle;
class IVariable;
class Texture;
class IndexBuffer;
class EmitorTextureGraph;
typedef ushort TexGraphIndex;
class BaseWorld;


extern ParticleSystem* g_pParticleSystem;

PUBLIC_API ParticleSystem* CreateParticleSystem();


enum ParticleFlags
{
	PEF_RANDOM_FRAME		= BITF(0),
	PEF_LIFETIME_BY_ANIM	= BITF(1),
	PEF_ANIM_ONCE			= BITF(2),
	PEF_RANDOM_ROTATION	= BITF(3),
	PEF_REPEAT				= BITF(4),
	PEF_RANDOM_ANGLE		= BITF(5),
	PEF_COLLIDING			= BITF(6),
	PEF_SORT					= BITF(7),
	PEF_ANIMATE				= BITF(8),
	PEF_FLUID				= BITF(9),
	PEF_SPRING				= BITF(10),
	PEF_VEL_ANGLE			= BITF(11),
	PEF_WIND					= BITF(12),

	//some optimizations flags
	PEF_STRETCH				= BITF(13), 
	PEF_POINTSPRITE		= BITF(14), ///<Optimize: HW pointsprites
	PEF_NO_AIR_RESIST		= BITF(15), ///<Optimize: air resistance is not computed
	PEF_OFFSET				= BITF(16), ///<Optimize: Is emitor displaced from the origin?
	PEF_ANGLES				= BITF(17), ///<Optimize: Is emitor displaced from the origin?
	PEF_NO_ROTATION		= BITF(18), ///<NOT needed yet
	PEF_NO_ANGULAR_VEL	= BITF(19),
	PEF_NO_ANGULAR_VEL_GRAPH = BITF(20)

};

enum EmitorParm
{
	//! Vector3 R/W
	EP_CONEANGLE = 1,
	
	//! Vector3 R/W
	EP_EMITOFFSET,
	
	//! Float R/W
	EP_VELOCITY,
	
	//! Float R/W
	EP_VELOCITY_RND,
	
	//! Float R/W
	EP_AVELOCITY,
	
	//! Float R/W
	EP_SIZE,
	
	//! Float R/W
	EP_STRETCH,

	//! Bool R/W
	EP_RANDOM_ANGLE,
	
	//! Bool R/W
	EP_RANDOM_ROT,

	//! Float R/W
	EP_AIR_RESISTANCE,
	
	//! Float R/W
	EP_AIR_RESISTANCE_RND,

	//! Float R/W
	EP_GRAVITY_SCALE,

	//! Float R/W
	EP_GRAVITY_SCALE_RND,
	
	//! Float R/W
	EP_BIRTH_RATE,

	//! Float R/W
	EP_BIRTH_RATE_RND,

	//! Float R/W
	EP_LIFETIME,

	//! Float R/W
	EP_LIFETIME_RND,

	//! Bool R/W
	EP_LIFETIME_BY_ANIM,

	//! Bool R/W
	EP_ANIM_ONCE,

	//! Bool R/W
	EP_RAND_FRAME,

	//! Float R/W
	EP_EFFECT_TIME,

	//! Bool R/W
	EP_REPEAT,

	//! Float R/W
	EP_CURRENT_TIME,

	//! Int R
	EP_ACTIVE_PARTICLES,

	//! Bool R/W
	EP_SORT,

	//! Bool R/W
	EP_WIND,

	//! Float R/W
	EP_SPRING
};

//-----------------------------------------------------------------------------
enum SimState
{
	SS_IDLE,
	SS_SUBMITED,
	SS_DELETED,
};

class PUBLIC_API ParticleEffectInstance: public VObjectInstance
{
private:
	friend class ParticleSystem;
	friend struct PEmitor;
	friend class ParticleEffect;
	friend class ParticleEffectUpdateJob;

	PEmitor**		m_pEmitors;
	Vector4			m_vPrevPos;	//for fluent emiting
	Vector3			m_vVelocity;
	float				m_fVelocityAffection;
	uint				m_iLastResult;
	bool				m_bFirstUpdate;
	IL_INT32			m_eState;
	AAB				m_BoundingBox;
#ifdef _DEBUG
	int				m_iLocked;
#endif

	bool Init();
	void Unload();

	const PEmitor* GetEmitor(uint emitor) const;
	PEmitor* GetEmitor(uint emitor);

public:
	ENF_DECLARE_ITEM(ParticleEffectInstance, VObjectInstance);
	virtual void Destroy();

	ENF_INLINE ParticleEffect* GetVObject() const
	{
		return (ParticleEffect*)m_pObject;
	}
	ENF_INLINE void SetVelocity(const Vector3& vel)
	{
		enf_assert(!BadVec(vel));
		m_vVelocity = vel;
	}
	ENF_INLINE void SetVelocityAffection(float affect)
	{
		m_fVelocityAffection = affect;
	}

	/*!
	Called upon deserialization
	\param adapter Input adapter
	*/
	void Serialize(SerializeInputAdapter* adapter);

	/*!
	Called upon serialization
	\param adapter Output adapter
	*/
	void Serialize(SerializeOutputAdapter* adapter);

	AAB ComputeWorldBoundBox(const Matrix43& mat, uint flags) const;

/** \name Get parameters method
Getting parameters set in ParticleEditor
*/
//@{
	const Vector3& GetConeAngle(uint emitor) const;
	const Vector3& GetEmitOffset(uint emitor) const;
	float GetVelocity(uint emitor) const;
	float GetAngularVelocity(uint emitor) const;
	float GetSize(uint emitor) const;
	float GetStretch(uint emitor) const;
	bool GetRandomAngle(uint emitor) const;
	bool GetRandomRotation(uint emitor) const;
	float GetAirResistance(uint emitor) const;
	float GetAirResistanceRand(uint emitor) const;
	float GetGravityScale(uint emitor) const;
	float GetGravityScaleRand(uint emitor) const;
	bool GetKillOldest(uint emitor) const;
	float GetBirthRate(uint emitor) const;
	float GetBirthRateRand(uint emitor) const;
	float GetLifetime(uint emitor) const;
	float GetLifetimeRand(uint emitor) const;
	float GetEffectTime(uint emitor) const;
	bool GetRepeat(uint emitor) const;
	float GetElapsedTime(uint emitor) const;
	uint GetActiveParticles(uint emitor) const;
	bool GetSort(uint emitor) const;
	bool GetWind(uint emitor) const;
	float GetSpring(uint emitor) const;
	uint GetTotalActiveParticles() const;
	bool HasActiveParticles() const;
	bool IsLooped() const;
//@}

/** \name Set parameters method
Setting parameters set in ParticleEditor
Whenever you use -1 as emitor number, the value is set to all emitors at once
*/
//@{
	void SetBirthRate(uint emitor, float value);
	void SetBirthRateRND(uint emitor, float value);
	void SetSize(uint emitor, float value);
	void SetLifetime(uint emitor, float value);
	void SetVelocity(uint emitor, float value);
	void SetRepeat(uint emitor, bool repeat);
//@}

	/*!
	Sets parameter by value in IVariable
	\param emitor Index of emitor
	\param type Type of parameter
	\param value Pointer to IVariable with value
	\return True when successfully set
	*/
	bool SetParm(uint emitor, EmitorParm type, IVariable *value);

	/*!
	Gets parameter value to IVariable
	\param emitor Index of emitor
	\param type Type of parameter
	\param value Pointer to IVariable for the value
	\return True when successfully get
	*/
	bool GetParm(uint emitor, EmitorParm type, IVariable *value) const;

	/*!
	Resets last position of emitor, used for fluent emiting
	over movement. It takes translation from actual Entity matrix.
	*/
	void ResetPosition(Entity* ent);

	ParticleEffectInstance():
		m_pEmitors(NULL),
		m_bFirstUpdate(false)
	{
	}

	ParticleEffectInstance(VObject* obj, Entity *owner);
	~ParticleEffectInstance();
};

enum ParticleUpdateFlags
{
	PUF_FORCE_LOOP						= BITF(0),
	PUF_NONINTERPOLATE_MOVEMENT	= BITF(1)
};

//-----------------------------------------------------------------------------
class PUBLIC_API ParticleEffect: public VObject
{
private:
	friend class ParticleSystem;
	friend class ParticleEffectInstance;
	friend class ParticleEffectUpdateJob;
	friend class LoadParticleEffectResource;

	EmitorDef*		m_pEmitorDefs;
	uint				m_iNumEmitorDefs;
	uint				m_iAllFlags;
	float				m_fVelocityAffection;
	uint				m_iSortVersion;
	

protected:
	void Unload();
	void Init();

	LoadResource* CreateResourceLoader();

	/*!
	\param ent Animated entity
	\param vi ParticleEffectInstance
	\param time Time-slice
	\param flags See enf::ParticleUpdateFlags
	\param movement Not used
	*/
	int UpdateAnim(Entity* ent, VObjectInstance* vi, float time, uint flags, AnimMovement* movement);
	int InternalUpdate(BaseWorld* world, Entity* ent, ParticleEffectInstance* pa, float time, uint flags, const Matrix43& mat, const AAB& bbox, const Vector3& velocity);

	EmitorDef* GetEmitor(uint emitor);
	const EmitorDef* GetEmitor(uint emitor) const;

	~ParticleEffect() {}

public:
	ENF_DECLARE_ITEM(ParticleEffect, VObject);

	bool Load(CInputStream* instream);
	bool Render(const TVisibleEntity& vent, Pipeline* pipeline);

	ParticleEffect()
	{
		Init();
	}
	
	VObjectInstance*	AllocInstance(Entity* ent, CStr options = NULL);	
	virtual size_t GetSize() const;

	//!Returns number of emitor
	ENF_INLINE uint GetNumEmitors() const
	{
		return m_iNumEmitorDefs;
	}

	/*!
	Returns name of emitor
	\param emitor Index of emitor
	\return Name of emitor
	*/
	const char* EmitorName(uint emitor) const;

	/*!
	Returns emitor index by its name
	\param name Name of emitor
	\return Index of emitor or INDEX_NOT_FOUND when not found
	*/
	uint GetEmitorIndex(const char* name) const;

	/*!
	Gets all emitor names at once
	\param names Array of pointers which will hold the names
	\param maxnum Maximum size of supplied array
	\return The number of emiters
	*/
	uint GetEmitorNames(const char **names, uint maxnum) const;

	/** \name Get parameters method
	Getting parameters set in ParticleEditor
	*/
	//@{
	const Vector3& GetConeAngle(uint emitor) const;
	const Vector3& GetEmitOffset(uint emitor) const;
	float GetVelocity(uint emitor) const;
	float GetAngularVelocity(uint emitor) const;
	float GetSize(uint emitor) const;
	float GetStretch(uint emitor) const;
	bool GetRandomAngle(uint emitor) const;
	bool GetRandomRotation(uint emitor) const;
	float GetAirResistance(uint emitor) const;
	float GetAirResistanceRand(uint emitor) const;
	float GetGravityScale(uint emitor) const;
	float GetGravityScaleRand(uint emitor) const;
	bool GetKillOldest(uint emitor) const;
	float GetBirthRate(uint emitor) const;
	float GetBirthRateRand(uint emitor) const;
	float GetLifetime(uint emitor) const;
	float GetLifetimeRand(uint emitor) const;
	float GetEffectTime(uint emitor) const;
	bool GetRepeat(uint emitor) const;
	float GetElapsedTime(uint emitor) const;
	float GetSpring(uint emitor) const;
	bool GetParm(uint n, EmitorParm type, IVariable* value) const;
	//@}

};
//@}
}
#endif //ENF_PARTICLEEFFECT_H
