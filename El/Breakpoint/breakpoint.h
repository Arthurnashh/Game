// Copyright (c) 2000 Mike Morearty <mike@morearty.com>
// Original source and docs: http://www.morearty.com/code/breakpoint

#ifndef _BREAKPOINT_H_
#define _BREAKPOINT_H_

class MemoryBreakpoint
{
public:
	MemoryBreakpoint(); 
	~MemoryBreakpoint();

	// The enum values correspond to the values used by the Intel Pentium,
	// so don't change them!

	void Set(void* address, int len=4 /* 1, 2, or 4 */, bool read=false);
	void Clear();

protected:

	inline void SetBits(unsigned long& dw, int lowBit, int bits, int newValue)
	{
		int mask = (1 << bits) - 1; // e.g. 1 becomes 0001, 2 becomes 0011, 3 becomes 0111

		dw = (dw & ~(mask << lowBit)) | (newValue << lowBit);
	}

	bool _set[4]; // false means not set; true means we've set that hardware bp
  int Find() const
  {
    for (int i=0; i<4; i++)
    {
      if (!_set[i]) return i;
    }
    return -1;
  }
};

#endif // _BREAKPOINT_H_
