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

#if defined(_WIN32)
#include "../src-windows/common_Win.h"
#endif
#include "../MTCommon.h"
#include "../Synchronized.h"
//#include "../../common/LateConstructed.cpp"

/*
namespace MultiThread
{
  SynchronizeControlSingleton *SynchronizeControlSingleton::Instance()
  {
    static SynchronizeControlSingleton _instance;
    return &_instance;
  }


  bool SynchronizeControlSingleton::Lock(SynchronizeDesc *lockDesc, unsigned int timeout)
  {
    bool res;
    do
    {
      //enter to critical section of the lock
      _lock.Lock();
      //find any overlapping locked area
      LockMapIt found=_map.find(SynchronizeDesc::SynchronizeDescKeyPtr(lockDesc->GetKey()));
      //found?
      if (found==_map.end())
      {     
        //no, okay, we can lock this region
        _map.insert(LockPair(SynchronizeDesc::SynchronizeDescKeyPtr(lockDesc->GetKey()),lockDesc));
        //exit critical section
        _lock.Unlock();
        //return success
        return true;
      }

      //we found some region in collision 
      SynchronizeDesc *collision=found->second;
      //it is our region?
      if (collision->_owner==lockDesc->_owner)
      {
        //leave critical section        
        _lock.Unlock();
        //collision pointer is safe, because it is our, so no other thread can release it.

        //we found complette same region, or region is inside
        if (lockDesc->_key._begin>=collision->_key._begin && lockDesc->_key._end<=collision->_key._end)
        {
          //no special locking is needed;
          //write empty region - no unlock needed during Unlock()
          lockDesc->_key._begin=0;
          lockDesc->_key._end=0;
          //return success
          return true;
        }
        
        //regions are not the same, but they have same starting points
        if (lockDesc->_key._begin==collision->_key._begin)
        {
          //exclude already locked region
          lockDesc->_key._begin=collision->_key._end;
          //try to lock remaining region
          return Lock(lockDesc,timeout);
        }
        
        //regions are not the same, but they have same ending points
        if (lockDesc->_key._end==collision->_key._end)
        {
          //exclude already locked region
          lockDesc->_key._end=collision->_key._begin;
          //try to lock remaining region
          return Lock(lockDesc,timeout);       
        }
        //remaining part of program must solve situation, when locking region is bigger then already locked region, and both
        //(begin and end pointers) are outside of the region.
        
        //save requested region
        SynchronizeDesc::SynchronizeDescKey save=lockDesc->_key;
        //exclude already locked region on left
        lockDesc->_key._end=collision->_key._begin;

        //try to lock this region;
        if (!Lock(lockDesc,timeout)) return false;

        if (lockDesc->_key._begin==0 && lockDesc->_key._end==0)
        {
          //finally, locking on this side is not necesery
          //use descriptor to lock other side
          lockDesc->_key._begin=collision->_key._end;
          lockDesc->_key._end=save._end;
          return Lock(lockDesc,timeout);
        }

        //create new descriptor for other side
        SynchronizeDesc *additional=new SynchronizeDesc(collision->_key._end,save._end-collision->_key._end);
        
        if (!Lock(additional,timeout))
        {
          //lock failed, unlock and delete
          delete additional;
          Unlock(lockDesc);
          return false;
        }
        
        //right side don't need to be locked
        if (additional->_key._begin==0 && additional->_key._end==0)
        {
          //delete additional, and return success
          delete additional;
          return true;
        }
        
        //glue all parts (they can be created by recursive locking)
        SynchronizeDesc *nx=lockDesc;
        while (nx->_next!=0) nx=nx->_next;
        nx->_next=additional;

        //now lockDesc contains linked list of all pieces
        //we are done;
        return true;
      }

      //found collision region belogns to different thread
      //announce the region, that we want it
      collision->_announces++;
      //blocker was not inited yet?
      if (!collision->_blockOther.IsInited()) 
        //initialize blocker,in blocked mode
        new(collision->_blockOther) EventBlocker(EventBlocker::Blocked_ManualReset);
      //exit critical section, we going to wait
      _lock.Unlock();
      //wait for releasing the region
      res=collision->_blockOther->Acquire(timeout);    
      //wait done, so enter return into critical section 
      _lock.Lock();
      //unannounce self 
      collision->_announces--;
      //we are the latest leaving thread and owner thread waiting for us to finish work with descriptor
      if (collision->_announces==0 && collision->_blockMe.IsInited())
        //notify it, that descriptor is free
        collision->_blockMe->Unblock();
      //exit critical section
      _lock.Unlock();    
    }
    //depends on result of waiting
    //in case successful wait, repeat locking procedure, may be we will get region successfully
    while (res);
    //in case timeout, exit loop and return error
    return false;
  }

  void SynchronizeControlSingleton::Unlock(SynchronizeDesc *lockDesc)
  {    
    //first, unlock all connected parts
    if (lockDesc->_next)
    {
      Unlock(lockDesc->_next);
      delete lockDesc->_next;
      lockDesc->_next=0;
    }
    //no region defined, exit immediatelly    
    if (lockDesc->_key._begin==0 && lockDesc->_key._end==0) return;
    //enter critical section
    _lock.Lock();
    //remove region from the map
    _map.erase(SynchronizeDesc::SynchronizeDescKeyPtr(lockDesc->GetKey()));
    //is there any annoucemens?
    if (lockDesc->_announces)
    {
      //init blocker
      if (!lockDesc->_blockMe.IsInited()) new (lockDesc->_blockMe) EventBlocker(EventBlocker::Blocked_AutoReset);
      //we can leave the critical section
      _lock.Unlock();
      //unblock any waiting thread
      lockDesc->_blockOther->Pulse();
      //block me until all threads finish with this structure waiting
      lockDesc->_blockMe->Acquire();
    }
    else
    {
      //no announces, leave critical section and finish
      _lock.Unlock();
    }
  }
}
*/      
