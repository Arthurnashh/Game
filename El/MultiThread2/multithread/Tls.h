/*
Copyright (C)2006 bo Ondrej Novak
http://bredy.jinak.cz
novak.on@centrum.cz


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "../common/Assert.h"

#pragma once

#pragma warning( disable : 4290 )

namespace MultiThread
{


  enum TlsBaseExceptionEnum {TlsOutOfIndexes};

  class TlsBase
  {
    int _index;
  public:
    TlsBase(void) throw (TlsBaseExceptionEnum);
    TlsBase(int zero) throw (TlsBaseExceptionEnum);
    ~TlsBase(void);
    void SetValue(void *ptr);
    void *GetValue();
  };

template<class T>
class Tls:public TlsBase
{
public:
    Tls(void) {}
    Tls(int zero):TlsBase(zero) 
    {
        AssertDesc(zero==0,"Only zero can be assigned to TLS pointer");
    }
  void SetValue(T *ptr)
  {
    TlsBase::SetValue(reinterpret_cast<void *>(ptr));
  }

  T *GetValue()
  {
    return reinterpret_cast<T *>(TlsBase::GetValue());
  }

  T *operator=(T *val) {SetValue(val);return val;}
  operator T *() {return GetValue();}
  T *operator->() {return GetValue();}
};

};

///Declaration of TLS variable
/**
* Each variable declared as TLS can hold different value for each thread
* TLS variables always holds only pointers. If you want to create thread
* specific data, just allocated space on heap and store pointer to the TLS
* variable. Don't forget to free memory before thread exits
*
* @note Count of TLS variables is limited
*
* @note In MSVC, macro uses _declspec(thread), that allows unlimited count of
* TLS variables. This technique does not work in dynamically loaded DLLs (through
* LoadLibrary). If you plan to use TLS in this kind of DLL, define _DLL_BUILD
* symbol in you project configuration. Then TLS macro will use OS TLS services
*/

#if _MSC_VER>1000 && !defined(_DLL_BUILD)
#define TLS(type) _declspec(thread) type *
#else
#define TLS(type) MultiThread::Tls<type>
#endif

