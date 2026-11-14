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

#pragma once
//NOTE: Internal header - don't use in your project.

/* Types defined in this header is platform depend */



namespace MultiThread
{
  struct ThreadWaitInfo;

  ///Various wait functions
  /**
  Each thread can define implementation of wait function. Depend on thread type,
  wait function can 

  - suspend thread normally.<br>
  - during waiting, watch and pop up system messages.<br>
  - detect APC states.<br>
  etc.

  Each thread can define different type of wait function, or implement own alternative. 
  All function defined in thread library will respect this (except "fast lockers", because
  they excepts, that thread will not wait for long time).

  To define wait function for thread, override WaitForSingleObject and WaitForMultiObject
  function in ThreadBase class

  If you want to extend this class, derive it, and define new sub-classes and implementation
  of desired type of wait functions.
  */
  namespace WaitFn
  {

    const unsigned long InfiniteWait=0xFFFFFFFF;
    ///Function defines normal waiting, thread is suspended with no exceptions.
    /**
    This is default behavior.
    */
    bool DefaultWait(ThreadWaitInfo &waitInfo, unsigned long timeout=InfiniteWait);
    
	 ///Function defines UI waiting, waiting thread can process all important messages
    bool UIWait(ThreadWaitInfo &waitInfo, unsigned long timeout=InfiniteWait);

	 ///Function defines AlterAble waiting, thread can process user APC routines during waiting
    /**
    Each waiting enters the thread into "alterable" state. Thread can process any APC routine.
    Instead of WinAPI, processing the APC routine will not interrupt the waiting.
    */
    bool AlterAbleWait(ThreadWaitInfo &waitInfo,unsigned long timeout=InfiniteWait);  
  };

};

