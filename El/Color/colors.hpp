#ifdef _MSC_VER
#pragma once
#endif

#ifndef _COLORS_HPP
#define _COLORS_HPP

#if 0 //def _KNI
  // TODO: consider using SSE / AltiVec math
	#include "colorsK.hpp"
	#define Color ColorK
	#define HWhite HWhiteK
	#define HBlack HBlackK
#else
	#include "colorsFloat.hpp"
	#define Color ColorP
	#define HWhite HWhiteP
	#define HBlack HBlackP
#endif

#endif
