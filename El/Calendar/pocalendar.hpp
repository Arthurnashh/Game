#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   pocalendar.hpp
    @brief  Calendar of discrete-simulation type.

    Copyright &copy; 2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  15.4.2002
    @date   2.7.2002
*/

#ifndef _POCALENDAR_H
#define _POCALENDAR_H

//-------------------------------------------------------------------------
//  Support:

/**
    Event types - for call-back use only.
*/
enum CalEventType {
    cetNone,                                ///< Common (inactive) state.
    cetTimeout,                             ///< Regular event timeout.
    cetCancel,                              ///< The event was cancelled.
    cetMove,                                ///< The event is being moved.
    cetDestruct                             ///< The event (and even the whole calendar) is destructed.
    };

#define EV_ROBUST_FLAG              0x0001  ///< Event must be signaled in every case (even if canceled).
#define EV_REVOLVING_FLAG           0x0002  ///< Event is revolved with period 'revolvingTime'.
/**
    All event states (even the control ones) are signaled.
    Used only for semaphore-based events, callback-based events are signaled in all states.
*/
#define EV_CONTROL_FLAG             0x0004

#define TIME_EPSILON                 10000  ///< Default time granularity in microseconds.
#define WAIT_BATCH                       5  ///< Default time-batch size (in intervals of size 'epsilon').

#define EV_TYPE_NULL                     0  ///< Dummy event type.
#define EV_TYPE_REGISTERED      0x80000000  ///< Flag for registered event types.

//-------------------------------------------------------------------------
//  CalendarEvent:

class CalendarEvent : public RefCountSafe {

protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

    /**
        Standard constructor.
        Initializes all members to safe values.
    */
    CalendarEvent ();

    /**
        Event trigger.
        Must be called inside the enter() lock.
    */
    virtual void fire ();

public:

    unsigned64 time;                        ///< Trigger time in microseconds (see getSystemTime()).

    unsigned64 timeDelta;                   ///< Time difference between the actual time and wanted (trigger) time. In Microseconds.

    CalEventType status;                    ///< The actual event status.

    unsigned userType;                      ///< User type of the event.
                                            ///< Either registered (PoCalendar::registerEvent()) or statically defined.
    int dataInt;                            ///< Integer (user) data associated with the event.

    void *dataPtr;                          ///< Pointer (user) data associated with the event.

    unsigned flags;                         ///< Event flags. See EV_*_FLAG constants.

    unsigned64 revolvingTime;               ///< Revolving time in microseconds (EV_REVOLVING_FLAG).

    Ref<CalendarEvent> next;                ///< Double-linked list of events.

    Ref<CalendarEvent> prev;                ///< Double-linked list of events.

    /**
        Fires the given event.
        @result <code>true</code> if the event should be re-inserted into the calendar queue again.
    */
    bool fire ( CalEventType st );

    /**
        Cancels this event.
        fire(cetCancel) should be called.
    */
    virtual void cancel ();

    virtual ~CalendarEvent ();

    inline void unlink ()
    {
        prev->next = next;
        next->prev = prev;
        prev = NULL;
        next = NULL;
    }

    };

//-------------------------------------------------------------------------
//  CalendarEventCallback:

#include <Es/Memory/normalNew.hpp>

/**
    Call-back function for PoCalendar.
    @param  ev EVent the routine is called for (all info is stored here, including state, associated data, etc.).
*/
typedef void CalendarCallback ( CalendarEvent &ev );

class CalendarEventCallback : public CalendarEvent {

protected:

    CalendarCallback *callback;             ///< Actual call-back function or NULL.

    /**
        Event trigger.
        Must be called inside the enter() lock.
    */
    virtual void fire ();

public:

    CalendarEventCallback ( CalendarCallback *cb );

    virtual ~CalendarEventCallback ();

    /**
        MT-safe new operator.
    */
    static void* operator new ( size_t size );

    static void* operator new ( size_t size, const char *file, int line );

    /**
        MT-safe delete operator.
    */
    static void operator delete ( void *mem );

#ifdef __INTEL_COMPILER

    /**
        Intel compiler needs this for exception unwind..
    */
    static void operator delete ( void *mem, const char *file, int line );

#endif

    };

//-------------------------------------------------------------------------
//  CalendarEventSemaphore:

class CalendarEventSemaphore : public CalendarEvent {

protected:

    PoSemaphore *sem;                       ///< Simple semaphore or NULL.

    /**
        Event trigger.
        Must be called inside the enter() lock.
    */
    virtual void fire ();

public:

    CalendarEventSemaphore ( PoSemaphore *s );

    virtual ~CalendarEventSemaphore ();

    /**
        MT-safe new operator.
    */
    static void* operator new ( size_t size );

    static void* operator new ( size_t size, const char *file, int line );

    /**
        MT-safe delete operator.
    */
    static void operator delete ( void *mem );

#ifdef __INTEL_COMPILER

    /**
        Intel compiler needs this for exception unwind..
    */
    static void operator delete ( void *mem, const char *file, int line );

#endif

    };

//-------------------------------------------------------------------------
//  CalendarEventSemaphorePtr:

class CalendarEventSemaphorePtr : public CalendarEvent {

protected:

    PoSemaphoreTitbit<void*> *sem;          ///< Titbit-semaphore (pointer) or NULL.

    /**
        Event trigger.
        Must be called inside the enter() lock.
    */
    virtual void fire ();

public:

    CalendarEventSemaphorePtr ( PoSemaphoreTitbit<void*> *s );

    virtual ~CalendarEventSemaphorePtr ();

    /**
        MT-safe new operator.
    */
    static void* operator new ( size_t size );

    static void* operator new ( size_t size, const char *file, int line );

    /**
        MT-safe delete operator.
    */
    static void operator delete ( void *mem );

#ifdef __INTEL_COMPILER

    /**
        Intel compiler needs this for exception unwind..
    */
    static void operator delete ( void *mem, const char *file, int line );

#endif

    };

//-------------------------------------------------------------------------
//  PoCalendar:

#include <Es/Memory/debugNew.hpp>

class PoCalendar : public RefCountSafe {
protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

public:

    /**
        Starts operation of this calendar.
        Creates a special thread dedicated to asynchronous event handling.
        @param  eps Time granularity in microseconds.
        @param  bat Time-batch size (number of 'eps' intervals).
    */
    PoCalendar ( unsigned64 eps =TIME_EPSILON, unsigned bat =WAIT_BATCH );

    /**
        Inserts a new event into the calendar.
    */
    void insertEvent ( CalendarEvent *ev );

    /**
        Removes the not-processed-yet event from the calendar.
        The 'cetCancel' state is signaled for the given event.
    */
    void removeEvent ( CalendarEvent *ev );

    /**
        Moves the not-processed-yet event to the new time.
        The 'cetMove' state is signaled for the given event.
    */
    void moveEvent ( CalendarEvent *ev, unsigned64 newTime );

    /**
        Removes all active events (using 'removeEvent()').
    */
    void removeAllEvents ();

    /**
        Finds the first event with the given 'userType'.
    */
    Ref<CalendarEvent> findEvent ( unsigned uType ) const;

    /**
        Finds the first event with the given 'dataInt' value (and 'userType' if specified).
    */
    Ref<CalendarEvent> findEvent ( int data, unsigned uType =EV_TYPE_NULL ) const;

    /**
        Finds the first event with the given 'dataPtr' value (and 'userType' if specified).
    */
    Ref<CalendarEvent> findEvent ( void* data, unsigned uType =EV_TYPE_NULL ) const;

    /**
        Removes all events with the given 'userType'.
        @return Number of removed events.
    */
    unsigned removeEvents ( unsigned uType );

    /**
        Stops operation of this calendar.
        All the active events will be canceled.
    */
    void close ();

    ~PoCalendar ();

    //--- event-type registration ---------------------------------------------

    /// Register new user type of calendar events.
    unsigned registerType ();

protected:

    friend THREAD_PROC_RETURN THREAD_PROC_MODE calendarOperation ( void *param );

    bool operating;                         ///< The calendar is in operating (active) state. Used to quit the dedicated thread.

    ThreadId thread;                        ///< Thread dedicated to asynchronous calendar operations.

    Ref<CalendarEvent> head;                ///< Head of the double-linked list (queue).

    unsigned64 epsilon;                     ///< Time granularity (minimum amount to wait for) in microseconds.

    unsigned64 roundEps;                    ///< Half of the epsilon.

    unsigned batch;                         ///< Size of time-batch used to wait for the closest event. 1 .. the finest (exact) time slicing.

    void removeEventUnsafe ( CalendarEvent *ev );

    unsigned nextType;                      ///< Next assigned user type.

    };

#endif
