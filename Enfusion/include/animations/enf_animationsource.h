//-----------------------------------------------------------------------------
// File: vobject/enf_animationsource.h
//
// Desc: Animation source
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ANIMATIONSOURCE_H
#define ENF_ANIMATIONSOURCE_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_mathlib.h"

namespace enf
{
/** @addtogroup Animations
@{*/
//-----------------------------------------------------------------------------
class PUBLIC_API AnimChannelSource
{
private:
	friend class Animation;
	friend class AnimationSource;

	DString			m_strName;
	Array<QKey>		m_Keys;
	Array<ushort>	m_Indices;

public:
	ENF_DECLARE_SMALLOBJECT(AnimChannelSource);
	AnimChannelSource(CStr name): m_strName(name) {}
	void AddKeyframe(const QKey& key, uint index)
	{
		m_Keys.Insert(key);
		m_Indices.Insert(index);
	}
	void Reserve(uint size)
	{
		m_Keys.Reserve(size);
		m_Indices.Reserve(size);
	}
};

//-----------------------------------------------------------------------------
class PUBLIC_API AnimationSource
{
protected:
	friend class Animation;
	Array<AnimChannelSource*>	m_Channels;

public:
	ENF_DECLARE_SMALLOBJECT(AnimationSource);
	AnimationSource() {}
	~AnimationSource();
	AnimChannelSource* AddChannel(CStr name);
};

//@}
}

#endif //ENF_ANIMATIONSOURCE_H