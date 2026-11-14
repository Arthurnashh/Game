// Terrain Synthesizer Helpers
// by Jan Dupej 2009

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TERSYNTH_HELPERS_H
#define _TERSYNTH_HELPERS_H

#ifndef _WIN32
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#define __fastcall

inline void *_aligned_malloc(size_t size, size_t alignment)
{
  // int posix_memalign(void **memptr, size_t alignment, size_t size);
  void *retPtr;
  if ( posix_memalign(&retPtr, alignment, size)==0 ) //success
    return retPtr;
  return NULL; //some error
}
#define _aligned_free(var) free(var)
#endif


// analytic representation of a line in 2d (p: ax+by+c=0)
typedef struct
{
  float a;
  float b;
  float c;
} TS_LINE_ANALYTIC;

// parametric representation of a line in 2d (p: (x,y)=(x0,y0) + (dx,dy)*r )
typedef struct
{
  float x0;
  float y0;
  float dx;
  float dy;
} TS_LINE_PARAMETRIC;

typedef struct
{
  int x, y;
} TS_POINT;

typedef struct
{
  int row0, row1;
  int col0, col1;
} TS_INVALID;

static const float Pi = 3.141592f;//6538979f;
static const float PiHalf = Pi / 2.0f;


__forceinline float lerp(float a, float b, float r)
{
  return a * (1.0f-r) + b * r;
}

__forceinline float cerp(float a, float b, float r);

float SmoothStep(const float x);
float ArcCosine(const float x);

/* This is the noise function used for terrain synthesis.
*
* @param	nIndex specifies which noise function we are using
* @param	nSeed	the seed
* @return      a random value in range of (-1, 1)
*
* Note: must be defined here to be compiled by GCC
*/
inline float Noise(const int nIndex, const int nSeed)
{
  int x = (nSeed<<13) ^ nSeed;

  return ( 1.0f - float((x* (x*x*15731 + 789221 + (74<<nIndex)) + 1376312589) & 0x7fffffff) / 1073741824.0f); 
  //return ( 1.0f - float((x^ (x*x*15731 + 78721 ^ (1471<<nIndex)) + 1376312589) & 0x7fffffff) / 1073741824.0f); 
}

int __fastcall GetPxHash(const int x, const int y);

void ClampToRect(TS_POINT* pPt, int nLeft, int nTop, int nRight, int nBottom);
bool CollideLines(const TS_LINE_PARAMETRIC* pLine1, const TS_LINE_ANALYTIC* pLine2, float* pR);

float BicosInterpolatePoint(const float* pIn, int nWidth, int nHeight, int nX, int nY);
float BilinearInterpolatePoint(const float* pIn, int nWidth, int nHeight, int nX, int nY);

int InterpolateCorner2(float* pBuffer, const float fCorner, const float fDer1, const float fDer2 ,int nX, int nY, int nWidth, int nHeight, int nCorner, int nStride);
int InterpolateBorderHorz(float* pBuffer, const float* pDer, const float* pCol, int nDistance, int nHeight, int nWidth, int nDirection, int nStride);
int InterpolateBorderVert(float* pBuffer, const float* pDer, const float* pRow, int nDistance, int nHeight, int nWidth, int nDirection, int nStride, int nStart=0, int nEnd=128);

int BilinearInterpolateMul_xmm(float* pBuffer, const float* pIn, int nWidth, int nHeight, TS_INVALID* tInvalid);
int BilinearInterpolate_xmm(float* pBuffer, const float* pIn, int nWidth, int nHeight, TS_INVALID* pInvalid);
int AddPerlin_xmm(float* pBuffer, int nX, int nY, int nWidth, int nHeight, int nOctave, float fAmplitude, TS_INVALID* pInvalid);
int AddPerlin(float* pBuffer, int nX, int nY, int nWidth, int nHeight, int nOctave, float fAmplitude, TS_INVALID* pInvalid);
int AddFloatArrs_xmm(float* pDest, float* pSrc, int nFloats);


#endif
