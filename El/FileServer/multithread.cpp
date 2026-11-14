#include "multithread.hpp"

int WaitForMultiple(const MultiThread::BlockerSimpleBase **bar, int count, bool waitAll, unsigned long timeout)
{
  /// BlockerArray is helper array to create list of Blockers for WaitForMultiple function
  typedef BredyLibs::Array<const MultiThread::BlockerSimpleBase *> BlockerAr;
  BlockerAr blar(bar, count); //or sizeof(bar)/sizeof(*bar) ?
  return MultiThread::BlockerSimpleBase::WaitForMultiple(blar, waitAll, timeout);
}
