#include "wpch.hpp"
#include "objLink.hpp"

/*
Specialization needs to be declared, otherwise it will not work in other modules
*/

#pragma warning(disable:4073)
#pragma init_seg(lib)

typedef SoftLinkIdTraits<ObjectId,ObjectIdAutoDestroy> ObjIdTraits;

#if USE_FAST_ALLOCATORS
FastAllocType TrackSoftLinks<ObjIdTraits>::_allocatorF INIT_PRIORITY_HIGH = sizeof(TrackSoftLinks);
#endif

Ref< TrackSoftLinks<ObjIdTraits> > TrackSoftLinks<ObjIdTraits>::_nil INIT_PRIORITY_HIGH
  = new TrackSoftLinks(NULL,ObjIdTraits::GetNullId());

//template <>
//FastCAlloc TrackSoftLinks< SoftLinkIdTraits<ObjectIdAutoDestroy> >::_allocatorF INIT_PRIORITY_HIGH= sizeof(TrackSoftLinks);
