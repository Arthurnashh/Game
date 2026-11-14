#include "wpch.hpp"
#include "depMake.hpp"
#include <El/Interfaces/iFileMake.hpp>

static FileMakeFunctions SMakeFunctionsDefault;

FileMakeFunctions *GFileMakeFunctions = &SMakeFunctionsDefault;

RString GlobResolveTexture(RString name)
{
  // dummy implementation - no resolving available
  return name;
}
