/*
Internal blocker for Multithread library simulates blocking on posix threads
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

#ifndef _MULTITHREAD_INTERNAL_LINUX_BLOCKER_H_
#define _MULTITHREAD_INTERNAL_LINUX_BLOCKER_H_

#include <queue>
#include "../SyncRefCount.h"

namespace MultiThread
{

    class BlockerSlot;
    class BlockerPlug: public SyncRefCount
    {
        pthread_condattr_t _blockerattr;
        pthread_cond_t _blockervar;
        pthread_mutex_t _mutex;
        bool _sigstate;
        BlockerSlot *_sender;
    public:
    
        BlockerPlug();
        virtual ~BlockerPlug();
        
        void EnterToBlockingPhase();
        void ExitBlockingPhase();
        
        ///Waits for any signal
        /**
         * @param timeout defines timeout in miliseconds. Default value means infinite. Zero always return false.
         * @retval true Wait successfull, object has been signaled.
         * @retval false Wait unsuccessfull. Use GetSignalState to retrieve state. If this state is true,
         * than wait was unsucessfull due timeout. Otherwise, wait was interrupted by an error condition.
         */
        bool Wait(struct timespec *timeout=0);
        
        ///signals waiting thread
        /**
         * @param state state connected to signal. If true, thread has been signaled in normal condition.
         * If false, thread has been signaled in error condition - in this case, thread should
         * not continue normally, because it doesn't get ownership of object. 
         */
        void Signal(BlockerSlot *sender, bool state);
                
        bool GetSignalState() const {return _sigstate;}
        
        BlockerSlot *GetSignalSender() const {return _sender;}
        
        static struct timespec *CalcTimeout(unsigned long timeout,struct timespec &storage); 
    };

    class BlockerSlot
    {
        typedef std::queue<Ref<BlockerPlug> > PlugQueue;
        PlugQueue _plugQueue;        
        pthread_mutex_t _mutex;

    protected:
         /** Subscibes plug into queue
         */
        void Subscribe(BlockerPlug *plug, bool nowait);
    

    public:
    
    	BlockerSlot();
    	virtual ~BlockerSlot();
        
        
        /**
         * @param plug plug that want acquire object
         * @param nowait if true, function will not register the object
         * @retval false object has been added into queue - you must wait for the object. After 
         *  wait successed, object gets ownership automatically
         * @retval true object gets ownership
         */
        virtual bool Acquire(BlockerPlug *plug, bool nowait=false);
        //called by wait function, when plug want to release ownership of slot
        virtual void Release(BlockerPlug *plug) {}
        //called when plug timeouted waiting and it wants to signoff from object
        virtual void SignOff(BlockerPlug *plug);
        
        Ref<BlockerPlug> ReleaseOne(bool state=true, bool locked=false);
        
        void ReleaseAll(bool state=true, bool locked=false);
        
        void Lock();
        
        void Unlock();
        
                
    };

}

#endif /*BLOCKER_H_*/
