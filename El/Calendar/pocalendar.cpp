/**
    @file   pocalendar.cpp
    @brief  Calendar of discrete-simulation type.

    Copyright &copy; 2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  23.3.2002
    @date   19.9.2002
*/

#include <El/elementpch.hpp>

#include <Es/Common/global.hpp>
#ifdef _WIN32
#include <Es/common/win.h>
#endif

#include <Es/Threads/pocritical.hpp>
#include <Es/Threads/pothread.hpp>
#include <Es/Threads/posemaphore.hpp>

#include <Es/Framework/potime.hpp>

#include "pocalendar.hpp"

//-------------------------------------------------------------------------
//  CalendarEvent:

CalendarEvent::CalendarEvent ()
{
    time = timeDelta = revolvingTime = 0;
    status = cetNone;
    userType = EV_TYPE_NULL;
    dataInt = 0;
    dataPtr = NULL;
    flags = EV_ROBUST_FLAG;
    next = NULL;
    prev = NULL;
}

void CalendarEvent::fire ()
{
}

bool CalendarEvent::fire ( CalEventType st )
{
    enter();
        // event preprocessing:
    status = st;
    unsigned64 now = getSystemTime();
    timeDelta = now - time;
        // the fire action itself:
    fire();
        // event postprocessing:
    bool result = false;
    if ( st == cetTimeout &&
         (flags & EV_REVOLVING_FLAG) ) {    // revolving event => fire it again in 'revolvingTime'
        time = now + revolvingTime;
        result = true;                      // re-insert it again
        }
    leave();
    return result;
}

void CalendarEvent::cancel ()
{
    if ( flags & EV_ROBUST_FLAG )
        fire(cetCancel);
}

CalendarEvent::~CalendarEvent ()
{
    enter();
    status = cetDestruct;
    dataPtr = NULL;
    next = NULL;
    prev = NULL;
    leave();
}

//-------------------------------------------------------------------------
//  CalendarEventCallback:

CalendarEventCallback::CalendarEventCallback ( CalendarCallback *cb )
{
    LockRegister(lock,"CalendarEventCallback");
    callback = cb;
}

#include <Es/Memory/normalNew.hpp>

void* CalendarEventCallback::operator new ( size_t size )
{
    return safeNew(size);
}

void* CalendarEventCallback::operator new ( size_t size, const char *file, int line )
{
    return safeNew(size);
}

void CalendarEventCallback::operator delete ( void *mem )
{
    safeDelete(mem);
}

#ifdef __INTEL_COMPILER

void CalendarEventCallback::operator delete ( void *mem, const char *file, int line )
{
    safeDelete(mem);
}

#endif

#include <Es/Memory/debugNew.hpp>

void CalendarEventCallback::fire ()
{
    if ( callback )
        (*callback)(*this);
}

CalendarEventCallback::~CalendarEventCallback ()
{
    enter();
    if ( status != cetDestruct &&
         (flags & EV_ROBUST_FLAG) )
        CalendarEvent::fire(cetDestruct);
    callback = NULL;
    leave();
}

//-------------------------------------------------------------------------
//  CalendarEventSemaphore:

CalendarEventSemaphore::CalendarEventSemaphore ( PoSemaphore *s )
{
    LockRegister(lock,"CalendarEventSemaphore");
    sem = s;
}

#include <Es/Memory/normalNew.hpp>

void* CalendarEventSemaphore::operator new ( size_t size )
{
    return safeNew(size);
}

void* CalendarEventSemaphore::operator new ( size_t size, const char *file, int line )
{
    return safeNew(size);
}

void CalendarEventSemaphore::operator delete ( void *mem )
{
    safeDelete(mem);
}

#ifdef __INTEL_COMPILER

void CalendarEventSemaphore::operator delete ( void *mem, const char *file, int line )
{
    safeDelete(mem);
}

#endif

#include <Es/Memory/debugNew.hpp>

void CalendarEventSemaphore::fire ()
{
    if ( sem &&
         (status == cetTimeout || (flags & EV_CONTROL_FLAG)) )
        sem->signal();
}

CalendarEventSemaphore::~CalendarEventSemaphore ()
{
    enter();
    if ( status != cetDestruct &&
         (flags & EV_ROBUST_FLAG) )
        CalendarEvent::fire(cetDestruct);
    sem = NULL;
    leave();
}

//-------------------------------------------------------------------------
//  CalendarEventSemaphorePtr:

CalendarEventSemaphorePtr::CalendarEventSemaphorePtr ( PoSemaphoreTitbit<void*> *s )
{
    LockRegister(lock,"CalendarEventSemaphorePtr");
    sem = s;
}

#include <Es/Memory/normalNew.hpp>

void* CalendarEventSemaphorePtr::operator new ( size_t size )
{
    return safeNew(size);
}

void* CalendarEventSemaphorePtr::operator new ( size_t size, const char *file, int line )
{
    return safeNew(size);
}

void CalendarEventSemaphorePtr::operator delete ( void *mem )
{
    safeDelete(mem);
}

#ifdef __INTEL_COMPILER

void CalendarEventSemaphorePtr::operator delete ( void *mem, const char *file, int line )
{
    safeDelete(mem);
}

#endif

#include <Es/Memory/debugNew.hpp>

void CalendarEventSemaphorePtr::fire ()
{
    if ( sem &&
         (status == cetTimeout || (flags & EV_CONTROL_FLAG)) )
        sem->signal(dataPtr);
}

CalendarEventSemaphorePtr::~CalendarEventSemaphorePtr ()
{
    enter();
    if ( status != cetDestruct &&
         (flags & EV_ROBUST_FLAG) )
        CalendarEvent::fire(cetDestruct);
    sem = NULL;
    leave();
}

//-------------------------------------------------------------------------
//  PoCalendar:

THREAD_PROC_RETURN THREAD_PROC_MODE calendarOperation ( void *param )
{
    PoCalendar *cal = (PoCalendar*)param;
    Assert( cal );
    CalendarEvent *head = cal->head;        // won't change anyway
    Assert( head );
    unsigned64 now;
    Ref<CalendarEvent> event;               // the nearest event

    while ( cal->operating ) {

        cal->enter();
        do {                                // check the nearest event:
            now = getSystemTime();          // actual time
            event = head->next;             // the nearest event
            if ( event != cal->head &&
                 event->time < now + cal->roundEps ) {
                    // fire it just now..
                event->unlink();
                cal->leave();
                if ( event->fire(cetTimeout) ) // re-insert it
                    cal->insertEvent(event);
                cal->enter();
                }
            else
                break;                      // I need a little sleep..
            } while ( true );
            // event == the nearest event or 'head'

            // microseconds to sleep (if I haven't better things to do..):
        unsigned64 sleepUs = cal->batch * cal->epsilon;
            // look for the first event:
        if ( event != head &&
             now + sleepUs >= event->time - cal->roundEps ) {
                // wait the whole interval to the first event:
            sleepUs = event->time - now;
            }
        cal->leave();                       // give the event-queue chance to be updated..

        SLEEP_MS( sleepUs / 1000 );

        }

    return (THREAD_PROC_RETURN)0;
}

PoCalendar::PoCalendar ( unsigned64 eps, unsigned bat )
{
    LockRegister(lock,"PoCalendar");
    epsilon = (eps < 1000) ? 1000 : eps;
    roundEps = epsilon >> 1;
    batch = (bat < 1) ? 1 : bat;
    head = new CalendarEventCallback(NULL);
    head->next = head;
    head->prev = head;
    head->time = ((unsigned64)UINT_MAX) << 32;
    nextType = 0;
    operating = true;
    if ( !poThreadCreate(&thread,64*1024,&calendarOperation,this) )
        operating = false;
}

void PoCalendar::insertEvent ( CalendarEvent *ev )
{
    if ( !ev ) return;
    if ( (ev->flags & EV_REVOLVING_FLAG) &&
         ev->revolvingTime < epsilon )
        ev->revolvingTime = epsilon;
  again:
    enter();
        // check if the event can be fired immediately:
    unsigned64 now = getSystemTime();
    if ( ev->time < now + roundEps &&
         head->next->time > ev->time ) {
            // fire it immediately and exit..
        leave();
        if ( ev->fire(cetTimeout) )
            goto again;
        return;
        }
        // insert the event into the queue:
    CalendarEvent *ptr = head->next;
    while ( ptr->time <= ev->time ) ptr = ptr->next;
        // insert 'ev' before 'ptr':
    ev->prev = ptr->prev;
    ev->next = ptr;
    ptr->prev->next = ev;
    ptr->prev = ev;
    leave();
}

void PoCalendar::removeEvent ( CalendarEvent *ev )
{
    if ( !ev ) return;
    enter();
    CalendarEvent *tmp = head->next;
    while ( tmp != head && tmp != ev )
        tmp = tmp->next;
    if ( tmp == ev ) {
        ev->unlink();
        ev->cancel();
        }
    leave();
}

void PoCalendar::removeEventUnsafe ( CalendarEvent *ev )
{
    ev->unlink();
    ev->cancel();
}

void PoCalendar::removeAllEvents ()
{
    enter();
    while ( head->next != head )
        removeEventUnsafe(head->next);
    leave();
}

void PoCalendar::moveEvent ( CalendarEvent *ev, unsigned64 newTime )
{
    Assert( false );                        // !!! not implemented yet !!!
}

Ref<CalendarEvent> PoCalendar::findEvent ( unsigned uType ) const
{
    if ( uType == EV_TYPE_NULL ) return NULL;
    enter();
    Ref<CalendarEvent> ptr = head->next;
    head->userType = uType;
    while ( ptr->userType != uType ) ptr = ptr->next;
    leave();
    return( (ptr == head) ? (Ref<CalendarEvent>)NULL : ptr );
}

Ref<CalendarEvent> PoCalendar::findEvent ( int data, unsigned uType ) const
{
    enter();
    Ref<CalendarEvent> ptr = head->next;
    head->dataInt  = data;
    head->userType = uType;
    if ( uType == EV_TYPE_NULL )            // check the dataInt item only
        while ( ptr->dataInt != data ) ptr = ptr->next;
    else                                    // check both userType and dataInt
        while ( ptr->dataInt != data || ptr->userType != uType ) ptr = ptr->next;
    leave();
    return( (ptr == head) ? (Ref<CalendarEvent>)NULL : ptr );
}

Ref<CalendarEvent> PoCalendar::findEvent ( void* data, unsigned uType ) const
{
    enter();
    Ref<CalendarEvent> ptr = head->next;
    head->dataPtr  = data;
    head->userType = uType;
    if ( uType == EV_TYPE_NULL )            // check the dataPtr item only
        while ( ptr->dataPtr != data ) ptr = ptr->next;
    else                                    // check both userType and dataPtr
        while ( ptr->dataPtr != data || ptr->userType != uType ) ptr = ptr->next;
    leave();
    return( (ptr == head) ? (Ref<CalendarEvent>)NULL : ptr );
}

unsigned PoCalendar::removeEvents ( unsigned uType )
{
	unsigned count = 0;
	enter();
    Ref<CalendarEvent> ptr = head->next;
    head->userType = uType;
	do {									// find & remove one matching event
		while ( ptr->userType != uType ) ptr = ptr->next;
		if ( ptr == head ) break;
		count++;
		CalendarEvent *rem = ptr;
		ptr = rem->next;
		removeEventUnsafe(rem);
		} while ( true );
	leave();
	return count;
}

void PoCalendar::close ()
{
    if ( operating ) {
        operating = false;
        Verify( poThreadJoin(thread,NULL) );
        }
    removeAllEvents();
}

PoCalendar::~PoCalendar ()
{
    close();
    enter();
    head->next = NULL;
    head->prev = NULL;
    head = NULL;
    leave();
}

//-------------------------------------------------------------------------
//  Event-type registration:

unsigned PoCalendar::registerType ()
{
    enter();
    unsigned result = nextType++;
    leave();
    return( result | EV_TYPE_REGISTERED );
}
