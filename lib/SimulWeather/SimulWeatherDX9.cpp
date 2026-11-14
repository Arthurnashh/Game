#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulWeatherDX9.hpp"
#include "SimulSkyRendererDX9.hpp"
#include "SimulCloudRendererDX9.hpp"
#include "../d3d9/engdd9.hpp"

SimulWeatherDX9::SimulWeatherDX9(Engine* engine)
:SimulWeatherBase(engine)
{
  _engine = dynamic_cast<EngineDD9 *>(engine);
  DoAssert(_engine);
  if (!engine)
  {
    Fail("Invalid engine passed to SimulWeatherDX9!");
  }
}

SimulWeatherDX9::SimulWeatherDX9(EngineDD9* engine)
:SimulWeatherBase(engine), _engine(engine)
{  
}

SimulWeatherDX9::~SimulWeatherDX9()
{

}

void SimulWeatherDX9::CreateRenderers()
{
  DoAssert(_engine);

  _skyRendererDX9 = new SimulSkyRendererDX9(_engine);
  _skyRenderer = _skyRendererDX9;
  _cloudRendererDX9 = new SimulCloudRendererDX9(_engine);
  _cloudRenderer = _cloudRendererDX9;
}

void SimulWeatherDX9::UpdateSkyAndCloudHookup()
{
  if (_skyRendererDX9 && _cloudRendererDX9)
  {
    Ref<TextureSysGPUDX9> l1,l2,i1,i2;
    {
      PROFILE_SCOPE_EX(getTx,*);
      _skyRendererDX9->GetLossAndInscatterTextures(l1,l2,i1,i2);
    }
    {
      PROFILE_SCOPE_EX(setLT,*);
      _cloudRendererDX9->SetLossTextures(l1,l2);
    }
    {
      PROFILE_SCOPE_EX(setIT,*);
      _cloudRendererDX9->SetInscatterTextures(i1,i2);
    }
    {
      PROFILE_SCOPE_EX(setAT,*);
      _cloudRendererDX9->SetAltitudeTextureCoordinate(_skyRendererDX9->GetAltitudeTextureCoordinate());
    }
    {
      PROFILE_SCOPE_EX(setFI,*);
      _cloudRendererDX9->SetFadeInterpolation(_skyRendererDX9->GetFadeInterp());
    }
  }
}

#endif //_ARMA3_SIMUL_WEATHER
