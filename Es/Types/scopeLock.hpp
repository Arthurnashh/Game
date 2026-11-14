#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SCOPE_LOCK_HPP
#define _SCOPE_LOCK_HPP

/*!
	\file
	Scope locking related templates
*/

/// define scope locking properties
template <class Type>
struct ScopeTraits
{
  static void Lock(Type &item) {item.Lock();}
  static void Unlock(Type &item) {item.Unlock();}
};

//! lock object passed as argument during lifetime of ScopeLock object
template<class Lock, class Traits=ScopeTraits<Lock> >
class ScopeLock: private NoCopy
{
	protected:
	Lock &_lock;
	
	public:
	ScopeLock(Lock &lock ):_lock(lock){Traits::Lock(_lock);}
	/// copy constructor needed - we need to copy the value somehow
	/** we could transfer ownership instead (unlock source, lock destination)
	*/
	ScopeLock(const ScopeLock &scope):_lock(scope._lock){Traits::Lock(_lock);}
	~ScopeLock() {Traits::Unlock(_lock);}
};

//! unlock object passed as argument during lifetime of ScopeLock object
template<class Lock, class Traits=ScopeTraits<Lock> >
class ScopeUnlock: private NoCopy
{
	protected:
	Lock &_lock;
	
	public:
	ScopeUnlock(Lock &lock):_lock(lock){Traits::Unlock(_lock);}
	ScopeUnlock(const ScopeUnlock &scope):_lock(scope._lock){Traits::Unlock(_lock);}
	~ScopeUnlock() {Traits::Lock(_lock);}
};

#endif
