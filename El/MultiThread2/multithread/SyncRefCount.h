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

#ifndef _SYNCREFCOUNT_BREDYLIBS_MUTLITHREADS_
#define _SYNCREFCOUNT_BREDYLIBS_MUTLITHREADS_

#include "SyncTypes.h"

namespace MultiThread
{


  class SyncRefCount
  {
  protected:
    mutable SyncInt _count;
  public:

    SyncRefCount():_count(0) {}

    SyncRefCount( const SyncRefCount &src ):_count(0) 
    {
    }

    //! Copying of object (_count attribute will not be copied).
    void operator =( const SyncRefCount &src )
    {
    } 

    virtual ~SyncRefCount() {}

    long AddRef() const
    {
      return ++_count;
    }

    long Release() const
    {
      int ret=--_count;
      if (ret<1) OnUnused();
      return ret;

    }

    virtual void OnUnused() const {Destroy();}

    void Destroy() const
    {
      delete const_cast<SyncRefCount *>(this);
    }

    long GetRefCount() const
    {
      return _count;
    }
    long RefCounter() const
    {
      return _count;
    }
  };


};
#endif
