#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DEP_MAKE_HPP
#define _DEP_MAKE_HPP

#include <El/Interfaces/iFileMake.hpp>

extern FileMakeFunctions *GFileMakeFunctions;

/// resolve a texture name, perform a conversion if necessary
RString GlobResolveTexture(RString name);

#endif
