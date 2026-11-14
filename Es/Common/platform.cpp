/**
    @file   platform.cpp
    @brief  Linux-port helpers.

    Copyright &copy; 2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  21.7.2002
    @date   27.9.2002
*/

#include <ctype.h>
void unixPath ( char *path )
{
    if ( !path ) return;
    do {
        if ( *path == '\\' )
        *path = '/';
    else
    if ( isupper(*path) )
        *path = tolower(*path);
    } while ( *(++path) );
}

#ifndef _WIN32
#include "Es/platform.hpp"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <utime.h>

bool isSuffix ( const char *str, const char *suffix )
{
    if ( !suffix || !suffix[0] ) return true;
    if ( !str || !str[0] ) return false;
    int strLen = strlen(str);
    int suffLen = strlen(suffix);
    if ( strLen < suffLen ) return false;
    return( !strcmp(str+strLen-suffLen,suffix) );
}

long long atoi64 ( const char *str )
{
    if ( !str || !str[0] ) return 0;
    long long result = 0;
    while ( isdigit(*str) ) {
        result *= 10;
        result += *str++ - '0';
        }
    return result;
}

char *i64toa ( long long i, char *buf, int radix )
{
    if ( !buf || radix < 2 || radix > 10 ) return NULL;
    int len = 0;
    bool sign = false;
    if ( i < 0 ) {
        sign = true;
    len = 1;
    i = -i;
    }
    long long tmp = i;
    do
    len++;
    while ( (tmp /= radix) );
    char *ptr = buf + len;
    *ptr-- = (char)0;
    do
        *ptr-- = '0' + (i % radix);
    while ( (i /= radix) );
    if ( sign ) *ptr = '-';
    return buf;
}


long long _strtoi64(const char *nptr, char **endptr, int base) 
{ 
  return strtoll(nptr, endptr, base); 
}

char *strDup ( const char *src )
{
    if ( !src ) return NULL;
    char *res = (char*)malloc(strlen(src)+1);
    if ( res ) strcpy(res,src);
    return res;
}

const long long Origin = 0x19db1ded53e8000LL;

bool fileTime ( const char *fileName, long long win32Time )
    // returns 'true' on success
{
    if ( !fileName || !fileName[0] ) return false;
    LocalPath(fn,fileName);
    struct stat st;
    stat(fn,&st);
    struct utimbuf utim;
    if ( win32Time > Origin )
        win32Time = (win32Time - Origin) / 10000000LL;
    else
        win32Time = 1;
    utim.actime  = st.st_atime;
    utim.modtime = (time_t)win32Time;
    return( utime(fn,&utim) == 0 );
}

const size_t copyBuf = 1024;

bool fileCopy ( const char *src, const char *dest )
    // returns 'true' on success
{
    if ( !src || !src[0] || !dest || !dest[0] ) return false;
    LocalPath(sfn,src);
    LocalPath(dfn,dest);
    int f1 = open(sfn,O_RDONLY);
    if ( f1 < 0 ) return false;
    int f2 = open(dfn,O_CREAT|O_WRONLY|O_TRUNC);
    if ( f2 < 0 ) {
        close(f1);
    return false;
        }
    char buf[copyBuf];
    bool result = true;
    do {
        size_t rd = read(f1,buf,copyBuf);
    if ( !rd ) break;
    size_t wr = write(f2,buf,rd);
    if ( wr != rd ) {
        result = false;
        break;
        }
    } while ( true );
    close(f1);
    close(f2);
        // copy file attributes:
    struct stat sstat;
    struct stat dstat;
    stat(sfn,&sstat);
    stat(dfn,&dstat);
    struct utimbuf dtim;
    dtim.actime  = dstat.st_atime;   // not changed!
    dtim.modtime = sstat.st_mtime;
    return( utime(dfn,&dtim) == 0 && result );
}

void createDirectory ( const char *dir )
{
    if ( !dir || !dir[0] ) return;
    LocalPath(udir,dir);
    mkdir(udir,NEW_DIRECTORY_MODE);
}

void deleteFile ( const char *path )
{
    if ( !path || !path[0] ) return;
    LocalPath(upath,path);
    unlink(upath);
}

long filelength(int fd)
{
  struct stat st;
  int result = fstat(fd, &st);
  if (!result) return st.st_size;
  return -1; //error
}

void Sleep( unsigned ms )
{
  sleepMs(ms);
}

DIR *OpenDir(const char *name)
{
  LocalPath(newName, name);
  return opendir(newName);
}

#if 0
// Old Pepca's implementation
#include "Es/Framework/debugLog.hpp"

size_t linuxMemoryUsage ()
{
  char procfn[32];
  sprintf(procfn,"/proc/%d/statm",getpid());
  FILE *f = fopen(procfn,"rt");
  if ( !f ) return 0;
  int size = 0, resident = 0, share = 0, trs = 0, lrs = 0, drs = 0, dt = 0;
  fscanf(f,"%d %d %d %d %d %d %d",&size,&resident,&share,&trs,&lrs,&drs,&dt);
  fclose(f);
  size_t pageSize = getpagesize();
  LogF("Memory total: %u KB, shared: %u KB, data: %u KB",
    (size*pageSize)>>10,(share*pageSize)>>10,(drs*pageSize)>>10);
  return( drs * pageSize );
}
#else

#include <malloc.h>

size_t linuxMemoryUsage ()
{
  struct mallinfo mall;
  mall = mallinfo();
  size_t totalAllocated = mall.uordblks + mall.hblkhd; //mem + mmap (both affects available Address Space limits)
  return totalAllocated;
}

int GetModuleFileName(void *dummy /*HMODULE*/, char *fileName, int bufSize)
{
  int size = readlink("/proc/self/exe", fileName, bufSize);
  if (size<bufSize) fileName[size]='\0'; //zero terminate
  return size;
}

#endif

static inline long XAdd(long volatile *mem, long offset)
{
  asm volatile
    (
    "lock xadd %1, %0"
    : "=m" (*mem), "=q" (offset)
    : "m" (*mem), "1" (offset)
    );
  return offset;    
}

long InterlockedIncrement(long volatile *var)
{
  return XAdd(var,1)+1;   
}
long InterlockedDecrement(long volatile *var)
{
  return XAdd(var,-1)-1;   
}

long InterlockedExchangeAdd(long volatile *val, int x)
{
  return XAdd(val,x)+x;
}

long InterlockedExchange(long volatile *mem, long iParam)
{
  asm volatile
    (
    "lock xchg %1, %0"
    : "=m" (*mem), "=q" (iParam)
    : "m" (*mem), "1" (iParam)
    :"cc"
    );
  return iParam;     
}

int     GetCurrentDirectory(int len, char *buf)
{
  char *cwd = get_current_dir_name();
  if (buf==NULL || !len)
  { //just return actual size including the null terminating char
    char curDirTmp[4096]; //hopefully long enough
    if ( cwd!=NULL ) 
      return strlen(cwd)+1;
  }
  else
  {
    if ( cwd!=NULL )
    {
      strcpy(buf, cwd);
      return strlen(buf);
    }
  }
  return 0;
}

#endif

