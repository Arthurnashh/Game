//-----------------------------------------------------------------------------
// File: enf_event.h
//
// Desc: Superclass for every event inside Enforce
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_EVENT_H
#define ENF_EVENT_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "engine/enf_item.h"

namespace enf
{
/** @addtogroup Engine
@{*/

const uint EventID					= 20000;

enum EventTypes
{
	EventTypeID = EventID + 1,
	EntityEventTypeID,
	ReloadShadersEventTypeID,
	BeforeResetEventTypeID,
	AfterResetEventTypeID,
	LoadWorldProgressEventTypeID,
	ConsoleEventTypeID,
	KeyEventTypeID,
	KeyPressEventTypeID,
	KeyReleaseEventTypeID,
	MouseMoveEventTypeID,
	MousePositionEventTypeID,
	MouseButtonEventTypeID,
	MouseButtonPressEventTypeID,
	MouseButtonReleaseEventTypeID,
	MouseWheelEventTypeID,
	CharEventTypeID,
	ActionEventTypeID,
	ActionPressEventTypeID,
	ActionReleaseEventTypeID,
	WorldEventTypeID,
	WorldBeforeRenderEventTypeID,
	WorldAfterRenderEventTypeID,
	WorldAfterVisibilityEventTypeID,
	WorldAfterPostProcessEventTypeID,
	WorldOnRenderEventTypeID,
	AfterRenderEventTypeID,
	CrashLogEventTypeID,
	XContentDeviceEventTypeID,
	SignStatusEventTypeID,
	UserSettingsChangedEventTypeID,
	StorageChangedEventTypeID,
	SetPausedEventTypeID,
	TerminationEventTypeID,
};

class PUBLIC_API Event: public BaseItem
{
public:
	ENF_DECLARE_ITEM(Event, BaseItem);
	virtual ~Event()
	{
	}
};

class PUBLIC_API EventHandler
{
public:
	virtual ~EventHandler() {}
	virtual int HandleEvent(const Event& event){ return 0;}
};
//@}
}
#endif