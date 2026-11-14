#ifdef _MSC_VER
#pragma once
#endif

#ifndef _LLINKS_HPP
#define _LLINKS_HPP

#include <Es/Types/pointers.hpp>
#include <Es/Memory/fastAlloc.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Threads/multisync.hpp>

#include <Es/Memory/normalNew.hpp>

class RemoveLLinks;

class TrackLLinks: public RefCount
{
	mutable RemoveLLinks *_remove;

	public:
	explicit TrackLLinks( RemoveLLinks *obj ) {_remove=obj;}

	~TrackLLinks() {_remove=NULL;}

	__forceinline RemoveLLinks *GetObject() const {return _remove;}
	__forceinline void UnlinkObject() const {_remove=NULL;}

	USE_FAST_ALLOCATOR_ID(TrackLLinks)
};

#include <Es/Memory/debugNew.hpp>

class RemoveLLinks: public RefCount
{
	friend class TrackLLinks;
	mutable RefR<TrackLLinks> _track;

  void DestroyTracker() const
	{
    // make sure the old tracker points to nowhere
    _track->UnlinkObject();
    MemoryPublish();
    // and unbind us from it
    _track.Free();
	}
  void CreateTracker() const
	{
    TrackLLinks *tracker = new TrackLLinks(unconst_cast(this));
    MemoryPublish(); // make sure the tracker is in working condition before we attach to it
    _track = tracker;
		}

	public:
	RemoveLLinks() {CreateTracker();}
	RemoveLLinks( const RemoveLLinks &src ){CreateTracker();}
	void operator =( const RemoveLLinks &src ){} // keep the old tracker untouched - object identity unchanged

	/// make sure all existing links pointing to this object are set to NULL
  inline void DestroyLinkChain() const
	{
    // unbind us from the current tracker
    DestroyTracker();
    // create a new tracker for new LLinks
    CreateTracker();
	}
  /// check no links exist - useful for asserting in caching scenarios
  inline bool CheckNoLinks() const
  {
    // the only reference to the tracker can be the one we have
    return _track->RefCounter()==1;
  }

	~RemoveLLinks() {DestroyTracker();}

	__forceinline TrackLLinks *Track() const {return _track;}
};


#if _DEBUG
	#define COUNT_LLINKS 1
#else
	#define COUNT_LLINKS 0
#endif

extern Ref<TrackLLinks> LLinkNil;

template <class Type>
struct DefLLinkTraits
{
  static __forceinline TrackLLinks *Track(Type *ref) {return ref->Track();}
  static __forceinline RemoveLLinks *GetBase(Type *ref) {return ref;}
  static __forceinline Type *GetObject(TrackLLinks *tracker) {return static_cast<Type *>(tracker->GetObject());}
};

template <class Type, class Traits = DefLLinkTraits<Type> >
class LLink
{
	mutable Ref<TrackLLinks> _tracker;
	#if COUNT_LLINKS
		static int _notNullInstanceCount;
		static int _instanceCount;
	#endif

	public:
  //! make data type accessible
  typedef Type ItemType;

	LLink() : _tracker(LLinkNil)
	{
		#if COUNT_LLINKS
		_instanceCount++;
		#endif
	}
	LLink(const Ref<Type> &ref)
  :_tracker(ref.NotNull() ? Traits::Track(ref) : LLinkNil.GetRef())
	{
		#if COUNT_LLINKS
		if (ref.NotNull())
		{
			_notNullInstanceCount++;
		}
		_instanceCount++;
		#endif
	}
	LLink( Type *ref )
	:_tracker(ref ? Traits::Track(ref) : (TrackLLinks*)LLinkNil.GetRef())
	{
		#if COUNT_LLINKS
		if (ref)
		{
			_notNullInstanceCount++;
		}
		_instanceCount++;
		#endif
	}
	#if COUNT_LLINKS
	LLink(const LLink &src)
	{
		_tracker = src._tracker;
		if (_tracker != LLinkNil)
		{
			_notNullInstanceCount++;
		}
		_instanceCount++;
	}
	const LLink &operator =(const LLink &src)
	{
		if (_tracker != LLinkNil)
		{
			_notNullInstanceCount--;
		}
		_tracker = src._tracker;
		if (_tracker != LLinkNil)
		{
			_notNullInstanceCount++;
		}
		return *this;
	}
	~LLink()
	{
		if (_tracker != LLinkNil)
		{
			_notNullInstanceCount--;
		}
		_instanceCount--;
	}
	#endif

	__forceinline Type *GetLink() const
	{
    return Traits::GetObject(_tracker);
	}
	__forceinline operator Type *() const {return GetLink();}
	__forceinline Type * operator ->() const {return GetLink();}

	__forceinline bool NotNull() const {return Traits::GetObject(_tracker) != NULL;}
	__forceinline bool IsNull() const {return Traits::GetObject(_tracker) == NULL;}

  int CompareTrackers(const LLink &with) const
  {
    if ((intptr_t)_tracker.GetRef() < (intptr_t)with._tracker.GetRef()) return -1;
    if ((intptr_t)_tracker.GetRef() > (intptr_t)with._tracker.GetRef()) return 1;
    return 0;
  }

	ClassIsMovable(LLink);
};

#define TypeContainsLLink TypeIsMovable
#define ClassContainsLLink ClassIsMovable

#if COUNT_LLINKS

template <class Type, class Traits>
int LLink<Type,Traits>::_notNullInstanceCount=0;

template <class Type, class Traits>
int LLink<Type,Traits>::_instanceCount=0;

#endif

template <class Type>
struct LLinkArrayKeyTraits
{
	typedef const Type *KeyType;
	static bool IsEqual(KeyType a, KeyType b)
	{
		return a==b;
	}
	static KeyType GetKey(const LLink<Type> &a) {return a.GetLink();}
};

//! array of LLink-s
template <class Type,class Allocator=MemAllocD>
class LLinkArray: public FindArrayKey< LLink<Type>, LLinkArrayKeyTraits<Type>, Allocator >
{
	typedef FindArrayKey< LLink<Type>, LLinkArrayKeyTraits<Type>, Allocator > base;
	typedef LLinkArrayKeyTraits<Type> traits;

	public:
	int Count() const;
	
	public:
	/// delete any NULL entries
	void RemoveNulls();

	//! delete at given location
	__forceinline void Delete( int index ) {base::DeleteAt(index);}
	//! delete at given location, given number of items
  __forceinline void Delete( int index, int count ) {base::DeleteAt(index,count);}
	//! delete, given pointer
	__forceinline bool Delete(const Type *src) {return this->DeleteKey(src);}
	//! delete, given Link
	__forceinline bool Delete(const LLink<Type> &src) const {return this->DeleteKey(traits::GetKey(src));}

	//! if link is provided, get a key from it
	__forceinline int Find(const LLink<Type> &src) const {return this->FindKey(traits::GetKey(src));}
	//! if pointer is provided to search, avoid temporary Link creation
	__forceinline int Find(const Type *src) const {return this->FindKey(src);}

  /// add, reuse any NULL slots if possible
  int AddReusingNull(Type *item)
  {
    int free = base::FindKey(NULL);
    if (free>=0)
    {
      base::Set(free) = item;
      return free;
    }
    else
    {
      return this->Add(item);
    }
  }
  
	ClassIsMovable(LLinkArray)
};

template <class Type,class Allocator>
int LLinkArray<Type,Allocator>::Count() const
{
	int i, n = 0;
	for (i=0; i<base::Size(); i++)
		if (base::Get(i)!=NULL) n++;
	return n;
}

template <class Type, class Allocator>
void LLinkArray<Type,Allocator>::RemoveNulls()
{
	int d=0;
	for( int s=0; s<base::Size(); s++ )
	{
		if( base::Get(s)!=NULL )
		{
			if( s!=d ) base::Set(d)=base::Get(s);
			d++;
		}
	}
	base::Resize(d);
}

#endif


