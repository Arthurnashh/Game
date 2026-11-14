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

#ifndef _IRUNNABLE_INTERFACE_BISTUDIO_
#define _IRUNNABLE_INTERFACE_BISTUDIO_
#pragma once

#include "../common/IMultiInterface.h"

namespace MultiThread
{


  ///Common interface for all runnable classes. 
  /** 
  Interface is abstract.
  It is declares Run method
  */
  class IRunnable: public BredyLibs::IMultiInterface
  {
  public:
    ///Virtual destructor - You can destroy any runnable object
    virtual ~IRunnable(void) {}

    ///Run method 
    /**
    Callers: call this to start runnable class
    Implementors: implement any runnable...
    */
    virtual unsigned long Run()=0;
  };


};
#endif
