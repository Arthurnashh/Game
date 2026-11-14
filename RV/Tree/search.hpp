#ifndef _search_hpp_
#define _search_hpp_

//! This function will locate an item in the sorted array.
/*!
  This algorithm is based on the interval division.
  It has the time complexity log2(ItemsCount).
  Note that the array must be sorted in way to correspond the comp callback. Usually it
  is the smallest number at position 0;
  \param Array Array of items to locate required item in.
  \param ItemsCount Number of items of the array.
  \param RequiredItem Pointer to the similar item the index of we are looking for.
  \param comp Callback for items comparison.
  \return Index of the required item or -1 if there is no such in the array.
*/
template <class Type>
static int FindInSortedArray(
              Type *Array,
              int ItemsCount,
              Type *RequiredItem,
              int (*comp)(const Type *, const Type *)) {

  // Indexes of the first and last item of the interval to search in
  int First = 0;
  int Last = ItemsCount - 1;

  // Main cycle
  while (First <= Last) {

    // Index to the middle item
    int Middle = (First + Last) / 2;

    // Comparing of the RequiredItem and the item in the middle of the interval
    int CompareResult = comp(RequiredItem, &Array[Middle]);

    // Required item can be in the left interval
    if (CompareResult < 0) Last = Middle - 1;
    // Required item can be in the right interval
    else if (CompareResult > 0) First = Middle + 1;
    // We found the required item
    else return Middle;
  }

  // If there was no required item in the array...
  return -1;
}

#endif