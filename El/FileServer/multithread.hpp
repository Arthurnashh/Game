/// Interface for MultiThread library
/// There are common includes to use this library and helpers:
///   BlockerArItem to fill in the array for helper function WaitForMultiple

#ifndef MULTI_THREAD_HPP
#define MULTI_THREAD_HPP

// Test of MultiThread library
#ifdef _WIN32
#include <Es/Common/win.h>
#include <El/MultiThread2/multithread/src-windows/common_Win.h>
#else
//#include <El/MultiThread2/multithread/src-linux/common_linux.h>
#endif
#include <El/MultiThread2/common/Array.h>
#include <El/MultiThread2/multithread/Thread.h>
#include <El/MultiThread2/multithread/Synchronized.h>

typedef const MultiThread::BlockerSimpleBase * BlockerArItem;

int WaitForMultiple(const MultiThread::BlockerSimpleBase **bar, int count, bool waitAll, unsigned long timeout);

#endif
