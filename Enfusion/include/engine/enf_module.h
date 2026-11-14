//-----------------------------------------------------------------------------
// File: enf_module.h
//
// Desc: Superclass for every functional module inside Enforce
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MODULE_H
#define ENF_MODULE_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "engine/enf_item.h"

namespace enf
{
/** @addtogroup Engine
@{*/

class Engine;
class Event;
class TiXmlElement;

/*!
Base for all BaseItem class factories
*/
class PUBLIC_API ItemFactory
{
public:
	virtual BaseItem* CreateItem() { return NULL;};

	CStr GetTypeName() const {return m_TypeName;}
	uint GetTypeID() const {return m_TypeID;}

protected:
	ItemFactory(CStr typeName, uint typeID) : m_TypeName(typeName), m_TypeID(typeID) {}
public:
	virtual ~ItemFactory() {}

protected:
	CStr	m_TypeName;
	uint	m_TypeID;
};

#define DECLARE_ITEM_FACTORY( T )\
class T ## Factory : public enf::ItemFactory\
{\
public:\
	T ## Factory(enf::CStr typeName, enf::uint typeID) : enf::ItemFactory(typeName, typeID) {}\
	enf::BaseItem* CreateItem()\
    {\
        return ENF_NEW T ();\
    }\
};\
T ## Factory& Get ## T ## Factory();

#define DEFINE_ITEM_FACTORY( T )\
T ## Factory& Get ## T ## Factory()\
{\
	static T ## Factory s_factory(#T, T ## TypeID);\
	return s_factory;\
}

#define ITEM_FACTORY( T ) (Get ## T ## Factory())

const uint ModuleTypeID					= 100000;

enum ModuleID
{
	ResourceManagerTypeID = ModuleTypeID + 1,
	ParticleSystemTypeID,
	DecalManagerTypeID,
	NetworkTypeID,
	MaterialSystemTypeID,
	RendererTypeID,
	BaseSoundSystemTypeID,
	EAXSoundSystemTypeID,
	InputDeviceTypeID,
	WinInputDeviceTypeID,
	PS3InputDeviceTypeID,
	AppleInputDeviceTypeID,
	FileSystemTypeID,
	WidgetManagerTypeID,
	PathEngineTypeID,
	PipelineTypeID,
	DefaultPipelineTypeID,
	PostProcessManagerTypeID,
	AnimationSystemTypeID,
	BulletManagerTypeID,
	WorldManagerTypeID,
	PhysicsManagerTypeID,
	EntityManagerTypeID,
	WeatherManagerTypeID,
	GrassMaterialManagerTypeID,
	AddonManagerTypeID,
};

class PUBLIC_API ClassFactoryCollection
{
protected:
	Set<Pair<ItemFactory*, uint> >	m_ClassFactorySet;
	Array<ItemFactory*>	m_ClassFactories;
public:
	void Register(ItemFactory& factory);
	void Unregister(ItemFactory& factory);
	BaseItem* Create(CStr typeName);
	BaseItem* Create(uint typeID);
	uint GetNumFactories() const
	{
		return m_ClassFactories.GetCardinality();
	}
	const ItemFactory* GetFactory(uint n) const
	{
		return m_ClassFactories[n];
	}
};

/*!
Base superclass for every functional module of Enforce engine. Provides
basic communication and cooperation between modules inside engine.
*/
class PUBLIC_API Module: public Item, public ClassFactoryCollection
{
protected:
friend class Engine;
	Engine*	m_pEngine;
	Array<ClassFactoryCollection*> m_ClassFactories;

	//!Called by engine, when is released. Should implement cleanup code
	virtual ~Module() {};

public:
	ENF_DECLARE_ITEM(Module, Item);
	ENF_DECLARE_MMOBJECT(Module);

	Module():
		m_pEngine(NULL)
	{
	}

	/*!
	Return pointer to the engine this module belongs to, or NULL, when not inserted into engine yet;
	*/
	ENF_INLINE Engine* GetEngine() const
	{
		return m_pEngine;
	}
	//!Register another class factory. Usable for overriding instance creation without inheriting the module
	void RegisterClassFactory(ClassFactoryCollection* classFactory);
	void UnregisterClassFactory(ClassFactoryCollection* classFactory);

	/*!
	Called when owner engine is set as current.
	*/
	virtual void SetCurrent() {}
	/*!
	Called when this module is inserted into engine.
	\param xmlnode
	Root node of global settings
	*/
	virtual void OnInsert(TiXmlElement* xmlnode) {}
	/*!
	Called when this module is removed from engine
	*/
	virtual void OnRemove() {}
	/*!
	Called when some module is inserted to engine
	\param mod
	Pointer to inserted module
	*/
	virtual void OnModuleInserted(Module* mod) {}
	/*!
	Called when some module is removed from engine
	\param mod
	Pointer to removed module
	*/
	virtual void OnModuleRemoved(Module* mod) {}
	/*!
	Called when engine is reseted from various reasons. Most probably it's lost device state
	when user switched app from fullscreen mode to windowed and back.
	*/
	virtual void OnReset() {}
	/*!
	Called when rendertarget is resized, or fullscreen/windowed state is changed
	*/
	virtual void OnResize(uint width, uint height, bool windowed) {}

	/*!
	Called when someone changed global settings.
	\param xmlnode
	Root node of global settings
	\return
	True when module was able to accept new settings, false when engine needs to
	be restarted.
	*/
	virtual bool OnChangeSettings(TiXmlElement* xmlnode) {return true;}

	/*!
	Called when some event occur.
	\param event
	Pointer to event. 
	*/
	virtual void OnEvent(const Event& event) {}

	/*!
	Called upon begin of frame
	*/
	virtual void OnBeginFrame() {}

	/*!
	Create method of "class factory"
	\param typeId
	Identifier of type we want to create
	\return
	Pointer to created instance, or NULL, when we don't know this type
	*/
	virtual BaseItem* CreateInstance(uint typeId);

	/*!
	Create method of "class factory"
	\param typeName
	Name of type we want to create
	\return
	Pointer to created instance, or NULL, when we don't know this type
	*/
	virtual BaseItem* CreateInstance(const char* typeName);
};
//@}
}
#endif //ENF_MODULE