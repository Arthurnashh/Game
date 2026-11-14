//-----------------------------------------------------------------------------
// File: enf_xsoundset.cpp
//
// Desc: XSoundset class
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "enf_engine.h"
#include "sound/enf_xsoundset.h"
#include "sound/enf_isoundsystem.h"
#include "tinyxml/tinyxml.h"
#include "common/enf_staticbuffer.h"

namespace enf
{

//-----------------------------------------------------------------------------
uint XSoundsetDef::GetIndex(CStr id) const
{
	return m_Defs.GetIndexOf(Def(id));
}

//-----------------------------------------------------------------------------
const XSoundsetDef* ISoundSystem::GetSoundsetDef(CStr name) const
{
	return m_SoundsetDefHash.Find(name);
}

//-----------------------------------------------------------------------
XSoundset* ISoundSystem::GetSoundset(CStr strName)
{
	XSoundset* ss;
	if(m_SoundsetHash.Find(strName, ss))
	{
		ss->AddRef();
		return ss;
	}

	TiXmlDocument doc;

	if(!doc.LoadFile(strName.cstr()))
		return NULL;

	CStr defname = doc.RootElement()->Attribute("def");
	if(defname.IsEmpty())
		return NULL;

	const XSoundsetDef* def = GetSoundsetDef(defname);

	if(def == NULL)
		return NULL;

	ss = ENF_NEW XSoundset(def);
	ss->m_pOwner = this;

	if(ss->Load(doc.RootElement()))
	{
		ss->SetName(strName);
		m_SoundsetHash.Insert(ss->GetName(), ss);
		m_Soundsets.Insert(ss);
		return ss;
	}
	ss->Release();
	return NULL;
}


//-----------------------------------------------------------------------------
XSoundsetDef::~XSoundsetDef()
{
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------------
XSoundsetDef::XSoundsetDef(CStr defname, const XSoundsetDef* parent)
{
	m_strName = defname.Static();

	if(parent)
	{
		for(uint a = 0; a < parent->m_Defs.GetCardinality(); a++)
			m_Defs.Insert(parent->m_Defs[a]);
	}
	MemStats::m_iSoundSystem += sizeof(*this);
}

//-----------------------------------------------------------------------------
bool XSoundsetDef::Load(TiXmlNode* node)
{
	SArray<TiXmlElement*, 256> elems;

	for(TiXmlElement* el = node->FirstChildElement("sound"); el; el = el->NextSiblingElement("sound"))
	{
		if(!elems.IsFull())
			elems.Insert(el);
	}

	m_Defs.Reserve(elems.GetCardinality());

	for(uint n = 0; n < elems.GetCardinality(); n++)
	{
		CStr id = elems[n]->Attribute("id");
		if(id.IsEmpty())
			continue;

		const char* desc = NULL;

		for(TiXmlNode* comment = elems[n]->FirstChild(); comment; comment->NextSibling())
		{
			if(comment->Type() == TiXmlNode::COMMENT)
			{
				desc = g_pStatic->AddString(comment->Value());
				break;
			}
		}
		m_Defs.Insert(Def(id.Static(), desc));
	}
	return !elems.IsEmpty();
}

//-----------------------------------------------------------------------------
XSoundset::XSoundset(const XSoundsetDef* def):
   m_pDef(def)
{
	enf_assert(def);
	m_Sounds.SetSize(def->m_Defs.GetCardinality());
	MemStats::m_iSoundSystem -= sizeof(*this);
}

//-----------------------------------------------------------------------------
uint XSoundset::GetIDIndex(const char* id) const
{
	return m_pDef->GetIndex(id);
}

//-----------------------------------------------------------------------------
void XSoundset::SetSound(uint idindex, CStr soundname, XSoundShader* sound)
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());
	XSoundSetRef& ref = m_Sounds[idindex];

	ref.m_strShaderName = soundname;
	ref.m_pShader = sound;
}

//-----------------------------------------------------------------------------
void XSoundset::RemoveSound(uint idindex)
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());
	XSoundSetRef& ref = m_Sounds[idindex];

	if(ref.IsSet())
	{
		ref.m_strShaderName.Clear();
		ref.m_pShader.Release();
	}
}

//-----------------------------------------------------------------------------
void XSoundset::Init()
{
	m_Sounds.Clear();
	m_pDef = NULL;
}

//-----------------------------------------------------------------------------
void XSoundset::Unload()
{
	for(uint n = 0; n < m_Sounds.GetCardinality(); n++)
	{
		XSoundSetRef& ref = m_Sounds[n];
		if(ref.IsSet())
		{
			ref.m_pShader.Release();
		}
		if(ref.m_pElement)
			delete ref.m_pElement;
	}

	m_Sounds.Clear();
}

//-----------------------------------------------------------------------------
XSoundset::~XSoundset()
{
	Unload();
	if(m_pOwner)
	{
		m_pOwner->m_Soundsets.Remove(this);
		m_pOwner->m_SoundsetHash.Remove(GetName());
	}
	MemStats::m_iSoundSystem -= sizeof(*this);
}

//-----------------------------------------------------------------------------
const XSoundSetRef* XSoundset::GetSound(CStr id) const
{
	uint idindex = this->m_pDef->GetIndex(id.cstr());

	if(idindex == INDEX_NOT_FOUND || !m_Sounds[idindex].IsSet())
		return NULL;

	return &m_Sounds[idindex];
}

//-----------------------------------------------------------------------------
XSoundSetRef* XSoundset::GetSound(CStr id)
{
	uint idindex = this->m_pDef->GetIndex(id.cstr());

	if(idindex == INDEX_NOT_FOUND || !m_Sounds[idindex].IsSet())
		return NULL;

	return &m_Sounds[idindex];
}

//-----------------------------------------------------------------------------
const XSoundSetRef* XSoundset::GetSound(uint idindex) const
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());

	if(!m_Sounds[idindex].IsSet())
		return NULL;

	return &m_Sounds[idindex];
}

//-----------------------------------------------------------------------------
XSoundSetRef* XSoundset::GetSound(uint idindex)
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());

	if(!m_Sounds[idindex].IsSet())
		return NULL;

	return &m_Sounds[idindex];
}

//-----------------------------------------------------------------------------
bool XSoundset::Save()
{
	TiXmlDocument doc(GetName().cstr());

	TiXmlElement* root = doc.InsertEndChild(TiXmlElement("soundset"))->ToElement();
	root->SetAttribute("version", "1.0");
	root->SetAttribute("name", GetName().cstr());
	root->SetAttribute("def", m_pDef->GetName().cstr());
	
	static const TiXmlElement soundElem("sound");

	const Set<XSoundsetDef::Def>& defs = m_pDef->GetDefs();

	for(uint n = 0; n < m_Sounds.GetCardinality(); n++)
	{
		if(!m_Sounds[n].IsSet())
			continue;

		TiXmlElement* TempElm = root->InsertEndChild(soundElem)->ToElement();
		TempElm->SetAttribute("id", defs[n].m_strID.cstr());
		TempElm->SetAttribute("shader", m_Sounds[n].m_strShaderName.cstr());
	}

	return doc.SaveFile(this->GetName().cstr());
}

//-----------------------------------------------------------------------------
bool XSoundset::Load(TiXmlElement* root)
{
	bool res = true;

	m_Sounds.SetSize(m_pDef->m_Defs.GetCardinality());

	for(TiXmlElement* elem = root->FirstChildElement("sound"); elem; elem = elem->NextSiblingElement("sound"))
	{
		CStr idname = elem->Attribute("id");
		uint index = m_pDef->GetIndex(idname);

		if(index == INDEX_NOT_FOUND)
			continue;

		CStr sname = elem->Attribute("shader");

		if(!sname.IsEmpty())
		{
			m_Sounds[index].m_strShaderName = sname;
			m_Sounds[index].m_pShader = m_pOwner->GetShader(sname, 0);
			const TiXmlElement* customElement = elem->FirstChildElement();
			if(customElement)
			{
				m_Sounds[index].m_pElement = ENF_NEW TiXmlElement(*customElement);
			}
		}
	}
	enf_assert(res);
	return res;
}

//-----------------------------------------------------------------------------
bool ISoundSystem::r_LoadXSoundsetDefinition(TiXmlElement* elem, const XSoundsetDef* parent)
{
	XSoundsetDef* def = ENF_NEW XSoundsetDef(elem->Attribute("name"), parent);
	def->Load(elem);
	m_SoundsetDefs.Insert(def);
	m_SoundsetDefHash.Insert(def->GetName().cstr(), def);

	for(TiXmlElement* child = elem->FirstChildElement("setdef"); child; child = child->NextSiblingElement("setdef"))
	{
		r_LoadXSoundsetDefinition(child, def);
	}
	return true;
}

//-----------------------------------------------------------------------------
bool ISoundSystem::LoadSoundsetDefinitions(CStr name)
{
	TiXmlDocument doc;

	if(doc.LoadFile(name.cstr()))
	{
		for(TiXmlElement* elem = doc.RootElement()->FirstChildElement("setdef"); elem; elem = elem->NextSiblingElement("setdef"))
			r_LoadXSoundsetDefinition(elem, NULL);
		return true;
	}
	return false;
}


}