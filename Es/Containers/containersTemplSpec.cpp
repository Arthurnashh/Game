#ifndef _MSC_VER

#include <Es/essencepch.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Containers/boolArray.hpp>

// specialization - optimization to avoid 1x loop
template <>
bool PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::IsEmpty() const
{
  return _data[0].IsEmpty();
}

template <>
bool PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::Get( int pos ) const
{
  if( pos>=PackedBoolArray::NBools) return false;
  return _data.Get(0).Get(pos);
}

template <>
int PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::Test( int pos ) const
{
  if( pos>=PackedBoolArray::NBools) return false;
  return _data.Get(0).Test(pos);
}

#if 0 //if this section is enabled, the above IsEmpty specialization is known by _WIN32 compiler
bool ForceTemplateSpecializationInstantiationRenderFlagsType()
{
  typedef PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> > RenderFlagsType;
  RenderFlagsType _renderFlags;
  return _renderFlags.IsEmpty();
}
#endif

#endif
