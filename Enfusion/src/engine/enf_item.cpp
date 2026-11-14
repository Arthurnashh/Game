//-----------------------------------------------------------------------------
// File: enf_eitem.cpp
//
// Desc: Various object superclass
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------
//#include "precomp.h"
#include "engine/enf_item.h"
//#include "common/enf_serialize.h"

namespace enf
{
//-----------------------------------------------------------------------
void BaseItem::Serialize(SerializeInputAdapter* adapter)
{
}

//-----------------------------------------------------------------------
void BaseItem::Serialize(SerializeOutputAdapter* adapter)
{
}

//-----------------------------------------------------------------------
BaseItem::~BaseItem()
{
}

//-----------------------------------------------------------------------
void Item::Serialize(SerializeInputAdapter* adapter)
{
  RlsAssert2(0, "not implemented");
  /*
	m_pUserData = adapter->ReadReference(ENF_MAKEFOURCC('U', 'D', 'T', 'A' ));
	m_iRefs = adapter->ReadInt32();
	SetName(adapter->ReadString());
  */
}

//-----------------------------------------------------------------------
void Item::Serialize(SerializeOutputAdapter* adapter)
{
	RlsAssert2(0, "not implemented");
  /*
  adapter->WriteReference(ENF_MAKEFOURCC('U', 'D', 'T', 'A' ), m_pUserData);
	adapter->WriteInt32((const int)m_iRefs);
	adapter->WriteString(m_strName.c_str());
  */
}

//---------------------------------------------------------------------------
CStr Item::GetName() const
{
	return m_strName.c_str();
}

//---------------------------------------------------------------------------
void Item::SetName(CStr name)
{
	m_strName = name.cstr();
}

//---------------------------------------------------------------------------
Item::~Item()
{
}

#ifdef ENF_WATCHMAN
BaseItemWatchHandler g_BaseItemWatchHandler;
ItemWatchHandler g_ItemWatchHandler;

const char* BaseItemWatchHandler::GetTypeName(const void* ptr) const
{
	const Item* it = reinterpret_cast<const Item*>(ptr);
	if(it) return it->GetTypeName().cstr();
	return "";
}

const char* ItemWatchHandler::GetObjectName(const void* ptr) const
{
	const Item* it = reinterpret_cast<const Item*>(ptr);
	if(it) return it->GetName().cstr();
	return "";
}

const char* BaseItemWatchHandler::GetInfo(const void* ptr) const
{
	const Item* it = reinterpret_cast<const Item*>(ptr);
	static char buff[4096];
	if(it)
	{
		sprintf(buff, "Type %s", it->GetTypeName().cstr());
		return buff;
	}
	return "";
}

const char* ItemWatchHandler::GetInfo(const void* ptr) const
{
	const Item* it = reinterpret_cast<const Item*>(ptr);
	static char buff[4096];
	if(it)
	{
		sprintf(buff, "Type %s Name %s", it->GetTypeName().cstr(), it->GetName().cstr());
		return buff;
	}
	return "";
}

#endif


}