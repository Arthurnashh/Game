//-----------------------------------------------------------------------------
// File: enf_bitfield64.h
//
// Desc: Bitfield optimized for multiplies of 64bits
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_BITFIELD64_H
#define ENF_BITFIELD64_H

#include "enf_config.h"
#include "common/enf_inttypes.h"

namespace enf
{
/** @addtogroup Containers
@{*/

/*!
Bitfield of defined size. Optimized for multiplies of 64
as it stores data in __in64[]
*/
template <const uint MAX_INDEX>class BitField64
{
private:
	union
	{
		uint64	Mask[(MAX_INDEX + 63) >> 6];
		uint		IMask[(MAX_INDEX + 63) >> 5];
	};

public:
	ENF_INLINE uint GetUInt32Count() const
	{
		return MAX_INDEX >> 5;
	}

	ENF_INLINE uint GetUInt32(uint index) const
	{
		return IMask[index];
	}

	ENF_INLINE void SetUInt32(uint index, uint value)
	{
		IMask[index] = value;
	}

	ENF_INLINE BitField64& Zero()
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
			Mask[n] = 0;
		return *this;
	}

	ENF_INLINE BitField64()
	{
		Zero();
	}

	ENF_INLINE BitField64(uint index)
	{
		Zero();
		enf_assert(index < MAX_INDEX);
		Mask[index >> 6] = ULONG64(1) << (index & 0x3f);
	}

	ENF_INLINE BitField64& SetAll()
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
			Mask[n] = ULONG64(0xffffffffffffffff);
		return *this;
	}

	ENF_INLINE BitField64& operator=(const BitField64& mask)
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
			Mask[n] = mask.Mask[n];
		return *this;
	}

	ENF_INLINE BitField64(uint mask0, uint mask1, uint mask2 = 0, uint mask3 = 0) //prepared for 128 bits...
	{
		Mask[0] = (uint64)mask0 | ((uint64)mask1 << 32);
		Mask[1] = (uint64)mask2 | ((uint64)mask3 << 32);
	}

	ENF_INLINE BitField64& SetBit(uint index)
	{
		enf_assert(index < MAX_INDEX);
		Mask[index >> 6] |= (ULONG64(1) << (index & 0x3f));

		return *this;
	}

	ENF_INLINE BitField64& SetBitSafe(uint index)
	{
		if(index < MAX_INDEX)
			Mask[index >> 6] |= (ULONG64(1) << (index & 0x3f));

		return *this;
	}

	ENF_INLINE BitField64& operator |=(const BitField64& mask)
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
			Mask[n] |= mask.Mask[n];
		return *this;
	}

	ENF_INLINE BitField64& ClearBit(uint index)
	{
		enf_assert(index < MAX_INDEX);
		Mask[index >> 6] &= ~(ULONG64(1) << (index & 0x3f));

		return *this;
	}

	ENF_INLINE bool operator==(const BitField64& mask) const
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
		{
			if(Mask[n] != mask.Mask[n])
				return false;
		}
		return true;
	}

	ENF_INLINE bool operator!=(const BitField64& mask) const
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
		{
			if(Mask[n] != mask.Mask[n])
				return true;
		}
		return false;
	}

	ENF_INLINE bool IsZero() const
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
		{
			if(Mask[n] != 0)
				return false;
		}
		return true;
	}

	ENF_INLINE bool IsSetBit(uint index) const
	{
		enf_assert(index < MAX_INDEX);
		return (Mask[index >> 6] & (ULONG64(1) << (index & 0x3f))) != 0;
	}

	ENF_INLINE bool IsSet(const BitField64& mask) const
	{
		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
		{
			if((Mask[n] & mask.Mask[n]) != 0)
				return true;
		}

		return false;
	}

	ENF_INLINE BitField64 operator&(const BitField64& mask) const
	{
		BitField64 result;

		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
		{
			result.Mask[n] = Mask[n] & mask.Mask[n];
		}

		return result;
	}

	ENF_INLINE BitField64 operator~() const
	{
		BitField64 result;

		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
		{
			result.Mask[n] = ~Mask[n];
		}

		return result;
	}

	ENF_INLINE BitField64 operator^(const BitField64& mask) const
	{
		BitField64 result;

		for(uint n = 0; n < ((MAX_INDEX + 63) >> 6); n++)
		{
			result.Mask[n] = Mask[n] ^ mask.Mask[n];
		}

		return result;
	}
};
//@}
}
#endif //ENF_BITFIELD64_H