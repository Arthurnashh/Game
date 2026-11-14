#ifndef _ARMA3_SIMULWEATHERINCLUDES_HPP
#define _ARMA3_SIMULWEATHERINCLUDES_HPP

#if _ARMA3_SIMUL_WEATHER

// Simul weather uses STL - we have to use normal new 
#include <Es/Memory/normalNew.hpp>

#ifdef _MSC_VER
// disable exception warnings (stl...)
#pragma warning(push)
#pragma warning(disable:4530)
#endif

#include "Simul/Clouds/BaseCloudRenderer.h"
#include "Simul/Clouds/TextureGenerator.h"
#include "Simul/Sky/BaseSky.h"
#include "Simul/Sky/BaseSkyRenderer.h"
#include "Simul/Sky/TextureGenerator.h"
#include "Simul/Sky/SkyNode.h"
#include "Simul/Base/Timer.h"
#include "Simul/Clouds/CloudKeyframer.h"

#include <strstream>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define SIMUL_WEATHER_LOG_TEXTURE_FILLS 0

// return to debug new if needed
#include <Es/Memory/debugNew.hpp>

#endif //_ARMA3_SIMUL_WEATHER
#endif //_ARMA3_SIMULWEATHERINCLUDES_HPP