// Copyright (c) 2000 Mike Morearty <mike@morearty.com>
// Original source and docs: http://www.morearty.com/code/breakpoint
// Adapted by BIStudio

#include <El/elementpch.hpp>
#include "breakpoint.h"
#include <Es/Common/win.h>

void MemoryBreakpoint::Set(void* address, int len, bool read)
{
  #if _ENABLE_REPORT
    #if _M_IX86
	    // make sure this breakpoint isn't already set

	    CONTEXT cxt;
	    HANDLE thisThread = GetCurrentThread();

	    switch (len)
	    {
	    case 1: len = 0; break;
	    case 2: len = 1; break;
	    case 4: len = 3; break;
  	    default:
          Assert(false); // invalid length
          return;
	    }

	    // The only registers we care about are the debug registers
	    cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	    // Read the register values
	    if (!GetThreadContext(thisThread, &cxt))
      {
		    Fail("GetThreadContext");
        return;
      }

	    // Find an available hardware register
      int index = -1;
	    for (index = 0; index < 4; index++)
	    {
		    if ((cxt.Dr7 & (1 << (index*2))) == 0)
			    break;
	    }
      if (index>=4)
      {
        Fail("No more memory breakpoints available");
        return;
      }

	    switch (index)
	    {
	    case 0: cxt.Dr0 = (DWORD) address; _set[0]=true; break;
	    case 1: cxt.Dr1 = (DWORD) address; _set[0]=true; break;
	    case 2: cxt.Dr2 = (DWORD) address; _set[0]=true; break;
	    case 3: cxt.Dr3 = (DWORD) address; _set[0]=true; break;
	    default: Assert(false); // m_index has bogus value
	    }

	    int when = read ? 3 : 1;
	    SetBits(cxt.Dr7, 16 + (index*4), 2, when);
	    SetBits(cxt.Dr7, 18 + (index*4), 2, len);
	    SetBits(cxt.Dr7, index*2,        1, 1);

	    // Write out the new debug registers
	    if (!SetThreadContext(thisThread, &cxt))
      {
		    Fail("SetThreadContext");
      }
    #endif
  #endif
}


MemoryBreakpoint::MemoryBreakpoint()
{
  for (int i=0; i<4; i++)
  {
    _set[i]=0;
  }
}

MemoryBreakpoint::~MemoryBreakpoint()
{
  Clear();
}

void MemoryBreakpoint::Clear()
{
  #if _ENABLE_REPORT
    #if _M_IX86
      bool some = false;
	    for (int i=0; i<4; i++)
      {
        if (_set[i]) some = true;
      }
      if (some)
      {
		    CONTEXT cxt;
		    HANDLE thisThread = GetCurrentThread();

		    // The only registers we care about are the debug registers
		    cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;

		    // Read the register values
		    if (!GetThreadContext(thisThread, &cxt))
        {
  		    Fail("GetThreadContext");
          return;
        }
	      for (int i=0; i<4; i++)
	      {

		      // Zero out the debug register settings for this breakpoint
		      SetBits(cxt.Dr7, i*2, 1, 0);
	        switch (i)
	        {
	        case 0: cxt.Dr0 = 0; break;
	        case 1: cxt.Dr1 = 0; break;
	        case 2: cxt.Dr2 = 0; break;
	        case 3: cxt.Dr3 = 0; break;
          }
          _set[i] = false;
	      }
		    // Write out the new debug registers
		    if (!SetThreadContext(thisThread, &cxt))
        {
  		    Fail("SetThreadContext");
        }
      }
    #endif
  #endif
}
