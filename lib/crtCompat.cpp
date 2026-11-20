#include <stdio.h>
#include <string.h>

// VC10 static Speex expects the pre-UCRT contiguous stdio FILE array.
// The VC10 static CRT used by this build already defines __iob_func, so the
// compatibility shim is only required for newer toolsets (UCRT based).
#if !defined(_MSC_VER) || _MSC_VER >= 1900
extern "C" FILE *__cdecl __iob_func(void)
{
  static FILE legacyIob[3];
  static bool initialized = false;
  if (!initialized)
  {
    memcpy(&legacyIob[0], stdin, sizeof(FILE));
    memcpy(&legacyIob[1], stdout, sizeof(FILE));
    memcpy(&legacyIob[2], stderr, sizeof(FILE));
    initialized = true;
  }
  return legacyIob;
}
#endif
