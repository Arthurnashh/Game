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

#include <stdio.h>
#include <ucontext.h>
#include "common_linux.h"
#include "Fiber.h"
#include "../Fiber.h"
#include "../Tls.h"

namespace MultiThread
{
    
    static size_t SFiberMinStack=4096;
    static size_t SFiberDefaultStack=65536;
    static size_t SFiberMaxStack=0x7FFFFFFF;
    
    void Fiber_SetStackParameters(size_t min, size_t def, size_t limit)
    {
        if (min) SFiberMinStack=min;
        if (def) SFiberDefaultStack=def;
        if (limit) SFiberMaxStack=limit;    
    }
    
    static TLS(Fiber) SActiveFiber;
    
    class Fiber_StartupClass
    {   
    friend class Fiber;
    friend class MasterFiber;
        void *_fibStack;
        ucontext_t _context;
        unsigned int _boundsCheck;
        
        static const unsigned int BoundsCheckMagic=0xABCDEF01;
    public:
    
        static void Start()
        {
            Fiber *fib=Fiber::GetCurrentFiber();
            DebugPrint2("Fiber has been created id: %08X, inside of the thread: %08X\n",fib,pthread_self());
            unsigned long res=fib->Run();
            DebugPrint3("Fiber has exited id: %08X, inside of the thread: %08X, exit code %d\n",fib,pthread_self(),res);
            fib->_exitCode=res;     
            Fiber_StartupClass *x=reinterpret_cast<Fiber_StartupClass *>(fib->_fiberHandle);
            free(x->_fibStack);
            fib->_fiberHandle=0;
            SActiveFiber=fib->_creator;
            fib->_creator=0;
            fib->_backPtr=0;
        }
    
        bool InitContext(void *stack, size_t size)
        {           
            _fibStack=stack;
            getcontext(&_context);
            _context.uc_stack.ss_sp=_fibStack;
            _context.uc_stack.ss_size=size;
            _context.uc_link=GetCreatorContext();
            makecontext(&_context,&Start,0);
            _boundsCheck=BoundsCheckMagic;
            *reinterpret_cast<unsigned int *>(_fibStack)=BoundsCheckMagic; 
            return true;
        }
        
        bool BoundsCheck()
        {
            AssertDesc(_boundsCheck==BoundsCheckMagic,"Detected a corruption fiber's stack structure - stack underflow!");
            if (_boundsCheck==BoundsCheckMagic)
            {
                AssertDesc(*reinterpret_cast<unsigned int *>(_fibStack)==BoundsCheckMagic,"Detected a corruption fiber's stack structure - stack overflow!");                
                if (*reinterpret_cast<unsigned int *>(_fibStack)==BoundsCheckMagic)
                {
                    return true;
                }
            }
            return false;
        }        
            
        static ucontext_t *GetActiveContext()
        {
            return &reinterpret_cast<Fiber_StartupClass *>(SActiveFiber->_fiberHandle)->_context;
        }
        
        static ucontext_t *GetCreatorContext()
        {
            return &reinterpret_cast<Fiber_StartupClass *>(SActiveFiber->_creator->_fiberHandle)->_context;
        }
    };
 
    Fiber::Fiber(size_t stackSize):_fiberHandle(0),_backPtr(0),_creator(0),_maxStack(stackSize),_exitState(false),_exitCode(0)    
    {
        
    }               
    
    
    bool Fiber::Start(StartMode mode)
    {
        AssertDesc(GetCurrentFiber()!=0,"No active fiber, did you constructed the MasterFiber?");
        AssertDesc(GetCurrentFiber()!=this,"Fiber is already started");
        AssertDesc(_fiberHandle==0,"Fiber is already started");

        size_t stackSize=_maxStack;
        if (stackSize==0) stackSize=SFiberDefaultStack;
        if (stackSize<SFiberMinStack) stackSize=SFiberMinStack;
        if (stackSize>SFiberMaxStack) stackSize=SFiberMaxStack;
        
        
        void *stack=malloc(stackSize+sizeof(Fiber_StartupClass));
        AssertDesc(stack!=0,"Fiber stack allocation failed!");
        if (stack==0) return false;
        
        void *clsPtr=reinterpret_cast<char *>(stack)+stackSize;
        Fiber_StartupClass *startup=new(clsPtr) Fiber_StartupClass;
        
        bool init=startup->InitContext(stack,stackSize);
        AssertDesc(init,"Fiber context initialization failed!");
        if (!init) return false;
        
        
        _fiberHandle=startup;
        _creator=GetCurrentFiber()->_creator;
        
        
        if (mode==StartNormal) Resume();
                
        return true;
    }        
    
    bool Fiber::Resume()
    {
        Fiber *active=SActiveFiber;
        AssertDesc(active!=this,"Fiber cannot resume itself.");
        
        if (active==this) return false;
        
        if (!IsActive())
        {
            DebugPrint1("Cannot resume inactive fiber (%08X)\n",this);
            return false;
        }
        
        Fiber_StartupClass *oldsc=reinterpret_cast<Fiber_StartupClass *>(active->_fiberHandle);
        Fiber_StartupClass *newsc=reinterpret_cast<Fiber_StartupClass *>(_fiberHandle);
        AssertDesc(oldsc!=0,"Current fiber has no descriptor! Did you constructed MasterFiber");
        AssertDesc(newsc!=0,"Cannot resume stopped fiber. Call Start before Resume");
        if (oldsc==0 || newsc==0) return false;
#ifdef _DEBUG        
        if (oldsc->BoundsCheck()==false) return false;
        if (newsc->BoundsCheck()==false) return false;
#endif
        ucontext_t *oldc=&oldsc->_context;
        ucontext_t *newc=&newsc->_context;      
        
        _backPtr=active;     
        
        SActiveFiber=this;   
        
        int res=swapcontext(oldc,newc);
        AssertDesc(res==0,"Fiber swapping failed");
        return res==0;;        
    }
          
    Fiber *Fiber::GetCurrentFiber()
    {
        return SActiveFiber;
    }
    
    bool Fiber::Stop()
    {
        Fiber *active=SActiveFiber;
        AssertDesc(active!=this,"Fiber cannot stop self.");
        if (active==this) return false;        

        Fiber_StartupClass *x=reinterpret_cast<Fiber_StartupClass *>(_fiberHandle);
        free(x->_fibStack);
        _fiberHandle=0;
        _creator=0;
        _backPtr=0;
         DebugPrint("Fiber id %08X has been terminated. Note, terminating fibers can cuase memory/resource leaks\n",this);
        return true;        
    }

    bool Fiber::Yield()
    {
        AssertDesc(this==SActiveFiber,"Cannot Yield control of suspended fiber");
        Fiber *next=_backPtr;
        _backPtr=SActiveFiber;
        return  next->Resume();
        
    }

    Fiber::~Fiber()
    {
        if (IsActive()) Stop();
    }

    MasterFiber::MasterFiber()
    {
        AssertDesc(SActiveFiber==0,"Cannot have two or more master fibers!");
         _creator=this;
        size_t stacksz=sizeof(Fiber_StartupClass)+16;
        void *stack=malloc(stacksz);
        Fiber_StartupClass *nw=new(reinterpret_cast<char *>(stack)+16) Fiber_StartupClass();
        SActiveFiber=this;
        nw->InitContext(stack,stacksz);
        _fiberHandle=nw;
         DebugPrint1("MasterFiber id %08X has been created\n",this);
    }
    
    MasterFiber::~MasterFiber()
    {
        Fiber_StartupClass *cls=reinterpret_cast<Fiber_StartupClass *>(_fiberHandle);
        if (cls!=0) free(cls->_fibStack); 
        cls=0;           
        _fiberHandle=0;
        SActiveFiber=0;
        _creator=0;
        _backPtr=0;
         DebugPrint1("MasterFiber id %08X has been destroyed\n",this);
    }

}

