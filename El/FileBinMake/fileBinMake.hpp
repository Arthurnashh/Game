#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_FILE_MAKE_HPP
#define _EL_FILE_MAKE_HPP

#include <El/Interfaces/iFileMake.hpp>

class BinMakeFunctions: public FileMakeFunctions
{
public:
  //! Virtual method
  virtual int Make(const char *src, const char *tgt, const char *rules, BinMakeHandle *handle);
};

#endif
