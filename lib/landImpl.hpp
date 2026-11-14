#ifdef _MSC_VER
#pragma once
#endif

#ifndef _LAND_IMPL_HPP
#define _LAND_IMPL_HPP

/**
@file Stuff needed in multiple landXxxx.cpp files, but not outside of them.
*/

#include "landscape.hpp"

// describe structure of rvmat files used for terrain (PSTerrain...)

/// texture stage with satellite texture
static const int LandMatStageSat = 0;
/// texture stage with layer mask
static const int LandMatStageMask = 1;
/// texture stage with middle detail texture
static const int LandMatStageMidDet = 2;


/// first stage used by layers
static const int LandMatStageLayers = 3;
/// number of stages in one layer
static const int LandMatStagePerLayer = 2;
/// relative stage used for normal map texture in a layer
static const int LandMatNoInStage = 0;
/// relative stage used for color detail texture in a layer
static const int LandMatDtInStage = 1;
/// number of layers used in total
static const int LandMatLayerCount = 6;
/// satellite normal map stage
static const int LandMatStageSatNorm = 14;

static inline int GetLayerMask(const TexMaterial *mat)
{
  Assert(mat->GetPixelShaderID()==PSTerrainX || mat->GetPixelShaderID()==PSTerrainSNX);

  if (mat->GetPixelShaderID()==PSTerrainSNX)
    return (1<<(LandMatLayerCount-1))-1; //last layer used for satellite normal map
  else
  return (1<<LandMatLayerCount)-1;

  //return mat->GetPixelShaderID()-PSTerrain1+1;
}

/// given stage order, return pass ID (0..3, identifying ARGB)
int GetPassFromStageOrder(int passMask, int stageOrder, bool useSatNormal);

#endif

