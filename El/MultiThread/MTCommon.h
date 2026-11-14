/**
This header defines references for all common headers, such as El/Es headers, etc used by MultiThread library widely.
*/

#ifndef _USE_BREDY_LIBS
#include <es/containers/array.hpp>


#else
#include <Assert.h>
#include <Arrays.h>
#define lenof(x) (sizeof(x)/sizeof(x[0]))
#endif
