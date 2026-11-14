#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulSkyRendererBase.hpp"
#include "../scene.hpp"
#include "../global.hpp"
#include "../world.hpp"
#include "../landscape.hpp"

SimulSkyRendererBase::SimulSkyRendererBase(bool renderSky, bool useFadeTextures, bool useColourSky)
:simul::sky::BaseSkyRenderer()
,_renderSky(renderSky), _useFadeTextures(useFadeTextures), _useColourSky(useColourSky), _useAutoBrightness(false)
,_initialized(false)
{
  EnableColourSky(useColourSky);

  DoAssert(fadeTableInterface);
  if (fadeTableInterface)
    fadeTableInterface->SetEarthTest(false);

  DoAssert(skyKeyframer);
  if (skyKeyframer)
  {
    skyKeyframer->SetUseKeyframerDate(false);
  }

  EnableMoon(true);
  SetAutoBrightness(true, 0.005);
}

SimulSkyRendererBase::~SimulSkyRendererBase()
{
}

void SimulSkyRendererBase::SetAutoBrightness(bool enable, float strength)
{
  if (skyKeyframer)
  {
    skyKeyframer->SetAutoBrightnessStrength(strength);
    skyKeyframer->SetAutoBrightness(enable);

    if (!enable)
    {
      for(int i=0;i<skyKeyframer->GetNumKeyframes();i++)
      {
        simul::sky::SkyKeyframer::SkyKeyframe *k=(simul::sky::SkyKeyframer::SkyKeyframe *)skyKeyframer->GetKeyframe(i);
        k->BrightnessAdjustment = 1.0f;
      }
    }
  }

  _useAutoBrightness = enable;

}

void SimulSkyRendererBase::SetSkyTextureSize(unsigned size)
{
  if (!GetInitialized())
    return; //not properly initialized yet
  if (skyTexSize == size)
    return;

  skyTexSize=size;
  if (_renderSky)
    CreateSkyTextures();
}

void SimulSkyRendererBase::SetFadeTextureSize(unsigned width,unsigned height,unsigned num_alts)
{
  if (!GetInitialized())
    return; //not properly initialized yet
  if(fadeTexWidth==width && fadeTexHeight==height && numAltitudes == num_alts)
    return;

  fadeTexWidth=width;
  fadeTexHeight=height;
  numAltitudes=num_alts;
  if (_useFadeTextures)
    CreateFadeTextures();
  if (_renderSky)
    CreateSunlightTextures();
}

void SimulSkyRendererBase::FillFadeTextureBlocks(int,int,int,int,int,int,int,
                                                 const float *,
                                                 const float *)
{
  // in Arma (=DirectX engine) we use sequential filling,so we don't need to implement this
}

bool SimulSkyRendererBase::Init()
{
  _initialized = InitRenderer();

  if (_initialized)
  {
    DoAssert(fadeTableInterface);
    if (fadeTableInterface)
    {
      fadeTableInterface->SetCallback(NULL);
      fadeTableInterface->SetCallback(this);
    }
  }

  return _initialized;
}

void SimulSkyRendererBase::DeInit()
{
  DeInitRenderer();
  fadeTexWidth=fadeTexHeight=numAltitudes=0;
  if (fadeTableInterface)
    fadeTableInterface->SetCallback(NULL);
  _initialized = false;
}

bool autoDir = false;

void SimulSkyRendererBase::Simulate(float deltaT)
{
  if (GScene && GScene->GetCamera())
  {
    // update camera position
    SetCameraPosition(GScene->GetCamera()->Position()[0], GScene->GetCamera()->Position()[1], GScene->GetCamera()->Position()[2]);
  }

  if (skyKeyframer)
  {
    if (GScene && GScene->MainLight())
    {
      skyKeyframer->SetSunOrMoon(true, GScene->MainLight()->SunOrMoon());
    }
    else
    {
      skyKeyframer->SetSunOrMoon(false, 1);
    }

    SynchronizeTimeAndPlace();

		if (autoDir)
			skyKeyframer->TimeStep(deltaT);

    skyKeyframer->Update();
  }


}


void SimulSkyRendererBase::SynchronizeTimeAndPlace()
{
  if (!skyKeyframer)
    return;

  // synchronize ingame time daytime with simul library

  simul::sky::SiderealSkyInterface *siderealSkyInterface = GetSiderealSkyInterface();
  if (siderealSkyInterface)
  {
    if (GWorld && skyKeyframer->GetSkyInterface())
    {
			if (autoDir)
			{
				siderealSkyInterface->SetAutomaticMoonPosition(false);
				siderealSkyInterface->SetAutomaticSunPosition(false);

				const Vector3& lightDir = GScene->MainLight()->SunDirection();
				simul::sky::float4 skyLightDir;
				static Vector3 A( 1,-1, 1);

				skyLightDir.x = A.x*lightDir.z;
				skyLightDir.y = A.y*lightDir.y;
				skyLightDir.z = A.z*lightDir.x;
				skyLightDir.w = 1.0f;

				skyKeyframer->GetSkyInterface()->SetDirectionToSun(skyLightDir);
				skyKeyframer->SetAutomaticMoonPosition(false);
				skyKeyframer->SetAutomaticSunPosition(false);

				const Vector3& moonDir = GScene->MainLight()->SunDirection();
				skyLightDir.x = -moonDir.x;
				skyLightDir.y = -moonDir.y;
				skyLightDir.z = -moonDir.z;
			
				skyKeyframer->GetSkyInterface()->SetDirectionToMoon(skyLightDir);
			}
			else
			{
				siderealSkyInterface->SetAutomaticMoonPosition(true);
				siderealSkyInterface->SetAutomaticSunPosition(true);

				// setup world data - year, day, longitude, latitude
				int year, month, day, hour, minute;
				GWorld->GetDate(year, month, day, hour, minute);
				siderealSkyInterface->SetDate(month, day);
				siderealSkyInterface->SetYear(year);

				// in SW latitude is positive north
				siderealSkyInterface->SetLatitudeRadians(-GWorld->GetLatitude());

				// set both timezone and longitude to 0, so that there is no shift in solar time
				static float timeZone = 0.0f;
				siderealSkyInterface->SetTimezone(timeZone);
				siderealSkyInterface->SetLongitudeRadians(0);//GWorld->GetLongitude());
			}
		}
  }

  float dayTime = Glob.clockWeather.GetTimeOfDay();
  if (dayTime > 1.0f)
    dayTime -= 1.0f;

  skyKeyframer->SetUseKeyframerDate(false);
  skyKeyframer->SetDaytime(dayTime);
}

float SimulSkyRendererBase::GetFadeInterp() const
{
  return skyKeyframer ? skyKeyframer->GetInterpolation() : 0.0f;
}

void SimulSkyRendererBase::InitFromCfg(ParamEntryPar entry)
{
  DoAssert(fadeTableInterface);
  if (!fadeTableInterface)
    return;

  DoAssert(skyKeyframer);
  if (!skyKeyframer)
    return;

  DoAssert(skyInterface);
  if (!skyInterface)
    return;

  skyKeyframer->SetMaxDistanceKm(entry >> "fadeMaxDistanceKm");
  skyKeyframer->SetMaxAltitudeKm(entry >> "fadeMaxAltitudeKm");
  int numElev = entry >> "fadeNumElevations";
  skyKeyframer->SetTableHeight(numElev);
  int numDists = entry >> "fadeNumDistances";
  skyKeyframer->SetTableWidth(numDists);
  int numAlts = entry >> "fadeNumAltitudes";
  skyKeyframer->SetNumAltitudes(numAlts);
  skyKeyframer->SetEarthTest(entry >> "fadeEarthTest");

  skyKeyframer->SetAutoBrightnessStrength(entry >> "autoBrightnessStrength");

  _useAutoBrightness = entry >> "autoBrightness";
  skyKeyframer->SetAutoBrightness(_useAutoBrightness);

  skyInterface->SetMoonIrradianceCoef(entry >> "moonIrradianceCoef");
}

void SimulSkyRendererBase::InitKeyframes(int numKeyframes, simul::sky::SkyKeyframer::SkyKeyframe& keyframe)
{
  DoAssert(skyKeyframer);
  if (!skyKeyframer)
    return;
  Assert(numKeyframes > 0);
  if (numKeyframes <= 0)
    return;

  skyKeyframer->SetUniformKeyframes(numKeyframes);
  for(int i=0;i<skyKeyframer->GetNumKeyframes();i++)
  {
    simul::sky::SkyKeyframer::SkyKeyframe *k=(simul::sky::SkyKeyframer::SkyKeyframe *)skyKeyframer->GetKeyframe(i);
    Assert(k);
    if (!k)
      continue;

    ApplyKeyframe(*k, keyframe, 0.0f); //overcast is really not important here, so 0.0f is ok
  }
}

void SimulSkyRendererBase::ApplyKeyframe(simul::sky::SkyKeyframer::SkyKeyframe& dest, const simul::sky::SkyKeyframer::SkyKeyframe& src, float overcast)
{
  dest.Rayleigh = src.Rayleigh;				//< The Rayleigh scattering coefficients (x=red,y=green,z=blue).
  dest.Mie = src.Mie;						//< The Mie scattering coefficients (x=red,y=green,z=blue).
  dest.Haze = src.Haze;						//< The amount of haze, mist or fog.
  dest.HazeBaseKm = src.HazeBaseKm;				//< The base altitude, above which haze starts to decrease in density.
  dest.HazeScaleKm = src.HazeScaleKm;				//< The vertical scale over which haze reduces with altitude.
  dest.HazeEccentricity = src.HazeEccentricity;			//< The anisotropy of Mie scattering.
  dest.SunAzimuth = src.SunAzimuth;				//< Horizontal angle to the sun. This will be overridden if sun position is automatic.
  dest.SunElevation = src.SunElevation;				//< Angle of the sun above the horizon. This will be overridden if sun position is automatic.
  dest.MoonElevation = src.MoonElevation;			//< Angle of the moon above the horizon. This will be overridden if moon position is automatic.
  dest.MoonAzimuth = src.MoonAzimuth;				//< Horizontal angle to the moon. This will be overridden if moon position is automatic.
  dest.BrightnessAdjustment = src.BrightnessAdjustment;		//< A multiplier for brightness of the scene at this time.
  dest.UseMoon = src.UseMoon;					//< Whether the moon, and not the sun, should be used for light direction.   

  bool calcAutoBrightness = true;
#if _ARMA3_NEW_LIGHTING_CFG
  if (GLandscape && GScene && GLandscape->UseNewLandLighting())
  {
    if (skyKeyframer && skyKeyframer->GetSkyInterface())
    {
			// sun cosinus
			float cosSun;

			if (autoDir)
      {
				cosSun = -GScene->MainLight()->SunDirection().Y();
			}
			else
			{
				// we have to shift the time and compute needed values
				float oldTime = skyKeyframer->GetSkyInterface()->GetTime();
				skyKeyframer->GetSkyInterface()->SetTime(dest.daytime);

				// get sun direction at given time
				simul::sky::float4 dirSun = skyKeyframer->GetSkyInterface()->GetDirectionToSun();
			  Vector3 direction = Vector3(-dirSun.x, -dirSun.z, -dirSun.y);

				// restore time
				skyKeyframer->GetSkyInterface()->SetTime(oldTime);
				cosSun = -direction.Y();
			}

      // height
      // FIX - always use height slightly above ground, because we want the values to change only with time, not height (values under the sea may be wrongly configured)
      const float height = 1.0f;
      //if (GScene && GLandscape && GScene->GetCamera())
      //  height = GScene->GetCamera()->Position()[1] - GLandscape->GetSeaLevelNoWave();

      // compute lighting
      LandLighting::LightingResult result;
      if (GLandscape)
      {
        GLandscape->GetLightingNew(result, height, overcast, cosSun, 0.0f);

        // override some values by values from lighting config
        Color col = result._mie;
        if (col.SquareSize() > 0)    
          dest.Mie.Set(col.R(), col.G(), col.B(), col.A());

        col = result._rayleigh;
        if (col.SquareSize() > 0)    
          dest.Rayleigh.Set(col.R(), col.G(), col.B(), col.A());

        dest.BrightnessAdjustment = result._swBrightness;
        calcAutoBrightness = false;
      }

    }

  }
#endif //_ARMA3_NEW_LIGHTING_CFG

  if (skyKeyframer)
  {
    if (calcAutoBrightness)
    {
      if (_useAutoBrightness)
      {
        skyKeyframer->SetAutoBrightness(true);
        skyKeyframer->CalcAutoBrightnessForKeyframe(&dest);
      }
    }
    else
    {
      skyKeyframer->SetAutoBrightness(false);
    }
  }
}

#endif //_ARMA3_SIMUL_WEATHER
