#pragma once
#ifndef DAYZ_SIMUL_WEATHER_DX11_HPP
#define DAYZ_SIMUL_WEATHER_DX11_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulWeatherBase.hpp"
#include "SimulSkyRendererDX11.hpp"
#include "SimulCloudRendererDX11.hpp"

class SimulWeatherDX11 : public SimulWeatherBase
{
public:
  SimulWeatherDX11(Engine *gameEngine, Dx11DeviceAccess *deviceAccess);
  virtual ~SimulWeatherDX11();

private:
  virtual void CreateRenderers();
  virtual void UpdateSkyAndCloudHookup();

  Dx11DeviceAccess *_engine;
  simul::base::SmartPtr<SimulSkyRendererDX11> _skyRendererDX11;
  simul::base::SmartPtr<SimulCloudRendererDX11> _cloudRendererDX11;
};

#endif
#endif
