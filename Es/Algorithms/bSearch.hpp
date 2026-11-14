#ifdef _MSC_VER
#pragma once
#endif

#ifndef __SUMA_CLASS_B_SEARCH_HPP
#define __SUMA_CLASS_B_SEARCH_HPP

/*!
\file
Binary search template
Adapted from Microsoft Visual C++ CRT source:
*bsearch.c - do a binary search
*       Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*/

//! binary search template
/*!
Type - searched type
KeyType - key type
Compare - functor: compare two values: int Compare(KeyType a, Type b).
\param data array pointer
\param num number of array elements
\param compare comparison functor
\param key searched value
\return any value that matches given key, -1 if no such value is found
*/

template <class Type,class KeyType,class Compare>
int BSearch
(
  const Type *data, size_t num,
  Compare compare, KeyType key
)
{
	int lo = 0;
	int hi = num - 1;
	int mid;
	unsigned int half;
	int result;

	while (lo <= hi)
	{
		if (half = num / 2)
		{
			mid = lo + (num & 1 ? half : (half - 1));
			if (!(result = compare(key,data[mid])))
			{
				return mid;
			}
			else if (result < 0)
			{
				hi = mid - 1;
				num = num & 1 ? half : half-1;
			}
			else
			{
				lo = mid + 1;
				num = half;
			}
		}
		else if (num)
		{
			return compare(key,data[lo]) ? -1 : lo;
		}
		else
		{
			break;
		}
	}
	return -1;
}

//! binary search template
/*!
Type - searched type
KeyType - key type
Compare - functor: compare two values: int Compare(KeyType a, Type b).
\param data array pointer
\param num number of array elements
\param compare comparison functor
\param key searched value
\return index of any value that is equal to key
or (if no such value exists) first value that is greater than key,
or (if no such value exists) num
*/

template <class Type,class KeyType,class Compare>
int BSearchPos
(
  const Type *data, size_t num,
  Compare compare, KeyType key
)
{
	unsigned int lo = 0;
	unsigned int hi = num - 1;
	unsigned int mid;


	// always true:
	// key <= data[hi] or key>data[num-1]
	// key >= data[lo] or key<data[0]
	while (lo <= hi)
	{
		unsigned int half = num / 2;
		if (half)
		{
			mid = lo + (num & 1 ? half : (half - 1));
			int result = compare(key,data[mid]);
			if (result==0)
			{
				return mid;
			}
			else if (result < 0)
			{
				hi = mid - 1;
				num = num & 1 ? half : half-1;
			}
			else
			{
				lo = mid + 1;
				num = half;
			}
		}
		else if (num)
		{
			// half==0 && num!=0 => num==1
			Assert( hi==lo )
			int result = compare(key,data[lo]);
			if (result<=0) return lo;
			return hi+1;
		}
		else
		{
			return lo;
		}
	}
	return lo;
}

#endif

