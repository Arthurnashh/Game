#include "common_Win.h"


namespace MultiThread
{

bool ThreadInternal::WaitForSingleObject(HANDLE object, DWORD timeout)
{
  return ::WaitForSingleObject(object,timeout)!=WAIT_OBJECT_0;
}

int ThreadInternal::WaitForMultipleObjects(HANDLE *objects, int count, bool waitAll, unsigned long timeout)
{
  DWORD res=::WaitForMultipleObjects(count,objects,waitAll,timeout);
  if (res==WAIT_TIMEOUT) return -1;
  return res-WAIT_OBJECT_0;
}

};