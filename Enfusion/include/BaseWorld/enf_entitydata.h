//-----------------------------------------------------------------------------
// File: baseworld/enf_entitydata.h
//
// Desc: Base world implementation
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ENTITYDATA_H
#define ENF_ENTITYDATA_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_hashmap.h"
#include "common/enf_set.h"
#include "common/enf_strings.h"
#include "common/enf_mathlib.h"
#include "engine/enf_module.h"
#include "script/enf_scriptvariable.h"

namespace enf
{
/** @addtogroup Entity
@{*/
class TiXmlElement;
class TiXmlNode;
class TiXmlAttribute;
class TiXmlDocument;
class EntityManager;
class EntityDef;
class EntityData;
class Entity;
class EntitySource;
class BaseWorld;
enum VType;
struct Key;

extern PUBLIC_API EntityManager* g_pEntityManager;

ENF_INLINE EntityManager* GetEntityManager()
{
	return g_pEntityManager;
}

class PUBLIC_API EntityDataFilter
{
public:
	virtual ~EntityDataFilter() {}
	virtual bool Filter(const EntityDef* def) const=0;
};


//-----------------------------------------------------------------------
struct ScriptedEntity
{
	DString	ClassName;
	const EntitySource* Source;
};

struct GameScriptContext
{
	Set<Pair<uint, uint> >	Entity2Scripted;
	Array<ScriptedEntity>	Scripted;
};

class GameScriptErrorHandler
{
public:
	virtual ~GameScriptErrorHandler() {}
	virtual void Report(const char* message, ...)
	{
	}
	virtual void ReportError(CStr entityName, CStr eventName, uint lineNumber, CStr error)
	{
	}
};

//-----------------------------------------------------------------------
/*!
Manager responsible for entity source data definition
(See EntityDef), and loading (See EntityData and EntitySource)
*/
class PUBLIC_API EntityManager: public Module
{
public:
	enum Mode
	{
		RUNTIME, EDITOR
	};

	ENF_DECLARE_ITEM(EntityManager, Module);

	void SetCurrent()
	{
		g_pEntityManager = this;
	}
	EntityManager();

	//!Returns number of entity definitions
	uint GetNumEntityDefs() const
	{
		return m_EntityDefs.GetCardinality();
	}

	/*!
	Returns definition of entity
	\param index
	Index of entity definition
	\return
	Pointer to definition
	*/
	const EntityDef* GetEntityDef(uint index) const
	{
		return m_EntityDefs[index];
	}

	/*!
	Returns definition of entity
	\param name
	Name of entity definition
	\return
	Pointer to definition or NULL when not found
	*/
	const EntityDef* GetEntityDef(CStr name) const;
	EntityDef* GetEntityDef(CStr name);

	/*!
	Loads definitions from file
	\param name
	Name of definition file
	\return
	True when success
	*/
	bool LoadEntityDefinitions(CStr name);

	/*!
	Loads entity source data and creates interface for accessing them
	\param name
	Name of file
	\param mode
	Mode (RUNTIME/EDITOR)
	\param filter
	Filter to load just selected entities
	\return
	Interface to entity source data
	*/
	virtual EntityData* LoadEntityData(CStr name, Mode mode = RUNTIME, const EntityDataFilter* filter = NULL);

	virtual EntityData* CreateEntityData(CStr name, Mode mode = RUNTIME);

	virtual Entity* CreateEntity(BaseWorld* world, const EntitySource* ent, CStr className = NULL);

	bool AddEntityTemplates(EntityData* edata)
	{
		m_Templates.Insert(edata);
		return true;
	}

	inline const Array<EntityData*>& GetEntityTemplates() const
	{
		return m_Templates;
	}

	const EntitySource* GetTemplate(CStr name) const;

	/*!
	Virtual implementation of entity loader
	*/
	virtual bool LoadEntities(BaseWorld* world, const EntityData& edata);
	bool LoadGameScript(ScriptModule* scriptModule, const EntityData& edata, GameScriptContext& context, GameScriptErrorHandler* errorHandler);

	void SetGameScriptHandler(GameScriptErrorHandler* handler)
	{
		m_pScriptHandler = handler;
	}
	GameScriptErrorHandler* GetGameScriptHandler() const
	{
		return m_pScriptHandler;
	}

protected:
	~EntityManager();
	HashMap<const char*, EntityDef*, StringHashFuncCaseSensitive>	m_EntityDefHash;
	Set<EntityDef*>	m_EntityDefs;
	Array<TiXmlDocument*> m_aDoc;
	Array<EntityData*>			m_Templates;
	GameScriptErrorHandler* m_pScriptHandler;
};

//-----------------------------------------------------------------------
/*!
Definition of entity. Contains registered keys (variables)
and XML nodes/attributes related to entity definition
*/
class PUBLIC_API EntityDef
{
public:
	ENF_DECLARE_SMALLOBJECT(EntityDef);

	struct Key
	{
		//!Name of the key
		CStr				Name;
		//!Default value, visible in tools.
		CStr				Default;
		//!Variable type of the key
		VType				Type;
		//!GUI type
		CStr				GUI;
		//!params
		CStr				Params;

		//!Custom definitions in XML objects
		TiXmlElement*	Element;

		bool operator<(const Key& key) const
		{
			return Name < key.Name;
		}
		bool operator==(const Key& key) const
		{
			return Name == key.Name;
		}
		bool operator>(const Key& key) const
		{
			return Name > key.Name;
		}
		Key(CStr keyname): Name(keyname), Type(VARTYPE_VOID), Element(NULL)
		{
		}
		Key(): Type(VARTYPE_VOID), Element(NULL)
		{
		}
	};

	struct MethodDef
	{
		CStr	Name;
		CStr	Params;
		CStr	Help;
		//!Custom definitions in XML objects
		TiXmlElement*	Element;
		MethodDef(): Element(NULL) {}
	};

	//!Returns class name (name of entity definition)
	CStr GetClass() const
	{
		return m_strName;
	}

	//!Returns number of keys
	uint GetNumKeys() const
	{
		return m_Keys.GetCardinality();
	}

	/*!
	Returns index of key
	\param name
	Name of key
	\return
	Index or INDEX_NOT_FOUND
	*/
	uint GetKeyIndex(CStr name) const;
	uint KeyIndex(CStr name) const;

	/*!
	Returns type of key
	\param index
	Index of key
	\return
	Type
	*/
	VType GetKeyType(uint index) const
	{
		return m_Keys[index].Type;
	}

	const char* GetKeyName(uint index) const
	{
		return m_Keys[index].Name.cstr();
	}

	/*!
	Returns complete definition of key
	\param index
	Index of key
	\return
	Definition of key
	*/
	const Key& GetKey(uint index) const
	{
		return m_Keys[index];
	}
	Key& GetKey(uint index)
	{
		return m_Keys[index];
	}

	/*!
	Inserts new key
	\param key new key
	\returns Index
	*/
	uint AddKey(const EntityDef::Key& key);

	/*!
	Returns complete definition of method
	\param index
	Index of key
	\return
	Definition of method
	*/
	const MethodDef& GetMethodDef(uint index) const
	{
		return m_Methods[index];
	}

	//!Returns number of method definitions
	uint GetNumMethodDefs() const
	{
		return m_Methods.GetCardinality();
	}

	/*!
	Inserts new method
	\param method new method
	\returns Index
	*/
	uint AddMethod(const EntityDef::MethodDef& method);

	/*!
	Returns pointer to XML node with all data related to entity def
	\return
	Pointer to XML node
	*/
	TiXmlElement* GetElement() const
	{
		return m_pElement;
	}

	/*!
	Returns pointer to entity definition we are inherited from
	\return
	Pointer to parent entity definition
	*/
	const EntityDef* GetParent() const
	{
		return m_pParent;
	}

	EntityDef(CStr classname, const EntityDef* parent, bool superdef);

	bool IsInheritedFrom(const char* classname) const;
	bool IsInheritedFrom(const EntityDef* entityDef) const;

	//!Tells whether this def is final entity definition or just superdef
	ENF_INLINE bool IsSuperDef() const
	{
		return m_bSuperDef;
	}

	ENF_INLINE void SetUserData(void* data) const
	{
		m_pUserData = data;
	}

	ENF_INLINE void* GetUserData() const
	{
		return m_pUserData;
	}

	ENF_INLINE int GetPriority() const
	{
		return m_iPriority;
	}
private:
	friend class EntityManager;

	mutable void*		m_pUserData;
	int					m_iPriority;
	TiXmlElement*		m_pElement;
	CStr					m_strName;
	Array<Key>					m_Keys;
	HashMap<CStr, uint, CStrHashFuncCS> m_KeysHash;
		//Set<Pair<CStr,uint> >	m_KeysSet;

	Array<MethodDef>	m_Methods;
	const EntityDef*	m_pParent;
	bool					m_bSuperDef;
};

//-----------------------------------------------------------------------
/*!
Source data for entity creation. The data are either
registered keys (stored in their native format), or
free format data stored as XML nodes/attributes
*/
class PUBLIC_API EntitySource
{
public:
	ENF_DECLARE_SMALLOBJECT(EntitySource);

	ENF_INLINE void* GetUserData() const
	{
		return m_pUserData;
	}
	ENF_INLINE void* SetUserData(void* data) const
	{
		void* prev = m_pUserData;
		m_pUserData = data;
		return prev;
	}

	//!Returns name of entity. Might be empty for unnamed entities
	virtual CStr GetName() const=0;

	/*!
	Because entities do not need name, we need to identify them otherwise
	\return
	Unique ID of entity
	*/
	ENF_INLINE uint GetID() const
	{
		return m_iID;
	}
	ENF_INLINE uint GetParentID() const
	{
		return m_iParentID;
	}
	ENF_INLINE uint GetSiblingID() const
	{
		return m_iSiblingID;
	}

	ENF_INLINE uint GetChildrenID() const
	{
		return m_iChildrenID;
	}

	/*!
	Tells whether EntitySource has certain key set
	\param key
	Name of key
	\return
	True when key is set
	*/
	bool IsKeySet(CStr key) const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
			return m_pTemplate->IsKeySet(key);

		return (index != INDEX_NOT_FOUND);
	}

	/*!
	Tells whether EntitySource has certain key set, but inherited from template
	\param key
	Name of key
	\return
	True when key is set in template
	*/
	bool IsKeyFromTemplate(CStr key) const
	{
		//it's our key
		if(KeyIndex(key) != INDEX_NOT_FOUND)
			return false;

		//is it from template?
		return (m_pTemplate != NULL && m_pTemplate->IsKeySet(key));
	}

	virtual void ClearKey(CStr key)=0;

	/*!
	Returns name of key.
	\param key Index of key (0>= && < EntitySource::GetNumKeys())
	\return Name of key
	*/
	virtual CStr GetKeyName(uint key) const=0;

	/*!
	Sets key to a value
	\param key
	Name of key
	\param value
	Value of key
	*/
	virtual void SetValue(CStr key, CStr value)=0;

	/*!
	Returns string value of key
	\param key Name of key
	\return Value of key
	*/
	virtual CStr GetString(CStr key) const=0;

	/*!
	Returns integer value of key
	\param key Name of key
	\return Value of key
	*/
	virtual int GetInt(CStr key) const=0;

	/*!
	Returns float value of key
	\param key Name of key
	\return Value of key
	*/
	virtual float GetFloat(CStr key) const=0;

	/*!
	Returns vector value of key
	\param key Name of key
	\return Value of key
	*/
	virtual const Vector3& GetVector(CStr key) const=0;

	/*!
	Returns value of key. If not found/set, the default value is used.
	\param key Name of key
	\param def Default value of key
	\return Value of key
	*/
	CStr GetStringDef(CStr key, CStr def = "") const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_STRING) ? def : m_pTemplate->GetString(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_STRING) ? def : GetString(index);
	}
	CStr GetStringDef(uint key, CStr def = "") const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_STRING) ? def : m_pTemplate->GetString(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_STRING) ? def : GetString(index);
	}
	//! @copydoc GetStringDef
	int GetIntDef(CStr key, int def = 0) const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_INT) ? def : m_pTemplate->GetInt(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_INT) ? def : GetInt(index);
	}
	int GetIntDef(uint key, int def = 0) const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_INT) ? def : m_pTemplate->GetInt(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_INT) ? def : GetInt(index);
	}
	//! @copydoc GetStringDef
	float GetFloatDef(CStr key, float def = 0.0f) const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_FLOAT) ? def : m_pTemplate->GetFloat(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_FLOAT) ? def : GetFloat(index);
	}
	float GetFloatDef(uint key, float def = 0.0f) const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_FLOAT) ? def : m_pTemplate->GetFloat(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_FLOAT) ? def : GetFloat(index);
	}
	//! @copydoc GetStringDef
	const Vector3& GetVectorDef(CStr key, const Vector3& def = Vector3().Zero()) const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_VECTOR) ? def : m_pTemplate->GetVector(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_VECTOR) ? def : GetVector(index);
	}
	const Vector3& GetVectorDef(uint key, const Vector3& def = Vector3().Zero()) const
	{
		uint index = KeyIndex(key);
		if(index == INDEX_NOT_FOUND && m_pTemplate)
		{
			index = m_pTemplate->KeyIndex(key);
			return (index == INDEX_NOT_FOUND || m_pTemplate->GetKeyType(index) != VARTYPE_VECTOR) ? def : m_pTemplate->GetVector(index);
		}
		return (index == INDEX_NOT_FOUND || GetKeyType(index) != VARTYPE_VECTOR) ? def : GetVector(index);
	}
	/*!
	Returns number of set keys
	*/
	virtual uint GetNumKeys() const=0;
	/*!
	Returns type of key. According to this type, you should
	use Get[type] method to obtain value
	\param index
	Index of key
	\return
	Type of key
	*/
	virtual VType GetKeyType(uint index) const=0;
	virtual const EntityDef::Key& GetKeyDef(uint index) const=0;

	/*!
	Returns integer value of key
	\param key Index of key
	\return Value of key
	*/
	virtual int GetInt(uint key) const=0;
	/*!
	Returns float value of key
	\param key Index of key
	\return Value of key
	*/
	virtual float GetFloat(uint key) const=0;
	/*!
	Returns string value of key
	\param key Index of key
	\return Value of key
	*/
	virtual CStr GetString(uint key) const=0;
	/*!
	Returns vector value of key
	\param key Index of key
	\return Value of key
	*/
	virtual const Vector3& GetVector(uint key) const=0;

	virtual TiXmlNode* FirstXMLNode(const char* name)=0;
	virtual TiXmlNode* AddXMLNode(const TiXmlNode& node)=0;
	virtual TiXmlAttribute* FirstXMLAttribute()=0;
	virtual void AddXMLAttribute(const TiXmlAttribute& attr)=0;

	virtual uint GetLayerID() const=0;
	virtual void SetLayerID(uint layer)=0;

	virtual ~EntitySource() {}
	EntitySource(const EntityDef* entitydef);
	CStr GetClass() const;
	ENF_INLINE const EntitySource* GetTemplate() const
	{
		return m_pTemplate;
	}

	ENF_INLINE void SetTemplate(const EntitySource* templ)
	{
		m_pTemplate = templ;
	}

	/*!
	Returns pointer to entity definition. See EntityDef
	*/
	const EntityDef* GetDefinition() const
	{
		return m_pDef;
	}

	virtual void Copy(const EntitySource* src)=0;

protected:
	friend class EntityData;

	const EntityDef*		m_pDef;
	const EntitySource*	m_pTemplate;
	mutable void*		m_pUserData;
	uint					m_iID;
	uint					m_iParentID;
	uint					m_iSiblingID;
	uint					m_iChildrenID;

	virtual void ClearKeys()=0;
	/*!
	Returns internal index. May be used for GetXXX(uint index) methods.
	It's valid until some update method is called!
	\param key
	Name of key
	\return
	Index or INDEX_NOT_FOUND when key is not set
	*/
	virtual uint KeyIndex(CStr key) const=0;
	virtual uint KeyIndex(uint key) const=0;

	virtual void ClearKey(uint index)=0;
};

//!Editor version of EntitySource. Used for read/write
class PUBLIC_API RWEntitySource: public EntitySource
{
public:
	ENF_DECLARE_SMALLOBJECT(RWEntitySource);

	virtual CStr GetName() const;
	virtual void ClearKey(CStr key);
	virtual void SetValue(CStr key, CStr value);
	virtual CStr GetString(CStr key) const;
	virtual int GetInt(CStr key) const;
	virtual float GetFloat(CStr key) const;
	virtual const Vector3& GetVector(CStr key) const;
	virtual TiXmlNode* AddXMLNode(const TiXmlNode& node);
	virtual TiXmlNode* FirstXMLNode(const char* name);
	virtual void AddXMLAttribute(const TiXmlAttribute& attr);
	virtual TiXmlAttribute* FirstXMLAttribute();
	virtual void ClearKeys();
	void SetKeys(uint num, const char** strings);
	virtual ~RWEntitySource();
	virtual void Copy(const EntitySource* src);
	virtual uint GetLayerID() const;
	virtual void SetLayerID(uint layer);

	RWEntitySource(const EntityDef* def, CStr name);
	RWEntitySource();

protected:
	typedef ubyte LayerID;

	friend class RWEntityData;
	virtual void ClearKey(uint index);
	virtual uint GetNumKeys() const;
	virtual VType GetKeyType(uint index) const;
	virtual const EntityDef::Key& GetKeyDef(uint index) const;
	virtual CStr GetString(uint index) const;
	const DString& GetDString(uint index) const;
	virtual int GetInt(uint index) const;
	virtual float GetFloat(uint index) const;
	virtual const Vector3& GetVector(uint index) const;
	virtual CStr GetKeyName(uint key) const;
	virtual uint KeyIndex(CStr key) const;
	virtual uint KeyIndex(uint key) const;

	DString			m_strName;
	Array<Key*>		m_Keys;
	TiXmlElement*	m_pXMLNode;
	LayerID			m_iLayerID;
};

//-----------------------------------------------------------------------
/*!
Abstract class representing source entity data.
For now it's just list of entitites, their
name, class and key pairs (keyname = value).
In future this will work on top of XML document
where everyone (runtime, game, editor) can set
their private data and not only the key/values
*/
class PUBLIC_API EntityData: public RefCountObject
{
public:
	virtual uint GetNumEntities() const=0;
	virtual EntitySource* FindEntity(CStr name)=0;
	virtual EntitySource* GetEntity(uint index)=0;
	virtual uint FindEntityByClass(uint from, CStr name) const=0;
	virtual uint FindEntityByClass(uint from, const EntityDef* entityClass) const=0;
	virtual EntitySource* FindEntityByID(uint id)=0;

	const EntitySource* FindEntity(CStr name) const
	{
		return ((EntityData*)this)->FindEntity(name);
	}
	const EntitySource* GetEntity(uint index) const
	{
		return ((EntityData*)this)->GetEntity(index);
	}
	const EntitySource* FindEntityByID(uint id) const
	{
		return ((EntityData*)this)->FindEntityByID(id);
	}
	
	//!Creates new unique ID
	virtual uint CreateID()=0;

	/*!
	Creates new Entitysource
	\param def
	Definition of Entity
	\param name
	Optional name
	\param id
	Optional ID. When 0, it's assigned automatically. When non-zero, it must be issued by CreateID
	\param templ
	Template we are inherited from
	\return
	New EntitySource. When already existing ID, it returns NULL!
	*/
	virtual EntitySource* CreateEntity(const EntityDef* def, CStr name, uint id = 0, const EntitySource* templ = NULL)=0;

	bool AddChild(EntitySource* parent, EntitySource* child);
	bool RemoveChild(EntitySource* parent, EntitySource* child);

	virtual bool RenameEntity(EntitySource* ent, CStr name)=0;
	virtual bool DestroyEntity(EntitySource* entity)=0;
	virtual bool CommitEntity(EntitySource* entity)=0;
	virtual void Clear()=0;
	
	/*!
	EntityData must be locked prior to any changes (CreatEntity/DestroyEntity/RenameEntity/CommitEntity).
	It must be unlocked then.
	\return
	When false, EntityData are locked
	*/
	virtual bool Lock()=0;

	/*!
	See Lock
	*/
	virtual void Unlock()=0;

	/*!
	See Lock
	*/
	virtual bool IsLocked()=0;

	virtual bool Save(CStr name)=0;
	EntityManager* GetEntityManager() const
	{
		return m_pManager;
	}

	EntityData(EntityManager* manager, CStr name): m_pManager(manager), m_pXMLNode(NULL), m_Name(name)
	{
		enf_assert(manager != NULL);
	}

	virtual TiXmlNode* FirstXMLNode(const char* name);
	virtual TiXmlNode* AddXMLNode(const TiXmlNode& node);
	virtual TiXmlAttribute* FirstXMLAttribute();
	virtual void AddXMLAttribute(const TiXmlAttribute& attr);
	CStr GetName() const
	{
		return m_Name.cstr();
	}
protected:
	virtual ~EntityData()
	{
	}
	EntityManager*	m_pManager;
	TiXmlElement* m_pXMLNode;
	DString		m_Name;
};
//@}
}

#endif //ENF_ENTITYDATA_H