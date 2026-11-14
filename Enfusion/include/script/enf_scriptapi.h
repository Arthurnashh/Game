//-----------------------------------------------------------------------------
// File: enf_scriptapi.h
//
// Desc: Script-engine API 
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTAPI_H
#define ENF_SCRIPTAPI_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_mathlib.h"
#include "script/enf_scriptmain.h"
#include "script/enf_scriptcontext.h"
#include "BaseWorld/enf_entity.h"
#include "engine/enf_event.h"

namespace enf
{
/** @defgroup EnforceScriptAPI Script API
@{*/

#define	VARTYPE_LAYER		VARTYPE_USER0
#define	VARTYPE_VOBJECT	VARTYPE_USER1
#define	VARTYPE_HSOUND		VARTYPE_USER2

extern VarType  *g_vtVObjectType;
extern VarType  *g_vtVSoundType;
extern VarType  *g_vtSoundType;

class BaseWorld;
class EntityDef;
class PacketSerializeOutputAdapter;
class PacketSerializeInputAdapter;

//-----------------------------------------------------------------------
class PUBLIC_API EntityClass: public Class
{
public:
	ENF_DECLARE_CLASS_IMP(EntityClass, Entity, Class);
	
	virtual void InitEntityDef(const EntityDef* def) {}
	void Initialize(Instance* inst, const void *extra) const;
	void Delete(Instance* inst) const;
};

struct PhysicsMaterial;

//-----------------------------------------------------------------------
// API classes
//-----------------------------------------------------------------------
class PUBLIC_API Contact: public Instance
{
public:
	PhysicsMaterial*	m_pMaterial1;
	PhysicsMaterial*	m_pMaterial2;
	uint			m_iMaterial1;
	uint			m_iMaterial2;
	float			m_fImpulse;
	float			m_fRelativeNormalVelocityAfter;
	float			m_fRelativeNormalVelocityBefore;

	void*			m_pNormal;
	void*			m_pPosition;
	void*			m_pRelativeVelocityBefore;
	void*			m_pRelativeVelocityAfter;
	Vector3		m_vNormal;
	Vector3		m_vPosition;
	Vector3		m_vRelativeVelocityBefore;
	Vector3		m_vRelativeVelocityAfter;

	Vector3 NMETHOD GetNormalImpulse() const;
	float NMETHOD GetRelativeVelocityBefore(Vector3& vel) const
	{
		vel = m_vRelativeVelocityBefore;
		return m_fRelativeNormalVelocityBefore;
	}
	float NMETHOD GetRelativeVelocityAfter(Vector3& vel) const
	{
		vel = m_vRelativeVelocityAfter;
		return m_fRelativeNormalVelocityAfter;
	}

	Vector3 NMETHOD GetRelativeTangentialVelocityBefore() const;
	Vector3 NMETHOD GetRelativeTangentialVelocityAfter() const;

	Contact():
		m_pMaterial1(NULL),
		m_pMaterial2(NULL),
		m_iMaterial1(0),
		m_iMaterial2(0),
		m_pNormal(NULL),
		m_pPosition(NULL),
		m_pRelativeVelocityBefore(NULL),
		m_pRelativeVelocityAfter(NULL)
	{
		m_vNormal.Zero();
		m_vPosition.Zero();
		m_vRelativeVelocityBefore.Zero();
		m_vRelativeVelocityAfter.Zero();
	}
};

//-----------------------------------------------------------------------
//script wrapper to PacketSerializeOutputAdapter class
//-----------------------------------------------------------------------
class PUBLIC_API PacketOutputAdapter: public Instance
{
public:
	void NMETHOD WriteBool(int value);
	void NMETHOD WriteInt(int value);
	void NMETHOD WriteFloat(float value);
	void NMETHOD WriteString(const char* value);
	void NMETHOD WriteVector(const Vector3& value);

	void SetAdapter(PacketSerializeOutputAdapter* adapter)
	{
		m_pAdapter = adapter;
	}

	PacketOutputAdapter():
		m_pAdapter(NULL)
	{
	}
private:
	PacketSerializeOutputAdapter* m_pAdapter;
};

//-----------------------------------------------------------------------
//script wrapper to PacketSerializeInputAdapter class
//-----------------------------------------------------------------------
class PUBLIC_API PacketInputAdapter: public Instance
{
public:
	int NMETHOD ReadBool() const;
	int NMETHOD ReadInt() const;
	float NMETHOD ReadFloat() const;
	SCAPI(ReadString);
	Vector3 NMETHOD ReadVector() const;

	void SetAdapter(PacketSerializeInputAdapter* adapter)
	{
		m_pAdapter = adapter;
	}

	PacketInputAdapter():
		m_pAdapter(NULL)
	{
	}
private:
	PacketSerializeInputAdapter* m_pAdapter;
};

class CBoneMask: public Instance
{
public:
	void*	Pointer;
	uint	Mask[3];
};

//registruje funkci z API namespace (hack proti duplicitnim nazvum)
//#define RegFunctAPI(b) LinkFunction(#b, (SCAPI_FUNC)API:: b)

void Variable2Text(const Variable *var, char *tmp, bool type, bool name);


//-----------------------------------------------------------------------------
// Engine script context
//-----------------------------------------------------------------------------
class DefaultEntityEventHandler:public EventHandler
{
	int HandleEvent(const Event& event);
};

class EnforceVMErrorHandler: public VMErrorHandler
{
public:
	ErrorAction HandleError(const char* description);
};

class PUBLIC_API EnforceScriptContext: public ScriptContext
{
protected:

public:
	Class*	m_pEntityClass;
	EnforceVMErrorHandler	m_VMErrorHandler;

	bool InitInstance(Instance *inst, const void *extra);
	void StoreThread(Instance *ins, ScriptThread *rt);
	static const char RTE_DeletedEntityPointerError[];

	void*	AllocMem(int size, const void *ptr, const char* file, int line);
	void FreeMem(void *ptr);
//API user class instances
	Contact	m_Contact;
	PacketOutputAdapter m_PacketOutputAdapter;
	PacketInputAdapter m_PacketInputAdapter;
//

	DefaultEntityEventHandler	m_EntityEventHandler;

	EnforceScriptContext(StaticBuffer* stat);

	~EnforceScriptContext()
	{
	}

	bool LoadScript(const char *name, bool checklinking = true);
	bool CompileModule(const char *name, ScriptModule* module, bool checkLinking);

	void Frame(BaseWorld *map);

	void InitAPI();
	void AfterCompile();
	void InitPhysicsAPI();
};

extern PUBLIC_API EnforceScriptContext* g_pScript;

ENF_INLINEFUNC EnforceScriptContext* GetScriptContext()
{
	return g_pScript;
}

//-----------------------------------------------------------------------
// utility funkce pro validaci pointeru na entitu v API funkcich
//-----------------------------------------------------------------------
Entity *Parm2Entity(IVariable* var);
Entity *AssertEntity(Instance *en);
//@}
}

#endif //ENF_SCRIPTAPI_H