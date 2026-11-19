// SDZ: compatibility shim for prebuilt VS2010 static libraries (rsaMT.lib,
// libspeexMt.lib) that still reference the old CRT __iob_func symbol.
// The UCRT exposes the same FILE array through __acrt_iob_func; index 0 is
// stdin and the entries are contiguous, so the legacy code sees the usual
// stdin/stdout/stderr layout. Link-only fix, no game logic involved.
#include <stdio.h>

extern "C" FILE * __cdecl __acrt_iob_func(unsigned index);

extern "C" FILE * __cdecl __iob_func(void)
{
  return __acrt_iob_func(0);
}
