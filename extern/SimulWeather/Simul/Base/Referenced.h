#ifndef SIMUL_BASE_REFERENCED
#define SIMUL_BASE_REFERENCED

#include "Simul/Base/Export.h"
#include <list>

namespace simul
{
	namespace base
	{
		SIMUL_BASE_EXPORT_CLASS Observer;
		//! Base class for classes that use reference-counting. This class handles the reference counts
		//! and deletes the instance when the count reaches zero. Designed to be used with
		//! simul::base::SmartPtr.
		SIMUL_BASE_EXPORT_CLASS Referenced
		{
			void DeleteObservers();
			public:
				Referenced();
				Referenced(const Referenced&){}
				inline Referenced& operator = (const Referenced&) { return *this; }

				//! Increment the reference count by one.
				inline void intrusive_ptr_add_ref() const;
		        
				//! Decrement the reference count by one, delete if zero.
				inline void intrusive_ptr_release()
				{
					bool needDelete=false;
					--_refCount;
					needDelete=_refCount<=0;
					if(needDelete)
					{
						this->PrepareToDestroy();
						delete this;
					}
				}
		        
				//! Return the number of pointers referencing this object.
				inline int referenceCount() const
				{
					return _refCount;
				}
				//! Add an Observer that is observing this object, notify the Observer when this object gets deleted.
				void AddObserver(Observer* observer);

				//! Add an Observer that is observing this object, notify the Observer when this object gets deleted.
				void RemoveObserver(Observer* observer);
			protected:
				virtual ~Referenced()
				{
					DeleteObservers();
				}
		//! Enact all destruction code that needs to rely on the derived class's virtual functions
		//! because destructors can't use virtuals properly.
				virtual void PrepareToDestroy()
				{
				}
				mutable int _refCount;
				void*                   observers;
		};

		inline void Referenced::intrusive_ptr_add_ref() const
		{
			++_refCount;
		}


	}
}

inline void intrusive_ptr_add_ref( simul::base::Referenced* r)
{
	r->intrusive_ptr_add_ref();
}
inline void intrusive_ptr_release( simul::base::Referenced* r)
{
	r->intrusive_ptr_release();
}
#endif
