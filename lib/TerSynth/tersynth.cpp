// Terrain Synthesizer Class
// by Jan Dupej 2009

// #ifdef _PROFILE
// #pragma optimize("", off)
// #endif


#include <El/elementpch.hpp>
#include "tersynth.h"
#include "tersynth_helpers.h"

#include <math.h>
#include <malloc.h>
#include <Es/Algorithms/qsort.hpp>

  #include <xmmintrin.h>

//static const float Pi = 3.141592f;//65358979f;
static const float Infinity = 1e+10;

// the power distribution of the Perlin noise (first octave - first value)
//static const float NoiseCoeffs[5] = {0.68f, 0.16f, 0.081f, 0.02f, 0.011f};
static const float NoiseCoeffs[5] = {0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f};

TerrainSynth::TerrainSynth(void) : 
  m_Initialized(false)
{
  m_PerlinBuff = (float*) _aligned_malloc( 128*128*sizeof(float), 16 );  
  DoAssert(intptr_t(m_PerlinBuff)%sizeof(__m128)==0);
}

TerrainSynth::~TerrainSynth(void)
{
  _aligned_free( m_PerlinBuff );    
}


/* Synthesizes a patch of terrain - not thread-safe
 *
 * @param	pDest	output 2d buffer
 * @param	pRect	patch dimensions
 * @param nLOD  level of detail within <1,16> and a power of 2
 * @return  1 if ok, 0 otherwise
 */
int TerrainSynth::GetPatch(float* pDest, TSRECT* pRect, int nLOD, TS_INVALID* pInvalid) const
{
  if( !pDest || !pRect || !m_Initialized || nLOD < 1 || nLOD > 16)
    return 0;
  
  // calculate lowland/seabed altitude in all corners of the patch
  float pCorners[4] = { m_Altitude[ GetAngleIndex(pRect->left, pRect->top) ],
      m_Altitude[ GetAngleIndex(pRect->left + pRect->width , pRect->top) ],
      m_Altitude[ GetAngleIndex(pRect->left, pRect->top + pRect->height) ],
      m_Altitude[ GetAngleIndex(pRect->left + pRect->width, pRect->top + pRect->height) ] };

  float pMaxCorners[4] = { m_MaxAltitude[ GetAngleIndex(pRect->left, pRect->top) ],
      m_MaxAltitude[ GetAngleIndex(pRect->left + pRect->width , pRect->top) ],
      m_MaxAltitude[ GetAngleIndex(pRect->left, pRect->top + pRect->height) ],
      m_MaxAltitude[ GetAngleIndex(pRect->left + pRect->width, pRect->top + pRect->height) ] };

  // interpolate all values in the patch
  
  int nSize = 128 / nLOD;

  // calculate 5 octaves of Perlin noise and add them together in a buffer
  memset( m_PerlinBuff, 0x0, sizeof(float)*nSize*nSize );
  int i;

  TS_INVALID tInvalid;
    tInvalid.row0 = pInvalid ? pInvalid->row0 : 0;
    tInvalid.row1 = pInvalid ? pInvalid->row1 : nSize;
    tInvalid.col0 = pInvalid ? pInvalid->col0 : 0;
    tInvalid.col1 = pInvalid ? pInvalid->col1 : nSize;

  int nMaxOctaveXMM = 5;    // how many octaves we can generate with SSE
  int nMaxOctave = 5;       // how many we need altogether

  // if LOD too low, reduce frequency range of generated noise
  if(nLOD == 16)
  {
    nMaxOctaveXMM = 0;    
    nMaxOctave = 2;
  }
  else if(nLOD ==8)
  {
    nMaxOctaveXMM = 0;
    nMaxOctave = 4;
  }
  else if(nLOD == 4)  
    nMaxOctaveXMM = 3;  
  else if(nLOD == 2)  
    nMaxOctaveXMM = 4;  
  else if(nLOD == 1)  
    nMaxOctaveXMM = 5;  
  else
    return 0;   // we need LOD to be power of 2

  // add perlin noise of i-th octave with SSE if possible
  for(i=0; i<nMaxOctaveXMM; i++)
    AddPerlin_xmm( m_PerlinBuff, pRect->left, pRect->top, nSize, nSize, i, NoiseCoeffs[i], &tInvalid );  

  // add perlin noise of i-th octave with FPU if neccessary
  for(; i<nMaxOctave; i++)
    AddPerlin( m_PerlinBuff, pRect->left, pRect->top, nSize, nSize, i, NoiseCoeffs[i], &tInvalid );  


  float fTemp;

  for(i=0; i<4; i++)
  {
    fTemp = pMaxCorners[i]+pCorners[i];
    pMaxCorners[i] -= pCorners[i];    
    pCorners[i] = fTemp * 0.5f;
  }
    
  // calculate lowland
  BilinearInterpolate_xmm( pDest, pCorners, nSize, nSize, &tInvalid );
  
  // scale Perlin noise to match analyzed terrain
  BilinearInterpolateMul_xmm( m_PerlinBuff, pMaxCorners, nSize, nSize, &tInvalid);

  // put it all together
  AddFloatArrs_xmm( pDest, m_PerlinBuff, (tInvalid.col1 - tInvalid.col0) * (tInvalid.row1 - tInvalid.row0)  );

  
  return 1;
}


/* Samples the terrain in one point.
 *
 * @param	nX x-coordinate of the sample
 * @param	nY y-coordinate of the sample
 * @return  altitude (0.0f if error - do not use this to determine error)
 */
float TerrainSynth::GetPoint(int nX, int nY) const
{
  if( !m_Initialized )
    return 0.0f;

  int nLeft = nX & 0xffffff80;//floor(nX / 128.0) * 128;
  int nTop = nY & 0xffffff80; //floor(nY / 128.0) * 128;
  int nSize;
  
  int pPoints[4] = {GetAngleIndex(nLeft, nTop) ,
      GetAngleIndex(nLeft + 128, nTop) ,
      GetAngleIndex(nLeft, nTop + 128) ,
      GetAngleIndex(nLeft + 128, nTop + 128) };
  
  float pCorners[4];
  float pMaxCorners[4];

  int i;

  for(i=0; i<4; i++)
  {
    pCorners[i] = 0.5f * (m_MaxAltitude[ pPoints[i] ] + m_Altitude[ pPoints[i] ]);
    pMaxCorners[i] = m_MaxAltitude[ pPoints[i] ] - m_Altitude[ pPoints[i] ];    
  }
  
  // calculate lowland altitude
  float fLowland = BicosInterpolatePoint( pCorners, 128, 128, nX - nLeft, nY - nTop );

  // hill accumulator
  float fHills = 0.0f;

  // noise function values
  float pNoise[4];

  for(i=0; i<5; i++)  // add all octaves of the Perlin noise to the hill accumulator
  {
    // calculate closest aligned square coords
    nSize = 1 << (7-i);
    nLeft = nX & (0xffffffff << (7-i));
    nTop = nY & (0xffffffff << (7-i));
    
    // calculate Perlin noise in every corner
    pNoise[0] = Noise(i, GetPxHash(nLeft, nTop));
    pNoise[1] = Noise(i, GetPxHash(nLeft+nSize, nTop));
    pNoise[2] = Noise(i, GetPxHash(nLeft, nTop+nSize));
    pNoise[3] = Noise(i, GetPxHash(nLeft+nSize, nTop+nSize));

    fHills += BicosInterpolatePoint( pNoise, nSize, nSize, nX - nLeft, nY - nTop ) * NoiseCoeffs[i];
  }

  nLeft = nX & 0xffffff80;//floor(nX / 128.0) * 128;
  nTop = nY & 0xffffff80; //floor(nY / 128.0) * 128;

  fHills *= BilinearInterpolatePoint( pMaxCorners, 128, 128, nX - nLeft, nY - nTop ); 

  return fLowland + fHills;
}

struct CompareIntTS
{
    int operator () (const int *a, const int *b) const
    { // reversed order, bigger first
      return *a-*b;
    }
  };

void TerrainSynth::GetMinMaxAltitudes(float &minH, float &maxH, int nLeft, int nTop, int nRight, int nBottom) const
{
  int angle[4] = {GetAngleIndex(nLeft,nTop),GetAngleIndex(nRight,nTop),GetAngleIndex(nLeft,nBottom),GetAngleIndex(nRight,nBottom)};

  // sort all angles 
  QSort(angle,4,CompareIntTS());

  // check if we should wrap around
  if (angle[0]-angle[3]>AnalysisDensity/2)
  {
    for (int i=0; i<3; i++)
    {
      if (angle[i]-angle[3]>AnalysisDensity/2) angle[i] += AnalysisDensity;
    }
    QSort(angle,4,CompareIntTS());
  }

  // now we have min..max angle
  float minY = FLT_MAX;
  float maxY = FLT_MIN;
  for (int a=angle[0]; a<=angle[3]; a++)
  {
    int angleIndex = a>=AnalysisDensity ? a-AnalysisDensity : a;
    if (minY>m_Altitude[angleIndex]) minY = m_Altitude[angleIndex];
    if (maxY<m_MaxAltitude[angleIndex]) maxY = m_MaxAltitude[angleIndex];
  }
  
  // returned value may be out of min..max range when sum of NoiseCoeffs is larger than 0.5
  // see TerrainSynth::GetPoint
  // returned value may be out of min..max range when sum of NoiseCoeffs is larger than 0.5
  // see TerrainSynth::GetPoint
  float noiseSum = 0;
  for (int i=0; i<5; i++) noiseSum += NoiseCoeffs[i];
  
  minH = (maxY+minY)*0.5f-(maxY-minY)*noiseSum;
  maxH = (maxY+minY)*0.5f+(maxY-minY)*noiseSum;
}
float TerrainSynth::GetMinHeight(int nLeft, int nTop, int nRight, int nBottom) const
{
  float minY,maxY;
  GetMinMaxAltitudes(minY,maxY,nLeft, nTop, nRight, nBottom);
  return minY;
}

float TerrainSynth::GetMaxHeight(int nLeft, int nTop, int nRight, int nBottom) const
{
  float minY,maxY;
  GetMinMaxAltitudes(minY,maxY,nLeft, nTop, nRight, nBottom);
  return maxY;
}


/* Analyzes the terrain passed as argument and calculates the minimal altitudes.
 *
 * @param	pTerrain	2d buffer that contains the heightmap
 * @param	pTerrainRect	pointer to the map dimensions
 * @return				1 if ok, 0 otherwise
 */
int TerrainSynth::AnalyzeByArray(float* pTerrain, TSRECT* pTerrainRect, float terrainGrid)
{
  if( !pTerrain || !pTerrainRect )
    return 0;

  // make a copy of the terrain rect
  memcpy( &m_TerrainRect, pTerrainRect, sizeof(TSRECT) );

  int nAngle;
  float fAngle;
  
  // parametric representation of the analyzed line
  TS_LINE_PARAMETRIC tLine = { 
      float(m_TerrainRect.left) + float(m_TerrainRect.width) * 0.5f,
      float(m_TerrainRect.top) + float(m_TerrainRect.height) * 0.5f,
      0.0f, 0.0f };
  
  // analytic representation of the original terrain boundaries
  TS_LINE_ANALYTIC pMapBounds[4] = {
    { 0.0f, 1.0f, float(m_TerrainRect.top) },
    { 0.0f, 1.0f, float(m_TerrainRect.top - m_TerrainRect.height) },
    { 1.0f, 0.0f, float(m_TerrainRect.left) },
    { 1.0f, 0.0f, float(m_TerrainRect.left - m_TerrainRect.width) } };

  int nBound;
  float fR;

  // start the analysis this many pixels before the map boundary
  float fAnalysisLength = AnalysisLength / terrainGrid;

  // there's no point in the analyzed part being longer than the map itself
  float fMaxLength = __min( m_TerrainRect.width, m_TerrainRect.height ) * 0.75f;
  if( fAnalysisLength > fMaxLength )
    fAnalysisLength = fMaxLength;

  TS_POINT ptStart, ptEnd;

  // for "each" angle calculate the minimum altitude within last
  // AnalysisLength meters
  for( nAngle = 0; nAngle < AnalysisDensity; nAngle++ )
  {
    fAngle = 2.0f * Pi * float(nAngle) / float(AnalysisDensity);
        
    tLine.dx = cosf( fAngle );
    tLine.dy = sinf( fAngle );
    
    float maxfR = sqrt(2.0f) * 0.51f * __max( m_TerrainRect.width, m_TerrainRect.height);

    for( nBound = 0; nBound < 4; nBound++ )
      if( CollideLines( &tLine, pMapBounds + nBound, &fR ) ) // lines not parallel
        //FIX HLADAS maxfR - used to be only 1500, which was to small for 4096 sized maps
        if( fR > 0.0f && fR < maxfR ) // we only want to analyze in forward direction
        {          
          ptStart.x = int((fR - fAnalysisLength) * tLine.dx + tLine.x0);
          ptStart.y = int((fR - fAnalysisLength) * tLine.dy + tLine.y0);

          ptEnd.x = int(fR * tLine.dx + tLine.x0);
          ptEnd.y = int(fR * tLine.dy + tLine.y0);

          break;
        }

    // make sure the calculated points are both inside the map rectangle - we wouldn't wanna read outside the terrain array
    ClampToRect( &ptStart, m_TerrainRect.left, m_TerrainRect.top,
        m_TerrainRect.left + m_TerrainRect.width - 1,
        m_TerrainRect.top + m_TerrainRect.height - 1);
    ClampToRect( &ptEnd, m_TerrainRect.left, m_TerrainRect.top,
        m_TerrainRect.left + m_TerrainRect.width - 1,
         m_TerrainRect.top + m_TerrainRect.height - 1);
    
    // search the altmap for the lowest and highest altitude on the specified line
    m_Altitude[nAngle] = GetMinAlt( pTerrain, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y, &m_MaxAltitude[nAngle] );

  }
 
  // set the initialized flag
  m_Initialized = true;

  return 1;
}


/* Calculates the minimum altitude on a line between points (nX0, nY0) and (nX1, nY1).
 *
 * @param pAlt
 * @param	nX0
 * @param	nY0
 * @param	nX1
 * @param	nY1
 * @return the minimum altitude found
 */
float TerrainSynth::GetMinAlt(float* pAlt, int nX0, int nY0, int nX1, int nY1, float* pMax) const
{
  if( !pAlt )
    return 0.0f;

  // use the dda algorithm to "walk the line" and get the minimum altitude

  bool bSteep = abs(nY1 - nY0) > abs(nX1 - nX0);
  int nSwapTemp;

  if( bSteep )
  {
    nSwapTemp = nX0;
    nX0 = nY0;
    nY0 = nSwapTemp;

    nSwapTemp = nX1;
    nX1 = nY1;
    nY1 = nSwapTemp;
  }

  if( nX0 > nX1 )
  {
    nSwapTemp = nX0;
    nX0 = nX1;
    nX1 = nSwapTemp;
    
    nSwapTemp = nY0;
    nY0 = nY1;
    nY1 = nSwapTemp;
  }

  int nDx = nX1 - nX0;
  int nDy = abs(nY1 - nY0);

  float fErr = 0.0f;
  float fDelta = float(nDy) / float(nDx);

  int nYStep = (nY0 < nY1) ? 1 : -1;
  int nY = nY0;
  int nX, nIndex;

  // the accumulator of minimal altitude
  float fMinAlt = Infinity;
  float fMaxAlt = -Infinity;

  for( nX = nX0; nX < nX1; nX++)
  {
    if( bSteep )          
      nIndex = XYToIndex( nY, nX );
    else
      nIndex = XYToIndex( nX, nY );

    if( fMinAlt > pAlt[nIndex] )
      fMinAlt = pAlt[nIndex];

    if( fMaxAlt < pAlt[nIndex] )
      fMaxAlt = pAlt[nIndex];

    fErr += fDelta;

    if( fErr >= 0.5f )
    {
      nY += nYStep;
      fErr -= 1.0f;
    }
  }

  if( pMax )
    *pMax = fMaxAlt;

  return fMinAlt;
}


/* From given coordinates, calculate the index of analysis data
 *
 * @param nX
 * @param	nY 
 * @return the index
 */
int TerrainSynth::GetAngleIndex(int nX, int nY) const
{
  float fDx = float(nX - m_TerrainRect.left - m_TerrainRect.width/2);
  float fDy = float(nY - m_TerrainRect.top - m_TerrainRect.height/2);
  float fD = sqrtf( fDx*fDx + fDy*fDy);

  float invFD = fD>0 ? 1/fD : 0.0f;
  fDx *= invFD; fDy *= invFD;

  float fAngle = fDy < 0.0f ? Pi + acosf( -fDx ) : acosf( fDx );
  
  fAngle *= 0.5f / Pi * float(AnalysisDensity);

  return int( fAngle + 0.5f); 
}