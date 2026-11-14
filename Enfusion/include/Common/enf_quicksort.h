//-----------------------------------------------------------------------------
// File: enf_quicksort.h
//
// Desc: QuickSort template
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------
#pragma once
#ifndef ENF_QUICKSORT_H
#define ENF_QUICKSORT_H

#include "enf_config.h"
#include "common/enf_inttypes.h"

namespace enf
{
template<class T> class DefaultCompareT
{
public:
	int operator()(const T& a, const T& b) const
	{
		if(a < b) return -1;
		if(a > b) return 1;
		return 0;
	}
};

template<class T> class ReverseCompareT
{
public:
	int operator()(const T& a, const T& b) const
	{
		if(a > b) return -1;
		if(a < b) return 1;
		return 0;
	}
};

template<class T, class CompareT> void QuickSortCmp(uint num, T* base)
{
	const int ENF_CUTOFF = 8;	// testing shows that this is good value
	CompareT	compare;

	T* lo, *hi;					// ends of sub-array currently sorting
	T* mid;						// points to middle of subarray
	T* loguy, *higuy;			// traveling pointers for partition step
	uint size;				// size of the sub-array
	T* lostk[32];
	T* histk[32];
	int stkptr;					// stack for saving sub-array to be processed

	if (num < 2)
		return;					// nothing to do

	stkptr = 0;					// initialize stack
	lo = base;
	hi = base + num - 1;		// initialize limits

recurse:
	size = (hi - lo) + 1;			// number of el's to sort

	if (size <= ENF_CUTOFF)
	{
		T *p, *max;

		while (hi > lo)
		{
			max = lo;
			for (p = lo + 1; p <= hi; p++ )
			{
				if ( compare(*p, *max) > 0)
				{
					max = p;
				}
			}
			enf_swap(*max, *hi);
			hi--;
		}
	}
	else
	{
		mid = lo + (size / 2);	// find middle element
		if(compare(*lo, *mid) > 0)
			enf_swap(*lo, *mid);
		if(compare(*lo, *hi) > 0)
			enf_swap(*lo, *hi);
		if(compare(*mid, *hi) > 0)
			enf_swap(*mid, *hi);

		loguy = lo;
		higuy = hi;

		for (;;)
		{
			if(mid > loguy)
			{
				do
				{
					loguy++;
				} while (loguy < mid &&	compare(*loguy, *mid) <= 0 );
			}

			if(mid <= loguy)
			{
				do
				{
					loguy++;
				} while (loguy <= hi && compare(*loguy, *mid) <= 0 );
			}

			do
			{
				higuy--;
			} while (higuy > mid && compare(*higuy, *mid) > 0 );

			if (higuy < loguy)
				break;

			enf_swap(*loguy, *higuy);

			if (mid == higuy)
				mid = loguy;
		}

		higuy++;
		if (mid < higuy)
		{
			do
			{
				higuy--;
			} while (higuy > mid && compare(*higuy, *mid) == 0);
		}
		if (mid >= higuy)
		{
			do
			{
				higuy--;
			} while (higuy > lo && compare(*higuy, *mid) == 0);
		}

		if ( higuy - lo >= hi - loguy )
		{
			if (lo < higuy)
			{
				lostk[stkptr] = lo;
				histk[stkptr] = higuy;
				++stkptr;
			}									/* save big recursion for later */

			if (loguy < hi)
			{
				lo = loguy;
				goto recurse;				/* do small recursion */
			}
		}
		else
		{
			if (loguy < hi)
			{
				lostk[stkptr] = loguy;
				histk[stkptr] = hi;
				++stkptr;					/* save big recursion for later */
			}

			if (lo < higuy)
			{
				hi = higuy;
				goto recurse;				/* do small recursion */
			}
		}
	}

	--stkptr;
	if (stkptr >= 0)
	{
		lo = lostk[stkptr];
		hi = histk[stkptr];
		goto recurse;	// pop subarray from stack
	}

	return;				// all subarrays done
}


template<class T> void QuickSort(uint num, T* base)
{
	QuickSortCmp<T, DefaultCompareT<T> >(num, base);
}

}
#endif //ENF_QUICKSORT_H