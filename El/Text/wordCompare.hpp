// word comparision and similiarity testing

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _EL_WORDCOMPARE_HPP
#define _EL_WORDCOMPARE_HPP

//! compare whole words
/*!
\param a first word to compare
\param b second word to compare
\param maxAcceptable treshold when words can be quick rejected
*/

float WordDifference(const char *a, const char *b, float maxAcceptable=2);
//! compare whole words, including substring match

/*!
\param a compared word
\param b the word we search for (search mask)
\param maxAcceptable treshold when words can be quick rejected
*/

float WordDifferenceSubstring(const char *a, const char *b, float maxAcceptable=2);

#endif
