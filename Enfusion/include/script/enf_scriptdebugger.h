//-----------------------------------------------------------------------------
// File: enf_scriptdebugger.h
//
// Desc: Interface to remote script debugger
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SCRIPTDEBUGGER_H
#define ENF_SCRIPTDEBUGGER_H

#include "enf_config.h"
#include "system/enf_mutex.h"

#ifdef __APPLE__
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	//#include < netdb.h >
#endif

namespace enf
{
/** @addtogroup EnforceScript
@{*/

enum DebugCommand
{
	DBGCMD_ERROR,

	//VM to debugger
	DBGCMD_CONNECT,			//just connect
	DBGCMD_DISCONNECT,		//just disconnect
	DBGCMD_INIT,				//dbg info follows
	DBGCMD_ADDMODULE,
	DBGCMD_REMOVEMODULE,

	DBGCMD_BREAK,				//waits for STEP/RUN cmd
	DBGCMD_VARIABLES,			//strictly follows

	//debugger to VM
	DBGCMD_REPLY_DONE,		//when we have everything dbg->VM sent after DBGCMD_INIT
	DBGCMD_BREAKPOINTS,		//int[] follows
	
	DBGCMD_STEPINTO,
	DBGCMD_STEPOVER,
	DBGCMD_STEPOUT,
	DBGCMD_STEPCONTINUE,
	DBGCMD_STEPTO,
	DBGCMD_RUN,
	DBGCMD_DEBUG
};

class DbgConnection
{
private:
	CMutex	m_Mutex;

protected:
	int newsockfd;
	int portno;

	~DbgConnection()
	{
	}

	DbgConnection()
	{
	}

	struct CallStack
	{
		ScriptModule*	Module;
		int				Line;
		char				ClassName[24];
		char				FunctionName[32];
	};

	struct DbgBreak
	{
		ScriptModule*	Module;
		int				Line;
		int				StackSize;
		bool				ResetVars;
		CallStack		Stack[64];
	};

	void HandleError()
	{
#if defined(ENF_XENON) || defined(WIN32)
		int err = WSAGetLastError();

		enf_assert(err != WSAEMSGSIZE);
		enf_assert(err != WSAENETDOWN);
		enf_assert(err != WSAENETUNREACH);
		enf_assert(err != WSAENETRESET);
		enf_assert(err != WSAECONNABORTED);
		enf_assert(err != WSAECONNRESET);
		enf_assert(err != WSAENOBUFS);
		enf_assert(err != WSAENOTCONN);
		enf_assert(err != WSAESHUTDOWN);
		enf_assert(err != WSAETIMEDOUT);
		enf_assert(err != WSAEHOSTDOWN);
		enf_assert(err != WSAEDISCON);
#endif
	}

public:
	struct DbgStepTo
	{
		const ScriptModule*	Module;
		int						Line;
	};

	//sent over inet
	struct SDebugVariable
	{
		char			m_strValue[128];
		VType			m_eType;
		int			m_iItem;
		int			m_iParent;
	};


	void Lock()
	{
		m_Mutex.Lock();
	}

	void Unlock()
	{
		m_Mutex.Unlock();
	}

	int ReceiveData(void *buff, int size)
	{
		int off = 0;
		while(size > 0)
		{
			int rsize = recv(newsockfd, (char*)buff + off, enf_min(size, 16384), 0);

			if(rsize <= 0)
				return 0;

			size -= rsize;
			off += rsize;
		}

		return off;
	}

	int ReceiveString(char *buff, int size)
	{
		int len;

		if(!ReceiveInt(len))
			return 0;

		len = enf_min(len, size);
		if(!ReceiveData(buff, len))
			return 0;
		
		buff[len] = 0;

		return len;
	}

	bool ReceiveInt(int& value)
	{
		int n;

		if(ReceiveData(&n, sizeof(n)) != sizeof(n))
			return false;

		value = ntohl((unsigned int)n);
		return true;
	}

	DebugCommand ReceiveCommand(int* psize = NULL)
	{
		int cmd, size;
		
		if(!ReceiveInt(cmd))
			return DBGCMD_ERROR;
		if(!ReceiveInt(size))
			return DBGCMD_ERROR;

		if(psize)
			*psize = size;

		return (DebugCommand)cmd;
	}

	bool SendData(const void* data, int size)
	{
		int off = 0;
		
		while(size)
		{
			int sndsize = send(newsockfd, ((const char*)data) + off, enf_min(size, 16384), 0);
			if(sndsize <= 0)
				return false;

			size -= sndsize;
			off += sndsize;
		}

		return true;
	}

	bool SendString(const char* str, int len = 0)
	{
		if(len == 0)
			len = strlen(str);

		if(!SendInt(len))
			return false;
		return SendData(str, len);
	}

	bool SendInt(int n)
	{
		n = htonl((unsigned int)n);
		return SendData(&n, sizeof(n));
	}

	bool SendCommand(DebugCommand cmd, const void *parms = NULL, int size = 0)
	{
		if(!SendInt((int)cmd))
			return false;
		if(!SendInt((int)size))
			return false;
		if(size == 0)
			return true;

		return SendData(parms, size);
	}
};
//@}
}

#endif //ENF_SCRIPTDEBUGGER_H