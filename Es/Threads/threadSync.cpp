#include <Es/essencepch.hpp>
#include <Es/Common/win.h>
#include <Es/Framework/debugLog.hpp>
#include "multisync.hpp"
#include "threadSync.hpp"

#define MT_LOG 0

#ifdef _WIN32

int SignaledObject::WaitForMultiple( SignaledObject *events[], int n, int timeout )
{
	const int maxHandles = 32;
	HANDLE handles[maxHandles];
	Assert(n < maxHandles);
	for (int i=0; i<n; i++)
  {
    HANDLE handle = events[i]->_handle;
    if (!handle) return -1; // do not wait for uninitialized objects
    handles[i] = events[i]->_handle;
  }

	if (timeout < 0) timeout = INFINITE;
	DWORD ret = ::WaitForMultipleObjects
	(
		n, handles,
		FALSE, // wait for all or wait for one 
		timeout // time-out interval in milliseconds 
	);
	if (ret < WAIT_OBJECT_0 || ret >= WAIT_OBJECT_0 + n)
	{
		//ErrorMessage("MsgWaitForMultipleObjects error.");
		return -1;
	}
	return ret - WAIT_OBJECT_0;
}

#endif
