//-----------------------------------------------------------------------------
// File: baseworld/enf_baseworldlight.h
//
// Desc: 
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_BASEWORLDLIGHT_H
#define ENF_BASEWORLDLIGHT_H

#include "enf_config.h"
#include "pipeline\enf_ilight.h"

namespace enf
{
class Entity;
struct _WNode;
class MeshItem;
struct LightInteraction;
class SerializeInputAdapter;
class SerializeOutputAdapter;

//-----------------------------------------------------------------------
/*!
Dynamic light. They are handled in different way than static ones. There
is not used interaction system as dynamic lights are supposed to move
or vary every frame so it has to be relinked anyway. There is fixed number
of dynamic lights inside scene and maximum 32 of them can be gathered
for every visible frame.
*/
struct PUBLIC_API DynamicLight:public ILight
{
	ENF_DECLARE_SMALLOBJECT(DynamicLight);

/*!
Pointer to owners entity from which we get position and orientation.
*/
	Entity*	m_pEntity;
/*!
Pointer to first splitting scene node.
*/
	const _WNode*	m_pHeadNode;

	DynamicLight(): m_pEntity(NULL), m_pHeadNode(NULL)
	{
	}
};

//-----------------------------------------------------------------------
/*!
Static light. It's linked into scene tree and there are created
interactions between the light and VObjectInstance (entity)
*/
struct PUBLIC_API StaticLight: public ILight
{
	ENF_DECLARE_SMALLOBJECT(StaticLight);

	//!Pointer to scene leaf.
	_WNode*				m_pLeaf;

	uint					m_iInteractionCount;
	
	//!List of interactions on this light
	LightInteraction*	m_pInteractions;

	//!Serializes the object into output adapter.
	void Serialize(SerializeOutputAdapter* adapter);
	//!Deserializes the object from input adapter.
	void Serialize(SerializeInputAdapter* adapter);

	//!Copy operator.
	void operator=(const StaticLight &light)
	{
		memcpy(this, &light, sizeof(StaticLight));
		m_iCount = 0;
		m_iNum = 0;
		m_pInteractions = NULL;
		m_iInteractionCount = 0;
	}

	//!Static light constructor.
	StaticLight():
		m_pLeaf(NULL),
		m_pInteractions(NULL),
		m_iInteractionCount(0)
	{
	}
	~StaticLight();
};



//-----------------------------------------------------------------------
/*!
World light
*/
//-----------------------------------------------------------------------
struct PUBLIC_API WorldLight : public StaticLight
{
	float m_fHDRBrightness;		///< modificator of light brightness, usually it comes from scene measurement
	float m_fHDRmul;				///< multiplicator of middle gray value (0.5), user may set this to wanted scene brightness

	ENF_DECLARE_SMALLOBJECT(WorldLight);
	WorldLight(): StaticLight(), m_fHDRBrightness(1.0f), m_fHDRmul(1.0f)
	{
	}

	//!Serializes the object into output adapter.
	void Serialize(SerializeOutputAdapter* adapter);
	//!Deserializes the object from input adapter.
	void Serialize(SerializeInputAdapter* adapter);

	float GetGlobalHDRMul() const
	{
		return m_fHDRmul;
	}

	float GetGlobalHDRBrightness() const
	{
		return m_fHDRBrightness;
	}

	FColor GetDiffuseHDR() const
	{
		return FColor(m_Diffuse.r*m_fHDRBrightness, m_Diffuse.g*m_fHDRBrightness, m_Diffuse.b*m_fHDRBrightness, m_Diffuse.a);
	}

	FColor GetAmbientHDR() const
	{
		return FColor(m_Ambient.r*m_fHDRBrightness, m_Ambient.g*m_fHDRBrightness, m_Ambient.b*m_fHDRBrightness, m_Ambient.a);
	}

	FColor GetSpecularHDR() const
	{
		return FColor(m_Specular.r*m_fHDRBrightness, m_Specular.g*m_fHDRBrightness, m_Specular.b*m_fHDRBrightness, m_Specular.a);
	}
};


//-----------------------------------------------------------------------
/*!
Interaction link between StaticLight and Entity. Whenever entity is moved
and attached VObjects needs lighting, these interactions are recreated.
VObjectInstance is responsible for that.
*/
struct LightInteraction
{
	ENF_DECLARE_SMALLOBJECT(LightInteraction);

	//!Pointer to next interaction on owner entity
	LightInteraction*		m_pNextOnEntity;
	LightInteraction**	m_pPrevOnEntity;
	LightInteraction*		m_pNextOnLight;
	LightInteraction**	m_pPrevOnLight;

//!Pointer to StaticLight we belong to.
	StaticLight*			m_pLight;

//!Pointer to Entity we belong to.
	Entity*					m_pEntity;

	~LightInteraction()
	{
		if(*m_pPrevOnLight)
			(*m_pPrevOnLight)->m_pNextOnLight = m_pNextOnLight;
		*m_pPrevOnLight = m_pNextOnLight;

		if(m_pNextOnLight)
			m_pNextOnLight->m_pPrevOnLight = m_pPrevOnLight;

		if(*m_pPrevOnEntity)
			(*m_pPrevOnEntity)->m_pNextOnEntity = m_pNextOnEntity;

		*m_pPrevOnEntity = m_pNextOnEntity;
		if(m_pNextOnEntity)
				m_pNextOnEntity->m_pPrevOnEntity = m_pPrevOnEntity;
	}

	LightInteraction(Entity* ent, StaticLight* light);
};

}

#endif //ENF_BASEWORLDLIGHT_H