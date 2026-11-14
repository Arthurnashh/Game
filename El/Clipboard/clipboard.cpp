#include <El/elementpch.hpp>

#include "clipboard.hpp"
#include <Es/Common/win.h>

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#if defined _WIN32

void ExportToClipboard(const char *data, int size)
{
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		HANDLE handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size + 1);
		if (handle)
		{
			char *mem = (char *)GlobalLock(handle);
			if (mem)
			{
				memcpy(mem, data, size);
				mem[size] = 0;
			}
			GlobalUnlock(handle);
			SetClipboardData(CF_TEXT, handle);
		}
		CloseClipboard();
	}
}

RString ImportFromClipboard()
{
	RString result;

	if (OpenClipboard(NULL))
	{
		HANDLE handle = GetClipboardData(CF_TEXT);
		if (handle)
		{
			char *mem = (char *)GlobalLock(handle);
			if (mem) result = mem;
			GlobalUnlock(handle);
		}
		CloseClipboard();
	}
	return result;
}

void ExportUnicodeToClipboard(const wchar_t *data, int size)
{
  if (OpenClipboard(NULL))
  {
    EmptyClipboard();
    HANDLE handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(wchar_t) * (size + 1));
    if (handle)
    {
      wchar_t *mem = (wchar_t *)GlobalLock(handle);
      if (mem)
      {
        memcpy(mem, data, sizeof(wchar_t) * size);
        mem[size] = 0;
      }
      GlobalUnlock(handle);
      SetClipboardData(CF_UNICODETEXT, handle);
    }
    CloseClipboard();
  }
}

RStringCT<wchar_t> ImportUnicodeFromClipboard()
{
  RStringCT<wchar_t> result;

  if (OpenClipboard(NULL))
  {
    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if (handle)
    {
      wchar_t *mem = (wchar_t *)GlobalLock(handle);
      if (mem) result = mem;
      GlobalUnlock(handle);
    }
    CloseClipboard();
  }
  return result;
}

#else

void ExportToClipboard(const char *data, int size)
{
}

RString ImportFromClipboard()
{
	return RString();
}

void ExportUnicodeToClipboard(const wchar_t *data, int size)
{
}

RStringCT<wchar_t> ImportUnicodeFromClipboard()
{
  return RStringCT<wchar_t>();
}

#endif

void ExportToClipboardAndFile(const char *data, int size, const char *filename)
{
#ifndef _WIN32
	LocalPath(fn, filename);
	int handle = ::open(fn, O_CREAT|O_APPEND|O_WRONLY, S_IREAD|S_IWRITE);
#else
	int handle = ::open(filename, O_CREAT|O_APPEND|O_BINARY|O_WRONLY, S_IREAD|S_IWRITE);
#endif

	if (handle >= 0)
	{
		::write(handle, data, size);
		::close(handle);
	}

	ExportToClipboard(data, size);
}
