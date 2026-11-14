// IdFlags.cpp
// Author Roderick Kennedy @date 13/11/2005

//---------------------------------------------------------------------------
#include "precomp.h"

#include "IdFlags.h"
#include <math.h>
using namespace simul;
using namespace base;

///  \brief Default constructor.
IdFlags::IdFlags()
{
	First64Bits=0;
	Second64Bits=0;
}

///  \brief Construct from a 64 bit \em num.
IdFlags::IdFlags(unsigned __int64 num)
{              
	First64Bits=num;
	Second64Bits=0;
}

///  \brief Set equal to \em num.
IdFlags& IdFlags::operator=(unsigned __int64 num)
{
	First64Bits=num;
	Second64Bits=0;
	return *this;
}
///  \brief Check equality of two IdFlags.
bool IdFlags::operator==(const IdFlags &num)
{
	return (First64Bits==num.First64Bits&&
		Second64Bits==num.Second64Bits);
}                        
///  \brief Test whether this IdFlags is less than \em num.
bool IdFlags::operator<(const IdFlags &num) const
{
	if(Second64Bits<num.Second64Bits)
		return true;
	if(Second64Bits==num.Second64Bits)
		return (First64Bits<num.First64Bits);
	return false;
}
///  \brief Return true if \em num is not equal to this IdFlags.
bool IdFlags::operator!=(const IdFlags &num)
{
	return (First64Bits!=num.First64Bits||
		Second64Bits!=num.Second64Bits);
}
///  \brief Return the value right-shifted by \em i bits.
IdFlags IdFlags::operator>>(int i)
{
	IdFlags ret;
	if(i<=64)
	{
		ret.First64Bits=First64Bits>>i;
		ret.First64Bits|=Second64Bits<<(64-i);
		ret.Second64Bits=Second64Bits>>i;
	}
	else
	{
		ret.First64Bits=Second64Bits>>(i-64);
	}
	return ret;
}
///  \brief Shift the IdFlags left by \em i bits.
void IdFlags::operator<<=(int i)
{
	if(i<64)
	{
		Second64Bits<<=i;
		Second64Bits|=First64Bits>>(64-i);
		First64Bits<<=i;
	}
	else
	{
		Second64Bits|=(First64Bits<<(i-64));
		First64Bits=0;
	}
}
///  \brief If \em flag and this class are both either zero or non-zero, return true. Otherwise
/// return false.
bool IdFlags::operator==(bool flag)
{
	if(flag)
	{
		if(First64Bits||Second64Bits)
			return true;
		else
			return false;
	}
	else
	{
		if(!First64Bits&&!Second64Bits)
			return true;
		else
			return false;
	}
}
///  \brief If \em flag and this class are both either zero or non-zero, return false. Otherwise
/// return true.
bool IdFlags::operator!=(bool flag)
{              
	return(!operator==(flag));
}
///  \brief Return the bitwise AND of this and \em Other.
IdFlags IdFlags::operator&(const IdFlags &Other) const
{
	IdFlags ret;
	ret.First64Bits=First64Bits&Other.First64Bits;
	ret.Second64Bits=Second64Bits&Other.Second64Bits;
	return ret;
}
///  \brief Return the bitwise OR of this and \em Other.
IdFlags IdFlags::operator|(const IdFlags &Other) const
{
	IdFlags ret;
	ret.First64Bits=First64Bits|Other.First64Bits;
	ret.Second64Bits=Second64Bits|Other.Second64Bits;
	return ret;
}
///  \brief Bitwise XOR of this and \em Other.
void IdFlags::operator^=(const IdFlags &Other)
{
	First64Bits^=Other.First64Bits;
	Second64Bits^=Other.Second64Bits;
}
///  \brief Return the bitwise complement.
IdFlags IdFlags::operator~()
{
	IdFlags ret;
	ret.First64Bits=~First64Bits;
	ret.Second64Bits=~Second64Bits;
	return ret;
}
///  \brief Bitwise AND of this and \em Other.
void IdFlags::operator&=(const IdFlags &Other)
{
	First64Bits&=Other.First64Bits; 
	Second64Bits&=Other.Second64Bits;
}
///  \brief Subtract \em Other from this.
void IdFlags::operator-=(const IdFlags &Other)
{
	First64Bits-=Other.First64Bits;  
	Second64Bits-=Other.Second64Bits;
}
///  \brief Bitwise OR of this and \em Other.
void IdFlags::operator|=(const IdFlags &Other)
{
	First64Bits|=Other.First64Bits;   
	Second64Bits|=Other.Second64Bits;
}
///  \brief Return false if non-zero, else return true.
bool IdFlags::operator!() const
{
	return (First64Bits==0&&Second64Bits==0);
}
///  \brief Clear all bits up to \em i.
IdFlags MaskBitsTo(int i)
{
	IdFlags f=0;
	if(i<64)
	{
		f.First64Bits=((unsigned __int64)1<<i)-1;
		f.Second64Bits=0;
	}
	else if(i<128)
	{
		f.First64Bits=~((unsigned __int64)0);
		f.Second64Bits=((unsigned __int64)1<<(i-64))-1;
	}
	else
	{
		f.First64Bits=~((unsigned __int64)0);
		f.Second64Bits=~((unsigned __int64)0);
	}
	return f;
}
///  \brief Find the index of the first non-zero bit, and clear it.
int ExtractFirstId(IdFlags &Flags)
{
	if(!Flags)
	{
		return -2;
	}
	if(Flags.First64Bits)
	for(int i=0;i<64;i++)
	{
		unsigned __int64 val=(unsigned __int64)1<<i;
		if(Flags.First64Bits&val)
		{
			Flags.First64Bits-=val;
			return i;
		}
	}
	if(Flags.Second64Bits)
	for(int i=0;i<64;i++)
	{
		unsigned __int64 val=(unsigned __int64)1<<i;
		if(Flags.Second64Bits&val)
		{
			Flags.Second64Bits-=val;
			return 64+i;
		}
	}
	return -2;
}
///  \brief Find which bit is set.

/// If one, and only one bit is set, FlagsToSingleId returns its index. If no bits are set,
/// -2 is returned. If more than one bit is set, -1 is returned.
int FlagsToSingleId(const IdFlags &Flags)
{
#if !defined(WIN32) || defined(__GNUC__)
	int Id=-1;
	if(!Flags.First64Bits&&!Flags.Second64Bits)
		return -2;
	__int64 F64=Flags.First64Bits;
	__int64 S64=Flags.Second64Bits;
	for(int i=0;i<128;i++)
	{
		if(F64==1)
        {
        	if(S64)
            	return -1;
            return i;
        }
		else if(F64&1)
        	return -1;
		F64=F64>>1;
		if(S64&1)
			F64|=((unsigned __int64)1<<63);
		S64=S64>>1;
	}
	return Id;
#else
	if(!Flags.First64Bits&&!Flags.Second64Bits)
		return -2;
	_asm
	{
		mov edx,Flags
		mov ecx,[edx]			// the first 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne test_0				// Jump if zf flag is set (i.e. ecx was not zero)
//start_32:
		mov ecx,[edx+4]			// the 2nd 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne test_32				// Jump if zf flag is set (i.e. ecx was not zero)
//start_64:
		mov ecx,[edx+8]			// the 3rd 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne test_64				// Jump if zf flag is set (i.e. ecx was not zero)
//start_96:
		mov ecx,[edx+12]		// the 4th 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne test_96				// Jump if zf flag is set (i.e. ecx was not zero)
//return_minus_2:
		mov eax,-2
		jmp return__eax
test_0:
		// Result in the first 32 bits.
		btr ecx,eax				// clear the eax bit.
		cmp ecx,0				// Is ecx now zero?
		jne return_minus_1		// more than one bit set? return -1.
		mov ecx,[edx+4]
		cmp ecx,0				// 2nd 32 bits non-zero?
		jne return_minus_1		// more than one bit set? return -1.
		mov ecx,[edx+8]
		cmp ecx,0				// Is eax now zero?
		jne return_minus_1		// more than one bit set? return -1.
		mov ecx,[edx+12]
		cmp ecx,0				// Is eax now zero?
		jne return_minus_1		// more than one bit set? return -1.
		jmp return__eax			// OK to return result.
test_32:
		// Result in the 2nd 32 bits.
		btr ecx,eax				// clear the eax bit.
		cmp ecx,0				// Is ecx now zero?
		jne return_minus_1		// more than one bit set? return -1.
		mov ecx,[edx+8]
		cmp ecx,0				// Is eax now zero?
		jne return_minus_1		// more than one bit set? return -1.
		mov ecx,[edx+12]
		cmp ecx,0				// Is eax now zero?
		jne return_minus_1		// more than one bit set? return -1.
		add eax,32
		jmp return__eax			// OK to return result.
test_64:
		// Result in the 3rd 32 bits.
		btr ecx,eax				// clear the eax bit.
		cmp ecx,0				// Is ecx now zero?
		jne return_minus_1		// more than one bit set? return -1.
		mov ecx,[edx+12]
		cmp ecx,0				// Is eax now zero?
		jne return_minus_1		// more than one bit set? return -1.
		add eax,64
		jmp return__eax			// OK to return result.
test_96:
		// Result in the 4th 32 bits.
		btr ecx,eax				// clear the eax bit.
		cmp ecx,0				// Is ecx now zero?
		jne return_minus_1		// more than one bit set? return -1.
		add eax,96
		jmp return__eax			// OK to return result.
return_minus_1:
		mov eax,-1
Return__eax:
	}
#endif
}
///  \brief From a list of indices make a bitfield.
IdFlags IntListToFlags(std::vector<int>&IntList)
{
	IdFlags Flags=0;
	for(size_t i=0;i<IntList.size();++i)
	{
		unsigned __int64 Bit;
		if(IntList[i]<=63)
        {
			Bit=((unsigned __int64)1<<IntList[i]);
    		Flags.First64Bits|=Bit;
        }
		else if(IntList[i]<=127)
		{
        	Bit=((unsigned __int64)1<<(IntList[i]-64));  
    		Flags.Second64Bits|=Bit;
        }
#ifdef SIMUL_EXCEPTIONS
        else
        	throw IdFlags::FlagOverflow();
#endif
	}
	return Flags;
}
//------------------------------------------------------------------------------
///  \brief 1<<Value.
IdFlags IntToFlag(int Value)
{
	if(Value<0)
		return 0;
	return OneBitShifted[Value];
}
//------------------------------------------------------------------------------
///  \brief Get the numeric value of \em f, if in range.
int GetInt(const IdFlags &f)
{
	return (int)f.First64Bits;
}

#if !defined(WIN32) || defined(__GNUC__)
///  \brief Get the index of the first non-zero bit.
int GetFirstId(const IdFlags &Flags)
{
	if(!Flags)
		return -2;
	int i=0;
	for(;i<64;i++)
	{
		if(Flags.First64Bits&(OneBitShifted[i].First64Bits))
			return i;
	}
	if(!Flags.Second64Bits)
		return -2;
	for(;i<128;i++)
	{
		if(Flags.Second64Bits&(OneBitShifted[i].Second64Bits))
			return i;
	}
	return -2;
}
///  \brief Get the index of the first non-zero bit after i.
int GetNextId(const IdFlags &Flags,int i)
{
	if(!Flags)
		return -2;
	for(i++;i<64;i++)
	{
		if(Flags.First64Bits&(OneBitShifted[i].First64Bits))
			return i;
	}
	if(!Flags.Second64Bits)
		return -2;
	for(;i<128;i++)
	{
		if(Flags.Second64Bits&(OneBitShifted[i].Second64Bits))
			return i;
	}
	return -2;
}
#else
int GetFirstId(const IdFlags &Flags)
{
	_asm
	{
		mov edx,Flags

		mov ecx,[edx]			// the first 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne return_eax			// Jump if zf flag is set (i.e. ecx was not zero)
//start_32:
		mov ecx,[edx+4]			// the 2nd 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_32	// Jump if zf flag is set (i.e. ecx was not zero)
//start_64:
		mov ecx,[edx+8]			// the 3rd 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_64	// Jump if zf flag is set (i.e. ecx was not zero)
//start_96:
		mov ecx,[edx+12]		// the 4th 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_96	// Jump if zf flag is set (i.e. ecx was not zero)
//return_minus_2:
		mov eax,-2
		jmp return_eax
return_eax_plus_96:
		add eax,32
return_eax_plus_64:
		add eax,32
return_eax_plus_32:
		add eax,32
return_eax:
	}
}
#if 1
int GetNextId(const IdFlags &Flags,int i)
{
	_asm
	{
		mov edx,Flags
		mov ecx,i
		mov ebx,1
		add ecx,1			// step forward 1 (we start from -1)

		cmp ecx,32
		jnb offset_32
// i definitely <32.
		shl ebx,cl				// 1<<i
		add ebx,-1				// Mask 
		not ebx					// All 1<<i and above

		mov ecx,[edx]			// the first 32 bits
		and ecx,ebx				// the bits remaining

		bsf eax,ecx				// Bit scan forward.
		jne return_eax			// Jump if zf flag is set (i.e. ecx was not zero)
//start_32:
		mov ecx,[edx+4]			// the 2nd 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_32	// Jump if zf flag is set (i.e. ecx was not zero)
start_64:
		mov ecx,[edx+8]			// the 3rd 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_64	// Jump if zf flag is set (i.e. ecx was not zero)
start_96:
		mov ecx,[edx+12]		// the 4th 32 bits
		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_96	// Jump if zf flag is set (i.e. ecx was not zero)
		jmp return_minus_2
offset_32:
		cmp ecx,64
		jnb offset_64

		and ecx,31				// What offset into this 32-bit number?
		shl ebx,cl				// 1<<i
		add ebx,-1				// Mask 
		not ebx					// All 1<<i and above

		mov ecx,[edx+4]			// the 2nd 32 bits
		and ecx,ebx				// the bits remaining

		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_32	// Jump if zf flag is set (i.e. ecx was not zero)
		jmp start_64			// Start from 64, no need to mask
offset_64:
		cmp ecx,96
		jnb offset_96

		and ecx,31				// What offset into this 32-bit number?
		shl ebx,cl				// 1<<i
		add ebx,-1				// Mask 
		not ebx					// All 1<<i and above

		mov ecx,[edx+8]			// the 3rd 32 bits
		and ecx,ebx				// the bits remaining

		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_64	// Jump if zf flag is set (i.e. ecx was not zero)
		jmp start_96			// Start from 64, no need to mask
offset_96:
		cmp ecx,128
		jnb return_minus_2

		and ecx,31				// What offset into this 32-bit number?
		shl ebx,cl				// 1<<i
		add ebx,-1				// Mask 
		not ebx					// All 1<<i and above

		mov ecx,[edx+12]			// the 4th 32 bits
		and ecx,ebx				// the bits remaining

		bsf eax,ecx				// Bit scan forward.
		jne return_eax_plus_96	// Jump if zf flag is set (i.e. ecx was not zero)

return_minus_2:
		mov eax,-2
		jmp return_eax
return_eax_plus_96:
		add eax,32
return_eax_plus_64:
		add eax,32
return_eax_plus_32:
		add eax,32
return_eax:
	}
}
#else
int GetFirstId(IdFlags &Flags,int i)
{
	_asm
	{
		mov ebx,1
		mov eax,i
		add eax,1			// step forward 1 (we start from -1)
		mov ecx,eax			
		and ecx,31			// First five bits - loops at 32.
		shl ebx,cl			// Shift by the low 5 bits of ecx.
		mov edx,Flags
//	if(!Flags)
//		return -2;

		cmp eax,32			// Do we want to start at the second 32-bit number?
		jnb test_32			// Then ignore this test and test the 2nd word

		mov ecx,[edx]
		cmp ecx,0
		jne non_zero
		mov eax,32			// jump forward to the 32nd bit
		mov ebx,1
test_32:

		cmp eax,64			// Do we want to start at the third 32-bit number?
		jnb test_64			// Then ignore this test and test the 3nd word

		mov ecx,[edx+4]		// First 32 bits are zero. So we should start at 2nd loop or later.
		cmp ecx,0
		jne ge_32			// 2nd 32 bits are non-zero, so start from there.
		mov eax,64			// jump forward to the 64th bit
		mov ebx,1
//	if(!Flags.Second64Bits)
//		return -2;
test_64:

		cmp eax,96			// Do we want to start at the fourth 32-bit number?
		jnb test_96			// Then ignore this test and test the 4th word

		mov ecx,[edx+8]
		cmp ecx,0
		jne ge_64
		mov eax,96			// jump forward to the 96th bit
		mov ebx,1
test_96:

		mov ecx,[edx+12]
		cmp ecx,0
		jne ge_96
		mov eax,-2
		jmp return_eax
non_zero:
//	for(i++;i<64;i++)
//	{
//		if(Flags.First64Bits&(OneBitShifted[i].First64Bits))
//			return i;
//	}
		mov ecx,[edx]		// First 32 bits of flags
first_loop:
			mov esi,ecx
			and esi,ebx		// first 32 bits & 1<<i
			jne return_eax
			shl ebx,1		// obs next.
			add eax,1
			cmp eax,32
jne first_loop
		mov ebx,1			// Reset test to 1.
ge_32:
		cmp eax,64
		jge ge_64
		mov ecx,[edx+4]		// Next 32 bits of flags
second_loop:
			mov esi,ecx
			and esi,ebx		// first 32 bits & 1<<i
			jne return_eax
			shl ebx,1		// obs next.
			add eax,1
			cmp eax,64
jne second_loop
//	for(i;i<128;i++)
//	{
//		if(Flags.Second64Bits&(OneBitShifted[i].Second64Bits))
//			return i;
//	}
		mov ebx,1			// Reset test to 1.
ge_64:
		cmp eax,96
		jge ge_96
		mov ecx,[edx+8]		// Next 32 bits of flags
third_loop:
			mov esi,ecx
			and esi,ebx		// first 32 bits & 1<<i
			jne return_eax
			shl ebx,1		// obs next.
			add eax,1
			cmp eax,96
jne third_loop
		mov ebx,1			// Reset test to 1.
ge_96:
		mov ecx,[edx+12]	// Next 32 bits of flags
		cmp ecx,0
		je none_left
fourth_loop:
			mov esi,ecx
			and esi,ebx		// first 32 bits & 1<<i
			jne return_eax
			shl ebx,1		// obs next.
			add eax,1
			cmp eax,128
jne fourth_loop
//	return -2;
none_left:
		mov eax,-2
		jmp return_eax
return_eax:
	}
}
#endif
#endif

///  \brief Get the number of non-zero bits.
int NumFlags(const IdFlags &f)
{
	int num=0;
	int Id=-1;
	while((Id=GetNextId(f,Id))>=0)
		num++;
	return num;
}

///  \brief Clear bit \em i, and shift the higher bits right by one.
void CutFlag(IdFlags &f,int i)
{
	IdFlags f2;
	int Id=-1;
	while((Id=GetNextId(f,Id))>=0)
	{
		if(Id<i)
			f2|=IntToFlag(Id);
		else if(Id>i)
			f2|=IntToFlag(Id-1);
	}
	f=f2;
}

IdFlags OneBitShifted[128];
struct InitShifted
{
	InitShifted()
	{
		for(int i=0;i<128;i++)
			OneBitShifted[i]=LeftShift1(i);
	}
};
InitShifted init;
