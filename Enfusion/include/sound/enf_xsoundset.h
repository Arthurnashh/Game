//-----------------------------------------------------------------------------
// File: sound/enf_xsoundset.h
//
// Desc: Sound set
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_XSOUNDSET_H
#define ENF_XSOUNDSET_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "engine/enf_item.h"
#include "sound/enf_xsoundsystem.h"
#include "sound/enf_xsoundshader.h"

namespace enf
{
class TiXmlNode;
struct XSoundSetRef;
class ISoundSystem;

/** @addtogroup XSoundSystem
@{*/

/*!
Definition for soundset. Soundset contains set of
sounds, identified by its ID, for example:
walk, run, idle, combat_idle, death_strong
This is more convenient than array of pointers
to SoundShader. This class is definition of these
IDs, comment for what they are used etc.
Created by SoundSystem::LoadSoundsetDefinitions
*/
class PUBLIC_API XSoundsetDef
{
public:
	ENF_DECLARE_SMALLOBJECT(XSoundsetDef);

	struct Def
	{
		//!Sound ID
		CStr	m_strID;
		
		//!Description string. Used by tools
		const char*	m_strDesc;

		bool operator==(const Def& def) const
		{
			return m_strID == def.m_strID;
		}
		bool operator<(const Def& def) const
		{
			return m_strID < def.m_strID;
		}
		bool operator>(const Def& def) const
		{
			return m_strID > def.m_strID;
		}
		Def(): m_strDesc(NULL) {}
		Def(CStr id, const char* desc = NULL): m_strID(id), m_strDesc(desc) {}
	};

	XSoundsetDef():
		m_strName(NULL)
	{
	}
	
	XSoundsetDef(CStr defname, const XSoundsetDef* parent);
	~XSoundsetDef();

	bool Load(TiXmlNode* node);

	/*!
	Returns index of sound ID
	\param id
	Sound ID
	*/
	uint GetIndex(CStr id) const;
	
	//!Returns name of the definition
	CStr GetName() const
	{
		return m_strName;
	}

	//!Returns array of sound IDs
	const Set<Def>& GetDefs() const
	{
		return m_Defs;
	}

private:
	friend class XSoundset;
	friend class ISoundSystem;

	CStr					m_strName;
	Set<Def>				m_Defs;
};


struct PUBLIC_API XSoundSetRef
{
public:
	//!Returns pointer to sound
	ENF_INLINE XSoundShader* GetShader() const
	{
		return m_pShader;
	}

	ENF_INLINE const TiXmlElement* GetXMLElement() const
	{
		return m_pElement;
	}
	ENF_INLINE CStr GetShaderName() const
	{
		return m_strShaderName.c_str();
	}

	ENF_INLINE bool IsSet() const
	{
		return !m_strShaderName.IsEmpty() && !m_pShader.IsNull();
	}
	XSoundSetRef(): m_pElement(NULL) {}
	XSoundSetRef(CStr name, XSoundShader* sound);

protected:

friend class XSoundset;

	ComPtr<XSoundShader>	m_pShader;
	TiXmlElement*			m_pElement;
	/*!
	For now, it's just because of tools, as they need to know
	name regardless sound exists. In future, this redundancy
	will disappear, as we will share strings memory anyway
	*/
	DString				m_strShaderName;
};


/*!
Soundset is instance of SoundsetDef. It contains
loaded sounds and all additional data
for them (see enf::XSoundSetRef)
Soundset is loaded through XSoundSystem::
*/
class PUBLIC_API XSoundset: public Item
{
private:
	friend class ISoundSystem;
	ISoundSystem*			m_pOwner;
	CArray<XSoundSetRef>	m_Sounds;
	const XSoundsetDef*	m_pDef;

	virtual void Init();
	virtual void Unload();
	~XSoundset();

public:
	ENF_DECLARE_ITEM(XSoundset, Item);
	ENF_DECLARE_SMALLOBJECT(XSoundset);
	/*!
	Method for tools modifying Soundset
	Removes sound from Soundset
	\param idindex
	Sound ID index
	*/
	void RemoveSound(uint idindex);

	/*!
	Method for tools modifying Soundset
	Removes sound from Soundset
	\param idindex
	Sound ID index
	\param soundname
	Name of sound
	\param sound
	Pointer to sound (might be NULL)
	*/
	void SetSound(uint idindex, CStr soundname, XSoundShader* sound);

	//!Returns total number of sound IDs defined by SoundsetDef
	uint GetNumIDs() const;

	/*!
	Returns ID index. It's little bit faster when you get indices of
	all sound IDs and use them instead of string ID.
	\param id
	ID of sound.
	\return
	Sound ID index, or INDEX_NOT_FOUND when not found!
	*/
	uint GetIDIndex(const char* id) const;

	/*!
	Returns sound by its sound ID
	\param id
	ID of sound. Defined by sounds id="XXX" in Soundset
	\return
	AnimRef or NULL when sound is not defined or not loaded
	*/
	const XSoundSetRef* GetSound(CStr id) const;
	XSoundSetRef* GetSound(CStr id);

	/*!
	Returns sound by its ID index. This can be get by GetIDIndex(const char* id).
	It's little bit faster, but less convenient
	\param idindex
	ID of sound. Defined by sounds id="XXX" in Soundset
	\return
	AnimRef or NULL when sound is not defined or not loaded
	*/
	const XSoundSetRef* GetSound(uint idindex) const;
	XSoundSetRef* GetSound(uint idindex);

	/*!
	Returns pointer to definition of this Soundset
	*/
	const XSoundsetDef* GetSoundsetDef() const
	{
		return m_pDef;
	}

	bool Load(TiXmlElement* elem);
	bool Save();

	XSoundset(const XSoundsetDef* def);
	XSoundset():
		m_pDef(NULL),
		m_pOwner(NULL)
	{
	}
};

//@}
}
#endif //ENF_SOUNDSET_H