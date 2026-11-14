/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once



#include <assert.h>
#include "float.h"
#include <bitset>

//
// This header requires that it includes no smEventGeneratorTrait derived
// classes.
//
#include "Util/smMacros.h"
#include "Util/smUtil.h"
#include "Physics/smPrecision.h"
#include "Physics/smOperatorMacros.h"
#include "Util/smManager.h"
#include "Util/smNamed.h"

/**
*	This class is designed to be used as a base class.
*	It shall always be possible to use this class as a trait(interface) with multiple
*	inheritance
*/
class smEventGeneratorTrait : public smNamed
{
public:
    /** Possible event types    */
    enum EventType
    {
        CONSTRUCTED,
        VALUE_CHANGED,
        NAME_CHANGED,

        NO_EVENT_TYPES
    };

	/**
	*	A mask type which can be used to filter the generated images.
	*	Using this mask it is possible to control which events will be generated
	*	for this trait and which will be not generated.
	*
	*	By default all events are generated.
	*/
	typedef	std::bitset<NO_EVENT_TYPES>		EventMask;

    /**
    *	This event handler is called everytime an event it posted from the smEventGeneratorTrait.
    *	The functor objects are called immediately and in the order they have been added.
    *   To get events from an smEventGeneratorTrait, derive a new class from Event class
    *	override operator()
    */
    class EventHandler
    {
    public:
        /**
        *   Derived classes shall re-implement this method
        */
        virtual void	handleEvent( smEventGeneratorTrait*, EventType /*type*/ ) {} ;
    };

public:
    explicit smEventGeneratorTrait()
    {
		// By default, all kinds of messages are generated
		m_eventMask.set() ;

        autoAddEventHandler() ;
        callEventHandlers( CONSTRUCTED ) ;
    }

    /** */
    virtual					~smEventGeneratorTrait()
    {
    }

	/** Re-implemented from smNamed to send NAME_CHANGED event.	*/
	virtual void setName( const std::string& name )
	{
		smNamed::setName( name ) ;
		callEventHandlers( NAME_CHANGED ) ;
	}

    /** */
    virtual void			addEventHandler( EventHandler* functor ) ;

    /** Remove all event handlers at once.		*/
    virtual void			removeEventHandlers()
    { m_eventHandlerObjects.removeAll() ; }

    /** Remove all event handlers at once.		*/
    virtual void			removeEventHandler( EventHandler* eh )
    { m_eventHandlerObjects.remove( eh ) ; }

    /** */
    virtual void			callEventHandlers( EventType type ) ;

	void	setEventMask( EventMask mask )
	{ m_eventMask = mask; }

    static void             addAutoEventHandler( EventHandler* handler )
    { s_autoEventHandlers.add( handler ) ; }

    static void             removeAllAutoEventHandlers( )
    { s_autoEventHandlers.removeAll() ; }

protected:
    void    autoAddEventHandler() ;

protected:
    /** List of handlers which will be called if an event happens.  */
    smManager<EventHandler>		m_eventHandlerObjects ;

    /** List of handlers which will be added to this event generator automatically during construction. */
    static smManager<EventHandler>    s_autoEventHandlers ;

	/** See EventMask	*/
	EventMask					m_eventMask ;
};

