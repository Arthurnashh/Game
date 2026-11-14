#ifdef _MSC_VER
#pragma once
#endif

/**
  * @file   stackMeter.hpp
  * @brief  Stack measurement for non-release builds.
  *
  * STACK_INIT(n)  fills stack segment with special pattern.
  * STACK_METER(n) determines how many bytes of the stack were used..
  * "n" tells how long stack segment should be probed.
  * <p>
  * Copyright &copy; 2003 by BIStudio (www.bistudio.com)
  * @author PE
  * @since  29.9.2003
  * @date   1.10.2003
  */

#ifndef _STACKMETER_HPP
#define _STACKMETER_HPP

// DEBUG target fills all automatic variables with some check-value
// SUPER_RELEASE target must not contain this stuff!
#if _SUPER_RELEASE

#define STACK_INIT(n)
#define STACK_METER(n) 0

#else

#define STACK_INIT(n)  StackInit<n>()
#define STACK_METER(n) StackMeter<n>()

const unsigned VALUE_0   = 0x36fc90a1;
const unsigned VALUE_INC = 113;

template <int size>
void StackInit ()
{
  unsigned value;
  unsigned i;
  unsigned stack[size>>2];                  // to be filled up
  //__asm { nop };
  for ( i = 0, value = VALUE_0; i < (size>>2); value += VALUE_INC )
    stack[i++] = value;
}

const unsigned METER_INC = 4096;
const unsigned METER_MATCH = 8;

template <int size>
unsigned StackMeter ()
{
  unsigned i;
  union
  {
    unsigned *uptr;
    char *cptr;
  } a;
  unsigned *ptr0;
  for ( ptr0 = &i; (&i - ptr0) < (size>>2); ptr0 -= (METER_INC>>2) )
  {
    a.uptr = ptr0;
      // try every 4-kilobyte border
    for ( i = 0; i++ < 4; a.cptr++ )
      if ( a.uptr[0] + VALUE_INC == a.uptr[1] )
      {
        i = 0;
        do
        {
          i++;
          a.uptr++;
        } while ( a.uptr[0] + VALUE_INC == a.uptr[1] );
          // segment ends
        if ( i > METER_MATCH )
            // a.uptr[0] is the last correct value
          return( size - (((a.uptr[0] - VALUE_0) / VALUE_INC) << 2) );
        break;
      }
  }
  return 0;
}

#endif

#endif
