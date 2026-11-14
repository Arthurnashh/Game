// BarrierBlocker.h: interface for the BarrierBlocker class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BARRIERBLOCKER_H__DBF84D17_4149_498B_A541_5CCA973AADD8__INCLUDED_)
#define AFX_BARRIERBLOCKER_H__DBF84D17_4149_498B_A541_5CCA973AADD8__INCLUDED_


#pragma once

#include "BlockerSimpleBase.h"
#include "Synchronized.h"


namespace MultiThread
{


class BarrierBlocker : public IBlocker  
{
  int _initialCount;
  SyncInt _threadCount;
  EventBlocker _bar1;
  EventBlocker _bar2;
public:
  BarrierBlocker(int initialCount):
	  _initialCount(initialCount),
	  _threadCount(initialCount),
	  _bar1(Unblocked_ManualReset),
	  _bar2(Blocked_ManualReset) {}

  virtual bool Acquire(unsigned long timeout=-1)
  {
	int z;
	do
	{
	  if (_bar1.Acquire(timeout)==false) return false;
	  z=--_threadCount;
	  if (z<0) ++_threadCount;
	}
	while (z<0);
	if (z==0) 
	{
	  _bar1.Block();
	  _bar2.Unblock();
	}
	if (_bar2.Acquire(timeout)==false)
	{
	  ++_threadCount;
	  return false;
	}
    if (++_threadCount==_initialCount)
	{
	  _bar2.Block();
	  _bar1.Unblock();
	}
	return true;
  }
};

};
#endif // !defined(AFX_BARRIERBLOCKER_H__DBF84D17_4149_498B_A541_5CCA973AADD8__INCLUDED_)
