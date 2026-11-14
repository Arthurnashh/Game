// The class that combines original and synthesized terrain
// by Jan Dupej 2009

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TERRAIN_H
#define _TERRAIN_H

#include "tersynth.h"

class Terrain
{
public:
  Terrain();
  ~Terrain();

private:
  int m_Width, m_Height;
  float*  m_Terrain;
  float* m_Border;

  float* m_BorderDerivative;

  TerrainSynth  m_TerSynth;
  
  int GetBorderSector(int nX, int nY) const;
  int GetBorder(float* pDest, int nSector, int nStart, int nLength) const;
  
  int CreateBorderInfo(float* pAlt, int nWidth, int nHeight);
  int DestroyBorderInfo(void);
  
  bool IsPatchBottomRight(int nLeft, int nTop) const;
  bool IsPatchBottomLeft(int nLeft, int nTop) const;
  bool IsPatchTopRight(int nLeft, int nTop) const;
  bool IsPatchTopLeft(int nLeft, int nTop) const;

public:
  int SetTerrain(float* pTerrain, int nWidth, int nHeight, float terrainGrid);

  float GetPixel(int nX, int nY) const;
  int GetPatchAligned(float* pDest, int nLeft, int nTop, int nLOD = 1, TSRECT* pSection = NULL) const;
  
  int GetPatch(float* pDest, int nLeft, int nTop, int nWidth, int nHeight, int nLOD = 1) const;
  int GetPatch(float* pDest, TSRECT* pRect, int nLOD = 1) const;
  
  float GetMinHeight(int nLeft, int nTop, int nRight, int nBottom) const;
  float GetMaxHeight(int nLeft, int nTop, int nRight, int nBottom) const;
};

#endif
