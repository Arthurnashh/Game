#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ES_SOFTLINKS_HPP
#define _ES_SOFTLINKS_HPP

#include <stddef.h>
#include <stdlib.h>

#include <Es/Types/pointers.hpp>
#include <Es/Strings/rString.hpp>
#include <Es/Memory/fastAlloc.hpp>
#include <Es/Containers/array.hpp>

#if _RELEASE
#define SOFTLINK_INVALIDATE_ON_UNLOCK 1
#define AUTO_LOCK_ON_REFERENCE 0
#define TRACK_LINK_LIST 0
#else
//! unlocked link is invalid if possible
#define SOFTLINK_INVALIDATE_ON_UNLOCK 1
#define AUTO_LOCK_ON_REFERENCE 0
//! each link pointing to tracker is also tracked by the tracker
#define TRACK_LINK_LIST 1
#endif

#if TRACK_LINK_LIST
#include <Es/Containers/listBidir.hpp>
#endif

#include <Es/Memory/normalNew.hpp>

//! default traits for link IDs
template <class IdType, class IdStorage>
struct SoftLinkIdTraits
{
  typedef IdType LinkId;
  typedef IdType LinkIdStorage;
  
  //! type able to AddRef / Release
  typedef RefCount RefCountType;

  static LinkIdStorage Store(const LinkId &id) {return id;}
  static LinkId GetNullId() {return LinkId();}
  static bool IsEqual(const IdType &id1, const IdType &id2)
  {
    return id1==id2;
  }
  static bool IsNull(const IdType &id) {return IsEqual(id,GetNullId());}
  static RString GetDebugName(const IdType &id)
  {
    return id.GetDebugName();
  }
};

template <class IdTraits>
class RemoveSoftLinks;

//! tracker - the object pointed by the link, pointing to the real object
template <class IdTraits>
class TrackSoftLinks: public RefCountBase
#if TRACK_LINK_LIST
  , public TListBidir<TLinkBidirD>
#endif
{
  protected:
  friend class RemoveSoftLinks<IdTraits>;

  typedef typename IdTraits::LinkId LinkId;
  typedef typename IdTraits::LinkIdStorage LinkIdStorage;
  
  typedef RemoveSoftLinks<IdTraits> RemoveLinksType;

  RemoveLinksType *_remove;
  LinkIdStorage _id;

  private:
  //! no copy
  TrackSoftLinks(TrackSoftLinks &src);
  //! no copy
  void operator =(TrackSoftLinks &src);

  public:
  TrackSoftLinks()
  {
  }
  explicit TrackSoftLinks( const RemoveLinksType *obj, const LinkId &id);
  ~TrackSoftLinks();

  LinkId GetId() const
  {
    if (_remove)
    {
      Assert( !_remove || IdTraits::IsEqual(_id,_remove->GetLinkId()) );
      return _remove->GetLinkId();
    }
    return _id;
  }

  bool IsNull() const
  {
    return IdTraits::IsNull(_id) && _remove==NULL;
  }
  bool NotNull() const
  {
    return !IdTraits::IsNull(_id) || _remove!=NULL;
  }

  //! Releasing of reference to this object.
  // release MUST be implemented in the child
  int Release() const
  {
    int ret=DecRef();
    if( ret==0 ) delete const_cast<TrackSoftLinks *>(this);
    return ret;
  }

  __forceinline RemoveLinksType *GetObject() const {return _remove;}
  RString GetDebugName() const
  {
    if (_remove) return _remove->GetDebugName();
    return IdTraits::GetDebugName(_id);
  }

  USE_FAST_ALLOCATOR

  static Ref<TrackSoftLinks> _nil;

};

template <class IdTraits>
TrackSoftLinks<IdTraits>::TrackSoftLinks( const RemoveLinksType *obj, const LinkId &id)
:_id(id)
{
  _remove=const_cast<RemoveLinksType *>(obj);
  if( _remove )
  {
    Assert( !_remove->Track() );
    _remove->SetTrack(this);
  }
}

template <class IdTraits>
TrackSoftLinks<IdTraits>::~TrackSoftLinks()
{
  if( _remove ) _remove->SetTrack(NULL);
  _remove=NULL;
}

//! base class of the real object, pointed to by the tracker
template <class IdTraits>
class RemoveSoftLinks: public /*typename*/ IdTraits::RefCountType
{

  //friend class TrackSoftLinks<IdTraits>;

  TrackSoftLinks<IdTraits> *_track;

  public:
  typedef typename IdTraits::LinkId LinkId;
  
  RemoveSoftLinks() {_track=NULL;}
  RemoveSoftLinks( const RemoveSoftLinks &src ){_track=NULL;}
  void operator =( const RemoveSoftLinks &src ){_track=NULL;}

  ~RemoveSoftLinks();

  void SoftLinksDestroy()
  {
    if (_track)
    {
      _track->_id = IdTraits::GetNullId();
    }
  }

  __forceinline TrackSoftLinks<IdTraits> *Track() const {return _track;}
  __forceinline void SetTrack(TrackSoftLinks<IdTraits> *track) {_track=track;}
  virtual LinkId GetLinkId() const = 0;
  virtual RString GetDebugName() const
  {
    char ptr[34];
    #ifdef _WIN32
    _ultoa((ptrdiff_t)this,ptr,16);
    #else
    sprintf(ptr, "%x", (ptrdiff_t)this);
    #endif
    return ptr;
  }
};


#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR_TEMPLATE(class LinkId,TrackSoftLinks,TrackSoftLinks<LinkId>)

template <class IdTraits>
inline RemoveSoftLinks<IdTraits>::~RemoveSoftLinks()
{
  if( _track )
  {
    _track->_remove=NULL;
  }
  _track=NULL;
}

//! SoftLink traits
template <class Type,class Base=Type>
struct SoftLinkTraits
{
  typedef typename Base::LinkId LinkId;
  typedef typename Base::LinkIdStorage LinkIdStorage;
  typedef SoftLinkIdTraits<LinkId,LinkIdStorage> IdTraits;

  typedef Base BaseType;
  typedef TrackSoftLinks<IdTraits> Tracker;

  static Base *RestoreLink(const LinkId &id)
  {
    return Base::RestoreLink(id);
  }
  static bool RequestRestoreLink(const LinkId &id, bool noRequest=false)
  {
    return Base::RequestRestoreLink(id,noRequest);
  }

  static LinkId GetLinkId(const Base *obj) {return obj->GetLinkId();}
  static LinkId GetNullId() {return IdTraits::GetNullId();}
  static void LockLink(Base *obj) {obj->LockLink();}
  static void UnlockLink(Base *obj) {obj->UnlockLink();}

  static bool NeedsRestoring(const Base *obj) {return obj->NeedsRestoring();}
  static bool IsAlwaysLocked() {return false;}

  // Support for links to non-primary interface
  static __forceinline TrackSoftLinks<IdTraits> *Track(Type *ref) {return ref->Track();}
  static __forceinline RemoveSoftLinks<IdTraits> *GetRemoveLinks(Type *ref) {return ref;}
  static __forceinline Type *GetObject(TrackSoftLinks<IdTraits> *tracker) {return static_cast<Type *>(tracker->GetObject());}
};

//! SoftLink traits, links are permanently locked during their lifetime
template <class Type,class Base=Type>
struct SoftLinkTraitsLocked: public SoftLinkTraits<Type,Base>
{
  static bool IsAlwaysLocked() {return true;}
};


#if _DEBUG

  template <class TgtType, class SrcType>
  TgtType SafeCast(SrcType a)
  {
    Assert( !a || dynamic_cast<TgtType>(a) );
    return static_cast<TgtType>(a);
  }
  #define INHERIT_SOFTLINK(Type,Base) \
    static Type *RestoreLink(const LinkId &id) \
    { \
      return SafeCast<Type *>(Base::RestoreLink(id)); \
    }

#else

  #define INHERIT_SOFTLINK(Type,Base) \
    static Type *RestoreLink(const LinkId &id) \
    { \
      return static_cast<Type *>(Base::RestoreLink(id)); \
    }

#endif

enum _softLinkById {SoftLinkById};
enum _softLinkByRefId {SoftLinkByRefId};

template <class Type,class Traits=SoftLinkTraits<Type> >
class SoftLink;

//! maintain link locked when dereferencing - safe, slower
/*!
  Pointed object is locked, and it is therefore guaranteed not to be unloaded,
  but it might be destroyed. If there is an attempt to reference destroyed object,
  NULL will be dereferenced, triggering access violation.
*/
template <class Type, class Traits=SoftLinkTraits<Type> >
class SoftLinkLock
{
  const SoftLink<Type,Traits> &_link;

  public:
  explicit SoftLinkLock(const SoftLink<Type,Traits> &link)
  :_link(link)
  {
    _link.Lock();
  }
  ~SoftLinkLock()
  {
    _link.Unlock();
  }

  SoftLinkLock(const SoftLinkLock &src)
  :_link(src._link)
  {
    _link.Lock();
  }
  void operator =(const SoftLinkLock &obj)
  {
    _link.Unlock();
    _link = obj._link;
    _link.Lock();
  }

  __forceinline Type *operator -> () const {return _link.GetLink();}

};

//! maintain link locked when dereferencing - unsafe, faster
/*!
  Note: this replaces using normal pointer with permanent link.

  Pointed object is locked, and it is therefore guaranteed not to be unloaded,
  
  Just with like normal pointer with permanent links, it is responsibility of programmer 
  to guarantee object is not destroyed during lifetime of this object.
*/

template <class Type, class Traits=SoftLinkTraits<Type> >
class SoftLinkLockedPtr
{
  Type *_object;

  public:
  explicit SoftLinkLockedPtr(const SoftLink<Type,Traits> &link)
  {
    link.Lock(); // increase lock count - will be decreased on destruction
    _object = link.GetLink();
    #if SOFTLINK_INVALIDATE_ON_UNLOCK
      // with debug settings Lock/Unlock pairs need to be done directly on the links
      if (_object) Traits::LockLink(_object);
      link.Unlock();
    #endif
  }

  SoftLinkLockedPtr()
  :_object(NULL)
  {
  }
  explicit SoftLinkLockedPtr(Type *object)
  :_object(object)
  {
    // object is assumed to be already locked when passed
    if (_object) Traits::LockLink(_object);
  }
  ~SoftLinkLockedPtr()
  {
    if (_object) Traits::UnlockLink(_object);
    _object = NULL;
  }
  void Free()
  {
    if (_object) Traits::UnlockLink(_object);
    _object = NULL;
  }

  SoftLinkLockedPtr(const SoftLinkLockedPtr &src)
  :_object(src._object)
  {
    if (_object) Traits::LockLink(_object);
  }
  void operator =(const SoftLinkLockedPtr &src)
  {
    if (src._object) Traits::LockLink(src._object);
    if (_object) Traits::UnlockLink(_object);
    _object = src._object;
  }
  SoftLinkLockedPtr &operator =(Type *src)
  {
    // while we could use default assignment, explicit implementation
    // avoids temporary object, which leads to locking and unlocking again
    if (src) Traits::LockLink(src);
    if (_object) Traits::UnlockLink(_object);
    _object = src;
    return *this;
  }

  operator SoftLink<Type,Traits>  () const
  {
    return SoftLink<Type,Traits>(_object);
  }

  __forceinline Type *operator -> () const {return _object;}
  __forceinline operator Type * () const {return _object;}

  __forceinline Type * GetPointer() const {return _object;}
  
  __forceinline bool IsNull() const {return _object==NULL;}
  __forceinline bool NotNull() const {return _object!=NULL;}
};

//! soft (autorestorable) link
template <class Type,class Traits /*=SoftLinkTraits<Type>*/ >
class SoftLink
#if TRACK_LINK_LIST
  : public TLinkBidirD
#endif
{
  typedef typename Traits::LinkId LinkId;
  typedef typename Traits::LinkIdStorage LinkIdStorage;
  typedef typename Traits::Tracker TrackerType;
  typedef typename Traits::IdTraits IdTraits;
  typedef typename Traits::BaseType Base;

 
  //! link pointing to the real object
  mutable Ref<TrackerType> _tracker;
  #if SOFTLINK_INVALIDATE_ON_UNLOCK
  mutable int _locked;
  #endif

  __forceinline Type *GetLinkInternal() const
  {
    // verify given object can be safely accessed with permanent link
    return static_cast<Type *>(_tracker->GetObject());
  }
  __forceinline Base *GetLinkInternalBase() const
  {
    // verify given object can be safely accessed with permanent link
    return static_cast<Base *>(_tracker->GetObject());
  }

  public:
  //! make data type accessible
  typedef Type ItemType;

  SoftLink():_tracker(TrackerType::_nil),_locked(0)
  {
    #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Insert(this);
    #endif
    Assert(CheckIntegrity());
    if (Traits::IsAlwaysLocked())
    {
      Lock();
    }
  }

  SoftLink(_softLinkById dummy, const LinkId &id)
  :_tracker(IdTraits::IsNull(id) ? TrackerType::_nil.GetRef() : new TrackerType(NULL,id))
  #if SOFTLINK_INVALIDATE_ON_UNLOCK
  ,_locked(0)
  #endif
  {
    #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Insert(this);
    #endif
    Assert(CheckIntegrity());
    if (Traits::IsAlwaysLocked())
    {
      Lock();
    }
  }

  SoftLink( Type *ref );

  SoftLink(const SoftLink &src)
  #if SOFTLINK_INVALIDATE_ON_UNLOCK
    :_tracker
    (
      (src._tracker && src.GetLinkInternalBase() && Traits::NeedsRestoring(src.GetLinkInternalBase()) )
      ?
      new TrackerType(NULL,src._tracker->GetId())
      :
      src._tracker.GetRef()
    )
  #else
    :_tracker(src._tracker)
  #endif
  {
    AssertMainThread ();
    #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Insert(this);
    #endif
    _locked = 0;
    Assert(CheckIntegrity());
    if (Traits::IsAlwaysLocked())
    {
      Lock();
    }
  }
  const SoftLink &operator =(const SoftLink &src)
  {
    AssertMainThread ();
    if (Traits::IsAlwaysLocked())
    {
      if (&src==this) return *this;
    }
    Assert(src.CheckIntegrity());
    Assert(CheckIntegrity());
    if (Traits::IsAlwaysLocked())
    {
      Unlock();
    }
    #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Delete(this);
    #endif
    #if SOFTLINK_INVALIDATE_ON_UNLOCK
      Assert(_locked==0);
      if (src._tracker && src.GetLinkInternalBase() && Traits::NeedsRestoring(src.GetLinkInternalBase()))
      {
        _tracker = new TrackerType(NULL,src._tracker->GetId());
      }
      else
      {
        _tracker = src._tracker;
       }
      _locked = 0;
    #else
      _tracker = src._tracker;
    #endif
    #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Insert(this);
    #endif
    if (Traits::IsAlwaysLocked())
    {
      Assert(CheckIntegrity());
      Lock();
    }
    Assert(CheckIntegrity());
    return *this;
  }

  ~SoftLink()
  {
    AssertMainThread ();
    Assert(CheckIntegrity());
    if (Traits::IsAlwaysLocked())
    {
      Unlock();
    }
    #if SOFTLINK_INVALIDATE_ON_UNLOCK
      #if _ENABLE_REPORT
      if (_locked>0)
      {
        RptF("Link to object %s locked (%d)",(const char *)_tracker->GetDebugName(),_locked);
      }
      #endif
      Assert(_locked==0);
    #endif
  }

  #if AUTO_LOCK_ON_REFERENCE
    friend class SoftLinkLock<Type,Traits>;

    __forceinline SoftLinkLock<Type,Traits> GetLink() const
    {
      return SoftLinkLock<Type,Traits>(*this);
    }
    __forceinline operator SoftLinkLock<Type,Traits> () const
    {
      return SoftLinkLock<Type,Traits>(*this);
    }
    __forceinline SoftLinkLock<Type,Traits> operator ->() const
    {
      return SoftLinkLock<Type,Traits>(*this);
    }
  #else
    __forceinline Type *GetLink() const
    {
      #if SOFTLINK_INVALIDATE_ON_UNLOCK
      DoAssert(_locked>0);
      #endif
      return static_cast<Type *>(_tracker->GetObject());
    }
    __forceinline operator Type *() const {return GetLink();}
    __forceinline Type * operator ->() const {return GetLink();}
  #endif

  //! Get ID identifying a link
  /*!
  Note: Non-restorable link always returns null id
  */
  __forceinline LinkId GetId() const {return _tracker->GetId();}

  //! return locked pointer which will auto-unlock when going out of scope
  SoftLinkLockedPtr<Type,Traits> GetLock() const
  {
    return SoftLinkLockedPtr<Type,Traits>(*this);
  }
  /// check if the link is ready (Lock will be fast), if not, request the data
  bool IsReady(bool noRequest=false) const
  {
    Assert(CheckIntegrity());
    Assert(_tracker);
    Base *obj = GetLinkInternalBase();
    if (obj)
    {
      return true;
    }
    // object is tracked, but was removed or unloaded
    const LinkId &id = _tracker->GetId();
    if (!IdTraits::IsNull(id))
    {
      return Traits::RequestRestoreLink(id,noRequest);
    }
    return true;
  }
  // before link can be used, it must be locked
  // link which is not locked points always to NULL
  // in SOFTLINK_INVALIDATE_ON_UNLOCK mode asserts to debugger
  void Lock() const
  {
    AssertMainThread ();
    Assert(CheckIntegrity());
    #if SOFTLINK_INVALIDATE_ON_UNLOCK
      Assert(_locked>=0);
      if (_locked==0)
      {
        Assert(!_tracker || GetLinkInternal()==NULL || !Traits::NeedsRestoring(GetLinkInternal()) );
      }
      _locked++;
    #endif
    Assert(_tracker);
    Base *obj = GetLinkInternalBase();
    if (obj)
    {
      Traits::LockLink(obj);
      Assert(CheckIntegrity());
      return;
    }
    // object is tracked, but was removed or unloaded
    const LinkId &id = _tracker->GetId();
    if (!IdTraits::IsNull(id))
    {
      #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Delete(const_cast<SoftLink *>(this));
      #endif
      Base *obj = Traits::RestoreLink(id);
      if (obj)
      {
        // object is already locked in RestoreLink function
        _tracker = obj->Track() ? obj->Track() : new TrackerType(obj,Traits::GetLinkId(obj));
      }
      else
      {
        // object cannot be restored - meaning it was removed
        _tracker = TrackerType::_nil;
      }
      #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Insert(const_cast<SoftLink *>(this));
      #endif
    }
    Assert(CheckIntegrity());
  }
  void Unlock() const
  {
    AssertMainThread ();
    Assert(CheckIntegrity());
    #if SOFTLINK_INVALIDATE_ON_UNLOCK
      _locked--;
      Assert(_locked>=0);
    #endif
    Assert(_tracker);
    Base *obj = GetLinkInternalBase();
    if (obj)
    {

      #if SOFTLINK_INVALIDATE_ON_UNLOCK
      if (_locked==0 && Traits::NeedsRestoring(obj))
      {
        #if TRACK_LINK_LIST
          if (_tracker) _tracker->Delete(const_cast<SoftLink *>(this));
        #endif
        // break link (do not point to the object any more)
        _tracker = new TrackerType(NULL,Traits::GetLinkId(obj));
        #if TRACK_LINK_LIST
          if (_tracker) _tracker->Insert(const_cast<SoftLink *>(this));
        #endif
      }
      #endif

      Traits::UnlockLink(obj);
    }
    // null links need to stay valid even when unlocked
    Assert(CheckIntegrity());
  }

  __forceinline bool IsNull() const {return _tracker->IsNull();}
  __forceinline bool NotNull() const {return _tracker->NotNull();}

  RString GetDebugName() const
  {
    Assert(CheckIntegrity());
    if (!_tracker) return "NULL Tracker";
    return _tracker->GetDebugName();
  }
  /**
  @param noRequest we may want to request loading when not loaded yet
  */
  bool IsLoaded(bool noRequest=true) const
  {
    Assert(CheckIntegrity());
    Assert(_tracker);
    Base *obj = GetLinkInternalBase();
    if (obj)
    {
      return true;
    }
    // object is tracked, but was removed or unloaded
    const LinkId &id = _tracker->GetId();
    if (!IdTraits::IsNull(id))
    {
      return Traits::RequestRestoreLink(id,noRequest);
    }
    // if NULL, it is not and cannot be loaded
    return false;
  }
  // export tracker for comparison purposes
  const TrackerType *GetTracker() const
  {
    return _tracker;
  }
  bool IsEqual(const Type *with) const
  {
    if (!Traits::IdTraits::IsEqual(GetId(),Traits::GetLinkId(with))) return false;
    if (!Traits::IdTraits::IsNull(GetId())) return true;
    Assert(GetTracker());
    return GetTracker()->GetObject() == with;
  }

  bool CheckIntegrity() const
  {
    #if TRACK_LINK_LIST
      if (_tracker)
      {
        Assert(IsInList());
        if (_tracker->GetObject())
        {
          // check if we are in the list of the tracker
          bool found = false;
          for (const TLinkBidirD *link = _tracker->First(); link; link=_tracker->Next(link))
          {
            if (link==this)
            {
              found =true;
            }
          }
          Assert(found);
        }
      }
    #endif
    bool ret = true;
    #if SOFTLINK_INVALIDATE_ON_UNLOCK
      if(!_tracker)
      {
        ret = false;
        Fail("Tracker must never be NULL");
        return false;
      }
      if (_locked)
      {
        Assert(_locked>0);
        if (_tracker->NotNull())
        {
          Type *obj = GetLinkInternal();
          if (obj)
          {
            LinkId id = Traits::GetLinkId(obj);
            if (!IdTraits::IsNull(id))
            {
              if (!IdTraits::IsEqual(id,_tracker->GetId()))
              {
                RptF("Object %s",(const char *)_tracker->GetDebugName());
                Fail("Broken id in the link tracker.");
                ret = false;
              }
            }
          }
        }
        else
        {
          if (GetLinkInternal()!=NULL)
          {
            Fail("NULL id Object not NULL");
            ret = false;
          }
        }
      }
      else
      {
        if (GetLinkInternal() && Traits::NeedsRestoring(GetLinkInternal()))
        {
          Fail("Internal link should be NULL on unlocked object");
          ret = false;
        }
      }
    #endif
    return ret;
  }
#ifdef TRACK_LINK_LIST
  ClassIsGeneric(SoftLink);
#else
  ClassIsMovable(SoftLink);
#endif
};

#ifdef TRACK_LINK_LIST
# define TypeContainsSoftLink TypeIsGeneric
# define ClassContainsSoftLink ClassIsGeneric
#else
# define TypeContainsSoftLink TypeIsMovable
# define ClassContainsSoftLink ClassIsMovable
#endif

#if SOFTLINK_INVALIDATE_ON_UNLOCK

  template <class Type,class Traits>
  SoftLink<Type,Traits>::SoftLink( Type *ref )
  :_tracker
  (
    ref
    ?
    (
      Traits::NeedsRestoring(ref)
      ?
      new TrackerType(NULL,Traits::GetLinkId(ref))
      :
      ( ref->Track() ? ref->Track() : new TrackerType(ref,Traits::GetLinkId(ref)) )
    )
    :
    TrackerType::_nil.GetRef()
  ),
  _locked(0)
  {
    #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Insert(const_cast<SoftLink *>(this));
    #endif
    Assert(CheckIntegrity());
    if (Traits::IsAlwaysLocked())
    {
      Lock();
    }
  }
#else

  template <class Type,class Traits>
  inline SoftLink<Type,Traits>::SoftLink( Type *ref )
  :_id(ref ? Traits::GetLinkId(ref) : Traits::GetNullId()),
  _tracker( ref ? ( ref->Track() ? ref->Track() : new TrackerType(ref) ) : TrackerType::_nil.GetRef() )
  {
    #if TRACK_LINK_LIST
      if (_tracker)	_tracker->Insert(const_cast<SoftLink *>(this));
    #endif
    Assert(CheckIntegrity());
    if (Traits::IsAlwaysLocked())
    {
      Lock();
    }
  }

#endif


template <class IdTraits>
Ref<TrackSoftLinks<IdTraits> > TrackSoftLinks<IdTraits>::_nil
  = new TrackSoftLinks(NULL,IdTraits::GetNullId());


template <class Type1, class Traits1, class Type2, class Traits2>
bool operator == (const SoftLink<Type1,Traits1> &l1, const SoftLink<Type2,Traits2> &l2)
{
  if (!Traits1::IdTraits::IsEqual(l1.GetId(),l2.GetId())) return false;
  if (!Traits1::IdTraits::IsNull(l1.GetId())) return true;
  Assert(l1.GetTracker());
  Assert(l2.GetTracker());
  if (l1.GetTracker()==l2.GetTracker()) return true;
  return l1.GetTracker()->GetObject() == l2.GetTracker()->GetObject();
}

template <class Type1, class Traits1, class Type2, class Traits2>
__forceinline bool operator != (const SoftLink<Type1,Traits1> &l1, const SoftLink<Type2,Traits2> &l2)
{
  return !(l1==l2);
}

//! soft link - permanent, no restore support used or required
template <class Type, class Traits=SoftLinkTraits<Type> >
class SoftLinkPerm
{
  typedef typename Traits::Tracker TrackerType;

  //! link pointing to the real object
  mutable Ref<TrackerType> _tracker;

  public:
  //! make data type accessible
  typedef Type ItemType;

  SoftLinkPerm():_tracker(TrackerType::_nil)
  {
  }
  SoftLinkPerm( Type *ref )
  :_tracker
  (
    ref
    ?
    ( Traits::Track(ref) ? Traits::Track(ref) : new TrackerType(Traits::GetRemoveLinks(ref),Traits::GetLinkId(ref)) )
    :
    TrackerType::_nil.GetRef()
  )
  {
    #if _ENABLE_REPORT
    if( !(ref==NULL || !Traits::NeedsRestoring(ref)) )
    {
      RptF("Permanent link to %s not allowed",(const char *)ref->GetDebugName());
      Fail("Permanent link cannot point to non-permanent object");
    }
    #endif
  }

  __forceinline Type *GetLink() const
  {
    Assert(_tracker);
    // verify given object can be safely accessed with permanent link
    Type *ret = static_cast<Type *>(Traits::GetObject(_tracker));
    Assert( ret==NULL || !Traits::NeedsRestoring(ret) );
    return ret;
  }
  __forceinline operator Type *() const {return GetLink();}
  __forceinline Type * operator ->() const {return GetLink();}

  ClassIsMovable(SoftLinkPerm);
};

/// search traits for SoftLinkArray
/**
Key is a pair of pointer + id. This is because for non-primary objects the Id is always Null.
*/

template <class Type,class Traits>
struct SoftLinkArrayKeyTraits
{
  struct KeyType
  {
    /// id - used for recoverable objects
    typename Traits::LinkId _id;
    /// for non-recoverable objects we store the object pointer
    /**
    In this case we know the object is never discarded and using plain pointer is safe
    It might still be destroyed, but this is something we do not mind, as keys are only temporary when searching.
    */
    const Type *_ptr;
    
    KeyType(typename Traits::LinkId id, const Type *ptr):_id(id),_ptr(ptr){}
  };
  //typedef typename Traits::LinkId KeyType;
  static bool IsEqual(KeyType a, KeyType b)
  {
    if (!Traits::IdTraits::IsEqual(a._id,b._id)) return false;
    if (!Traits::IdTraits::IsNull(a._id)) return true;
    return a._ptr==b._ptr;
  }
  
  static KeyType GetKey(const Type *a)
  {
    typename Traits::LinkId id = Traits::GetLinkId(a);
    if (!Traits::IdTraits::IsNull(id)) return KeyType(id,NULL);
    return KeyType(id,a);
  }
  static KeyType GetKey(const SoftLink<Type,Traits> &a)
  {
    typename Traits::LinkId id = a.GetId();
    if (!Traits::IdTraits::IsNull(id)) return KeyType(id,NULL);
    return KeyType(id,a.GetLink());
  }
};

//! array of SoftLink-s
template <class Type,class Traits=SoftLinkTraits<Type>, class Allocator=MemAllocD>
class SoftLinkArray: public FindArrayKey< SoftLink<Type,Traits>, SoftLinkArrayKeyTraits<Type,Traits>, Allocator >
{
  typedef FindArrayKey< SoftLink<Type,Traits>, SoftLinkArrayKeyTraits<Type,Traits>, Allocator > base;
  typedef SoftLinkArrayKeyTraits<Type,Traits> ArrayKeyTraits;

  public:
  int Count() const;
  void RemoveNulls();

  //! delete at given location
  __forceinline void Delete( int index ) {base::DeleteAt(index);}
  //! delete, given pointer
  __forceinline bool Delete(const Type *src) {return base::DeleteKey(ArrayKeyTraits::GetKey(src));}
  //! delete, given Link
  __forceinline bool Delete(const SoftLink<Type> &src) const {return base::DeleteKey(ArrayKeyTraits::GetKey(src));}

  //! if link is provided, get a key from it
  __forceinline int Find(const SoftLink<Type> &src) const {return base::FindKey(ArrayKeyTraits::GetKey(src));}
  //! if pointer is provided to search, avoid temporary Link creation
  __forceinline int Find(const Type *src) const {return base::FindKey(ArrayKeyTraits::GetKey(src));}

  /// add, reuse any NULL slots if possible
  int AddReusingNull(Type *item)
  {
    int free = base::FindKey(NULL);
    if (free>=0)
    {
      base::container[free] = item;
      return free;
    }
    else
    {
      return base::container.Add(item);
    }
  }
  ClassIsMovable(SoftLinkArray)
};

template <class Type,class Traits, class Allocator>
int SoftLinkArray<Type,Traits,Allocator>::Count() const
{
  int i, n = 0;
  for (i=0; i<base::Size(); i++)
    if (base::Get(i)!=NULL) n++;
  return n;
}

template <class Type,class Traits, class Allocator>
void SoftLinkArray<Type,Traits,Allocator>::RemoveNulls()
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

/// search traits for SoftLinkPermArray
template <class Type>
struct SoftLinkPermArrayKeyTraits
{
  typedef const Type *KeyType;
  static bool IsEqual(KeyType a, KeyType b) {return a==b;}
  static KeyType GetKey(const Type *a) {return a;}
  static KeyType GetKey(const SoftLink<Type> &a) {return a.GetLink();}
  static KeyType GetKey(const SoftLinkPerm<Type> &a) {return a.GetLink();}
};


//! array of SoftLinkPerm-s
template <class Type,class Traits = SoftLinkTraits<Type>,class Allocator=MemAllocD>
class SoftLinkPermArray: public FindArrayKey< SoftLinkPerm<Type,Traits>, SoftLinkPermArrayKeyTraits<Type>, Allocator >
{
  typedef FindArrayKey< SoftLinkPerm<Type,Traits>, SoftLinkPermArrayKeyTraits<Type>, Allocator > base;
  typedef SoftLinkPermArrayKeyTraits<Type> ArrayKeyTraits;

  public:
  int Count() const;
  void RemoveNulls();

  //! delete at given location
  __forceinline void Delete( int index ) {base::DeleteAt(index);}
  //! delete, given pointer
  __forceinline bool Delete(const Type *src) {return base::DeleteKey(src);}
  //! delete, given Link
  __forceinline bool Delete(const SoftLinkPerm<Type> &src) const {return base::DeleteKey(ArrayKeyTraits::GetKey(src));}

  //! if link is provided, get a key from it
  __forceinline int Find(const SoftLinkPerm<Type> &src) const {return base::FindKey(ArrayKeyTraits::GetKey(src));}
  //! if pointer is provided to search, avoid temporary Link creation
  __forceinline int Find(const Type *src) const {return base::FindKey(ArrayKeyTraits::GetKey(src));}
  ClassIsMovable(SoftLinkPermArray)
};

template <class Type,class Traits,class Allocator>
int SoftLinkPermArray<Type,Traits,Allocator>::Count() const
{
  int i, n = 0;
  for (i=0; i<base::Size(); i++)
    if (base::Get(i)!=NULL) n++;
  return n;
}

template <class Type,class Traits,class Allocator>
void SoftLinkPermArray<Type,Traits,Allocator>::RemoveNulls()
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


