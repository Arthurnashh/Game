// Terrain Synthesizer Helpers
// by Jan Dupej 2009

#include <El/elementpch.hpp>

#include <xmmintrin.h>

#include <math.h>

#include "tersynth_helpers.h"

#ifdef _WIN32
#define XMMALIGN __declspec(align(16))
#define GCCALIGN
#else
#define XMMALIGN
#define GCCALIGN __attribute__((aligned(16)))
#endif

// undef this to calc sin/cos in 2 iterations instead of 3 
#define _HIGH_TRIGONOMETRY_PRECISION

// undef this to calc Pelin noise with bicosine interpolation
#define _PERLIN_LINEAR



static const float Epsilon = 0.000001f;
static const float XMMALIGN PiHalf4[4] GCCALIGN = {PiHalf, PiHalf, PiHalf, PiHalf};
static const float XMMALIGN Unity[4] GCCALIGN = {1.0f, 1.0f, 1.0f, 1.0f};
static const float XMMALIGN Half[4] GCCALIGN = {0.5f, 0.5f, 0.5f, 0.5f};
static const float XMMALIGN Sixth[4] GCCALIGN = {1.0f/6.0f, 1.0f/6.0f, 1.0f/6.0f, 1.0f/6.0f};
static const float XMMALIGN TwentyFourth[4] GCCALIGN = {1.0f/24.0f, 1.0f/24.0f, 1.0f/24.0f, 1.0f/24.0f};
static const float XMMALIGN Recip120[4] GCCALIGN = {1.0f/120.0f, 1.0f/120.0f, 1.0f/120.0f, 1.0f/120.0f};
static const float XMMALIGN Recip720[4] GCCALIGN = {1.0f/720.0f, 1.0f/720.0f, 1.0f/720.0f, 1.0f/720.0f};

static const float XMMALIGN A[4] GCCALIGN = {-4.0f/3.0f, -4.0f/3.0f, -4.0f/3.0f, -4.0f/3.0f };
static const float XMMALIGN B[4] GCCALIGN = {2.0f, 2.0f, 2.0f, 2.0f };
static const float XMMALIGN C[4] GCCALIGN = {1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f};



/* Approximates arccosine
 *
 * @param	x	    angle
 * @param	dest	approx. arccos(x)
 */

float ArcCosine(const float x)
{
  float xsq = x*x;

  return PiHalf - x * (1.0f + xsq * (0.1666667f + 0.075f * xsq) );
}


/* Approximates cosine
 *
 * @param	x	    angle
 * @param	dest	approx. cos(x)
 */
__forceinline float Cosine(const float x)
{  
  float fAngle = x ; 

  if(fAngle <= PiHalf  )
  {
    float xsq = fAngle * fAngle;
    return 1.0f - xsq*0.5f + xsq*xsq/24.0f;    
  }
  else
  {
    fAngle = Pi-fAngle;
    float xsq = fAngle * fAngle;
    return -(1.0f - xsq*0.5f + xsq*xsq/24.0f);
  }

}

float SmoothStep(const float x)
{
  // Horner for the polynomial ax^3 + bx^2 + cx
  return x * ( C[0] + x * ( B[0] + A[0] * x) );
}

void SmoothStep_xmm(const __m128 x, __m128& dest)
{
  // polynomial ax^3 + bx^2 + cx - coefficients are constants A, B, C
  __m128 xAccum = x;
  
  xAccum = _mm_mul_ps(x, x);

  dest = _mm_add_ps( _mm_mul_ps(xAccum, _mm_load_ps(B)), _mm_mul_ps(x, _mm_load_ps(C)) );
  xAccum = _mm_mul_ps( xAccum, x);
  dest = _mm_add_ps(dest, _mm_mul_ps(xAccum, _mm_load_ps(A)) );
}

/* Approximates cosine of each element in a vector of 4 floats.
 *
 * @param	x	    input vector of 4 floats
 * @param	dest	vector of cosines 
 */
 void Cosine_xmm(const __m128 x, __m128& dest)
{
  // cos x = (approx.) 1-x^2/2
  __m128 xAccum, xPower;

  __m128 xPiHalf = _mm_load_ps(PiHalf4);
  __m128 xMask = _mm_cmpgt_ps( x, xPiHalf );

  xPiHalf = _mm_add_ps(xPiHalf, xPiHalf);
  
  // for better precision - if x > Pi/2, exploit symmetry
  __m128 xX = _mm_or_ps( _mm_and_ps( xMask, _mm_sub_ps( xPiHalf, x ) ),
                  _mm_andnot_ps( xMask, x ) );

  // 1
  dest = _mm_load_ps( Unity );

  // -(x^2)/2
  xPower = _mm_mul_ps( xX, xX );
  xAccum = _mm_load_ps( Half );
  xAccum = _mm_mul_ps( xPower, xAccum );
  dest = _mm_sub_ps( dest, xAccum );  
  
#ifdef _HIGH_TRIGONOMETRY_PRECISION
  // +(x^4)/24
  xPower = _mm_mul_ps( xPower, xPower );
  xAccum = _mm_load_ps( TwentyFourth );
  xAccum = _mm_mul_ps( xPower, xAccum );
  dest = _mm_add_ps( dest, xAccum );
#endif

  // for better precision - if x > Pi/2, exploit symmetry
  dest = _mm_or_ps( _mm_andnot_ps( xMask, dest ),
              _mm_and_ps( xMask, _mm_sub_ps( _mm_setzero_ps(), dest ) ) );
  
}

/* Calculates the hash of a point to be used as seed for the noise function
 *
 * @param	x
 * @param y
 * @return  the hash
 */
int __fastcall GetPxHash(const int x, const int y)
{
  return ((x << 6) + (y << 2)) ^ (x + y) ^ ((x+y) << 8);
  //return x ^ y - ((x << 2) - y);
}

/* Calculates and adds Perlin noise to the specified buffer. 
 *
 * @param	pBuffer	output 2d buffer with dims of nWidth and nHeight (must be aligned to 16B) 
 * @param	nWidth	width of output buffer - must be divisible by 4 and larger than 0
 * @param	nHeight	height of output buffer - must be larger than 0
 * @param nOctave specifies the wavelength of Perlin noise (=128/(2^nOctave))
 * @param fAmplitude sets the factor by which to scale the noise
 * @return				1 if ok, 0 otherwise
 */
int AddPerlin_xmm(float* pBuffer,int nX, int nY, int nWidth, int nHeight, int nOctave, float fAmplitude, TS_INVALID* pInvalid)
{
  if( !pBuffer || nHeight < 0 || nWidth < 0 || nOctave < 0 )
    return 0;

  int nSegments = 1 << nOctave;
  int nSegWidth = nWidth >> nOctave;
  int nSegHeight = nHeight >> nOctave;

  int nSegXReal = 0;
  int nSegYReal = 0;
  int nSegWidthReal = 128 >> nOctave;
  int nSegHeightReal = 128 >> nOctave;

  int nSegX = 0;
  int nSegY = 0;
  int i, j, ii, jj;
  
  int row0 = pInvalid ? pInvalid->row0 : 0;
  int row1 = pInvalid ? pInvalid->row1 : 128;
  int col0 = pInvalid ? pInvalid->col0 : 0;
  int col1 = pInvalid ? pInvalid->col1 : nWidth;
    int nWidth2 = col1 - col0;


  XMMALIGN float fCorners[4] GCCALIGN;
   __m128 pCorners[4];   // the corners of the output buffer, in order TL, TR, BL, BR
  

   // increment this value to Y coord after every cycle
  XMMALIGN float fYIncrement GCCALIGN = 1.0f / float(nHeight) * float(nSegments);
  __m128 xYIncrement = _mm_load1_ps( &fYIncrement );

  // increment these values to X coord after every cycle
  XMMALIGN float fXIncrement GCCALIGN = 1.0f / float(nWidth) * float(nSegments);
  XMMALIGN float pXInit[4] GCCALIGN = {0.0f, fXIncrement, 2.0f* fXIncrement, 3.0f * fXIncrement};

  __m128 xXInit = _mm_load_ps ( pXInit ); 
  fXIncrement *= 4.0f;
  __m128 xXIncrement = _mm_load1_ps( &fXIncrement );

  __m128 xX, xY, xTemp;
  __m128 xAccum1, xAccum2;
  __m128 pCoeffs[4]; // interpolation coefficients, {f(x), 1-f(x), f(y), 1-f(y)} f(x) is SmoothStep
  __m128 xUnity = _mm_load_ps( Unity );

  for(i = 0; i < nSegments; i++)  // cycle through vertical segments
  {
    nSegX = 0;       
    nSegXReal = 0;

    for(j=0; j < nSegments; j++) // horz segments
    {
      // calculate noise values in edges
      fCorners[0] = Noise( nOctave, GetPxHash(nX+nSegXReal,           nY+nSegYReal) ) * fAmplitude;
      fCorners[1] = Noise( nOctave, GetPxHash(nX+nSegXReal+nSegWidthReal, nY+nSegYReal) ) * fAmplitude;
      fCorners[2] = Noise( nOctave, GetPxHash(nX+nSegXReal,           nY+nSegYReal+nSegHeightReal) ) * fAmplitude;
      fCorners[3] = Noise( nOctave, GetPxHash(nX+nSegXReal+nSegWidthReal, nY+nSegYReal+nSegHeightReal) ) * fAmplitude;

      for(ii=0; ii<4; ii++)     
        pCorners[ii] = _mm_load1_ps( fCorners + ii);	      
      
       
      xY = _mm_setzero_ps();  // load zero

      // now do the rest
      for(ii=nSegY; ii<nSegY+nSegHeight; ii++)
      {
        
        if( ii >= row0 && ii < row1)
        {
          xX = xXInit;         

          pCoeffs[3] = xY;
          SmoothStep_xmm(xY, pCoeffs[3]);
          pCoeffs[2] = _mm_sub_ps( xUnity, pCoeffs[3] );        

          for(jj=nSegX; jj<nSegX+nSegWidth; jj+=4)
          { 
            if( jj >= col0 && jj < col1 )
            {
              pCoeffs[1] = xX;
              SmoothStep_xmm(xX, pCoeffs[1]);
              pCoeffs[0] = _mm_sub_ps( xUnity, pCoeffs[1] );          

               // interpolate upper pair
              xAccum1 = _mm_add_ps(_mm_mul_ps( pCorners[0], pCoeffs[0] ), _mm_mul_ps( pCorners[1], pCoeffs[1] ) );
              // interpolate lower pair
              xAccum2 = _mm_add_ps(_mm_mul_ps( pCorners[2], pCoeffs[0] ), _mm_mul_ps( pCorners[3], pCoeffs[1] ) );
          
              // put it all together, set 4 pixels of the buffer and move fwd.
              xTemp = _mm_load_ps( pBuffer + (ii-row0)*nWidth2 + jj - col0 );
              
              xTemp = _mm_add_ps( xTemp, _mm_add_ps( _mm_mul_ps( xAccum1, pCoeffs[2] ), _mm_mul_ps( xAccum2, pCoeffs[3])));      
              _mm_store_ps( pBuffer + (ii-row0)*nWidth2 + jj - col0, xTemp );      
            }
            xX = _mm_add_ps( xX, xXIncrement );
          } // jj

        } // row0, row1

        xY = _mm_add_ps( xY, xYIncrement );
      } // ii
      

      nSegX += nSegWidth;
      nSegXReal += nSegWidthReal;
    } // j

    nSegY += nSegHeight;
    nSegYReal += nSegHeightReal;
  } // i

  return 1;
}


/* Calculates and adds Perlin noise to the specified buffer. 
 *
 * @param	pBuffer	output 2d buffer with dims of nWidth and nHeight 
 * @param	nWidth	width of output buffer - must be larger than 0
 * @param	nHeight	height of output buffer - must be larger than 0
 * @param nOctave specifies the wavelength of Perlin noise (=128/(2^nOctave))
 * @param fAmplitude sets the factor by which to scale the noise
 * @return				1 if ok, 0 otherwise
 */
int AddPerlin(float* pBuffer,int nX, int nY, int nWidth, int nHeight, int nOctave, float fAmplitude, TS_INVALID* pInvalid)
{
  if( !pBuffer || nHeight < 0 || nWidth < 0 || nOctave < 0 )
    return 0;

  int nSegments = 1 << nOctave;
  int nSegWidth = nWidth >> nOctave;
  int nSegHeight = nHeight >> nOctave;

  int nSegXReal = 0;
  int nSegYReal = 0;
  int nSegWidthReal = 128 >> nOctave;
  int nSegHeightReal = 128 >> nOctave;

  int nSegX = 0;
  int nSegY = 0;
  int i, j, ii, jj;

  XMMALIGN float fCorners[4] GCCALIGN;
  XMMALIGN float fCoeffs[4] GCCALIGN;
 
  int row0 = pInvalid ? pInvalid->row0 : 0;
  int row1 = pInvalid ? pInvalid->row1 : 128;
  int col0 = pInvalid ? pInvalid->col0 : 0;
  int col1 = pInvalid ? pInvalid->col1 : nWidth;
    int nWidth2 = col1 - col0;
 
  float fIncrementY = 1.0f / float(nHeight) * float(nSegments); 

  // increment these values to X coord after every cycle
  XMMALIGN float fIncrementX GCCALIGN = 1.0f / float(nWidth) * float(nSegments);


  float fX, fY;
  float fAccum1, fAccum2;

  for(i = 0; i < nSegments; i++)  // cycle through vertical segments
  {
    nSegX = 0;       
    nSegXReal = 0;

    for(j=0; j < nSegments; j++) // horz segments
    {
      // calculate noise values in edges
      fCorners[0] = Noise( nOctave, GetPxHash(nX+nSegXReal,           nY+nSegYReal) ) * fAmplitude;
      fCorners[1] = Noise( nOctave, GetPxHash(nX+nSegXReal+nSegWidthReal, nY+nSegYReal) ) * fAmplitude;
      fCorners[2] = Noise( nOctave, GetPxHash(nX+nSegXReal,           nY+nSegYReal+nSegHeightReal) ) * fAmplitude;
      fCorners[3] = Noise( nOctave, GetPxHash(nX+nSegXReal+nSegWidthReal, nY+nSegYReal+nSegHeightReal) ) * fAmplitude;          
       
      fY = 0.0f;  // load zero

      // now do the rest
      for(ii=nSegY; ii<nSegY+nSegHeight; ii++)
      {
        if( ii >= row0 && ii < row1 )
        {
          fX = 0.0f;  
                  
          fCoeffs[3] = SmoothStep(fY);
          fCoeffs[2] = 1.0f - fCoeffs[3];        

          for(jj=nSegX; jj<nSegX+nSegWidth; jj++)
          { 
            if( jj >= col0 && jj < col1 )
            {
              fCoeffs[1] = SmoothStep(fX);
              fCoeffs[0] = 1.0f - fCoeffs[1];          

               // interpolate upper pair
              fAccum1 = fCorners[0] * fCoeffs[0] + fCorners[1] * fCoeffs[1];          

              // interpolate lower pair
              fAccum2 = fCorners[2] * fCoeffs[0] + fCorners[3] * fCoeffs[1];                    
          
              // put it all together, set 4 pixels of the buffer and move fwd.
              pBuffer[ii*nWidth2 + jj - col0] += fAccum1 * fCoeffs[2] + fAccum2 * fCoeffs[3];
              //xTemp = _mm_setzero_ps();          
            }

            fX += fIncrementX;
          } // jj
        }

        fY += fIncrementY;
      } // ii
      

      nSegX += nSegWidth;
      nSegXReal += nSegWidthReal;
    } // j

    nSegY += nSegHeight;
    nSegYReal += nSegHeightReal;
  } // i

  return 1;
}



__forceinline float cerp(float a, float b, float r)
{
  float f = (Cosine(Pi * r) + 1.0f) * 0.5f;
  return a * f + b * (1.0f - f);
}


/* Interpolates a row vector into a matrix with smooth interpolation
 *
 * @param	pBuffer	the buffer (dims = width * height)
 * @param pDer the first derivatives in that direction
 * @param	pRow	the row vector
 * @param nDistance how far from the expected position of pRow the buffer begins
 * @param	nHeight	height of output buffer
 * @param nWidth width of the output buffer
 * @param nDirection specifies if the pRow will be positioned on top or bottom of pBuffer (-1 = top , anything else = bottom)
 * @return				1 if ok, 0 otherwise
 */
int InterpolateBorderVert(float* pBuffer, const float* pDer, const float* pRow, int nDistance, int nHeight, int nWidth, int nDirection, int nStride,int nStart, int nEnd)
{
  int i, j, k;

  if(nDirection == -1)
  {
    float x;

    for(j=nStart/nStride; j<nEnd/nStride; j++)
    {      
      for(i=0; i<nHeight; i++)
      {        
        k = nHeight*nStride - i* nStride + nDistance;
        x = pRow[j*nStride] + k * pDer[j*nStride];       
        if(k < 64)       
          pBuffer[i*nWidth+j] = lerp(x,pBuffer[i*nWidth+j], SmoothStep(float(k) / 64.0f));
         
      }
    }
  
  
  }
  else
  {   
    float x;

    for(j=nStart/nStride; j<nEnd/nStride; j++)
    {      
      for(i=__max(0,nDistance/nStride); i<__min(nHeight,64/nStride + nDistance/nStride); i++)
      {     
        k = i * nStride -nDistance ;
        x = pRow[j*nStride] + k * pDer[j*nStride];       
        if(k < 64)       
          pBuffer[i*nWidth+j] = lerp(x,pBuffer[i*nWidth+j], SmoothStep(float(k) / 64.0f));
         
      }
    }
  }

  return 1;
}


/* Interpolates a column vector into a matrix with smooth interpolation
 *
 * @param	pBuffer	the buffer (dims = width * height)
 * @param pDer the first derivatives in that direction
 * @param	pRow	the row vector
 * @param nDistance how far from the expected position of pRow the buffer begins
 * @param	nHeight	height of output buffer
 * @param nWidth width of the output buffer
 * @param nDirection specifies if the pRow will be positioned on left or right of pBuffer (-1 = left , anything else = right)
 * @return				1 if ok, 0 otherwise
 */
int InterpolateBorderHorz(float* pBuffer, const float* pDer,const float* pCol, int nDistance, int nHeight, int nWidth, int nDirection, int nStride)
{
  int i, j, k;
  float x;

  if(nDirection == -1)
  {
    for(i=0; i<nHeight/nStride; i++)
    {
      for(j=0; j<nWidth; j++)
      {
        k = nWidth * nStride - j * nStride + nDistance;
        x = pCol[i*nStride] + (k) * pDer[i*nStride];

        if(k<64)
          pBuffer[i*nWidth+j] = lerp(x, pBuffer[i*nWidth+j], SmoothStep(float(k)/64.0f));       
      }
    }
  }
  else
  {
    for(i=0; i<nHeight/nStride; i++)
    {
      for(j=__max(0, nDistance/nStride); j< __min(nWidth, 64/nStride + nDistance/nStride); j++)
      //for(j=0; j< nWidth; j++)
      {
        k = (j) * nStride - nDistance;
        x = pCol[i*nStride] + k * pDer[i*nStride];

        if(k<64 )          
          pBuffer[i*nWidth+j] = lerp(x, pBuffer[i*nWidth+j], SmoothStep(float(k)/64.0f));        
      }
    }
  }

  return 1;
}


/* Interpolate a corner into a patch with first derivative and smooth interpolation
 *
 * @param	pBuffer	the buffer (dims = width * height)
 * @param	fCorner the value at the corner
 * @param fDer1 first derivative in one direction
 * @param	fDer2 first derivative in the other direction
 * @param nX reserved
 * @param nY reserved
 * @param nWidth reserved
 * @param nHeight reserved
 * @param nCorner specifies which corner's value is fCorner (5=TL 6=TR 7=BL 8=BR)
 * @return				1 if ok, 0 otherwise
 */
int InterpolateCorner2(float* pBuffer, const float fCorner, const float fDer1, const float fDer2 ,int nX, int nY, int nWidth, int nHeight, int nCorner, int nStride)
{
  int x0, x1, y0, y1, cx, cy;
  float fCorr;
  int x, y;
  
  nWidth *= nStride;
  nHeight *= nStride;

  switch(nCorner)
  {
  case 5:
    x0=0;                    y0=0;
    x1=nWidth;                      y1 = nHeight;
    cx = nWidth+nX;                    cy = nHeight+nY;
    fCorr = -1.0f;
    break;
  case 6:
    x0=__max(nX, 0);                y0 = 0;
    x1=__min(nWidth,nX+64);         y1 = nHeight;
    cx = nX;                        cy = nHeight+nY;
    fCorr = 1.0f;
    break;
  case 7:
    x0=0;                    y0 = __max(nY,0);
    x1=nWidth;                y1 = __max(0,__min(nHeight, nY+64));
    cx = nWidth+nX;           cy = nY;    
    fCorr = -1.0f;  
    
    break;
  case 8:
    x0=__max(nX,0);             y0=__max(nY,0);
    x1= __min(nWidth,nX+64); y1 = __min(nHeight,nY+64);
    cx = nX;                    cy = nY;
    fCorr = 1.0f;
    break;

  default:
    return 0;
    break;
  }

  float fExtr, fRadius, fAngle;
  int nLOD2 = nStride * nStride;

  x0 /= nStride;
  x1 /= nStride;
  y0 /= nStride;
  y1 /= nStride;
  cx /= nStride;
  cy /= nStride;

  nWidth /= nStride;

  for(y=y0; y<y1; y++)
  {
    for(x=x0; x<x1; x++)
    {
      fRadius = sqrtf(float(( x-cx) * (x - cx)*nLOD2 + (y-cy) * (y-cy) * nLOD2));    // HACK: approximate sqrtf
      fAngle = fabs(atan2f( float(y-cy)  , fCorr*float(x-cx ) ));                        // HACK: approximate atanf

      fExtr = fCorner + fRadius * lerp(fDer1, fDer2, SmoothStep(fAngle / PiHalf));
      
      if(fRadius < 64.0f && fRadius >=0.0f)
        pBuffer[y*nWidth+x] = lerp( fExtr, pBuffer[y*nWidth+x], SmoothStep(fRadius / 64.0f));
    }
  }
  
  return 1;
}

/* Bilinear interpolation
 *
 * @param	pIn   source values {top left, top right, bottom left, bottom right}
 * @param	nHeight	total height
 * @param nWidth total width
 * @param nX  position - x
 * @param nY  position - y
 * @return		interpolated value
 */
float BilinearInterpolatePoint(const float* pIn, int nWidth, int nHeight, int nX, int nY)
{
  float fCoeff2 = float(nX) / float(nWidth);
  float fCoeff1 = 1.0f - fCoeff2;

  float fUpper = pIn[0] * fCoeff1 + pIn[1] * fCoeff2;
  float fLower = pIn[2] * fCoeff1 + pIn[3] * fCoeff2;
  
  fCoeff2 = float(nY) / float(nHeight);
  fCoeff1 = 1.0f - fCoeff2;

  return fUpper * fCoeff1 + fLower * fCoeff2;
}

/* SmoothStep interpolation
 *
 * @param	pIn   source values {top left, top right, bottom left, bottom right}
 * @param	nHeight	total height
 * @param nWidth total width
 * @param nX  position - x
 * @param nY  position - y
 * @return		interpolated value
 */
float BicosInterpolatePoint(const float* pIn, int nWidth, int nHeight, int nX, int nY)
{
   float fCoeff2 = SmoothStep(float(nX) / float(nWidth));
  float fCoeff1 = 1.0f - fCoeff2;

  float fUpper = pIn[0] * fCoeff1 + pIn[1] * fCoeff2;
  float fLower = pIn[2] * fCoeff1 + pIn[3] * fCoeff2;
  
  fCoeff2 = SmoothStep(float(nY) / float(nHeight));
  fCoeff1 = 1.0f - fCoeff2;

  return fUpper * fCoeff1 + fLower * fCoeff2;
}




/* Fills the pBuffer as follows:
 * - corners (as defined by nWidth, nHeight) get the values in pIn's corners
 * - calculates the rest with smoothstep interpolation
 *
 * @param	pBuffer	output 2d buffer with dims of nWidth and nHeight (must be aligned to 16B)
 * @param	pIn			input 2x2 2d buffer that holds the corners of pBuffer (must be aligned to 16B)
 * @param	nWidth	width of output buffer - must be divisible by 4 and larger than 0
 * @param	nHeight	height of output buffer - must be larger than 0
 * @return				1 if ok, 0 otherwise
 */
int BilinearInterpolate_xmm(float* pBuffer, const float* pIn, int nWidth, int nHeight, TS_INVALID* pInvalid)
{
  // check for invalid input
  if( !pBuffer || !pIn || nWidth < 0 || nHeight < 0 || nWidth & 0x3 || nHeight & 0x3 )
    return 0;

  DoAssert(intptr_t(pBuffer)%sizeof(__m128)==0);
  int i;

  int row0 = pInvalid ? pInvalid->row0 : 0;
  int row1 = pInvalid ? pInvalid->row1 : nHeight;
  int col0 = pInvalid ? pInvalid->col0 : 0;
  int col1 = pInvalid ? pInvalid->col1 : nWidth;

  __m128 pCorners[4];   // the corners of the output buffer, in order TL, TR, BL, BR
  for(i=0; i<4; i++)
  {
    pCorners[i] = _mm_set_ps1(pIn[i]);
  }
  
  __m128 xIncrementX, xIncrementY;
  __m128 xInitX;
  __m128 pCoeff[4];     // {f(x), 1-f(x), f(y), 1-f(y)},  f(x) is smoothstep

  float fIncrementX = 1.0f / float(nWidth);
  XMMALIGN float pAux[4] GCCALIGN = {0.0f, fIncrementX, 2.0f * fIncrementX, 3.0f * fIncrementX};
  xInitX = _mm_load_ps(pAux);

  fIncrementX *= 4.0f;
  xIncrementX = _mm_load1_ps(&fIncrementX);
  
  float fIncrementY = 1.0f / float(nHeight);
  xIncrementY = _mm_load1_ps(&fIncrementY);

  int j;

  __m128 xX, xY, xAccum1, xAccum2;
  xY = _mm_setzero_ps();    // set y to 0

  // return value = (a*x + b*(1x))*y + (c*x + d*(1-x))*(1-y)
  // x,y within (0, 1)

  //pBuffer += nWidth * row0;

  for(i=0; i<nHeight; i++)
  { 
    if( i>= row0 && i < row1 )
    {
      xX = xInitX;

      //pCoeff[3] = xY;
      SmoothStep_xmm(xY, pCoeff[3]);
      pCoeff[2] = _mm_sub_ps(_mm_load_ps( Unity ), pCoeff[3]);
      
      for(j=0; j<nWidth; j+=4)
      {
        if( j>= col0 && j< col1 )
        {
          //Cosine_xmm(xX, pCoeff[0]); // calculate interpolation coeffs   
          //pCoeff[0] = _mm_mul_ps(_mm_add_ps( pCoeff[0], _mm_load_ps( Unity ) ), _mm_load_ps( Half ));
          //pCoeff[1] = xX;
          SmoothStep_xmm(xX, pCoeff[1]);
          pCoeff[0] = _mm_sub_ps(_mm_load_ps( Unity ), pCoeff[1]);
          
          // interpolate upper pair
          xAccum1 = _mm_add_ps(_mm_mul_ps( pCorners[0], pCoeff[0] ), _mm_mul_ps( pCorners[1], pCoeff[1] ) );
          // interpolate lower pair
          xAccum2 = _mm_add_ps(_mm_mul_ps( pCorners[2], pCoeff[0] ), _mm_mul_ps( pCorners[3], pCoeff[1] ) );
          
          // put it all together, set 4 pixels of the buffer and move fwd.
          _mm_store_ps( pBuffer, _mm_add_ps( _mm_mul_ps( xAccum1, pCoeff[2] ), _mm_mul_ps( xAccum2, pCoeff[3])));      
          pBuffer += 4;
        }
        xX = _mm_add_ps(xX, xIncrementX);
      } // for
    } // if
    xY = _mm_add_ps(xY, xIncrementY);
  } // for

  return 1;
}


/* Create a matrix of bilinear-interpolated values and multiply !per-element! with another matrix
 *
 * @param pBuffer  input matrix - must be aligned to 16 byte boundary
 * @param	pIn   source values {top left, top right, bottom left, bottom right}
 * @param	nHeight	total height
 * @param nWidth total width 
 * @return		0 if error, 1 if ok
 */
int BilinearInterpolateMul_xmm(float* pBuffer, const float* pIn, int nWidth, int nHeight, TS_INVALID* tInvalid)
{
  // check for invalid input
  if( !pBuffer || !pIn || nWidth < 0 || nHeight < 0 || nWidth & 0x3 || nHeight & 0x3 )
    return 0;

  int i;

  int row0 = tInvalid ? tInvalid->row0 : 0;
  int row1 = tInvalid ? tInvalid->row1 : nHeight;
  int col0 = tInvalid ? tInvalid->col0 : 0;
  int col1 = tInvalid ? tInvalid->col1 : nWidth;

  __m128 pCorners[4];   // the corners of the output buffer, in order TL, TR, BL, BR
  for(i=0; i<4; i++)  
    pCorners[i] = _mm_load1_ps(pIn + i);
	  
  
  __m128 xIncrementX, xIncrementY;
  __m128 xInitX;
  __m128 pCoeff[4];    

  float fIncrementX = 1.0f / float(nWidth);
  XMMALIGN float pAux[4] GCCALIGN = {0.0f, fIncrementX, 2.0f * fIncrementX, 3.0f * fIncrementX};
  xInitX = _mm_load_ps(pAux);

  fIncrementX *= 4.0f;
  xIncrementX = _mm_load1_ps(&fIncrementX);
  
  float fIncrementY = 1.0f / float(nHeight);
  xIncrementY = _mm_load1_ps(&fIncrementY);

  int j;

  __m128 xX, xY, xAccum1, xAccum2, xUnity;
  xUnity = _mm_load_ps(Unity);
  xY = _mm_setzero_ps();   

  for(i=0; i<nHeight; i++)
  {  
    if( i >= row0 && i < row1 )
    {
      xX = xInitX;    
      pCoeff[2] = _mm_sub_ps(xUnity, xY);

      for(j=0; j<nWidth; j+=4)
      {      
        if( j>= col0 && j < col1)
        {
          pCoeff[0] = _mm_sub_ps(xUnity, xX);  

          // interpolate upper pair
          xAccum1 = _mm_add_ps(_mm_mul_ps( pCorners[0], pCoeff[0] ), _mm_mul_ps( pCorners[1], xX ) );
          // interpolate lower pair
          xAccum2 = _mm_add_ps(_mm_mul_ps( pCorners[2], pCoeff[0] ), _mm_mul_ps( pCorners[3], xX ) );
          
          // put it all together, set 4 pixels of the buffer and move fwd.
          xAccum1 = _mm_add_ps( _mm_mul_ps( xAccum1, pCoeff[2] ), _mm_mul_ps( xAccum2, xY));      
          _mm_store_ps(pBuffer, _mm_mul_ps(_mm_load_ps(pBuffer), xAccum1));
          pBuffer += 4;
        }
        xX = _mm_add_ps(xX, xIncrementX);
      } // for
    } // if
    xY = _mm_add_ps(xY, xIncrementY);
  } // for

  return 1;
}



/* Calculates the collision of 2 lines in 2d
 *
 * @param	pLine1	parametric representation of line1
 * @param	pLine2	analytic representation of line2
 * @param pR			the distance from start of line1 where the collision was found (not changed if lines parallel) or NULL
 * @return        false if lines are parallel, true otherwise
 */
bool CollideLines(const TS_LINE_PARAMETRIC* pLine1, const TS_LINE_ANALYTIC* pLine2, float* pR)
{
  float fDenominator = pLine2->a * pLine1->dx + pLine2->b * pLine1->dy;

  if( fDenominator < Epsilon && fDenominator > -Epsilon ) // lines are parallel
    return false;

  if( pR )
    *pR = ( pLine2->a * pLine1->x0 + pLine2->b * pLine1->y0 + pLine2->c ) / fDenominator;
  
  return true;
}


/* Clamps a point to a rectangle
 *
 * @param	pPt	point to be examined
 * @param	nLeft	dimensions of the rectangle
 * @param	nTop	dimensions of the rectangle
 * @param	nRight	dimensions of the rectangle
 * @param	nBottom	dimensions of the rectangle 
 */
void ClampToRect(TS_POINT* pPt, int nLeft, int nTop, int nRight, int nBottom)
{
  if( pPt->x < nLeft )
    pPt->x = nLeft;
  else if ( pPt->x > nRight )
    pPt->x = nRight;

  if( pPt->y < nTop )
    pPt->y = nTop;
  else if( pPt->y > nBottom )
    pPt->y = nBottom;
}


/* Add two arrays of float
 *
 * @param	pDest   destination = destination + source (per-element) - must be aligned to 16B
 * @param	pSrc    source array  - must be aligned to 16B
 * @param nFloats size of both arrays must be divisble by 4 
 * @return		0 if error, 1 if ok
 */
int AddFloatArrs_xmm(float* pDest, float* pSrc, int nFloats)
{
  if( !pDest || !pSrc || nFloats & 0x3 )
    return 0;

  __m128* pDestXmm = (__m128*)pDest;
  __m128* pSrcXmm = (__m128*)pSrc;

  while(nFloats)
  {
    *pDestXmm = _mm_add_ps( *pDestXmm, *pSrcXmm );

    pDestXmm++;
    pSrcXmm++;

    nFloats -= 4;
  }

  return 1;
}