/*
Wait functions for posix threads
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
#pragma once
//NOTE: Internal header - don't use in your project.

/* Types defined in this header is platform depend */

#ifdef _LINUX

namespace MultiThread
{


  struct ThreadWaitInfo
  {
    BlockerSlot **hList;
    int count;
    bool waitAll;
    int index;
    ThreadWaitInfo(BlockerSlot **h):hList(h),count(1),waitAll(false) {}
    ThreadWaitInfo(BlockerSlot **h,int count,bool waitAll):hList(h),
      count(count),waitAll(waitAll) {}
  };

};

#endif

