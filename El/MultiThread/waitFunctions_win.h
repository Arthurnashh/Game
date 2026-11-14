#pragma once
//NOTE: Internal header - don't use in your project.

/* Types defined in this header is platform depend */

#ifdef _WIN32

namespace MultiThread
{


  struct ThreadWaitInfo
  {
    HANDLE *hList;
    int count;
    bool waitAll;
    int index;
    ThreadWaitInfo(HANDLE *h):hList(h),count(1),waitAll(false) {}
    ThreadWaitInfo(HANDLE *h,int count,bool waitAll):hList(h),
      count(count),waitAll(waitAll) {}
  };

};

#endif