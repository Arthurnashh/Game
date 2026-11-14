#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OBJ_LINK_HPP
#define _OBJ_LINK_HPP

// determine which kind of links is used to link to objects
#include <Es/Types/lLinks.hpp>
#include <Es/Strings/rString.hpp>

#include "objectId.hpp"

#if 1
  // use auto-recoverable links
  #include <Es/Types/softLinks.hpp>

/// specialization so that storage is different from link type
template <>
struct SoftLinkIdTraits<ObjectId,ObjectIdAutoDestroy>
{
  typedef ObjectId LinkId;
  typedef ObjectIdAutoDestroy LinkIdStorage;
  
  //! type able to AddRef / Release
  typedef RefCount RefCountType;

  static LinkIdStorage Store(const LinkId &id);
  static LinkId GetNullId() {return LinkId();}
  static bool IsEqual(const LinkId &id1, const LinkId &id2)
  {
    return id1==id2;
  }
  static bool IsNull(const LinkId &id) {return IsEqual(id,GetNullId());}
  static RString GetDebugName(const LinkId &id)
  {
    return id.GetDebugName();
  }
};

  // specialization declaration provided to solve initialization order problems

  #include <Es/Memory/normalNew.hpp>


  template <>
  class TrackSoftLinks<SoftLinkIdTraits<ObjectId,ObjectIdAutoDestroy> >: public RefCountBase
	#if TRACK_LINK_LIST
		, public TListBidir<TLinkBidirD>
	#endif
  {
    protected:
    typedef SoftLinkIdTraits<ObjectId,ObjectIdAutoDestroy> IdTraits;

	  friend class RemoveSoftLinks<IdTraits>;

    typedef IdTraits::LinkId LinkId;
    typedef IdTraits::LinkIdStorage LinkIdStorage;

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
	  explicit TrackSoftLinks( const RemoveLinksType *obj, const LinkId &id)
    :_id(IdTraits::Store(id))
	  {
		  _remove=const_cast<RemoveLinksType *>(obj);
		  if( _remove )
		  {
			  Assert( !_remove->Track() );
			  _remove->SetTrack(this);
		  }
	  }
	  ~TrackSoftLinks()
	  {
		  if( _remove ) _remove->SetTrack(NULL);
		  _remove=NULL;
	  }

		LinkId GetId() const
		{
			if (_remove)
			{
				Assert( IdTraits::IsNull(_id) || IdTraits::IsEqual(_id,_remove->GetLinkId()) );
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

	  __forceinline RemoveSoftLinks<IdTraits> *GetObject() const {return _remove;}
    RString GetDebugName() const
    {
      if (_remove) return _remove->GetDebugName();
      return IdTraits::GetDebugName(_id);
    }

	  USE_FAST_ALLOCATOR

    static Ref<TrackSoftLinks> _nil;
  };

  #include <Es/Memory/debugNew.hpp>

  class SerializedObject;
  typedef SerializedObject OSoftLinkBase;

  //! type based link
  #define OLink(Type) SoftLink<Type, SoftLinkTraitsLocked<Type> >
  #define OLinkArray(Type) SoftLinkArray<Type, SoftLinkTraitsLocked<Type> >
  #define OLinkArrayEx(Type,Alloc) SoftLinkArray<Type, SoftLinkTraitsLocked<Type>, Alloc >
  
  typedef OLink(Object) OLinkObject;

  //! Object based link
  #define OLinkO(Type) SoftLink<Type, SoftLinkTraitsLocked<Type,Object> >
  #define OLinkOArray(Type) SoftLinkArray<Type, SoftLinkTraitsLocked<Type,Object> >

  //! NetworkObject based link
  #define OLinkNO(Type) SoftLink<Type, SoftLinkTraitsLocked<Type,OSoftLinkBase> >
  #define OLinkNOArray(Type) SoftLinkArray<Type, SoftLinkTraitsLocked<Type,OSoftLinkBase> >


  //! type based link - recoverable (needs locking)
  #define OLinkL(Type) SoftLink<Type, SoftLinkTraits<Type> >
  #define OLinkArrayL(Type) SoftLinkArray<Type, SoftLinkTraits<Type>>

  #define OLinkLPtr(Type) SoftLinkLockedPtr<Type, SoftLinkTraits<Type> >

  //! Object based link - recoverable (needs locking)
  #define OLinkOL(Type) SoftLink<Type, SoftLinkTraits<Type,Object> >
  #define OLinkOLArray(Type) SoftLinkArray<Type, SoftLinkTraits<Type,Object> >
  #define OLinkOLPtr(Type) SoftLinkLockedPtr<Type, SoftLinkTraits<Type,Object> >

  //! NetworkObject based link - recoverable (needs locking)
  #define OLinkNOL(Type) SoftLink<Type, SoftLinkTraits<Type,OSoftLinkBase> >
  #define OLinkNOLArray(Type) SoftLinkArray<Type, SoftLinkTraits<Type,OSoftLinkBase> >



  //! permanent type based link
  #define OLinkPerm SoftLinkPerm
  #define OLinkPermArray SoftLinkPermArray

  //! permanent Object based link
  #define OLinkPermO(Type) SoftLinkPerm<Type, SoftLinkTraits<Type,Object> >
  #define OLinkPermOArray(Type) SoftLinkPermArray<Type, SoftLinkTraits<Type,Object> >

  //! permanent NetworkObject based link
  #define OLinkPermNO(Type) SoftLinkPerm<Type, SoftLinkTraits<Type,OSoftLinkBase> >
  #define OLinkPermNOArray(Type) SoftLinkPermArray<Type, SoftLinkTraits<Type,OSoftLinkBase> >
  
  //! link base class
  #define RemoveOLinks RemoveSoftLinks< SoftLinkIdTraits<ObjectId,ObjectIdAutoDestroy> >
  //! link tracker class
  #define TrackOLinks TrackSoftLinks< SoftLinkIdTraits<ObjectId,ObjectIdAutoDestroy> >

  //! type optimization information
  #define TypeContainsOLink TypeContainsSoftLink
  //! type optimization information
  #define ClassContainsOLink ClassContainsSoftLink

#else
  // use simple links, no recovery possible
  #include <Es/Types/lLinks.hpp>

  #define OLink LLink
  #define OLinkPerm LLink
  #define OLinkArray LLinkArray
  #define OLinkPermArray LLinkArray

  #define OLinkO(Type) LLink<Type>
  #define OLinkPermO(Type) LLink<Type>
  #define OLinkPermOArray(Type) LLinkArray<Type>

  #define OLinkNO(Type) LLink<Type>
  #define OLinkPermNO(Type) LLink<Type>
  #define OLinkPermNOArray(Type) LLinkArray<Type>

  #define RemoveOLinks RemoveLLinks
  #define TypeContainsOLink TypeContainsLLink
  #define ClassContainsOLink ClassContainsLLink
  #define TrackOLinks TrackLLinks
#endif

#endif
