//-----------------------------------------------------------------------------
// File: vobject/enf_particleeffectinternal.h
//
// Desc: Particle effect & emitors
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PARTICLEEFFECTINTERNAL_H
#define ENF_PARTICLEEFFECTINTERNAL_H

#include "enf_config.h"
#include "engine/enf_module.h"
#include "common/enf_mathlib.h"
#include "common/enf_color.h"
#include "resource/enf_particleeffect.h"
#include "resource/enf_resourcemanager.h"

namespace enf
{
/** @defgroup ParticleEffect Particle effect
	 @ingroup VObjects
@{*/

/*!
which graph to fill
*/
enum EMIT_GRAPH_TYPE
{
	UNUSED = 0,
	COLOR_R = 1,
	COLOR_G,
	COLOR_B,
	COLOR_A,
	MCOLOR_R,
	MCOLOR_G,
	MCOLOR_B,
	MCOLOR_A,
	SIZE,
	MSIZE,
	ANGLE,
	MANGLE,
	MBIRTHRATE,
	GT_SIZEOF
};


/*!
texture format to save params
*/
#ifdef ENF_PS3
	#define USE_FLOAT_PARAM_TEXTURE
#endif

#ifdef USE_FLOAT_PARAM_TEXTURE
	const uint PARAM_TEXTURE_WIDTH		= 64;

	#define ParamTextureItem				float
	#define PARAM_TEXTURE_FMT				TEXFMT_A32B32G32R32F
	#define PARAM_MULTIPLY					1
#else
	const uint PARAM_TEXTURE_WIDTH		= 256;

	#define ParamTextureItem				ubyte
	#define PARAM_TEXTURE_FMT				TEXFMT_A8R8G8B8 
	#define PARAM_MULTIPLY					255.0f
#endif


const uint PARAM_TEXTURE_SIZE		= 256;

const uint PART_GRAPH_SIZE			= 32;
const uint MAX_PARTICLE_EMITORS	= 32;				//how many emitors per particle effect
const float PART_INVGRAPH_SIZE	= (1.0f/(PART_GRAPH_SIZE - 1))*0.999f; ///< to convert from <0..PART_GRAPH_SIZE - 1> to linear time <0..0.999> for texture u coordinate sampling

typedef SArray<Pair<float, float>, PART_GRAPH_SIZE> TSrcGraph;

//we never have more than one buffer ahead
#ifdef ENF_CONSOLE
	const uint NUM_PARTBUFFERS = 2;
#else
	const uint NUM_PARTBUFFERS = 3;
#endif

//!streaks definitions 
const uint PARTICLE_MAX_STREAKS					= 8000;		///< maximum number of streaks rendered in one frame
#ifdef ENF_RENDERQUADS
	const uint PARTICLE_STREAK_INSTVERTS		= 12;			///< number of vertices per instances in IB
	const uint PARTICLE_STREAK_INSTPRIMITIVES = 3;			///< number of quads per instance
#else
	const uint PARTICLE_STREAK_INSTVERTS		= 18;			///< number of vertices per instances in IB
	const uint PARTICLE_STREAK_INSTPRIMITIVES = 6;			///< number of triangles per instance
#endif

#ifdef ENF_PS3 
	const uint PARTICLE_STREAK_VBVERTS			= 12;			///< number of vertices in VB, PS3 doesn't need index buffer
#else
	const uint PARTICLE_STREAK_VBVERTS			= 8;			///< number of vertices indexed by IB in VB
#endif


//!sprites definitions
const uint PARTICLE_MAX_SPRITES					= 10000;		///< maximum number of sprites rendered in one frame
#ifdef ENF_RENDERQUADS
	const uint PARTICLE_SPRITE_INSTVERTS		= 4;			///< number of vertices per instances in IB
	const uint PARTICLE_SPRITE_INSTPRIMITIVES = 1;			///< number of quads per instance
#else
	const uint PARTICLE_SPRITE_INSTVERTS		= 6;			///< number of vertices per instances in IB
	const uint PARTICLE_SPRITE_INSTPRIMITIVES = 2;			///< number of triangles per instance
#endif
const uint PARTICLE_SPRITE_VBVERTS				= 4;			///< number of vertices indexed by IB in VB


//!point sprites
const uint PARTICLE_POINT_VB_SIZE		= 8192;	///< how many particles in one vertexbuffer


//-----------------------------------------------------------------------------
template <class T, int S> struct PGraph
{
	T			m_Graph[S];
	void Fill(T from, T to);
	void Fill(const T* data, const float *time, uint count);
	ENF_INLINE T GetValue(uint pos, float frac) const;
};

/*!
index type for emitor texture graphs
*/
typedef ushort TexGraphIndex;
static const TexGraphIndex TEXGRAPH_BAD_INDEX = 0xFFFF;


//-----------------------------------------------------------------------------
//Keep it 16byte aligned!!!!
ENF_ALIGN16(class) EmitorDef
{
private:
	friend class ParticleEffect;
	
	PEmitor*			m_pDeletedEmitors;
	TexGraphIndex	m_iTexIndex;
	float				m_fGPUOffset;

public:
	ENF_DECLARE_MMOBJECTA16(EmitorDef);

	enum EmitorType
	{
		BOX = 1,
		POINT,
		BLOB,
		CONE,
		SPHERE,
		ELIPSE,
		ONSPHERE
	};

	EmitorType		m_eEmitorType;	
	
	Vector3			m_vEmitShape;
	float				m_fVelocity;

	Vector3			m_vEmitOffsets;
	float				m_fVelocityRND;
	
	Vector3			m_vEmitAngles;
	float				m_fGravityMult;

	Vector3			m_vEmitAngle;
	float				m_fGravityMultRND;

	float				m_fSize;
	float				m_fStretch;
	float				m_fLifetime;
	float				m_fLifetimeRND;
	
	uint				m_iMaxParticles;
	float				m_fBirthRate;
	float				m_fBirthRateRND;
	float				m_fBirthRateVel;
	
	float				m_fRestitution;
	float				m_fLifetimeShortening;
	uint				m_iFlags;
	float				m_fEmitorTime;
	
	float				m_fSpring;
	uint				m_iLOD;
	uint				m_iTilesCount;
	float				m_fAnimFPS;

	float				m_fAngleVelMult;
	float				m_fAirResistance;
	float				m_fAirResistanceRND;
	uint				m_iSortID;

	Material*		m_pMaterial;
	CStr				m_strName;

	typedef PGraph<float, PART_GRAPH_SIZE> FloatGraph;

//
//particle controls
	FloatGraph*		m_pAngleVelGraph;
//master controls
	FloatGraph*		m_pMAngleVelGraph;
	FloatGraph*		m_pMBirthRateGraph;


	float				m_fAngleVelConstant;
	float				m_fMAngleVelConstant;
	float				m_fMBirthRateConstant;
	uint				m_iPad0;

	void Optimize();
	void FillTexGraph(uint ioffs, const ParamTextureItem* data);

	/*!
	Create new emitor, or reuse cached one
	\param init
	When set to true, all parameters are initialized by defaults from EmitorDef.
	*/
	PEmitor* CreateEmitor(bool init);
	void DestroyEmitor(PEmitor *emitor);

	TexGraphIndex GetIndex() const
	{
		return m_iTexIndex;
	}

	//!index in texture graphs
	float GetGPUIndex() const
	{
		return m_fGPUOffset;
	}

	EmitorDef();
	~EmitorDef();
};

//---------------------------------------------------------------------------
class PUBLIC_API LoadParticleEffectResource: public LoadVObjectResource
{
protected:
	static const char* m_GraphNames[];

	struct Emitor
	{
	public:
		EmitorDef::EmitorType	m_eEmitorType;
		Vector3			m_vEmitShape;

		float				m_fVelocity;
		float				m_fVelocityRND;

		Vector3			m_vEmitOffsets;
		Vector3			m_vEmitAngles;

		Vector3			m_vEmitAngle;
		float				m_fGravityMult;
		float				m_fGravityMultRND;

		float				m_fSize;
		float				m_fStretch;
		float				m_fLifetime;
		float				m_fLifetimeRND;
		uint				m_iMaxParticles;
		float				m_fBirthRate;
		float				m_fBirthRateRND;
		float				m_fBirthRateVel;
		float				m_fRestitution;
		float				m_fLifetimeShortening;
		uint				m_iFlags;
		float				m_fEmitorTime;
		float				m_fSpring;
		uint				m_iLOD;

		uint				m_iTilesCount;
		float				m_fAnimFPS;

		float				m_fAngleVelMult;
		float				m_fAirResistance;
		float				m_fAirResistanceRND;

		Material*		m_pMaterial;
		SString<32>		m_strName;

		TSrcGraph*		m_Graphs[GT_SIZEOF];
		float				m_Constants[GT_SIZEOF];
	};

	Emitor*			m_pEmitorDefs;
	uint				m_iNumEmitorDefs;
	float				m_fVelocityAffection;
	size_t			m_iMemorySize;

public:
	ENF_DECLARE_ITEM(LoadParticleEffectResource, LoadVObjectResource);
	ENF_DECLARE_SMALLOBJECT(LoadParticleEffectResource);

	size_t GetDataSize() const
	{
		return m_iMemorySize;
	}

	ENF_INLINE ParticleEffect* GetObject() const
	{
		return static_cast<ParticleEffect*>(LoadResource::m_pResource);
	}

	LoadParticleEffectResource(ParticleEffect* obj);
	LoadParticleEffectResource():
		m_pEmitorDefs(NULL),
		m_iNumEmitorDefs(0),
		m_fVelocityAffection(0),
		m_iMemorySize(0)
	{}
	~LoadParticleEffectResource();

	bool Load(CInputStream* instream);
#ifdef ENF_DEVELOPER
	void Save(COutputStream* outstream, bool be);
#endif
	bool DoLoad();

	void Apply();

	CStr GetName() const
	{
		if(GetObject())
			GetObject()->GetName();
		return NULL;
	}
};

//-----------------------------------------------------------------------------
class PUBLIC_API ParticleSystem: public Module
{
friend class ParticleEffect;
friend class ParticleEffectInstance;
private:

	ComPtr<VertexBuffer>	m_pNewPointSpriteVB[NUM_PARTBUFFERS];
	ComPtr<VertexBuffer>	m_pNewSpriteVB[NUM_PARTBUFFERS];
	ComPtr<VertexBuffer>	m_pNewStreakVB[NUM_PARTBUFFERS];
	
	uint				m_iVB;
	uint				m_iSkipNextFrame;

	static int m_iLODDistances[4];
	static ENF_ALIGN16(Vector4)	m_vRandomVector[];
	static uint m_iRandomVector;

	~ParticleSystem();
	void OnInsert(TiXmlElement* xmlnode);

	EmitorTextureGraph* m_pTextureGraphs;
	BaseWorld* m_pInSimulation;
	uint m_iAllocIndex;

public:
	ENF_DECLARE_ITEM(ParticleSystem, Module);

	void SetCurrent()
	{
		g_pParticleSystem = this;
	}

	static const Vector4& RandVector()
	{
		return m_vRandomVector[m_iRandomVector++ & 255];
	}

	struct EmitorItem
	{
		uint		m_iSortValue;
		uint		m_iEmitor;
		Entity*	m_pEntity;

		EmitorItem() {}
		EmitorItem(uint sortvalue, uint emitor, Entity* ent):
			m_iSortValue(sortvalue),
			m_iEmitor(emitor),
			m_pEntity(ent)
		{
		}

		uint Distance() const
		{
			return m_iSortValue >> 19;
		}

		uint EmitorIndex() const
		{
			return (m_iSortValue >> 15) & 0xF;
		}

		uint UID() const
		{
			return m_iSortValue & 0x7FFF;
		}

		//for now, take just distance
		bool operator<(const EmitorItem& other) const { return m_iSortValue < other.m_iSortValue;/*Sort() < other.Sort()*/; }
		bool operator>(const EmitorItem& other) const { return m_iSortValue > other.m_iSortValue;/*Sort() > other.Sort()*/; }
		bool operator<=(const EmitorItem& other) const { return m_iSortValue <= other.m_iSortValue;/*Sort() <= other.Sort()*/; }
		bool operator>=(const EmitorItem& other) const { return m_iSortValue >= other.m_iSortValue;/*Sort() >= other.Sort()*/; }
	};

	struct BillboardSort;
	Array<EmitorItem>m_pEffectsToRender;
	Array< Pair<ParticleEffectInstance*, Entity*> > m_EffectsToSimulate;

	Texture* GetParamTexture(TexGraphIndex index);

	void BeginRender(Pipeline* pipeline);
	void OnBeginFrame();

	void OnSkipNextPresent()
	{
		m_iSkipNextFrame++;
	}

	void AddEffectToRender(Pipeline* pipeline, Entity* pe, int distance);

	void RenderBatch(Pipeline* pipeline, uint from, uint to, uint type, uint sort, uint verts, uint prims, Material* lmaterial, const BillboardSort& billsort);
	void RenderParticles(Pipeline* pipeline);

	void* XRenderPS(void* vptr, const ParticleEffectInstance* pi, uint emitor, const BillboardSort& billsort);
	void* XRenderSprites(void* vptr, const ParticleEffectInstance* pi, uint emitor, const BillboardSort& billsort);
	void* XRenderStreaks(void* vptr, const ParticleEffectInstance* pi, uint emitor, const BillboardSort& billsort);

	EmitorTextureGraph* GetTextureGraph()
	{
		return m_pTextureGraphs;
	}

	EmitorTextureGraph* GetTextureGraph() const 
	{
		return m_pTextureGraphs;
	}

	void BeginSimulation(BaseWorld* world);
	void EndSimulation(BaseWorld* world);

	ParticleSystem();
};


//@}
}
#endif //ENF_PARTICLEEFFECT_H
