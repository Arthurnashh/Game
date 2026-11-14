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
#include "waitFunctions.h"
#include "../waitFunctions.h"
#include "Blocker.h"
#include "PosixThread.h"

namespace MultiThread
{
namespace WaitFn
{
  bool DefaultWait(ThreadWaitInfo &waitInfo, unsigned long timeout)
  { 
    //space to store timeout time
    struct timespec storage;
    //calculate timeout time. You must use returned value, because some values have special meaning
    struct timespec *ts=BlockerPlug::CalcTimeout(timeout,storage);
    //get active thread
    Ref<BlockerPlug> plug=PosixThread::GetActiveThreadPlug();
    if (plug==0) return false;
    
    //wait all?
    if (waitInfo.waitAll)
    {
        //in wait all state, we will repeatly acquire all object, until they will by acquired
        bool res;
        //set starting position
        int pos=0;
        //last wait state
        bool waitstate;        
        do
        {
            //enter to blocking phase 
            plug->EnterToBlockingPhase();
            //new cycle, update count
            int count=waitInfo.count;
            //acquire the first object - in succes, set state to okay
            if (waitInfo.hList[pos]->Acquire(plug)) waitstate=true;
            //else try to wait
            else waitstate=plug->Wait(ts);
            //when waitstate is successfull        
            if (waitstate)
            {                
                //dec count of object
                count--;
                //save current pos (we will need it to the restore phase)
                int savepos=pos;
                //advance to the next object
                pos=(pos+1)%waitInfo.count;
                //until there is any unprocessed object
                while (count)
                {                    
                    //try to acquire (without waiting) - when failed, stop
                    if (!waitInfo.hList[pos]->Acquire(plug,true)) break;
                    //dec count of object remain
                    count--;
                    //advance to the next object
                    pos=(pos+1)%waitInfo.count;
                }
                //when count reaches the zero
                //all obejcts are signaled               
                //otherwise make some cleanup 
                if (count!=0)
                {   
                    //walk through owned object
                    while (savepos!=pos)
                    {
                        //release then
                        waitInfo.hList[savepos]->Release(plug);
                        //advance
                        savepos=(savepos+1)%waitInfo.count;
                    }
                    //pos now points to object that failed, okay, lets to try to acquire it                                       
                }
                else
                {
                    res=true;
                    waitstate=false;
                    waitInfo.index=1;
                }
                                
                                  
            }
            else
            {
                ///timeout - signoff self
                waitInfo.hList[pos]->SignOff(plug);
                res=false;                
            }
            plug->ExitBlockingPhase();
        }
        while (waitstate);
        return res;
    }
    else //waitAll is false
    {
        //in this point, we will wait for any of object.
        //enter to blocking phase 
        #ifdef DEBUG_MULTI_WAIT
        printf("Entering blocking phase (waitAll==false) ");
        #endif
        plug->EnterToBlockingPhase();
        #ifdef DEBUG_MULTI_WAIT
        printf("DONE\n");
        #endif       
        //register plug into each slot
        for (int i=0;i<waitInfo.count;i++)
        {
            //if there is free slot
            #ifdef DEBUG_MULTI_WAIT
            printf("Trying to Acquire plug ");
            #endif
            if (waitInfo.hList[i]->Acquire(plug,false))
            {
                #ifdef DEBUG_MULTI_WAIT
                printf(" ACQUIRED\n");
                #endif
                //work is nearly done, unregister currently registered slots
                for (int j=0;j<i;j++)  waitInfo.hList[j]->SignOff(plug);
                //mark index
                waitInfo.index=i+1;
                //exit from blocking phase
                plug->ExitBlockingPhase();
                //done okay
                return true;
            }
            #ifdef DEBUG_MULTI_WAIT
            printf(" DONE - not acquired\n");
            #endif
        }
        //registration done, now we can sit and wait
        bool res=plug->Wait(ts);
        //timeout?
        if (!res)
        {
            //unregister from all objects
            for (int i=0;i<waitInfo.count;i++)
            {
                waitInfo.hList[i]->SignOff(plug);                
            }
            waitInfo.index=0;
        }
        //okay?
        else
        {
            //find which object signaled the plug
            //and unregister remaining slots 
            for (int i=0;i<waitInfo.count;i++)
            {
                if (waitInfo.hList[i]==plug->GetSignalSender())
                    waitInfo.index=i+1;
                else
                    waitInfo.hList[i]->SignOff(plug);                
            }            
        }
        //leave blocking area
        plug->ExitBlockingPhase();
        //result    
        return res;
    }
  }

  bool UIWait(ThreadWaitInfo &waitInfo, unsigned long timeout)
  {
    return DefaultWait(waitInfo,timeout);
  }
};

};
