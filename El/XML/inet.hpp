#ifdef _MSC_VER
#pragma once
#endif

#ifndef _INET_HPP
#define _INET_HPP


//! Download file
/*!
	\param url URL address of source file
	\param filename name of destination file
*/

bool DownloadFile(const char *url, const char *filename, const char *proxyServer, size_t maxsize);

#if defined(_WIN32)
  struct InternetConnectInfo
  {
    char *serverName;
    char *userName;
    char *password;
    int   port;
    bool  secure;
    InternetConnectInfo() {secure = true;}
  };  
  char *DownloadFile(const char *url, size_t &size, const char *proxyServer, InternetConnectInfo *inetInfo=NULL, size_t maxsize=0);
#else
  char *DownloadFile(const char *url, size_t &size, const char *proxyServer=NULL, size_t maxsize=0);
#endif

//! free Win32 allocated memory when variable is destructed

class AutoFree
{
  //! handle to Win32 memory obtained by GlobalAlloc call
  void *_mem;

  private:
  //! no copy constructor
  AutoFree(const AutoFree &src);

  public:
  //! attach Win32 memory handle
  /*!
  \param mem handle to Win32 memory obtained by GlobalAlloc call
  */
  void operator = (void *mem)
  {
    if (mem==_mem) return;
    Free();
    _mem = mem;
  }
  //! copy - reassign Win32 handle to new object
  /*! source will be NULL after assignment */
  void operator = (AutoFree &src)
  {
    if (src._mem == _mem) return;
    Free();
    _mem = src._mem;
    src._mem = NULL;
  }
  //! empty constructor
  AutoFree(){_mem = NULL;}
  //! construct from Win32 memory handle
  AutoFree(void *mem){_mem = mem;}
  //! convert to pointer
  operator void * () {return _mem;}
  //! destruct - free pointer
  ~AutoFree(){Free();}
#ifdef _WIN32
  //! free pointer explicitely
  void Free(){if (_mem) GlobalFree(_mem),_mem = NULL;}
#else
  //! free pointer explicitely
  void Free(){if (_mem) free(_mem),_mem = NULL;}
#endif
};

#endif
