// #ifdef _PROFILE
// #pragma optimize("", off)
// #endif

// The class that combines original and synthesized terrain
// by Jan Dupej 2009

#include <El/elementpch.hpp>

#include "terrain.h"

#include <malloc.h>
#include <math.h>
#include "tersynth_helpers.h"

Terrain::Terrain(void) : 
  m_Terrain(NULL), m_BorderDerivative(NULL)
{
  m_Border = (float*) _aligned_malloc(128 * sizeof(float), 16);
}

Terrain::~Terrain(void)
{
  _aligned_free(m_Border);
  DestroyBorderInfo();
}


/* Sets the pointer to the original terrain and analyzes
 *
 * @param	pTerrain	    the float-array of the terrain's heightmap
 * @param	nWidth	    width of terrain
 * @param	nHeight	    height of terrain
 * @return  0 if error, 1 if ok
 */
int Terrain::SetTerrain(float* pTerrain, int nWidth, int nHeight, float terrainGrid)
{
  if(!pTerrain)
    return 0;

  m_Terrain = pTerrain;
  m_Width = nWidth;
  m_Height = nHeight;

  TSRECT rect = {0,0,nWidth,nHeight};

  CreateBorderInfo( pTerrain, nWidth, nHeight );

  if(!m_TerSynth.AnalyzeByArray( m_Terrain, &rect, terrainGrid))
    return 0;

  
  
  return 1;
}


int Terrain::CreateBorderInfo(float* pAlt, int nWidth, int nHeight)
{
  if( !pAlt || nWidth < 1 || nHeight < 1 || m_BorderDerivative )
    return 0;
  
  m_BorderDerivative = (float*) _aligned_malloc( 2 * (nWidth + nHeight) * sizeof(float), 16 );

  int i;
  
  // the analysis IIR filter tap  
  float fIIRTap[2] = {0.0f, 0.0f};
  float fLastHeight[2] = {0.0f, 0.0f};
  int j;

  // calculate the first derivative of the pixels near the border of the original map
  // enhanced by filtering out high frequency noise with an IIR filter

  // calculate the first derivative at the north and south border
  for(i=0; i<nWidth; i++)
  {
    // initialize the IIR filters
    fIIRTap[0] = pAlt[ i + 2*AnalysisDD * nWidth ];
    fIIRTap[1] = pAlt[ i + (nHeight-2*AnalysisDD) * nWidth ];
    
    // go mid-way to the border and get a filtered value there
    for(j=2*AnalysisDD-1; j>AnalysisDD; j--)
    {
      fIIRTap[0] = (fIIRTap[0] * AnalysisFeedbackCoeff + pAlt[ i + j*nWidth ]) * AnalysisIIRWeight;
      fIIRTap[1] = (fIIRTap[1] * AnalysisFeedbackCoeff + pAlt[ i + (nHeight-j-1)*nWidth ]) * AnalysisIIRWeight;
    }
  
    fLastHeight[0] = fIIRTap[0];
    fLastHeight[1] = fIIRTap[1];

    // go the rest of the way and get another filtered value
    for(j=AnalysisDD; j>=0; j--)
    {
      fIIRTap[0] = (fIIRTap[0] * AnalysisFeedbackCoeff + pAlt[ i + j*nWidth ]) * AnalysisIIRWeight;
      fIIRTap[1] = (fIIRTap[1] * AnalysisFeedbackCoeff + pAlt[ i + (nHeight-j-1)*nWidth ]) * AnalysisIIRWeight;
    }
    
    // calculate the first derivative
    m_BorderDerivative[i] = (fIIRTap[0] - fLastHeight[0]) / float(AnalysisDD);
    m_BorderDerivative[i + nWidth] = (fIIRTap[1] - fLastHeight[1]) / float(AnalysisDD);
  }



  // calculate the first derivative at the east and west borders
  for(i=0; i<nHeight; i++)
  {
    // initialize the IIR filters
    fIIRTap[0] = pAlt[ i * nWidth + 2*AnalysisDD ];
    fIIRTap[1] = pAlt[ i * nWidth + nWidth - 2*AnalysisDD - 1];
    
    // go mid-way to the border and get a filtered value there
    for(j=2*AnalysisDD-1; j>AnalysisDD; j--)
    {
      fIIRTap[0] = (fIIRTap[0] * AnalysisFeedbackCoeff + pAlt[ j + i * nWidth ]) * AnalysisIIRWeight;
      fIIRTap[1] = (fIIRTap[1] * AnalysisFeedbackCoeff + pAlt[  (i+1) * nWidth-1 -j ]) * AnalysisIIRWeight;
    }
  
    fLastHeight[0] = fIIRTap[0];
    fLastHeight[1] = fIIRTap[1];

    // go the rest of the way and get another filtered value
    for(j=AnalysisDD; j>=0; j--)
    {
      fIIRTap[0] = (fIIRTap[0] * AnalysisFeedbackCoeff + pAlt[ j + i * nWidth ]) * AnalysisIIRWeight;
      fIIRTap[1] = (fIIRTap[1] * AnalysisFeedbackCoeff + pAlt[  (i+1) * nWidth-1 -j]) * AnalysisIIRWeight;
    }
    
    // calculate the first derivative
    m_BorderDerivative[i + nWidth*2] = (fIIRTap[0] - fLastHeight[0]) / float(AnalysisDD);
    m_BorderDerivative[i + nWidth*2 + nHeight] = (fIIRTap[1] - fLastHeight[1]) / float(AnalysisDD);
  }
  return 1;
}

int Terrain::DestroyBorderInfo(void)
{
  if(m_BorderDerivative)
    _aligned_free(m_BorderDerivative);
  
  m_BorderDerivative = NULL;

  return 1;
}



/* Sample the altitude of the terrain
 *
 * @param	nX  x-coord (px)
 * @param	nY  y-coord (px) 
 * @return  the alititude at specified coords
 */
float Terrain::GetPixel(int nX, int nY) const
{
  if( !m_Terrain )
    return 0.0f;

  if(nX >=0 && nX < m_Width && nY >= 0 && nY < m_Height)
  {    
    return m_Terrain[ nY*m_Width+nX ];    
  }
  else
  { 
    // sample the point from the synthesizer
    float fAlt = m_TerSynth.GetPoint(nX, nY);

    // check if we need to interpolate with the original map border
    int nSector = GetBorderSector( nX, nY );

    if(nSector) // we do need to interpolate
    {
      float fB = 0.0f;    // closest border alt.
      float fD = 0.0f;    // 1st derivative at that point
      float fR = 0.0f;    // distance from border
      float dx, dy;

      switch(nSector) // see which border/corner we need to interpolate with
      {
      case 1:
        fB = m_Terrain[nX];
        fD = m_BorderDerivative[nX];
        fR = float(-nY);
        break;
      case 3:
        fB = m_Terrain[(m_Height-1) * m_Width + nX];
        fD = m_BorderDerivative[m_Width + nX];
        fR = float(nY - m_Height);
        break;
      case 2:
        fB = m_Terrain[nY * m_Width];
        fD = m_BorderDerivative[m_Width*2 + nY];
        fR = float(-nX);
        break;
      case 4:
        fB = m_Terrain[(nY+1) * m_Width -1 ];
        fD = m_BorderDerivative[m_Width * 2 + m_Height + nY];
        fR = float(nX - m_Width);
        break;
      case 5:
        fB = m_Terrain[0];
        dx = float( -nX ) ;
        dy = float( -nY );

        fD = lerp( m_BorderDerivative[m_Width*2], m_BorderDerivative[0],
            SmoothStep( fabs(atan2f( (dy) , (dx))) / PiHalf ));
        
        fR =  sqrtf( dx * dx + dy * dy ) ;
        break;
      case 6:
        fB = m_Terrain[m_Width - 1];

        dx = float( nX - m_Width ) ;
        dy = float( - nY);

        fD = lerp( m_BorderDerivative[m_Width-1], m_BorderDerivative[m_Width*2+m_Height],
            1.0f-SmoothStep( fabs(atan2f( (dy) , (dx))) / PiHalf ));
        
        fR =  sqrtf( dx * dx + dy * dy ) ;   
        break;
      case 7:
        fB = m_Terrain[m_Width * (m_Height-1)];
        dx = float( -nX ) ;
        dy = float( nY - m_Height );

        fD = lerp( m_BorderDerivative[m_Width*2+m_Height-1], m_BorderDerivative[m_Width],
            SmoothStep( fabs(atan2f( (dy) , (dx))) / PiHalf ));
        
        fR =  sqrtf( dx * dx + dy * dy ) ;       
        break;
      case 8:
        fB = m_Terrain[m_Width * m_Height -1];
        dx = float( nX - m_Width ) ;
        dy = float( nY - m_Height);

        fD = lerp( m_BorderDerivative[m_Width*2-1], m_BorderDerivative[m_Width*2+m_Height*2-1],
            1.0f-SmoothStep( fabs(atan2f( (dy) , (dx))) / PiHalf ));
        
        fR = sqrtf( dx * dx + dy * dy ) ;       
        break;
      } // switch (which border)

      // make it so - do the interpolation
      if( fR < 64.0f )
      {
        float fNew = fD * fR + fB;  // dx * r + x
        fAlt = lerp( fNew, fAlt, SmoothStep( fR / 64.0f ) );
      }
    } // if( need to interpolate border)

    // return the altitude
    return fAlt;
  }// if( point within map )
}

/* Get minimal possible height for given rectangle
 *
 * @param	nLeft  min. x-coord (grids)
 * @param	nRight  max  x-coord (grids) 
 * @param	nTop  min. y-coord (grids)
 * @param	nBottom  max  y-coord (grids) 
 * @return  the minimal altitude
 */
float Terrain::GetMinHeight(int nLeft, int nTop, int nRight, int nBottom) const
{
  float safetyReserve = 2.0f;
  #if 0 // _PROFILE || _DEBUG
    // validate the result by sampling 
    const int align = 16;
    int nLeftAligned = (nLeft)&~(align-1);
    int nTopAligned = (nTop)&~(align-1);
    // convert inclusive to exclusive
    int nRightAligned = (nRight+1+align-1)&~(align-1);
    int nBottomAligned = (nBottom+1+align-1)&~(align-1);

    // when testing, we need to test same region in both cases - use aligned for both
    float minY = m_TerSynth.GetMinHeight(nLeftAligned,nTopAligned,nRightAligned,nBottomAligned);
    
    float* mem = (float*)_aligned_malloc((nRightAligned-nLeftAligned)*(nBottomAligned-nTopAligned)*sizeof(float), 16);
    GetPatch(mem,nLeftAligned,nTopAligned,nRightAligned-nLeftAligned,nBottomAligned-nTopAligned);
    float minDY = FLT_MAX;
    for (int z=nTopAligned; z<nBottomAligned; z++) for (int x=nLeftAligned; x<nRightAligned; x++)
    {
      float y = mem[(z-nTopAligned)*(nRightAligned-nLeftAligned)+(x-nLeftAligned)];
      if (minDY>y) minDY = y;
    }
    _aligned_free(mem);
    if (minDY<minY-safetyReserve)
    {
      // failure - the real data should never be below what GetMinHeight has given us
      return minDY;
    }
  #else
    float minY = m_TerSynth.GetMinHeight(nLeft,nTop,nRight,nBottom);
  #endif
  return minY-safetyReserve;
  
}

/* Get minimal possible height for given rectangle
 *
 * @param	nLeft  min. x-coord (grids)
 * @param	nRight  max  x-coord (grids) 
 * @param	nTop  min. y-coord (grids)
 * @param	nBottom  max  y-coord (grids) 
 * @return  the minimal altitude
 */
float Terrain::GetMaxHeight(int nLeft, int nTop, int nRight, int nBottom) const
{
  #if 0 // _PROFILE || _DEBUG
    // validate the result by sampling 
    const int align = 16;
    int nLeftAligned = (nLeft)&~(align-1);
    int nTopAligned = (nTop)&~(align-1);
    // convert inclusive to exclusive
    int nRightAligned = (nRight+1+align-1)&~(align-1);
    int nBottomAligned = (nBottom+1+align-1)&~(align-1);

    // when testing, we need to test same region in both cases - use aligned for both
    float maxY = m_TerSynth.GetMaxHeight(nLeftAligned,nTopAligned,nRightAligned,nBottomAligned);
    
    float* mem = (float*)_aligned_malloc((nRightAligned-nLeftAligned)*(nBottomAligned-nTopAligned)*sizeof(float), 16);
    GetPatch(mem,nLeftAligned,nTopAligned,nRightAligned-nLeftAligned,nBottomAligned-nTopAligned);
    float maxDY = -FLT_MAX;
    for (int z=nTopAligned; z<nBottomAligned; z++) for (int x=nLeftAligned; x<nRightAligned; x++)
    {
      float y = mem[(z-nTopAligned)*(nRightAligned-nLeftAligned)+(x-nLeftAligned)];
      if (maxDY<y) maxDY = y;
    }
    _aligned_free(mem);
    if (minDY<minY-1e-2f)
    {
      // failure - the real data should never be below what GetMinHeight has given us
      return minDY;
    }
  #else
    float maxY = m_TerSynth.GetMaxHeight(nLeft,nTop,nRight,nBottom);
  #endif
  return maxY;
  
}



/* Get a patch (128/nLOD x 128/nLOD) of terrain - not thread-safe
 *
 * @param	pBuffer	    where to put the heightmap (must be aligned to 16-byte boundary)
 * @param	nLeft	      must be divisible by 128
 * @param	nHeight	    must be divisible by 128
 * @param nLOD        level of detail, power of 2, within <1,16>
 * @return  0 if error, 1 if ok
 */
int Terrain::GetPatchAligned(float* pDest, int nLeft, int nTop, int nLOD, TSRECT* pSection) const
{
  // check for pathological cases
  if(!pDest || nLOD < 1)
    return 0;

  if( !m_TerSynth.IsInitialized() )
    return 0;
  
  int i,j;

  // constrain generated terrain to the following rectangle
  int nRow0 = pSection ? pSection->top : 0;
  int nRow1 = pSection ? pSection->height + pSection->top : 128;
  int nCol0 = pSection ? pSection->left : 0;
  int nCol1 = pSection ? pSection->width + pSection->left : 128;
  //nRow0 = 16;
  //nRow1 = 48;
  int nRows = nRow1 - nRow0;
  int nCols = nCol1 - nCol0;

  TS_INVALID tInvalid;
    tInvalid.row0 = nRow0 / nLOD;
    tInvalid.row1 = nRow1 / nLOD;
    tInvalid.col0 = nCol0 / nLOD;
    tInvalid.col1 = nCol1 / nLOD;


  // we need to synthesize the thing
  // !!! assumes that both nLeft and nTop are divisible by 128
  if(!(nLeft >=0 && nLeft+128 <= m_Width && nTop >= 0 && nTop+128 <= m_Height))
  {
    TSRECT rect = { nLeft, nTop, 128, 128 };
    m_TerSynth.GetPatch( pDest, &rect, nLOD, &tInvalid );
  }
  
  // we also need to copy something
  if( (nLeft>=0 && nLeft < m_Width) && (nTop>=0 && nTop < m_Height) )
  {
    int a,b;
    int w = nCols / nLOD;

    a=0;

    for(i=nTop + nRow0; i<  __min(nTop+nRow1, m_Height)  ; i+=nLOD)    
    {
      b=0;
      for(j=nLeft + nCol0; j<  __min(nLeft+nCol1, m_Width) ; j+=nLOD)
      {        
        pDest[a*w + b] = m_Terrain[ i*m_Width+j ];    
        b++;
      }
      a++;
    }
  }


  // now comes the hard part - interpolation

  // we need to interpolate the lower edge
  if( (nTop+128 >= m_Height) && (nTop < m_Height + 128) && 
      (nLeft >= 0) && (nLeft < m_Width) )
  {
    int nStartX = __max(0, nLeft + nCol0);
    int nEndX = __min(m_Width, nLeft + nCol1);

    GetBorder( m_Border, 3, nStartX, nEndX - nStartX );
    InterpolateBorderVert( pDest, m_BorderDerivative + m_Width + nLeft + nCol0 ,m_Border, m_Height-nTop - nRow0, nRows/nLOD, nCols/nLOD, 1 , nLOD, 0, nEndX-nStartX);
   
  }

  // left edge
  if( (nLeft >= -128) && (nLeft < 0) &&
    (nTop >= 0) && (nTop < m_Height) )
  {
    int nStartY = __max(0, nTop + nRow0) ;
    int nEndY = __min(m_Height, nTop + nRow1);

     GetBorder( m_Border, 2, nStartY, nEndY-nStartY );
     InterpolateBorderHorz( pDest, m_BorderDerivative + 2*m_Width + nTop + nRow0, m_Border, 128 - nCol1, (nEndY - nStartY), nCols/nLOD, -1 , nLOD);
      
  }

  // we need to interpolate the right edge
  // same as before, just variables exchanged appropriately
  if( (nLeft+128 > m_Width) && (nLeft < m_Width + 64) &&
      (nTop+64 >= 0) && (nTop < m_Height) )
  {
     int nStartY = __max(0, nTop + nRow0);
     int nEndY = __min(m_Height, nTop + nRow1);

     GetBorder( m_Border, 4, nStartY, nEndY - nStartY );
     InterpolateBorderHorz( pDest, 
          m_BorderDerivative + 2*m_Width + m_Height + nTop + nRow0 , 
          m_Border, 
          m_Width - nLeft - nCol0 , 
          (nEndY - nStartY), 
          (nCols) /nLOD, 
          1 , nLOD);
  }

  // we may also need to interpolate the top edge - make it so!
  if( (nTop +128 >= 0) && (nTop < 0) && (nLeft>=0) && (nLeft <m_Width))
  {
    int nStartX = __max(0, nLeft + nCol0) ;
    int nEndX = __min(m_Width, nLeft + nCol1);

    GetBorder( m_Border, 1, nStartX, nEndX-nStartX );
    InterpolateBorderVert( pDest, m_BorderDerivative + nLeft + nCol0,  m_Border, 128-nRow1, nRows/nLOD, nCols/nLOD, -1 , nLOD, 0, nEndX-nStartX);
  }
  
  //the corners are probably the most sensitive

  // we need to correct the bottom-right corner (8)
  if( IsPatchBottomRight( nLeft, nTop ) )
  {    
    int dX = nLeft + nCol0 - m_Width;
    int dY = nTop + nRow0 - m_Height;

    InterpolateCorner2( pDest, 
          m_Terrain[m_Width * m_Height-1], 
          m_BorderDerivative[m_Width*2+m_Height*2-1],
          m_BorderDerivative[m_Width*2-1],
           -dX, -dY, nCols/nLOD, nRows/nLOD, 8, nLOD );        

       //return 1;
  }

  // we need to correct the bottom-left corner (7)
  if( IsPatchBottomLeft( nLeft, nTop ) )
  {    
    //int dX = 128+nLeft ;
    int dY = m_Height - nTop - nRow0 ;

    InterpolateCorner2( pDest, 
          m_Terrain[m_Width * (m_Height-1)], 
          m_BorderDerivative[m_Width*2+m_Height-1],
          m_BorderDerivative[m_Width],
          128-nCol1, dY, nCols/nLOD, nRows/nLOD, 7, nLOD );
  }

  if(IsPatchTopRight( nLeft, nTop ) )
  {
    int dX = m_Width - nLeft - nCol0;
    int dY = 128-nRow1;

    InterpolateCorner2( pDest, 
          m_Terrain[m_Width-1], 
          m_BorderDerivative[m_Width*2+m_Height], 
          m_BorderDerivative[m_Width-1],
          dX, dY, nCols/nLOD, nRows/nLOD, 6, nLOD );  
  }

  if(IsPatchTopLeft( nLeft, nTop ) )
  {    
    InterpolateCorner2( pDest, m_Terrain[0], m_BorderDerivative[m_Width*2],
          m_BorderDerivative[0], 128-nCol1, 128-nRow1, nCols/nLOD, nRows/nLOD, 5, nLOD );  
  }

  /*
  // consistency test
  for(i=0; i<nSize; i++)
    for(j=0; j<nSize; j++)
      pDest[i*nSize+j] = (pDest[i*nSize+j]-GetPixel(j*nLOD +nLeft, i*nLOD + nTop)) * 100.0f;
     // pDest[i*nSize+j] = GetPixel(j*nLOD +nLeft, i*nLOD + nTop);
  */

  return 1;
}



bool Terrain::IsPatchBottomRight(int nLeft, int nTop) const
{
  if( nTop + 128 < m_Height )
    return false;
  else if( nTop > m_Height + 64 )
    return false;
  else if( nLeft + 128 < m_Width )
    return false;
  else if( nLeft >= m_Width + 64 )
    return false;

  return true;
}

bool Terrain::IsPatchBottomLeft(int nLeft, int nTop) const
{
  if( nTop + 128 < m_Height )
    return false;
  else if( nTop > m_Height + 64 )
    return false;
  else if( nLeft + 128 < 0 )
    return false;
  else if( nLeft >=  0 )
    return false;

  return true;
}

bool Terrain::IsPatchTopRight(int nLeft, int nTop) const
{
  if( nTop + 128 < 0 )
    return false;
  else if( nTop >= 0 )
    return false;
  else if( nLeft + 128 < m_Width )
    return false;
  else if( nLeft >=  m_Width+64 )
    return false;

  return true;
}

bool Terrain::IsPatchTopLeft(int nLeft, int nTop) const
{
  if( nTop + 128 < 0 )
    return false;
  else if( nTop >= 0 )
    return false;
  else if( nLeft + 128 < 0 )
    return false;
  else if( nLeft >= 0)
    return false;

  return true;
}

/* Get a line from the border of original terrain
 *
 * @param	pDest	    the destination buffer
 * @param	nSector	    from which side to get the line (1-top, 2-left, 3-bottom, 4-right)
 * @param	nStart      the starting x or y coord
 * @param nLength     the length of the line
 * @return  0 if error, 1 if ok
 */
int Terrain::GetBorder(float* pDest, int nSector, int nStart, int nLength) const
{
  int i;

  switch(nSector)
  {
  case 1: //top
    for(i=nStart; i<__min(nStart+nLength,m_Width) ; i++)    
      pDest[i-nStart] = m_Terrain[i];    
    break;

  case 3: //bottom
    for(i=nStart; i<__min(nStart+nLength,m_Width) ; i++)    
      pDest[i-nStart] = m_Terrain[m_Width*(m_Height-1) + i];    
    break;

  case 2: //left
    for(i=nStart; i<__min(nStart+nLength,m_Height) ; i++)    
      pDest[i-nStart] = m_Terrain[m_Width * i];    
    break;
  
  case 4: //right
    for(i=nStart; i<__min(nStart+nLength,m_Height) ; i++)    
      pDest[i-nStart] = m_Terrain[m_Width * i + m_Width-1];    
    break;

  default:
    return 0;
    break;
  }

  return 1;
}


/* From given coords, determine the index of sector in which the given point lies (if close enough to the orig. terrain)
 *
 * @param	nX
 * @param	nY 
 * @return  index of the sector, 0 if n/a
 */
int Terrain::GetBorderSector(int nX, int nY) const
{
  if( nX >=0 && nX < m_Width )
  {
    if( nY >= -128 && nY <0 )
      return 1; // top
    else if( nY >= m_Height && nY < m_Height + 128 )
      return 3; // bottom
  }
  else if( nY >=0 && nY < m_Height )
  {
     if( nX >= -128 && nX <0 )
      return 2; // left
    else if( nX >= m_Width && nX < m_Width + 128 )
      return 4; // right
  }

  // check corners
  if( nX>= -128 && nX <0 )
  {
    if( nY >= -128 && nY < 0 )
      return 5; // top left
    else if( nY >= m_Height && nY < m_Height + 128 )
      return 7; //  bottom left
  }
  else if(nX>= m_Width && nX < m_Width +128 )
  {
    if( nY >= -128 && nY < 0 )
      return 6; // top right
    else if( nY >= m_Height && nY < m_Height + 128 )
      return 8; //  bottom right
  }

  return 0;
}


int Terrain::GetPatch(float* pDest, int nLeft, int nTop, int nWidth, int nHeight, int nLOD) const
{
  int nLeft1 = floor(nLeft /128.0f)*128.0f;
  int nTop1 = floor(nTop /128.0f)*128.0f; 
  int nRight1 = floor((nLeft+nWidth)/128.0f)*128.0f;
  int nBottom1 = floor((nTop+nHeight)/128.0f)*128.0f;

  TSRECT tSection;

  if(nLeft1==nRight1 && nTop1==nBottom1)  // whole rect contained within one aligned square
  {
    tSection.left =  nLeft - nLeft1;
    tSection.top =  nTop - nTop1;
    tSection.width = nWidth;
    tSection.height = nHeight;
    GetPatchAligned(pDest, nLeft1, nTop1, nLOD, &tSection);
  }
  else if(nLeft1==nRight1 && nTop1!=nBottom1)
  {
    tSection.left =  nLeft - nLeft1;
    tSection.top =  nTop - nTop1;
    tSection.width = nWidth;
    int y= tSection.height = 128-(nTop - nTop1);
    GetPatchAligned(pDest, nLeft1, nTop1, nLOD, &tSection);
    
    tSection.top =  0;    
    tSection.height = nHeight - y ;
    GetPatchAligned(pDest + (y * nWidth)/nLOD/nLOD, nLeft1, nTop1+128, nLOD, &tSection);
  }
  else if(nLeft1!=nRight1 && nTop1==nBottom1)
  {
    float* pBuffer = (float*)_aligned_malloc(nWidth*nHeight*sizeof(float), 16);
      
    tSection.top =  nTop - nTop1;
    tSection.height = nHeight;    

    tSection.left =  nLeft - nLeft1;
    int x = tSection.width = 128-(nLeft - nLeft1);
    GetPatchAligned(pBuffer, nLeft1, nTop1, nLOD, &tSection);
    
    int i,j;
    for(i=0; i<nHeight/nLOD; i++)
      for(j=0; j<x/nLOD; j++)
        pDest[i*nWidth/nLOD+j] = pBuffer[i*x/nLOD+j];

  
    tSection.left =  0;
    tSection.width = nWidth-x;
    GetPatchAligned(pBuffer, nLeft1+128, nTop1, nLOD, &tSection);
    
   for(i=0; i<nHeight/nLOD; i++)
      for(j=0; j<(nWidth-x)/nLOD; j++)
        pDest[i*nWidth/nLOD+j+x/nLOD] = pBuffer[i*tSection.width/nLOD +j];

    _aligned_free(pBuffer);
  }
  else  // the absolutely worst case - we need 4 blocks to put this monstrosity together
  {
    float* pBuffer = (float*)_aligned_malloc(nWidth*nHeight*sizeof(float), 16);
      
    tSection.top =  nTop - nTop1;
    int y= tSection.height = 128-(nTop - nTop1);
    tSection.left =  nLeft - nLeft1;
    int x = tSection.width = 128-(nLeft - nLeft1);
    GetPatchAligned(pBuffer, nLeft1, nTop1, nLOD, &tSection);
    
    tSection.top =  0;
    tSection.height = nHeight - y;
    GetPatchAligned(pBuffer+(y*x)/nLOD/nLOD, nLeft1, nTop1+128, nLOD, &tSection);

    int i,j;
    for(i=0; i<nHeight/nLOD; i++)
      for(j=0; j<x/nLOD; j++)
        pDest[i*nWidth/nLOD+j] = pBuffer[i*x/nLOD+j];

  
    tSection.top =  nTop - nTop1;
    tSection.height = y;
    tSection.left =  0;
    tSection.width = nWidth-x;
    GetPatchAligned(pBuffer, nLeft1+128, nTop1, nLOD, &tSection);
    
    tSection.top =  0;
    tSection.height = nHeight - y;
    GetPatchAligned(pBuffer+(y*tSection.width )/nLOD/nLOD, nLeft1+128, nTop1+128, nLOD, &tSection);

   for(i=0; i<nHeight/nLOD; i++)
      for(j=0; j<(nWidth-x)/nLOD; j++)
        pDest[i*nWidth/nLOD+j+x/nLOD] = pBuffer[i*tSection.width/nLOD +j];

    _aligned_free(pBuffer);
  }
  
  return 1;
}

int Terrain::GetPatch(float* pDest, TSRECT* pRect, int nLOD) const
{
  return GetPatch(pDest, pRect->left, pRect->top, pRect->width, pRect->height, nLOD);
}
