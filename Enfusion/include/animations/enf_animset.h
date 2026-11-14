//-----------------------------------------------------------------------------
// File: vobject/enf_animset.h
//
// Desc: Animation set
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ANIMSET_H
#define ENF_ANIMSET_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "animations/enf_animation.h"
#include "script/enf_scriptvariable.h"

namespace enf
{
class TiXmlNode;
class AnimRef;

/** @addtogroup Animations
@{*/

/*!
Definition of custom animation-hook action
Used internally in AnimsetDef,
created by AnimationSystem::LoadAnimsetDefinitions
*/
struct AnimAction
{
	//!The action to be invoked (scripted function for example)
	CStr		m_strAction;
	/*!
	Name of handler. It's used when you're using many AnimActionHandler
	and you need to distinguis who is handling what actions
	*/
	CStr		m_strHandler;

	//!Type (string,vector,float,int) of action parameter (See AnimHook)
	VType		m_ParmType;

	bool operator==(const AnimAction& action) const
	{
		return (m_strAction == action.m_strAction);
	}
	bool operator<(const AnimAction& action) const
	{
		return (m_strAction < action.m_strAction);
	}
	bool operator>(const AnimAction& action) const
	{
		return (m_strAction > action.m_strAction);
	}

	AnimAction(CStr action, CStr handler = NULL, VType parmtype = (VType)0):
		m_strAction(action),
		m_strHandler(handler),
		m_ParmType(parmtype)
	{
	}

	AnimAction():
		m_strAction(NULL),
		m_strHandler(NULL),
		m_ParmType((VType)0)
	{
	}
};

/*!
Definition of custom animation attribute
Used internally in AnimsetDef,
created by AnimationSystem::LoadAnimsetDefinitions
*/
struct AnimAttr
{
	//!Name of attribute
	CStr		m_strAttribute;

	//!Type of attribute value. Used for input method in tools
	VType		m_ValueType;

	bool operator==(const AnimAttr& attr) const
	{
		return (m_strAttribute == attr.m_strAttribute);
	}
	bool operator<(const AnimAttr& attr) const
	{
		return (m_strAttribute < attr.m_strAttribute);
	}
	bool operator>(const AnimAttr& attr) const
	{
		return (m_strAttribute > attr.m_strAttribute);
	}
	
	AnimAttr():
		m_strAttribute(NULL),
		m_ValueType((VType)0)
	{
	}

	AnimAttr(CStr attr, VType type = (VType)0):
		m_strAttribute(attr),
		m_ValueType(type)
	{
	}
};

/*!
Definition for animset. Animset contains set of
animation, identified by its ID, for example:
walk, run, idle, combat_idle, death_strong
This is more convenient than array of pointers
to animations. This class is definition of these
IDs, comment for what they are used etc.
Created by AnimationSystem::LoadAnimsetDefinitions
*/
class PUBLIC_API AnimsetDef: public RefCountObject
{
public:
	struct Def
	{
		//!Animation ID
		CStr	m_strID;
		
		//!Description string. Used by tools
		const char*	m_strDesc;

		bool operator==(const Def& def) const
		{
			return m_strID ==def.m_strID;
		}
		bool operator<(const Def& def) const
		{
			return m_strID < def.m_strID;
		}
		bool operator>(const Def& def) const
		{
			return m_strID > def.m_strID;
		}
		Def(): m_strDesc(NULL) { }
		Def(CStr id, const char* desc = NULL): m_strID(id), m_strDesc(desc) { }
	};

	AnimsetDef()
	{
	}
	
	AnimsetDef(CStr defname, AnimsetDef* parent);
	~AnimsetDef();

	bool Load(TiXmlNode* node);

	/*!
	Returns index of animation ID
	\param id
	Animation ID
	*/
	uint GetIndex(CStr id) const;
	
	//!Returns name of the definition
	CStr GetName() const
	{
		return m_strName;
	}

	//!Returns array of animation IDs
	const Set<Def>& GetDefs() const
	{
		return m_Defs;
	}

/** \name Attributes
*/
//@{
	ENF_INLINE uint GetAttributeIndex(CStr name) const
	{
		return m_AnimAttributes.GetIndexOf(AnimAttr(name));
	}

	ENF_INLINE const AnimAttr* GetAttribute(CStr name) const
	{
		uint index = m_AnimAttributes.GetIndexOf(AnimAttr(name));
		if(index == INDEX_NOT_FOUND)
			return NULL;
		return &m_AnimAttributes[index];
	}

	ENF_INLINE CStr GetAttributeName(uint attrid) const
	{
		return m_AnimAttributes[attrid].m_strAttribute;
	}

	ENF_INLINE const Set<AnimAttr>& GetAttributes() const
	{
		return m_AnimAttributes;
	}
//@}

/** \name Animation Actions
*/
//@{
	ENF_INLINE const Set<AnimAction>& GetActions() const
	{
		return m_AnimActions;
	}

	ENF_INLINE const AnimAction& GetAction(uint index) const
	{
		return m_AnimActions[index];
	}

	const uint GetActionIndex(CStr action) const;
//@}

private:
	friend class Animset;
	friend class AnimationSystem;
	friend class AnimInfo;

/*!
Internal method, used by AnimInfo::AddAnimHook and Animset::Load
\param animhook
Animhook to be set
\param frame Frame
\param actionindex Action index. See GetActionIndex()
\param param Parameters
*/
	bool SetupAnimHook(AnimHook& animhook, float frame, uint actionindex, CStr param) const;

	CStr					m_strName;
	Set<Def>				m_Defs;
	Set<AnimAttr>		m_AnimAttributes;
	Set<AnimAction>	m_AnimActions;
};


ENF_INLINEFUNC uint AnimsetDef::GetIndex(CStr id) const
{
	return m_Defs.GetIndexOf(Def(id.cstr()));
}

/*!
Definition of one animation in Animset.
See enf::AnimInfo for additional data
Used internally.
*/
class PUBLIC_API AnimRef: public AnimInfo
{
public:
	ENF_DECLARE_SMALLOBJECT(AnimRef);

	//!Returns pointer to animation
	ENF_INLINE Animation* GetAnimation() const
	{
		return m_pAnim;
	}

	ENF_INLINE CStr GetAnimationName() const
	{
#ifdef ENF_DEVELOPER
		return m_strAnimName.c_str();
#else
		return "";
#endif
	}

	AnimRef():
		m_pAnim(NULL)
	{
	}

	AnimRef(CStr name, Animation* anim);

protected:

friend class List<AnimRef>;
friend class ListIterator<AnimRef>;
friend class Animset;

	AnimRef*	ENF_LIST_NEXT;
	AnimRef*	ENF_LIST_PREV;

	ComPtr<Animation>	m_pAnim;
	/*!
	For now, it's just because of tools, as they need to know
	name regardless animation exists. In future, this redundancy
	will disappear, as we will share strings memory anyway
	*/
#ifdef ENF_DEVELOPER
	DString				m_strAnimName;
#endif

};

typedef List<AnimRef> TAnimRefList;

/*!
Animset is instance of AnimsetDef. It contains
loaded animations and all additional data
for them (see enf::AnimRef and enf::AnimInfo)
Animset is loaded thru ResourceManager::GetResourceObject
*/
class PUBLIC_API Animset: public ObjectResource
{
private:
	friend class LoadAnimsetResource;

	Array<TAnimRefList>	m_Anims;
	Animset*					m_pParent;
	ComPtr<AnimsetDef>	m_pDef;

	virtual void Init();
	virtual void Unload();
	bool Load(CInputStream* instream, bool preload);

public:
	ENF_DECLARE_ITEM(Animset, ObjectResource);
	void Swap(Animset& other);
	LoadResource* CreateResourceLoader();

	virtual size_t GetSize() const;


	/*!
	Method for tools modifying Animset
	Adds animation to Animset
	\param idindex
	Anim ID index
	\param ref
	AnimRef to be added to Animset
	*/
	void AddAnimation(uint idindex, AnimRef* ref);

	/*!
	Method for tools modifying Animset
	Removes animation from Animset
	\param idindex
	Anim ID index
	\param index
	Index of AnimRef to be removed
	*/
	void RemoveAnimation(uint idindex, uint index);

	/*!
	Method for tools modifying Animset
	Removes animation from Animset
	\param idindex
	Anim ID index
	\param index
	Index of AnimRef to be removed
	\param animname
	Name of animation
	\param anm
	Pointer to animation (might be NULL)
	*/
	void SetAnimation(uint idindex, uint index, CStr animname, Animation* anm);

	//!Returns total number of animation IDs defined by AnimsetDef
	uint GetNumIDs() const;

	/*!
	Returns ID index. It's little bit faster when you get indices of
	all anim IDs and use them instead of string ID.
	\param id
	ID of animation.
	\return
	Anim ID index, or INDEX_NOT_FOUND when not found!
	*/
	uint GetIDIndex(const char* id) const;

	/*!
	Returns animation by its anim ID
	\param id
	ID of animation. Defined by anims id="XXX" in animset
	\param index
	Index of animation in case there are more than one animation
	\return
	AnimRef or NULL when animation is not defined or not loaded
	*/
	const AnimRef* GetAnimation(const char* id, uint index) const;
	AnimRef* GetAnimation(const char* id, uint index);

	/*!
	Returns animation by its ID index. This can be get by GetIDIndex(const char* id).
	It's little bit faster, but less convenient
	\param idindex
	ID of animation. Defined by anims id="XXX" in animset
	\param index
	Index of animation in case there are more than one animation
	\return
	AnimRef or NULL when animation is not defined or not loaded
	*/
	const AnimRef* GetAnimation(uint idindex, uint index) const;
	AnimRef* GetAnimation(uint idindex, uint index);

	const TAnimRefList* GetAnimations(const char* id) const;
	const TAnimRefList* GetAnimations(uint idindex) const;

	/*!
	Returns number of anims for the id
	\param id
	Animation ID
	\return
	Number of anims, or INDEX_NOT_FOUND when ID not found!
	*/
	uint GetNumAnims(const char* id) const;

	/*!
	Returns number of anims for the idindex
	\param idindex
	Animation ID index
	\return
	Number of anims
	*/
	uint GetNumAnims(uint idindex) const;

	/*!
	Returns pointer to definition of this Animset
	*/
	const AnimsetDef* GetAnimsetDef() const
	{
		return m_pDef;
	}

	bool Load(CInputStream* instream);
	bool Save();

	Animset(AnimsetDef* def, Animset* parent);
	Animset():
		m_pDef(NULL),
		m_pParent(NULL)
	{
	}
};

//@}
}
#endif //ENF_ANIMSET_H