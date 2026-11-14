#include <El/elementpch.hpp>
#include "inet.hpp"

#include <ctype.h>
#include <Es/Common/win.h>
#if defined _WIN32
  #include <winInet.h>
#else
  #include <errno.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <stdio.h>
  #include <fcntl.h>
  #include <signal.h>
#endif

/*!
	\file
	Implementation file inet.cpp
*/


//////////////////////////
//
// Internet file support
//
//////////////////////////

//! defines size of buffer for download
#define BUFFER_SIZE		256
#define AGENT_NAME		"BIGameEngine"

#if defined _WIN32

//! Encapsulates HTTP (or FTP) source into stream
class QIHTTPStream
{
private:
	HMODULE _library;
	HINTERNET _hSession;
	HINTERNET _hFile;
	char _buffer[BUFFER_SIZE];
	DWORD _pos, _len;
	bool _fail, _eof;

public:
	//! constructor
	QIHTTPStream()
	{
		_pos = 0; _len = 0;
		_hSession = NULL; _hFile = NULL;
		_fail = false; _eof = false;
		_library = LoadLibrary("winInet.dll");
	}
	//! destructor
	~QIHTTPStream()
	{
		close();
		FreeLibrary(_library);
	}

	//! open file from internet (HTTP or FTP)
	/*!
		\param url URL address of file
		\param proxyServer address and port of proxy server
	*/
	void open(const char *url, const char *proxyServer, InternetConnectInfo *inetInfo=NULL);
	//! close file
	void close();
	//! read one char from stream
	/*!
		\return read char or -1 when error / eof occurs
	*/
	int get();
	//! undo reading of char
	void unget();

	//! error occurs
	bool fail() const {return _fail;}

	//! end of file occurs
	bool eof() const {return _eof;}
};

//! dynamic linked function InternetOpen from winInet library
typedef HINTERNET (CALLBACK* INTERNET_OPEN)(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);
//! dynamic linked function InternetCanonizeURL from winInet library
typedef BOOL (CALLBACK* INTERNET_CANONICALIZE_URL)(LPCSTR, LPSTR, LPDWORD, DWORD);
//! dynamic linked function InternetCloseHandle from winInet library
typedef BOOL (CALLBACK* INTERNET_CLOSE_HANDLE)(HINTERNET);
//! dynamic linked function InternetReadFile from winInet library
typedef BOOL (CALLBACK* INTERNET_READ_FILE)(HINTERNET, LPVOID, DWORD, LPDWORD);
//! dynamic linked function InternetOpenURL from winInet library
typedef HINTERNET (CALLBACK* INTERNET_OPEN_URL)(HINTERNET, LPCSTR, LPCSTR, DWORD, DWORD, DWORD);
//! dynamic linked function InternetConnect from winInet library
typedef HINTERNET (CALLBACK* INTERNET_CONNECT)(HINTERNET, LPCSTR, INTERNET_PORT, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR);
//! dynamic linked function HttpOpenRequest from winInet library
typedef HINTERNET (CALLBACK* INTERNET_HTTP_OPEN_REQUEST)(HINTERNET, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, DWORD, DWORD_PTR);
//! dynamic linked function HttpSendRequest from winInet library
typedef HINTERNET (CALLBACK* INTERNET_HTTP_SEND_REQUEST)(HINTERNET, LPCSTR, DWORD, LPVOID, DWORD);
//! dynamic linked function HttpQueryInfo from winInet library
typedef BOOL (CALLBACK* INTERNET_QUERY_INFO)(HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD);

/*!
	\todo use async reading instead
*/
void QIHTTPStream::open(const char *url, const char *proxyServer, InternetConnectInfo *inetInfo)
{
	if (!_library)
	{
		_fail = true;
		return;
	}

	char *proxyOverride = "<local>";
	if (!proxyServer)
	{
		// retrieve proxy from registry
		HKEY key;
		BYTE bufferServer[256];
		BYTE bufferOverride[256];
		DWORD sizeServer = sizeof(bufferServer);
		DWORD sizeOverride = sizeof(bufferOverride);
		if
		(
			::RegOpenKeyEx
			(
				HKEY_CURRENT_USER,
				"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
				0, KEY_READ, &key
			) == ERROR_SUCCESS
		)
		{
      int proxyEnable;
      DWORD sizeProxyEnable=sizeof(int);
      if (
        ::RegQueryValueEx(
          key, "ProxyEnable", NULL, NULL, (LPBYTE)(&proxyEnable), &sizeProxyEnable
        ) ==  ERROR_SUCCESS && proxyEnable!=0
      )
      {
        if
          (
          ::RegQueryValueEx
          (
          key, "ProxyServer", NULL, NULL, bufferServer, &sizeServer
          ) ==  ERROR_SUCCESS
          ) proxyServer = (char *)bufferServer;
        if
          (
          ::RegQueryValueEx
          (
          key, "ProxyOverride", NULL, NULL, bufferOverride, &sizeOverride
          ) ==  ERROR_SUCCESS
          ) proxyOverride = (char *)bufferOverride;
        ::RegCloseKey(key);
      }
		}
	}

	INTERNET_OPEN internetOpen =
		(INTERNET_OPEN)GetProcAddress(_library, "InternetOpenA");
	if (!internetOpen)
	{
		_fail = true;
		return;
	}
	if (proxyServer!=NULL)
    _hSession = internetOpen
	  (
		  AGENT_NAME, INTERNET_OPEN_TYPE_PROXY,
		  proxyServer, proxyOverride, 0
	  );
  else
    _hSession = internetOpen
    (
      AGENT_NAME, INTERNET_OPEN_TYPE_PRECONFIG, 
      NULL, NULL, 0
    );
	if (!_hSession)
	{
		_fail = true;
		LogF("InternetOpen: Error %x", GetLastError());
		return;
	}

  if (inetInfo!=NULL) 
  {
    INTERNET_CONNECT internetConnect =
      (INTERNET_CONNECT)GetProcAddress(_library, "InternetConnectA");
    if (!internetConnect)
    {
      _fail = true;
      return;
    }
    _hSession = internetConnect(
      _hSession, inetInfo->serverName, inetInfo->port, 
      inetInfo->userName, inetInfo->password, INTERNET_SERVICE_HTTP, 0, 0
    );
    if (!_hSession)
    {
      _fail = true;
      LogF("InternetConnect: Error %x", GetLastError());
      return;
    }
  }

	INTERNET_CANONICALIZE_URL internetCanonicalizeUrl =
		(INTERNET_CANONICALIZE_URL)GetProcAddress(_library, "InternetCanonicalizeUrlA");
	if (!internetCanonicalizeUrl)
	{
		_fail = true;
		return;
	}
	char name[256];
	DWORD size = 256;
	if (!internetCanonicalizeUrl(url, name, &size, 0))
	{
		_fail = true;
		LogF("InternetCanonicalizeUrl: Error %x", GetLastError());
		return;
	}
	
  if (inetInfo==NULL)
  {
    INTERNET_OPEN_URL internetOpenUrl =
      (INTERNET_OPEN_URL)GetProcAddress(_library, "InternetOpenUrlA");
    if (!internetOpenUrl)
    {
      _fail = true;
      return;
    }
    _hFile = internetOpenUrl
      (
      _hSession, url,
      NULL, -1, INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD | INTERNET_FLAG_PASSIVE, NULL
      );
    if (!_hFile)
    {
      _fail = true;
      LogF("InternetOpenUrl: Error %x", GetLastError());
      return;
    }

    TCHAR infoBuffer[512];
    DWORD dummy = 0;
    DWORD bufLen = 512;
	  INTERNET_QUERY_INFO internetQueryInfo =
		  (INTERNET_QUERY_INFO)GetProcAddress(_library, "HttpQueryInfoA");
	  if (!internetQueryInfo)
	  {
		  _fail = true;
		  return;
  }

    if(internetQueryInfo(_hFile, HTTP_QUERY_STATUS_CODE, infoBuffer, &bufLen, &dummy))
    {
      if( strcmp(infoBuffer, "200")==0 )
      {
        // File exists, all ok.
      }
      else if( strcmp(infoBuffer, "404")==0 )
      {
        //No such file.
        _fail = true;
        return;
      }
      else if( strcmp(infoBuffer, "500")==0 )
      {
        //Internal server error.
        _fail = true;
        return;
      }
    }
  }
  else
  {
    INTERNET_HTTP_OPEN_REQUEST httpOpenRequest =
      (INTERNET_HTTP_OPEN_REQUEST)GetProcAddress(_library, "HttpOpenRequestA");
    if (!httpOpenRequest)
    {
      _fail = true;
      return;
    }
    DWORD flags = INTERNET_FLAG_RELOAD|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_CACHE_WRITE|
        INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
    if (inetInfo->secure) flags |= INTERNET_FLAG_SECURE;
    _hFile = httpOpenRequest(
        _hSession, "GET", url,
        NULL, "", NULL, flags, 
        0 //m_regID
      );
    if (!_hFile)
    {
      _fail = true;
      LogF("HTTPOpenRequest: Error %x", GetLastError());
      return;
    }
    INTERNET_HTTP_SEND_REQUEST httpSendRequest =
      (INTERNET_HTTP_SEND_REQUEST)GetProcAddress(_library, "HttpSendRequestA");
    if (!httpSendRequest)
    {
      _fail = true;
      return;
    }
    HINTERNET result = httpSendRequest(_hFile, NULL, 0, NULL, 0);
    if (!result)
    {
      _fail = true;
      LogF("HTTPSendRequest: Error %x", GetLastError());
      return;
    }
  }
}

void QIHTTPStream::close()
{
	if (!_library) return;

	INTERNET_CLOSE_HANDLE internetCloseHandle =
		(INTERNET_CLOSE_HANDLE)GetProcAddress(_library, "InternetCloseHandle");
	if (!internetCloseHandle) return;

	if (_hFile)
	{
		internetCloseHandle(_hFile);
		_hFile = NULL;
	}
	if (_hSession)
	{
		internetCloseHandle(_hSession);
		_hSession = NULL;
	}
}

int QIHTTPStream::get()
{
	// get single character
	if (_pos >= _len)
	{
		if (_eof || _fail) return EOF;
		if (!_library)
		{
			_fail = true;
			return EOF;
		}
		INTERNET_READ_FILE internetReadFile =
			(INTERNET_READ_FILE)GetProcAddress(_library, "InternetReadFile");
		if (!internetReadFile)
		{
			_fail = true;
			return EOF;
		}
		if (!internetReadFile(_hFile, _buffer, BUFFER_SIZE, &_len))
		{
			_fail = true;
			LogF("InternetReadFile: Error %x", GetLastError());
			return EOF;
		}
		if (_len == 0)
		{
			_eof = true;
			return EOF;
		}
		_pos = 0;
	}
	return (unsigned char)_buffer[_pos++];
}

void QIHTTPStream::unget()
{
	if (_pos == 0)
	{
		Fail("No char to unget");
	}
	else _pos--;
}



#else

// in POSIX we are using libwww library

// Simple implementation: initialize (and de-initialize) libwww for each
//   QIHTTPStream instance

/*!
\patch 1.89 Date 10/28/2002 by Pepca.
- Fixed: XML download for Linux (can use no-protocol URI and proxy-server).
\patch-internal 1.90 Date 11/27/2002 by Pepca.
- Fixed: XML download won't use libwww any more.
*/

#if _SUPER_RELEASE
#undef HTTP_LOG
#undef HTTP_SUPER_LOG
#else
#define HTTP_LOG
#define HTTP_SUPER_LOG
#endif

//! Encapsulates HTTP (or FTP) source into stream
class QIHTTPStream {

protected:

  char *ptr;
  char *beginPtr;
  char *endPtr;

public:

  bool ok;

  QIHTTPStream ();

  ~QIHTTPStream ();

  //! open file from internet (HTTP, FTP via proxy only)
  /*!
  \param url URL address of file
  \param proxyServer address and port of proxy server
  */
  void open ( const char *url, const char *proxyServer );

  //! close file
  void close ();

  //! retrieve const-data-pointer to the downloaded data
  const unsigned char *getData ( unsigned &size );

  //! read one char from stream
  /*!
  \return read char or -1 when error / eof occurs
  */
  int get ();

  //! undo reading of char
  void unget ();

  //! error occurs
  bool fail () const
  { return !ok; }

  //! end of file occurs
  bool eof () const
  { return (ptr >= endPtr); }

};

QIHTTPStream::QIHTTPStream ()
{
  ok = false;
  ptr = beginPtr = endPtr = NULL;
}

QIHTTPStream::~QIHTTPStream ()
{
  close();
}

static bool httpInt = false;

void handlePipe ( int sig )
// handles SIGPIPE
{
  httpInt = true;
}

void QIHTTPStream::open ( const char *url, const char *proxyServer )
{
  close();
  if ( !url || !url[0] ) return;
  LogF("Fetching URL: '%s' via proxy: %s",url,proxyServer?proxyServer:"<direct>");
#ifdef NET_LOG_HTTP
#  ifdef NET_LOG_BRIEF
  NetLog("HTTP('%s','%s')",url,proxyServer?proxyServer:"<direct>");
#  else
  NetLog("Fetching URL: '%s' via proxy: %s",url,proxyServer?proxyServer:"<direct>");
#  endif
#endif
  struct sockaddr_in server;      // [host:port] a connection will be established to
  memset(&server,0,sizeof(server));
  server.sin_port = htons(80);
  char localHost[512];            // host to be asked
  char localUrl[512];             // requested URL
  char *port;
  struct hostent *hos;            // resolved host-name
  if ( proxyServer && proxyServer[0] ) {
    // I'll determine connecting host first:
    port = const_cast<char *>(strchr(proxyServer,':'));
    if ( port ) {               // colon found!
      if ( isdigit(port[1]) )
        server.sin_port = htons(atoi(port+1));
      port[0] = (char)0;
      hos = gethostbyname(proxyServer);
      port[0] = ':';
    }
    else {                      // proxy-port should be 80
      hos = gethostbyname(proxyServer);
    }
    if ( !hos ) return;
    memcpy((char*)&server.sin_addr,(char*)hos->h_addr,hos->h_length);
    // and then check the URL-format:
    if ( memcmp(url,"http://",7) &&
      memcmp(url,"ftp://",6) )
      snprintf(localUrl,sizeof(localUrl),"http://%s",url);
    else
      strncpy(localUrl,url,sizeof(localUrl));
    localUrl[sizeof(localUrl)-1] = (char)0;
  }
  else {                          // direct connection to a HTTP server
    const char *host = strchr(url,':');
    if ( !host )
      host = url;
    else {
      if ( host[1] == '/' && host[2] == '/' )
        host += 3;
      else
        host = url;
    }
    port = const_cast<char *>(strchr(host,':'));
    char *absolute = const_cast<char *>(strchr(host,'/'));
    if ( port && (!absolute || port < absolute) && isdigit(port[1]) ) {
      server.sin_port = htons(atoi(port+1));
      memcpy(localHost,host,port-host);
      localHost[port-host] = (char)0;
    }
    else {                      // no colon!
      if ( absolute ) {
        memcpy(localHost,host,absolute-host);
        localHost[absolute-host] = (char)0;
      }
      else
        strcpy(localHost,host);
    }
    hos = gethostbyname(localHost);
    if ( !hos ) return;
    memcpy((char*)&server.sin_addr,(char*)hos->h_addr,hos->h_length);
    if ( absolute ) {
      strncpy(localUrl,absolute,sizeof(localUrl));
      localUrl[sizeof(localUrl)-1] = (char)0;
    }
    else
      strcpy(localUrl,"/");
  }

#ifdef HTTP_LOG
  LogF("HTTP server: %08x:%u (%u), url: '%s'",ntohl(*((unsigned*)&server.sin_addr)),ntohs(server.sin_port),(unsigned)server.sin_family,localUrl);
#endif
#ifdef NET_LOG_HTTP
#  ifdef NET_LOG_BRIEF
  NetLog("HTTP(%08x:%u,%u,'%s')",ntohl(*((unsigned*)&server.sin_addr)),ntohs(server.sin_port),(unsigned)server.sin_family,localUrl);
#  else
  NetLog("HTTP server: %08x:%u (%u), url: '%s'",ntohl(*((unsigned*)&server.sin_addr)),ntohs(server.sin_port),(unsigned)server.sin_family,localUrl);
#  endif
#endif

  // handle the SIGPIPE (for connection-refuse):
  signal(SIGPIPE,handlePipe);
  httpInt = false;
  // make the HTTP request:
  char req[4096];
  int sock, i;
  if ( (sock = socket(AF_INET,SOCK_STREAM,0)) < 0 ) {
    LogF("HTTP: Cannot get a socket!");
    return;
  }
  server.sin_family = AF_INET;
  if ( fcntl(sock,F_SETFL,FNDELAY) < 0 ) {
    LogF("HTTP: Cannot set FNDELAY mode!");
    ::close(sock);
    return;
  }
  if ( connect(sock,(struct sockaddr*)&server,sizeof(server)) < 0 &&     // initiate the connection
    errno != EINPROGRESS ) {
      LogF("HTTP: Connection refused (errno=%d)!",errno);
      ::close(sock);
      return;
  }

  // check the timeout:
  struct timeval ti;
  ti.tv_sec  = 5;
  ti.tv_usec = 0;     // are 5 seconds enough?
  fd_set mask;
  FD_ZERO(&mask);
  FD_SET(sock,&mask);
  if ( !select(sock+1,NULL,&mask,NULL,&ti) ) {
    LogF("HTTP: Connection timeout!");
    ::close(sock);
    return;
  }
  if ( fcntl(sock,F_SETFL,0) < 0 ) {
    LogF("HTTP: Cannot set FDELAY mode!");
    ::close(sock);
    return;
  }

  // assembly the request string:
  strcpy(req,"GET ");
  ptr = req + 4;
  port = localUrl;
  while ( port[0] && ptr < req + sizeof(req) )
    if ( port[0] == ' ' ||
      port[0] == '<' ||
      port[0] == '>' ||
      port[0] == '"' ||
      port[0] == '#' ||
      port[0] == '%' ||
      port[0] == '{' ||
      port[0] == '}' ||
      port[0] == '|' ||
      port[0] == '\\' ||
      port[0] == '^' ||
      port[0] == '~' ||
      port[0] == '[' ||
      port[0] == ']' ||
      port[0] == '`' ) {     // percent-escape
        ptr += sprintf(ptr,"%%%02X",(unsigned)port[0]);
        port++;
    }
    else
      *ptr++ = *port++;
  if ( proxyServer && proxyServer[0] )
    snprintf(ptr,req+sizeof(req)-ptr,
    " HTTP/1.0\r\n"
    "User-Agent: Flashpoint/1.91\r\n"
    "\r\n");
  else
    snprintf(ptr,req+sizeof(req)-ptr,
    " HTTP/1.0\r\n"
    "Host: %s\r\n"
    "User-Agent: Flashpoint/1.91\r\n"
    "\r\n",localHost);
  req[sizeof(req)-1] = (char)0;
  int msgLen = strlen(req);
#ifdef HTTP_SUPER_LOG
  LogF("HTTP request: %s",req);
#endif

  // send the request string:
  int sent;
  ptr = req;
  while ( msgLen ) {
    ti.tv_sec  = 5;
    ti.tv_usec = 0;     // are 5 seconds enough?
    FD_ZERO(&mask);
    FD_SET(sock,&mask);
    if ( !select(sock+1,NULL,&mask,NULL,&ti) ) {
      LogF("HTTP: Error sending data (timeout)!");
      ::close(sock);
      return;
    }
    sent = send(sock,ptr,msgLen,0);
    if ( sent < 0 || httpInt ) {
      LogF("HTTP: Error sending data (errno=%d)!",errno);
      ::close(sock);
      return;
    }
    ptr    += sent;
    msgLen -= sent;
  }

  // ... and receive the response:
  ptr = req;
  int left = sizeof(req);
  endPtr = NULL;
  int errCounter = 12;
  do {
    ti.tv_sec  = 8;
    ti.tv_usec = 0;     // 8 seconds ... to be sure (for proxy-caches)
    FD_ZERO(&mask);
    FD_SET(sock,&mask);
    if ( !select(sock+1,&mask,NULL,NULL,&ti) || httpInt ) {
      LogF("HTTP: Error receiving data (timeout)!");
      ::close(sock);
      ptr = endPtr = NULL;
      return;
    }
    msgLen = recv(sock,ptr,left,0);     // receive at least the response header
    if ( msgLen < 0 ) {
      LogF("HTTP: Error receiving response header (%d)!",errno);
      if ( --errCounter < 0 ) break;
    }
    else {
      ptr  += msgLen;
      left -= msgLen;
    }
  } while ( !(endPtr = strstr(req,"\r\n\r\n")) && left > 0 );

  // look for "Content-Length:" header:
  if ( endPtr )
    endPtr[0] = (char)0;
  else {
    LogF("HTTP: Response header is too long!");
    ::close(sock);
    ptr = endPtr = NULL;
    return;
  }
  port = strcasestr(req,"\r\nContent-length:");
  if ( !port ) {
    LogF("HTTP: Missing Content-length field (header len=%d)!",endPtr-req);
#ifdef HTTP_SUPER_LOG
    LogF(req);
#endif
    ::close(sock);
    ptr = endPtr = NULL;
    return;
  }

  // prepare the data-array:
  int totalLen = atoi(port+17);
  endPtr += 4;
  beginPtr = (char*)malloc(totalLen);
  memcpy(beginPtr,endPtr,ptr-endPtr);
  ptr = beginPtr + (ptr - endPtr);
  endPtr = beginPtr + totalLen;
  left = endPtr - ptr;

  // read the rest of data:
  errCounter = 12;
  do {
    ti.tv_sec  = 5;
    ti.tv_usec = 0;     // are 5 seconds enough?
    FD_ZERO(&mask);
    FD_SET(sock,&mask);
    if ( !select(sock+1,&mask,NULL,NULL,&ti) ) {
      LogF("HTTP: Truncating data to %u bytes",ptr-beginPtr);
      endPtr = ptr;
      break;
    }
    msgLen = recv(sock,ptr,left,0);     // receive rest of the data
    if ( msgLen < 0 ) {
      LogF("HTTP: Error receiving data (%d)!",errno);
      if ( --errCounter < 0 ) {
        endPtr = ptr;
        break;
      }
    }
    else {
      ptr  += msgLen;
      left -= msgLen;
    }
  } while ( left );

  ::close(sock);
  LogF("HTTP: OK loading URL '%s' (%u bytes)",url,endPtr-beginPtr);
#ifdef NET_LOG_HTTP
#  ifdef NET_LOG_BRIEF
  NetLog("HTTP('%s',%u)",url,endPtr-beginPtr);
#  else
  NetLog("HTTP: OK loading URL '%s' (%u bytes)",url,endPtr-beginPtr);
#  endif
#endif

  ok = true;
  ptr = beginPtr;
}

void QIHTTPStream::close ()
{
  if ( beginPtr ) free(beginPtr);
  ptr = beginPtr = endPtr = NULL;
  ok = false;
}

const unsigned char *QIHTTPStream::getData ( unsigned &size )
{
  if ( !ok || !beginPtr ) return NULL;
  size = endPtr - beginPtr;
#ifdef HTTP_LOG
  LogF("HTTP-getData: %u bytes",size);
#endif
  return (const unsigned char*)beginPtr;
}

int QIHTTPStream::get ()
{
  if ( !ok || !ptr || ptr >= endPtr ) return -1;
  return *ptr++;
}

void QIHTTPStream::unget ()
{
  if ( beginPtr && ptr > beginPtr ) ptr--;
}

#endif

// Download file

/*!
\patch 1.01 Date 6/22/2001 by Ondra.
- Fixed: Better handling of http download errors.
*/

#ifdef _WIN32

class BufferedWrite
{
	enum {BufferSize=1024};
	char _buffer[BufferSize];
	int _used;
	HANDLE _handle;
	public:
	explicit BufferedWrite(HANDLE handle)
	{
		_handle = handle;
		_used = 0;
	}
	~BufferedWrite(){Flush();}
	int Put(char c)
	{
		int err = 0;
		if (_used>=BufferSize) err = Flush();
		_buffer[_used++]=c;
		return err;
	}
	int Flush()
	{
		if (_used<=0) return 0;
		DWORD w;
		WriteFile(_handle,_buffer,_used,&w,NULL);
		int ret = (_used==w ? 0 : -1 );
		_used = 0;
		return ret;
	}

};

bool DownloadFile(const char *url, const char *filename, const char *proxyServer, size_t maxsize)
{
	QIHTTPStream in;
	in.open(url, proxyServer);
	// FIX: when failed, do not create file
	if (in.fail()) return false;
	HANDLE out = CreateFile
	(
		filename,
		GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL
	);
	if (out==INVALID_HANDLE_VALUE) return false;
	BufferedWrite buf(out);
	bool ok = true;
	int c = in.get();
	//QOFStream out;
	//out.open(filename);
	size_t usedSize=0;
  while (!in.eof() && !in.fail() && (!maxsize || (usedSize<=maxsize)))
	{
		if (buf.Put(c)<0)
		{
			ok = false;
			break;
		}
		c = in.get();
    usedSize++;
	}
	buf.Flush();
	CloseHandle(out);
	if (in.fail() || (maxsize && (usedSize>maxsize))) ok = false;
	if (!ok)
	{
		DeleteFile(filename);
	}
	
	return ok;
}

#else

bool DownloadFile ( const char *url, const char *filename, const char *proxyServer, void *inetInfo, size_t maxsize )
{
  QIHTTPStream in;
  in.open(url, proxyServer);
  if ( in.fail() ) return false;
  LocalPath(fn,filename);
  int file = ::open(fn,O_CREAT|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
  if ( !file ) return false;
  unsigned size;
  const unsigned char *data = in.getData(size);
  if ( !data ) {
    ::close(file);
    return false;
  }
  int sizeWritten = ::write(file,data,size);
  ::close(file);
  return ( sizeWritten == size );
}

#endif

/*!
\return Windows handle of memory (acquired by GlobalAlloc).
Caller should use GlobalFree when memory is no longer required.
\patch 1.47 Date 3/9/2002 by Ondra
- Fixed: Server crashed when very large squad.xml was loaded.
*/

#if defined(_WIN32)

char *DownloadFile(const char *url, size_t &size, const char *proxyServer, InternetConnectInfo *inetInfo, size_t maxsize)
{
	size = 0;
	QIHTTPStream in;
	in.open(url, proxyServer,inetInfo);
	// FIX: when failed, do not create file
	if (in.fail()) return false;
	int allocated = 64*1024;
	int usedSize = 0;
	char *mem = (char *)GlobalAlloc(GMEM_FIXED,allocated);
	if (mem==NULL) return false;
	int c = in.get();
	while (!in.eof() && !in.fail() && (!maxsize || (usedSize<=maxsize)) )
	{
		if (usedSize>=allocated)
		{
			char *newMem = (char *)GlobalAlloc(GMEM_FIXED,allocated*2);
			if (!newMem)
			{
				GlobalFree(mem);
				return false;
			}
			memcpy(newMem,mem,usedSize);
			GlobalFree(mem);
			mem = newMem;
			allocated *= 2;
		}
		mem[usedSize++] = c;
		c = in.get();
	}
	if (in.fail() || (maxsize && usedSize>maxsize))
	{
		GlobalFree(mem);
		return NULL;
	}
	size = usedSize;
	return mem;
}

#else

char *DownloadFile ( const char *url, size_t &size, const char *proxyServer, void *inetInfo, size_t maxsize )
{
  QIHTTPStream in;
  in.open(url, proxyServer);
  size = 0;
  if ( in.fail() ) return NULL;
  unsigned s;
  const unsigned char *data = in.getData(s);
  if ( !data ) return NULL;
  size = s;
  char *copy = (char*)malloc(size);
  if ( copy ) memcpy(copy,data,size);
  return copy;
}

#endif

