//-----------------------------------------------------------------------------
// File: vobject/enf_animation.h
//
// Desc: Animation
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ANIMATION_H
#define ENF_ANIMATION_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "resource/enf_objectresource.h"

namespace enf
{
/** @defgroup Animations Animation system
@{*/
class AnimsetDef;
class AnimationSource;
class COutputStream;
class SimdVector;

/*!helper struct for decompressing source anim data
here we store indices for actual frame+next frame.*/
struct UnpackInfo
{
	//!index for quaternions
	ushort QIndex;
	//!index for translations
	ushort TIndex;
	//indices for scale, not used ATM

	UnpackInfo(): QIndex(0xffff), TIndex(0xffff)
	{
	}
};

//-----------------------------------------------------------------------------
ENF_INLINE float GetKeyframeProps(float t, int numFrames, int& f1, int& f2)
{
	int maxFrame = numFrames - 1;
	float ft = floor(t);
	int pf = (int)ft;

	enf_assert(pf <= numFrames);

	f1 = pf;
	f2 = f1 + 1;
	if(f2 > maxFrame)
	{
		f2 = f1;
		return f1;
	}

	return t;
}

//-----------------------------------------------------------------------------
/*!
Animation data for Skeleton/MeshObject. Contains compressed
key-frames in separated channels for translation, rotation and
scale.
Created either by ResourceManager::GetResourceObject, or
manually by 'new Animation' and Animation::Load()
*/
class PUBLIC_API Animation: public ObjectResource
{
public:
	ENF_DECLARE_ITEM(Animation, Item);
	class Channel;

	static const uint MAX_CHANNELS = 96;

	virtual size_t GetSize() const
	{
		return m_iSize + sizeof(*this);
	}

	void Swap(Animation& other);
	LoadResource* CreateResourceLoader();

	bool Build(const AnimationSource* source);

	/*!
	Returns number of frames
	*/
	ENF_INLINE uint GetNumFrames() const
	{
		return m_iNumFrames;
	}

	//!Tells whether Animation contains this channel
	ENF_INLINE bool HasChannel(uint index) const
	{
		enf_assert(index < MAX_CHANNELS);
		return (m_iChannelIndex[index] != 0xff);
	}

	/*!
	Returns animation data for one channel
	\param index
	Extern index
	*/
	const Channel* GetChannel(uint index) const;

	/*!
	Loads animation
	\return
	True when success
	*/
	bool Load(CInputStream* instream);
	Animation()
	{
		Init();
	}

	class Keys
	{
	private:
		uint FindFrame(uint frame, uint low, uint hight) const;

	public:
		union
		{
			struct  
			{
				union
				{
					struct 
					{
						float m_fBias, m_fScale;
					};

					float m_vBiasScale[2];
				};
				ushort*			m_pData;
				ushort*			m_pTime;
			};
			float	m_vData[4];
		};
		uint				m_iNumKeys;
		ushort			m_iMinTime, m_iMaxTime;

		void InitTimeLimits();

		ENF_INLINE uint GetFrame(uint frame, uint hint) const
		{
			uint numkeys = m_iNumKeys;
			
			//the fastest ways
			if(numkeys == 0)
				return INDEX_NOT_FOUND;
			if(frame <= m_iMinTime)
				return 0;
			if(frame >= m_iMaxTime)
				return numkeys - 1;

			//search by hint
			if(hint < numkeys)
			{
				if(frame >= m_pTime[hint])
				{
					while(true)
					{
						enf_assert((hint + 1) != numkeys);
						if(frame < m_pTime[hint + 1])
							return hint;
						hint++;
					}
				}
				else
				{
					while(true)
					{
						enf_assert(hint > 0);
						if(frame >= m_pTime[--hint])
							return hint;
					}
				}
			}

			return FindFrame(frame, 0, numkeys - 1);
		}

		ENF_INLINE uint GetFrame(uint frame) const
		{
			uint numkeys = m_iNumKeys;

			//the fastest ways
			if(numkeys == 0)
				return INDEX_NOT_FOUND;

			if(frame <= m_iMinTime)
				return 0;

			if(frame >= m_iMaxTime)
				return numkeys - 1;
			
			return FindFrame(frame, 0, numkeys - 1);
		}
	};

	class PUBLIC_API Channel
	{
		friend class Animation;

	private:

		Keys			m_TransChannel;
		Keys			m_QuatChannel;
		//Keys			m_ScaleChannel; //Not used ATM
		uint			m_iBoneIndex;
	public:

		/*ENF_INLINE const Keys& GetScale() const
		{
			return m_ScaleChannel;
		}*/

		ENF_INLINE const Keys& GetTrans() const
		{
			return m_TransChannel;
		}

		ENF_INLINE const Keys& GetQuat() const
		{
			return m_QuatChannel;
		}

//#ifdef ENF_XENON
//		void Keyframe(QKey& qkey, int f1, int f2, const SimdVector& frac, UnpackInfo& info) const;
//#endif
		/*!
		\param qkey Result
		\param f1 Frame we interpolate from
		\param f2 Frame we interpolate to
		\param frac Exact time (f1+frac)
		\param info Decompression helper
		*/
		void Keyframe(QKey& qkey, int f1, int f2, float frac, UnpackInfo& info) const;
		/*!
		\param qkey Result
		\param f1 Frame we interpolate from
		\param f2 Frame we interpolate to
		\param frac Exact time (f1+frac)
		*/
		void Keyframe(QKey& qkey, int f1, int f2, float frac) const;
		void FirstKeyframe(QKey& qkey) const;
		void LastKeyframe(QKey& qkey) const;
	};

private:
	friend class LoadAnimationResource;

	Channel*			m_pChannels;
	uint				m_iNumChannels;
	ubyte				m_iChannelIndex[MAX_CHANNELS];
	size_t			m_iSize;

	uint				m_iNumFrames;

protected:
	void Init();
	void Unload();

};

//-----------------------------------------------------------------------------
/*!
AnimHook is marker on animation time-line which
invoke AnimAction when it steps over this marker
during animation play.
Used internally, created by AnimSet
*/
struct AnimHook
{
	//!On frame
	float			m_fFrame;	
	
	//! The action that is invoked upon animhook. See AnimAction
	CStr			m_strAction;

	//!Parameter for the action. 
	CStr			m_strParam;

	/*!
	Optimization for AnimAction handling. AnimActionHandler
	can store here some handle for faster processing
	(i.e. index of script method called by action)
	*/
	void*			m_pAction;

	/*!
	Optimization for AnimAction handling. AnimActionHandler
	can store here some handle for faster processing
	(i.e. pointer to script variable)
	*/
	void*			m_pParam;

	bool operator==(const AnimHook& hook) const
	{
		return (m_fFrame == hook.m_fFrame &&
				m_strAction == hook.m_strAction &&
				m_strParam == hook.m_strParam);
	}
	
	bool operator<(const AnimHook& hook) const
	{
		if(m_fFrame > hook.m_fFrame) return false;
		if(m_fFrame < hook.m_fFrame) return true;
		if(m_strAction > hook.m_strAction) return false;
		if(m_strAction < hook.m_strAction) return true;
		if(m_strParam > hook.m_strParam) return false;
		if(m_strParam < hook.m_strParam) return true;
		return false;
	}
	
	bool operator>(const AnimHook& hook) const
	{
		return !(*this < hook);
	}

	AnimHook():
		m_fFrame(0.0f),
		m_strAction(NULL),
		m_strParam(NULL),
		m_pAction(NULL),
		m_pParam(NULL)
	{
	}
};

/*!
Defines value for animation attribute. Attribute is user defined variable
(definition is parsed from animset definition XML) which has some value.
Developers can use it for custom remarks for what and how they should use
the animation. For example, if animation is in place, or moving, if
character is aiming his gun, if he kneels etc.
Used internally, created by AnimationSystem::LoadAnimsetDefinitions()
*/
struct AnimAttrValue
{
	//!Index of attribute. (Index in Attribute list in ResourceManager)
	uint			m_iAttrID;
	
	//!Value of attribute
	DString		m_strValue;

	bool operator==(const AnimAttrValue& attr) const
	{
		return (m_iAttrID == attr.m_iAttrID);
	}
	bool operator<(const AnimAttrValue& attr) const
	{
		return (m_iAttrID < attr.m_iAttrID);
	}
	bool operator>(const AnimAttrValue& attr) const
	{
		return (m_iAttrID > attr.m_iAttrID);
	}
	AnimAttrValue():
		m_iAttrID(0),
		m_strValue(NULL)
	{
	}
	AnimAttrValue(uint attrid, const char* value = NULL):
		m_iAttrID(attrid),
		m_strValue(value)
	{
	}
};

/*!
Additional info for animation, usualy defined
in Animset. Contains:
1.collection of animation attributes
2.animhooks
3.framerate
4.blend-in/blend-out times
...
Used internally, created by AnimSet
*/
class PUBLIC_API AnimInfo
{
private:
	friend class Skeleton;
	friend class AnimationSystem;
	friend class Animset;

	Set<AnimAttrValue>	m_Attributes;
	Set<AnimHook>			m_Hooks;
	float						m_fFPS;			///<Default framerate
	float						m_fBlendIn;		///<Default time for blending in
	float						m_fBlendOut;	///<Default time for blending out

	/*!
	Internal API for loader. Sets all initialized animhooks at once
	*/
	void SetAnimHooks(const Array<AnimHook>& animhooks);

protected:
	AnimInfo():
		m_fFPS(0.0f),
		m_fBlendIn(0.0f),
		m_fBlendOut(0.0f)
	{
	}

public:
	ENF_DECLARE_SMALLOBJECT(AnimInfo);

/** \name Default parameters
*/
//@{
	void SetFPS(float fps);
	float GetFPS() const;
	void SetBlendIn(float blendin);
	float GetBlendIn() const;
	void SetBlendOut(float blendout);
	float GetBlendOut() const;
//@}

/** \name Animation hooks methods
	See enf::AnimHook
*/
//@{
	//!Returns set of hooks
	const Set<AnimHook>& GetAnimHooks() const
	{
		return m_Hooks;
	}

	/*!
	Adds animhook
	\param def
	AnimsetDef used for this definition
	\param frame
	On what frame is animhook invoked
	\param actionindex
	Index of action to be invoked (See AnimsetDef::GetActionIndex)
	\param param
	Parameter for the action
	\return
	Index in set of animhooks (this index may change by every modification of the set!)
	*/
	uint AddAnimHook(const AnimsetDef* def, float frame, uint actionindex, CStr param);

	/*!
	Return animhook
	\param index
	Index of animhook in set
	\return
	The animhook
	*/
	const AnimHook& GetAnimHook(uint index) const
	{
		return m_Hooks[index];
	}
	/*!
	Removes animhook
	\param index
	Index of animhook in set
	*/
	void RemoveAnimHook(uint index)
	{
		m_Hooks.RemoveElement(index);
	}
//@}

/** \name Animation attribute methods
	See enf::AnimAttrValue
*/
//@{
	/*!
	Removes attribute
	\param attrindex
	Attribute index (See AnimsetDef::GetAttributeIndex)
	*/
	void RemoveAttribute(uint attrindex);

	/*!
	Tells whether is attribute value set
	\param attrindex
	Attribute index (See AnimsetDef::GetAttributeIndex)
	\return
	True when set, false otherwise
	*/
	bool IsAttributeSet(uint attrindex) const;
	
	/*!
	Returns value of attribute
	\param attrindex
	Attribute index (See AnimsetDef::GetAttributeIndex)
	\return
	Value of the attribute, or NULL when not set
	*/
	CStr GetAttributeValue(uint attrindex) const;

	/*!
	Sets value of attribute
	\param attrindex
	Attribute index (See AnimsetDef::GetAttributeIndex)
	\param value
	Value of the attribute
	*/
	void SetAttributeValue(uint attrindex, CStr value);

	//!Returns set of attributes
	const Set<AnimAttrValue>& GetAttributes() const
	{
		return m_Attributes;
	}
//@}
};

//@}
};
#endif //ENF_ANIMATION_H