//-----------------------------------------------------------------------------
// File: enf_item.h
//
// Desc: Various object superclass
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ITEM_H
#define ENF_ITEM_H

#include "enf_config.h"
#include "common/enf_strings.h"
//#include "common/enf_watchman.h"
#include "Common\enf_mathlib.h"
#include "Es/Types/pointers.hpp"

#include <Es/Memory/normalNew.hpp>

namespace enf
{

/** @addtogroup Engine
@{*/

class SerializeInputAdapter;
class SerializeOutputAdapter;

//Helper
#define ENF_GLUE_STRINGS(a, b) a ## b

#define ENF_DECLARE_ABSTRACT_ITEM(classNameParam,parentClassNameParam) \
	virtual bool HasType(enf::uint cap) const{ if (cap==ENF_GLUE_STRINGS(classNameParam,TypeID)) return true; else return parentClassNameParam::HasType(cap);} \
	virtual enf::CStr GetTypeName() const{ return #classNameParam; } \
	virtual enf::uint GetTypeID() const{ return ENF_GLUE_STRINGS(classNameParam,TypeID);}\
	static classNameParam* SafeCast(enf::BaseItem* ptr) {if(ptr == NULL) return NULL; return (ptr->HasType(ENF_GLUE_STRINGS(classNameParam,TypeID))) ? (classNameParam*)ptr : NULL;}\
	static const classNameParam* SafeCast(const enf::BaseItem* ptr) {if(ptr == NULL) return NULL; return (ptr->HasType(ENF_GLUE_STRINGS(classNameParam,TypeID))) ? (const classNameParam*)ptr : NULL;}\
	static classNameParam* Cast(enf::BaseItem* ptr) {if(ptr == NULL) return NULL; Assert(ptr->HasType(ENF_GLUE_STRINGS(classNameParam,TypeID))); return (classNameParam*)ptr;}\
	static const classNameParam* Cast(const enf::BaseItem* ptr) {if(ptr == NULL) return NULL; Assert(ptr->HasType(ENF_GLUE_STRINGS(classNameParam,TypeID))); return (const classNameParam*)ptr;}

//Used to declare subclasses of Item
#define ENF_DECLARE_ITEM(classNameParam,parentClassNameParam) \
	ENF_DECLARE_ABSTRACT_ITEM(classNameParam,parentClassNameParam)


/*!
Base implementation of RTTI object
*/
class PUBLIC_API BaseItem
{
private:
public:
	ENF_DECLARE_MMOBJECT(BaseItem);

	/** \name RTTI related methods
	*/
	//@{
	/*!
	Tells whether the object has a ancestor of this type
	\param type Type-id we are asking for
	\return True when there is such type
	*/
	virtual bool HasType(uint type) const
	{
		return false;
	}

	//!Return type-id of the object
	virtual uint GetTypeID() const
	{
		return 0;
	}
	//!Returns type-name of the object
	virtual CStr GetTypeName() const
	{
		return "BaseItem";
	}
	//@}

	//!De-serializes the object from stream
	virtual void Serialize(SerializeInputAdapter* adapter);

	//!Serializes the object to stream
	virtual void Serialize(SerializeOutputAdapter* adapter);
	virtual ~BaseItem();
};

const uint ItemTypeID = 0; ///< Patient zero

class PUBLIC_API Item: public BaseItem, public RefCountObject
{
private:
	void*		m_pUserData;

protected:
	DString	m_strName;

public:
	ENF_DECLARE_ITEM(Item, BaseItem);

	//!Swap content of these two Items
	void Swap(Item& other)
	{
		enf_swap(m_pUserData, other.m_pUserData);
		m_strName.Swap(other.m_strName);
	}

	//!returns name of object
	CStr GetName() const;
	/*!
	Sets name of object
	\param name The name
	*/
	virtual void SetName(CStr name);

	/*!
	Sets pointer to user data
	\param data The data
	*/
	void SetUserData(void* data)
	{
		m_pUserData = data;
	}

	/*!
	Returns pointer to user data
	\return The data
	*/
	void* GetUserData()
	{
		return m_pUserData;
	}

	//!De-serializes the object from stream
	virtual void Serialize(SerializeInputAdapter* adapter);

	//!Serializes the object to stream
	virtual void Serialize(SerializeOutputAdapter* adapter);

	Item():
		//BaseItem(),
		m_strName(NULL),
		m_pUserData(NULL)
	{
	}

	virtual ~Item();
};

#ifdef ENF_WATCHMAN
class BaseItemWatchHandler: public WatchPointer
{
public:
	const char* GetTypeName(const void* ptr) const;
	const char* GetInfo(const void* ptr) const;
	BaseItemWatchHandler(BaseItem* v): WatchPointer(v) {}
	BaseItemWatchHandler(): WatchPointer(NULL) {}

};
class ItemWatchHandler: public enf::BaseItemWatchHandler
{
public:
	const char* GetObjectName(const void* ptr) const;
	const char* GetInfo(const void* ptr) const;
	ItemWatchHandler(Item* v): BaseItemWatchHandler(v) {}
	ItemWatchHandler(): BaseItemWatchHandler(NULL) {}

};

extern BaseItemWatchHandler g_BaseItemWatchHandler;
extern ItemWatchHandler g_ItemWatchHandler;
#endif
//@}
}
#endif //ENF_ITEM