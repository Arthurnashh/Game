#include <math.h>

// The June 2010 DirectX XAPO library imports floorf from the legacy CRT.
// Bridge that import to the toolset's current CRT without mixing CRT libraries.
static float __cdecl DayZX64Floorf(float value)
{
  return (float)floor((double)value);
}

extern "C" float (__cdecl *__imp_floorf)(float) = &DayZX64Floorf;
