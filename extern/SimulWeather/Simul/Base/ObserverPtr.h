#ifndef SIMUL_BASE_OBSERVERPTR_H
#define SIMUL_BASE_OBSERVERPTR_H
#include "Simul/Base/Export.h"
#include "Simul/Base/Referenced.h"
#ifdef _MSC_VER	// Using Visual Studio? Then this pragma works:

# pragma warning(push)
# pragma warning(disable:4284) // odd return type for operator->
#endif

namespace simul
{
	namespace base
	{
		SIMUL_BASE_EXPORT_CLASS Referenced;
		SIMUL_BASE_EXPORT_CLASS Observer
		{
		public:
			virtual ~Observer() {}
			virtual void objectDeleted(Referenced*) {}
		};
	
		//! \internal Smart pointer for observed objects, which automatically set pointers to them to null when they are deleted.
		template<class T> class ObserverPtr : public Observer
		{
		public:
			typedef T element_type;
	
			ObserverPtr() :_ptr(0)                            {}
			ObserverPtr(T* t):_ptr(static_cast<Referenced*>(t))
			{
				if (_ptr)
					_ptr->AddObserver(this);
			}
			ObserverPtr(const ObserverPtr& rp):Observer(), _ptr(rp._ptr)
			{
				if (_ptr)
					_ptr->AddObserver(this);
			}
			~ObserverPtr()
			{
				if (_ptr)
					_ptr->RemoveObserver(this);
				_ptr=0;
			}
	
			inline ObserverPtr& operator = (const ObserverPtr& rp)
			{
				if(_ptr==rp._ptr)
					return *this;
				if(_ptr)
					_ptr->RemoveObserver(this);
				
				_ptr=rp._ptr;
				if(_ptr)
					_ptr->AddObserver(this);
				return *this;
			}
	
			inline ObserverPtr& operator=(T* t)
			{
				Referenced *ptr=static_cast<Referenced*>(t);
				if (_ptr==ptr) return *this;
				if (_ptr) _ptr->RemoveObserver(this);
				
				_ptr = ptr;
				if (_ptr) _ptr->AddObserver(this);
	
				return *this;
			}
			
			virtual void objectDeleted(Referenced* ptr)
			{
				if (_ptr==ptr)
				{
					_ptr = 0;
				}
			}
	
			// comparison operators for ObserverPtr.
			inline bool operator == (const ObserverPtr& rp) const { return (_ptr==rp._ptr); }
			inline bool operator != (const ObserverPtr& rp) const { return (_ptr!=rp._ptr); }
			inline bool operator < (const ObserverPtr& rp) const { return (_ptr<rp._ptr); }
			inline bool operator > (const ObserverPtr& rp) const { return (_ptr>rp._ptr); }
	
			// comparison operator for const T*.
			inline bool operator == (const T* ptr) const { return (_ptr==ptr); }
			inline bool operator != (const T* ptr) const { return (_ptr!=ptr); }
			inline bool operator < (const T* ptr) const { return (_ptr<ptr); }
			inline bool operator > (const T* ptr) const { return (_ptr>ptr); }
	
	
			inline T& operator*()  { return *((T*)_ptr); }
	
			inline const T& operator*() const { return *((const T*)_ptr); }
	
			inline T* operator->() { return (T*)_ptr; }
	
			inline const T* operator->() const   { return (T*)_ptr; }
	
			inline bool operator!() const   { return _ptr==0L; }
	
			inline bool valid() const       { return _ptr!=0L; }
			
			inline T* get() { return (T*)_ptr; }
	
			inline const T* get() const { return (const T*)_ptr; }
	
		private:
			Referenced* _ptr;
		};
	} // namespace simul
}//namespace base
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif
