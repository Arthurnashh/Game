//-----------------------------------------------------------------------------
// File: enf_xsoundsource.h
//
// Desc: XSoundSource interface
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_XSOUNDSOURCE_H
#define ENF_XSOUNDSOURCE_H

#include "enf_config.h"
#include "common/enf_mathlib.h"

namespace enf
{
/** @addtogroup XSoundSystem
@{*/

typedef unsigned int SoundHandle;

const SoundHandle SOUND_NO_SHADER = 0xffffffff;
const SoundHandle SOUND_NOT_PLAYING = 0;

ENF_INLINEFUNC bool IsSoundHandleOK(SoundHandle handle)
{
	return (handle != SOUND_NO_SHADER && handle != SOUND_NOT_PLAYING);
}

class PUBLIC_API XEnvironmentVolume
{
protected:
	XEnvironmentVolume* m_pChildren;
	XEnvironmentVolume* m_pSibling;
	XEnvironmentVolume* m_pParent;
	uint	m_iIndex;

public:
	uint GetEnvironment() const
	{
		return m_iIndex;
	}
	void SetEnvironment(uint index) { m_iIndex = index; }

	virtual bool IsInside(const Vector3& point) const
	{
		return false;
	}
	XEnvironmentVolume* GetChildren() const { return m_pChildren; }
	XEnvironmentVolume* GetSibling() const { return m_pSibling; }
	XEnvironmentVolume* GetParent() const { return m_pParent; }
	void AddChild(XEnvironmentVolume* child);
	void RemoveChild(XEnvironmentVolume* child);

	XEnvironmentVolume(): m_pChildren(NULL), m_pSibling(NULL), m_pParent(NULL), m_iIndex(INDEX_NOT_FOUND) {}
	virtual ~XEnvironmentVolume() {}
};

class PUBLIC_API XSoundSource
{
protected:
	uint m_iNumSounds;

public:
	XSoundSource(): m_iNumSounds(0)
	{
	}
	virtual ~XSoundSource() {}
	uint AddSoundRef()
	{
		return ++m_iNumSounds;
	}
	uint RemoveSoundRef()
	{
		enf_assert(m_iNumSounds > 0);
		return --m_iNumSounds;
	}
	uint GetNumSoundRefs() const
	{
		return m_iNumSounds;
	}
	virtual Vector3 GetSoundVelocity() const
	{
		return Vector3(0.0f);
	}
	virtual Vector3 GetSoundCenter() const
	{
		return Vector3(0.0f);
	}
	virtual Matrix33 GetSoundOrientation() const
	{
    Matrix33 m;
    m.SetIdentity();
		return m;
	}
	virtual void OnSoundEnd(SoundHandle handle) {}
	//!diag identifier
	virtual CStr GetSourceName() const
	{
		return NULL;
	}
};

//@}
}

#endif //ENF_XSOUNDSOURCE_H
