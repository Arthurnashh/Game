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
#include "common_linux.h"
#include "Blocker.h"
#include <sys/time.h>

namespace MultiThread
{

    BlockerSlot::BlockerSlot()
    {
        pthread_mutex_init(&_mutex,0);        
    }
    
    BlockerSlot::~BlockerSlot()
    {
        AssertDesc(_plugQueue.empty(),"Destroying object in use!");        
        ReleaseAll(false);
        pthread_mutex_destroy(&_mutex);
    }

    BlockerPlug::BlockerPlug()
    {
        pthread_condattr_init( &_blockerattr);
        pthread_condattr_setclock( &_blockerattr, CLOCK_MONOTONIC);
        pthread_cond_init(&_blockervar,&_blockerattr);
        pthread_mutex_init(&_mutex,0);
        _sender=0;
        _sigstate=false;
    }
    
    BlockerPlug::~BlockerPlug()
    {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_blockervar);      
    }

    void BlockerSlot::Subscribe(BlockerPlug *plug, bool nowait)
    {
        if (nowait) return;
        _plugQueue.push(plug);
    }

    bool BlockerSlot::Acquire(BlockerPlug *plug, bool nowait)
    {
        Lock();
        Subscribe(plug,nowait);
        Unlock();
        return false;                                   
    }
    
    Ref<BlockerPlug> BlockerSlot::ReleaseOne(bool state,bool locked)
    {
        //zamkni se
        if (!locked) Lock();
        //sem dej to co je na top
        Ref<BlockerPlug> pp;
        //prazdna fronta, hmm, tak nic
        if (_plugQueue.empty()) pp=0;
        else
        {
            //pro kontrolu
            Ref<BlockerPlug> pp2;
            do
            {
                //vyzvedni vrsek fronty
                pp=_plugQueue.front();
                //protoze ten kdo je na vrcholu muze v tuto chvili byt 
                //ve stavu vyprseni timeout a zadat o odebrani ze seznamu
                //odemkni nejprve slot
                Unlock();
                //pak pozadej o pristup do plugi
                //mozna ze vlakno nyni rusi registraci.                    
                pp->EnterToBlockingPhase();
                //opet zamkni sama sebe
                Lock();
                //pokud je nyni fronta prazdna
                if (_plugQueue.empty())
                {                    
                    pp->ExitBlockingPhase();
                    //neni co delat
                    pp=0;
                    //konec
                    break;
                }
                //ziskej opet vrchol fronty
                pp2=_plugQueue.front();
                //pokud neni vrchol stejny,
                if (pp2!=pp) 
                {
                    //opust blokovaci fazi
                    pp->ExitBlockingPhase();
                }                   
            }
            //zkus to znova s novym vrcholem fronty
            while (pp2!=pp);
            //pokud je nejaky plug
            if (pp)
            {
                //vyhod ho z fronty
                _plugQueue.pop();
                //signalizuj mu, ze je pripraven
                pp->Signal(this,state);
                //ukonci blokujici fazi
                pp->ExitBlockingPhase();
            }
        }        
        //odemkni nas
        if (!locked) Unlock(); 
        return pp;                   
    }
    
    void BlockerSlot::ReleaseAll(bool state, bool locked)
    {
        while (ReleaseOne(state,locked));
    }


    void BlockerPlug::EnterToBlockingPhase()
    {
        pthread_mutex_lock(&_mutex);
    }
    void BlockerPlug::ExitBlockingPhase()
    {
        pthread_mutex_unlock(&_mutex);
    }
    
    void BlockerPlug::Signal(BlockerSlot *sender,bool state)
    {                    
        if (_sender==0) 
        {
            _sender=sender;
            _sigstate=state;
        }
        pthread_cond_signal(&_blockervar);
    }

    bool BlockerPlug::Wait(struct timespec *timeout)
    {
        _sigstate=true;
        _sender=0;
        if (timeout==0)
        {
            pthread_cond_wait(&_blockervar,&_mutex);
            return _sigstate;
        }
        else
        {       
              int rc;
              rc = pthread_cond_timedwait(&_blockervar, &_mutex, timeout);
              if (rc==0) return _sigstate;
              return false;                  
        }
    }
    
    struct timespec *BlockerPlug::CalcTimeout(unsigned long timeout,struct timespec &ts)
    {
      if (timeout==(unsigned long)-1) return 0;

      struct timespec ts_now;
      clock_gettime(CLOCK_MONOTONIC, &ts_now);

      unsigned long nsec = ts_now.tv_nsec + timeout%1000*1000000; 

      ts.tv_sec = (ts_now.tv_sec + timeout / 1000)
                 + nsec / 1000000000;
      ts.tv_nsec = nsec % 1000000000;
            
      return &ts;
    }
    
    void BlockerSlot::SignOff(BlockerPlug *plug)
    {       
        Lock();
        if (!_plugQueue.empty())
        {
            if (_plugQueue.front()==plug)
            {
                _plugQueue.pop();                
            }
            else
            {
                Ref<BlockerPlug> top=_plugQueue.front();
                Ref<BlockerPlug> x=top;
                do
                {
                    _plugQueue.pop();
                    if (x!=plug) _plugQueue.push(x);                    
                    x=_plugQueue.front();
                }
                while (x!=top);                
            }            
        }
        Unlock();
        
    }
    
    void BlockerSlot::Lock()
    {
        pthread_mutex_lock(&_mutex);        
    }
    
    void BlockerSlot::Unlock()
    {
        pthread_mutex_unlock(&_mutex);        
    }
}

