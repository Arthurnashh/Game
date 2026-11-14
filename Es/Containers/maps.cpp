/**
  @file   maps.cpp
  @brief  General hash-map templates

  Copyright (C) 2001-2003 by BIStudio (www.bistudio.com)
  @author PE
  @since 21.11.2001
  @date  5.10.2003
*/

#include "Es/essencepch.hpp"
#include "array.hpp"
#include "Es/Common/global.hpp"
#include "maps.hpp"

//------------------------------------------------------------
//  Support (traits):

template<> int ImplicitMapTraits<int>::null   = (int)0x80000000;
template<> int ImplicitMapTraits<int>::zombie = (int)0x80000001;

template<> unsigned ImplicitMapTraits<unsigned>::null   = 0xffffffff;
template<> unsigned ImplicitMapTraits<unsigned>::zombie = 0xfffffffe;

unsigned intToUnsigned (const int &id)
{
  return id;
}

//------------------------------------------------------------
//  hash-array size:

#define MaxPrime	24

const unsigned primes[MaxPrime] =
{
  5, 13, 31, 61, 127, 251, 509, 1021, 2039, 4051, 8111,
  16223, 32467, 64937, 129887, 259781, 519577, 1039169,
  2078339, 4156709, 8313433, 16626941, 33253889, 66507787
};          // that seems to be enough for 32-bit computers (maximum hash table size = 256MB !)

unsigned hashSize ( unsigned s )
{
  int i = 0;
  while ( i < MaxPrime && primes[i] < s ) i++;
  return primes[i];
}
