#ifndef _EL_COREL_ANALYS_HPP
#define _EL_COREL_ANALYS_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <Es/essencepch.hpp>


/// Perlin noise functor
/***
From http://mrl.nyu.edu/~perlin/noise/
JAVA REFERENCE IMPLEMENTATION OF IMPROVED NOISE - COPYRIGHT 2002 KEN PERLIN.

Function periodic in each of x,y,z with period 256

return value is between -1 .. +1
*/

struct PerlinNoise
{
  int p[512];
  static const int permutation[];

  PerlinNoise();
  float operator () (float x, float y, float z=0) const;

  static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
  static float lerp(float t, float a, float b) { return a + t * (b - a); }
  static float grad(int hash, float x, float y, float z)
  {
    int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
    float u = h<8 ? x : y;                 // INTO 12 GRADIENT DIRECTIONS.
    float v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
  }
};

extern PerlinNoise GPerlinNoise;

#endif
