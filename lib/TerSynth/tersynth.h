// Terrain Synthesizer Class
// by Jan Dupej 2009

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TERSYNTH_H
#define _TERSYNTH_H

#include "tersynth_helpers.h"

// how many rays will be shot from the center of the map
static const int AnalysisDensity = 512;

// the length of the "last mile" to be analyzed (in meters)
static const float AnalysisLength = 1200.0f;

// feedback coefficient for the iir filter for first derivative calculation
static const float AnalysisFeedbackCoeff = 5.2f;

// reciprocal weight for the iir filter
static const float AnalysisIIRWeight = 1.0f / (AnalysisFeedbackCoeff + 1.0f);

// dx for derivative calculation
static const int AnalysisDD = 20;


struct TSRECT
{
  int left, top, width, height;
};

class TerrainSynth
{
public:
  TerrainSynth();
  ~TerrainSynth();

private:
  bool m_Initialized;
  float m_Altitude[AnalysisDensity];
  float m_MaxAltitude[AnalysisDensity];
  
  TSRECT m_TerrainRect;

  float* m_PerlinBuff;
  

  

  float GetMinAlt(float* pAlt, int nX0, int nY0, int nX1, int nY1, float* pMax) const; 
  int XYToIndex(int nX, int nY) const {return nY * m_TerrainRect.width + nX;};
  int GetAngleIndex(int nX, int nY) const;
  
  int InterpolateBorderPatch(float* pBuffer, int nX, int nY, int nDirection);

public:  
  int AnalyzeByArray(float* pTerrain, TSRECT* pTerrainRect, float terrainGrid);
  float GetPoint(int nX, int nY) const;
  int GetPatch(float* pDest, TSRECT* pRect, int nLOD = 1, TS_INVALID* pInvalid = NULL) const;
  float GetMinHeight(int nLeft, int nTop, int nRight, int nBottom) const;
  float GetMaxHeight(int nLeft, int nTop, int nRight, int nBottom) const;

  void GetMinMaxAltitudes(float &minH, float &maxH, int nLeft, int nTop, int nRight, int nBottom) const;

  bool IsInitialized(void) const {return m_Initialized;};

};

#endif
