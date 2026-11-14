#pragma warning(disable:4073)
#pragma init_seg(lib)

#include <Es/essencepch.hpp>
#include <Es/Types/lLinks.hpp>

// should be included in source file with #pragma init_seg(lib)
// to guarantee initialization before all user variables
DEFINE_FAST_ALLOCATOR_ID(TrackLLinks,TrackLLinks) // one instance per application

Ref<TrackLLinks> LLinkNil INIT_PRIORITY_HIGH = new TrackLLinks(NULL);
