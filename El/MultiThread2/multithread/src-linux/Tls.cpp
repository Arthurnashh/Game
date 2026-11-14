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

#include "common_linux.h"

#include "../Tls.h"

namespace MultiThread
{


static int AllocateIndex()
{
    pthread_key_t kk;
    int res=pthread_key_create(&kk,0);
    if (res!=0) 
        throw TlsOutOfIndexes;
     return res;
}


TlsBase::TlsBase(int zero) throw (TlsBaseExceptionEnum):_index(AllocateIndex()) {}
TlsBase::TlsBase(void) throw (TlsBaseExceptionEnum):_index(AllocateIndex()) {}

TlsBase::~TlsBase(void)
{
    pthread_key_delete((pthread_key_t)_index);
}
 
void TlsBase::SetValue(void *ptr)
{
  pthread_setspecific((pthread_key_t)_index, ptr);
}
void *TlsBase::GetValue()
{
  return pthread_getspecific((pthread_key_t)_index);
}

};
