//----------------------------------------------------------------------------- 
// Pointer hashing that keeps address entropy in both 32-bit and 64-bit builds.
//-----------------------------------------------------------------------------
#pragma once
#ifndef ENF_POINTERHASH_H
#define ENF_POINTERHASH_H

#include "common/enf_inttypes.h"

namespace enf
{
inline uint HashPointer(const void *pointer)
{
  const uint64 address = reinterpret_cast<uint64>(pointer);
  const uint low = static_cast<uint>(address);
  const uint high = static_cast<uint>(address >> 32);
  return low ^ (high * 0x9e3779b1U);
}
}

#endif
