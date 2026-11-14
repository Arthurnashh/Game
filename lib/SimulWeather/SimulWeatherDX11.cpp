#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulWeatherDX11.hpp"

SimulWeatherDX11::SimulWeatherDX11(Engine *gameEngine,
  Dx11DeviceAccess *deviceAccess)
  : SimulWeatherBase(gameEngine), _engine(deviceAccess)
{
  DoAssert(_engine);
}

SimulWeatherDX11::~SimulWeatherDX11()
{
}

void SimulWeatherDX11::CreateRenderers()
{
  DoAssert(_engine);
  if (!_engine) return;

  _skyRendererDX11 = new SimulSkyRendererDX11(_engine);
  _skyRenderer = _skyRendererDX11;
  _cloudRendererDX11 = new SimulCloudRendererDX11(_engine);
  _cloudRenderer = _cloudRendererDX11;
}

void SimulWeatherDX11::UpdateSkyAndCloudHookup()
{
  if (!_skyRendererDX11 || !_cloudRendererDX11) return;
  Ref<TextureSysGPUDX11> loss1, loss2, inscatter1, inscatter2;
  _skyRendererDX11->GetLossAndInscatterTextures(
    loss1, loss2, inscatter1, inscatter2);
  _cloudRendererDX11->SetLossTextures(loss1, loss2);
  _cloudRendererDX11->SetInscatterTextures(inscatter1, inscatter2);
  _cloudRendererDX11->SetAltitudeTextureCoordinate(
    _skyRendererDX11->GetAltitudeTextureCoordinate());
  _cloudRendererDX11->SetFadeInterpolation(_skyRendererDX11->GetFadeInterp());
}

#endif
