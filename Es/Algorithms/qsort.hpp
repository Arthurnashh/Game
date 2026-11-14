#ifdef _MSC_VER
#pragma once
#endif

#ifndef _QSORT_HPP
#define _QSORT_HPP


// adapted from:

/***
*qsort.c - quick sort algorithm; qsort() library function for sorting arrays
*
*       Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       To implement the qsort() routine for sorting arrays.
*
*******************************************************************************/



/***
*Swap(a, b, width) - swap two elements
*
*Purpose:
*       swaps the two array elements of size width
*
*Entry:
*       char *a, *b = pointer to two elements to swap
*       unsigned width = width in bytes of each array element
*
*Exit:
*       returns void
*
*Exceptions:
*
*******************************************************************************/

template <class Type>
static inline void SwapQ(Type *p, Type *q )
{
  Type tmp=*p;
  *p=*q;
  *q=tmp;
}


/***
*shortsort(lo, hi, width, comp) - insertion sort for sorting short arrays
*
*Purpose:
*       sorts the sub-array of elements between lo and hi (inclusive)
*       side effects:  sorts in place
*       assumes that lo < hi
*
*Entry:
*       char *lo = pointer to low element to sort
*       char *hi = pointer to high element to sort
*       unsigned width = width in bytes of each array element
*       int (*comp)() = pointer to function returning analog of strcmp for
*               strings, but supplied by user for comparing the array elements.
*               it accepts 2 pointers to elements and returns neg if 1<2, 0 if
*               1=2, pos if 1>2.
*
*Exit:
*       returns void
*
*Exceptions:
*
*******************************************************************************/

// functor implementation
template <class Type,class Compare>
static void ShortSort(Type *lo, Type *hi, Compare comp)
{
  Type *p, *max;

  /* Note: in assertions below, i and j are alway inside original bound of
  array to sort. */

  while (hi > lo)
  {
    /* A[i] <= A[j] for i <= j, j > hi */
    max = lo;
    for (p = lo+1; p <= hi; p += 1)
    {
      /* A[i] <= A[max] for lo <= i < p */
      if (comp(p, max) > 0) {
        max = p;
      }
      /* A[i] <= A[max] for lo <= i <= p */
    }

    /* A[i] <= A[max] for lo <= i <= hi */

    SwapQ(max, hi);

    /* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

    hi -= 1;

    /* A[i] <= A[j] for i <= j, j > hi, loop top condition established */
  }
  /* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
  so array is sorted */
}

/* this parameter defines the cutoff between using quick sort and
insertion sort for arrays; arrays with lengths shorter or equal to the
below value use insertion sort */

#define CUTOFF 8            /* testing shows that this is good value */


/***
*qsort(base, num, wid, comp) - quicksort function for sorting arrays
*
*Purpose:
*       quicksort the array of elements
*       side effects:  sorts in place
*
*Entry:
*       char *base = pointer to base of array
*       unsigned num  = number of elements in the array
*       unsigned width = width in bytes of each array element
*       int (*comp)() = pointer to function returning analog of strcmp for
*               strings, but supplied by user for comparing the array elements.
*               it accepts 2 pointers to elements and returns neg if 1<2, 0 if
*               1=2, pos if 1>2.
*
*Exit:
*       returns void
*
*Exceptions:
*
*******************************************************************************/

/* sort the array between lo and hi (inclusive) */


/// functor based implementation
template <class Type, class Compare>
void QSort(Type *base, int num, Compare comp)
{
  Type *lo, *hi;              /* ends of sub-array currently sorting */
  Type *mid;                  /* points to middle of subarray */
  Type *loguy, *higuy;        /* traveling pointers for partition step */
  unsigned size;              /* size of the sub-array */
  Type *lostk[30], *histk[30];
  int stkptr;                 /* stack for saving sub-array to be processed */

  if (num < 2 )
    return;                 /* nothing to do */

  stkptr = 0;                 /* initialize stack */

  lo = base;
  hi = base + (num-1);        /* initialize limits */

recurse:

  size = (hi - lo) + 1;        /* number of el's to sort */

  if (size <= CUTOFF)
  {
    ShortSort(lo, hi, comp );
  }
  else
  {

    mid = lo + (size / 2);      /* find middle element */
    SwapQ(mid, lo);               /* swap it to beginning of array */

    loguy = lo;
    higuy = hi + 1;

    for (;;)
    {
      do
      {
        loguy ++;
      } while (loguy <= hi && comp(loguy, lo) <= 0);

      do
      {
        higuy --;
      } while (higuy > lo && comp(higuy, lo) >= 0);

      if (higuy < loguy)
        break;

      SwapQ(loguy, higuy);

    }

    SwapQ(lo, higuy);     /* put partition element in place */

    if ( higuy - 1 - lo >= hi - loguy )
    {
      if (lo + 1 < higuy)
      {
        lostk[stkptr] = lo;
        histk[stkptr] = higuy - 1;
        ++stkptr;
      }                           /* save big recursion for later */

      if (loguy < hi)
      {
        lo = loguy;
        goto recurse;           /* do small recursion */
      }
    }
    else
    {
      if (loguy < hi)
      {
        lostk[stkptr] = loguy;
        histk[stkptr] = hi;
        ++stkptr;               /* save big recursion for later */
      }

      if (lo + 1 < higuy)
      {
        hi = higuy - 1;
        goto recurse;           /* do small recursion */
      }
    }
  }

  --stkptr;
  if (stkptr >= 0)
  {
    lo = lostk[stkptr];
    hi = histk[stkptr];
    goto recurse;           /* pop subarray from stack */
  }
  else
    return;                 /* all subarrays done */
}

// context passing version

template <class Type, class ContextType, class Compare>
class ContextAdaptor
{
  const ContextType &_ctx;
  const Compare &_cmp;

public:
  ContextAdaptor(const Compare &cmp, const ContextType &ctx):_ctx(ctx),_cmp(cmp){}

  int operator () (const Type *a, const Type *b) const
  {
    return _cmp(a,b,_ctx);
  }
};

// context passing version
template <class Type,class ContextType, class Compare >
void QSort(Type *base, int num, ContextType context, Compare comp)
{
  ContextAdaptor<Type,ContextType,Compare> adapt(comp,context);
  QSort<Type,ContextAdaptor<Type,ContextType,Compare> >(base,num,adapt);
}

///QSort can accept Array type
template <class Array,class ContextType, class Compare >
void QSort(Array &array, ContextType context, Compare comp)
{
  QSort(array.Data(),array.Size(),context,comp);
}

///QSort can accept Array type
template <class Array,class Compare >
void QSort(Array &array, Compare comp)
{
  QSort(array.Data(),array.Size(),comp);
}

#endif
