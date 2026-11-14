#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulCloudRendererBase.hpp"
#include "../scene.hpp"
#include "../world.hpp"
#include "../landscape.hpp"

// temporary licence key
#define SIMUL_LICENSE_KEY "E2CBCE681D3B8719"

// This is an example of a noise filter:
class CircleFilter:public simul::math::NoiseFilter
{
public:
  float Filter(float val) const
  {
    static float range=0.134f;
    val=1.f-val;
    val=sqrt(1.f-val*val);
    val-=(1.f-range);
    val*=0.5f/range;
    val+=0.5f;
    if(val<0)
      val=0;
    if(val>1.f)
      val=1.f;
    return val;
  }
};
CircleFilter circle_f;

const float SimulCloudRendererBase::DefaultMaxFadeDistanceMetres = 300000.f;

SimulCloudRendererBase::SimulCloudRendererBase()
:simul::clouds::BaseCloudRenderer(false)  // we are NOT using thundercloud by default (it generates illumination texture, which is quite costly and we don't really need it right now)
, _initialized(false)
, _illuminationTexWidth(0)
, _illuminationTexDepth(0)
, _illuminationTexHeight(0)
, _cloudLighting(1, 1, 1, 1)
, _windSpeedCoef(1.0f)
,_directLightCoef(0.2f)
,_indirectLightCoef(0.015f)
,_maxFadeDistanceMetres(DefaultMaxFadeDistanceMetres)
,_fogStart(DefaultMaxFadeDistanceMetres)
,_fogEnd(DefaultMaxFadeDistanceMetres)

#if _ARMA3_SIMUL_WEATHER_ASYNC_RENDER
, _renderMode(RMVertexBufferAsync)
#else
, _renderMode(RMVertexBuffer)
#endif
{
  cloudNode->SetLicense(SIMUL_LICENSE_KEY);
  cloudInterface->Generate();

  AddChild(cloudKeyframer.get());
  cloudKeyframer->SetUse16Bit(false);

  /// in Arma we use y as vertical axis
  helper->SetYVertical(true);

  // A noise filter improves the shape of the clouds:
  cloudNode->GetNoiseInterface()->SetFilter(&circle_f);

  cloudKeyframer->SetFillTexturesAsBlocks(false);

  cloudInterface->SetCloudBaseZ(5000.f);
  cloudInterface->SetCloudWidth(40000.f);
  cloudInterface->SetCloudLength(40000.f);
  cloudInterface->SetCloudHeight(5000.f);

  for(int i=0;i<cloudKeyframer->GetNumKeyframes();i++)
  {
    simul::clouds::CloudKeyframer::Keyframe *k=(simul::clouds::CloudKeyframer::Keyframe *)cloudKeyframer->GetKeyframe(i);
    k->cloud_base_km = 4.0f;
    k->cloud_height_km = 5.0f;
    k->cloudiness = 0.4f;
    k->wind_speed = 100;
    k->wind_direction = 0.5f; //??
  }

  // LUKE TEST
  cloudInterface->SetGridLength(64);
  cloudInterface->SetGridWidth(64);
  cloudInterface->SetGridHeight(16);

  cloudInterface->SetAlphaSharpness(0.0f);
  cloudInterface->SetFractalAmplitude(1.4f);
  cloudInterface->SetFractalWavelength(120.f);
  // How many days for the time-based noise signal to repeat:
  cloudInterface->SetNoisePeriod(1.f);

  cloudInterface->SetOpticalDensity(.1f);
  cloudInterface->SetHumidity(0);

  cloudInterface->SetLightResponse(1.f);
  cloudInterface->SetSecondaryLightResponse(1.f);
  cloudInterface->SetAmbientLightResponse(1.f);

  cloudInterface->SetNoiseResolution(12);
  cloudInterface->SetNoiseOctaves(3);
  cloudInterface->SetNoisePersistence(.55f);

  helper->SetGrid(24,15);
  helper->SetCurvedEarth(true);
  helper->SetEffectiveEarthRadius(0.0f);
  helper->SetAdjustCurvature(true);

  int numLayers = 64;
  if (GEngine)
    numLayers = GEngine->GetSWGeometryLayersCount();

  helper->Initialize(numLayers,100000);

  // Try to use Threading Building Blocks?
  cloudInterface->SetUseTbb(true);

  //helper->Initialize(128,20000);

  // override wind values from keyframes - we will take wind values from Arma
  cloudKeyframer->SetOverrideWind(true);
  // we don't want keyframer to update wind, so set wind values to 0
  cloudKeyframer->SetWindSpeed(0);
}

SimulCloudRendererBase::~SimulCloudRendererBase()
{

}

void SimulCloudRendererBase::SetHelperGrid(unsigned el,unsigned az)
{
  DoAssert(helper);
  if (helper)
  {
    helper->SetGrid(el, az);
  }
}

//! returns max distance the helper is using for creating geometry (in meters)
float SimulCloudRendererBase::GetHelperMaxDistance()
{
  if (helper)
    return helper->GetMaxCloudDistance();
  else
    return 0.0f;
}



void SimulCloudRendererBase::SetHelperDetail(int numLayers, int maxDistance)
{
  DoAssert(helper);
  if (helper)
  {
    helper->Initialize(numLayers, maxDistance);
  }
}

void SimulCloudRendererBase::SetHelperNearCloudFade(float fade)
{
  DoAssert(helper);
  if (helper)
  {
    helper->SetNearCloudFade(fade);
  }
}
void SimulCloudRendererBase::SetCloudTextureSize(unsigned width_x,unsigned length_y,unsigned depth_z)
{
  if(!width_x||!length_y||!depth_z)
    return;
  if(width_x==cloud_tex_width_x&&length_y==cloud_tex_length_y&&depth_z==cloud_tex_depth_z)
    return;

  cloud_tex_width_x = width_x;
  cloud_tex_length_y = length_y;
  cloud_tex_depth_z = depth_z;

  CreateCloudTexture();
}

void SimulCloudRendererBase::FillCloudTextureBlock(int texture_index,int x,int y,int z,int w,int l,int d,const unsigned *uint32_array)
{
  // in Arma (=DirectX engine) we use sequential filling,so we don't need to implement this
}

void SimulCloudRendererBase::SetIlluminationGridSize(unsigned width_x,unsigned length_y,unsigned depth_z)
{
  if(!width_x||!length_y||!depth_z)
    return;
  if(width_x==_illuminationTexWidth&&length_y==_illuminationTexHeight&&depth_z==_illuminationTexDepth)
    return;

  _illuminationTexWidth = width_x;
  _illuminationTexHeight = length_y;
  _illuminationTexDepth = depth_z; 

  CreateIlluminationTexture();
}

void SimulCloudRendererBase::FillIlluminationBlock(int source_index,int x,int y,int z,int w,int l,int d,const unsigned char *uchar8_array)
{
  // in Arma (=DirectX engine) we use sequential filling,so we don't need to implement this
}

bool SimulCloudRendererBase::Init()
{
  _initialized = InitRenderer();

  if (_initialized)
  {
    // NOW can set the rendercallback, as we have a device to implement the callback fns with:
    DoAssert(cloudKeyframer);
    if (cloudKeyframer)
    {
      cloudKeyframer->SetRenderCallback(this);
      cloudKeyframer->InitKeyframesFromClouds();
    }
  }

  return _initialized;
}

void SimulCloudRendererBase::DeInit()
{
  DeInitRenderer();
  _illuminationTexWidth = _illuminationTexHeight = _illuminationTexDepth = 0;
  cloud_tex_width_x = cloud_tex_length_y = cloud_tex_depth_z = 0;
  //if (cloudKeyframer)
  //  cloudKeyframer->SetRenderCallback(NULL);
  _initialized = false;
}


void SimulCloudRendererBase::Simulate(float deltaT)
{
  UpdateCameraPos();

  // update wind
	float timeMul = 1.0f;
  if (GLandscape && cloudInterface)
  {
    Vector3 wind = GLandscape->GetWind();
    simul::math::Vector3 simulWind = cloudInterface->GetWindOffset();

		timeMul = 1.0f/floatMax(0.001f, Glob.clockWeather.GetTimeMultiplier());
    float deltaTSeconds = timeMul*deltaT * 60 * 60 * 24;

    simulWind.x += wind[0] * deltaTSeconds * _windSpeedCoef;
    simulWind.y += wind[2] * deltaTSeconds * _windSpeedCoef;  //we are using wind[2] instead of wind[1] (because SW has z vertical)

    cloudInterface->SetWindOffset(simulWind);

		if (helper)
		{
			//speed of wind change
			helper->SetChurn(_baseChurn*timeMul);
		}
	}

  if(cloudKeyframer)
  {
    cloudKeyframer->Update(cloudInterface->GetTime()*timeMul);
  }

  if(skyInterface)
  {
    simul::sky::float4 sun_dir=skyInterface->GetDirectionToLight();

    simul::sky::float4 fromPoint(cam_pos.x, cam_pos.z, cam_pos.y, 0 );
    
    //	calculate sun occlusion for any external classes that need it:
    sun_occlusion=1.f-cloudKeyframer->GetVisibility(fromPoint,sun_dir);
  }
  simul::graph::meta::TimeStepVisitor tsv;
  tsv.SetTimeStep(timeMul*deltaT);
  cloudNode->Accept(tsv);
}

void SimulCloudRendererBase::SetSkyInterface(simul::sky::BaseSkyInterface *si)
{
  skyInterface=si;
  if (cloudKeyframer)
    cloudKeyframer->SetSkyInterface(si);
}

void SimulCloudRendererBase::UpdateCameraPos()
{
  if (GScene && GScene->GetCamera())
  {
    cam_pos.x = GScene->GetCamera()->Position()[0];
    cam_pos.y = GScene->GetCamera()->Position()[1];
    cam_pos.z = GScene->GetCamera()->Position()[2];
    cam_pos.w = 1.0f;
  }
  else
  {
    cam_pos.x = cam_pos.y = cam_pos.z = cam_pos.w = 0;
  }
}

void SimulCloudRendererBase::DiagDraw()
{
#if _ENABLE_CHEATS
  if (!GEngine)
    return;

  GScene->DrawDiagArrow(_diagInfo.sunOcclusionCamPos, _diagInfo.sunOcclusionDir, 1.0f, Color(1, 0.2, 0.2, 1));
  //GEngine->DrawLine3D(-1, _diagInfo.sunOcclusionCamPos, _diagInfo.sunOcclusionDir * 1000.0f, PackedColor(255, 50, 50, 255), 10.0f, 0);
#endif
}

void SimulCloudRendererBase::RecalculateLighting()
{
  if (!GLandscape)
    return;
  if (!GScene)
    return;
  if (!GScene->MainLight())
    return;

#if _ARMA3_NEW_LIGHTING_CFG
  if (GLandscape->UseNewLandLighting())
  {
    _cloudLighting = GScene->GetCurrentLighting()._cloudsColor;
  }
  else
#endif
  {
    Color mainColor = GScene->MainLight()->GetAmbient() + GScene->MainLight()->GetDiffuse(); 
    Color sky,aroundSun;  
    float cosSun = -GScene->MainLight()->SunDirection().Y();
    GLandscape->GetSkyColors(sky,aroundSun,cosSun);
    float brightness = floatMax(mainColor.Brightness(), sky.Brightness());
    _cloudLighting = Color(brightness,brightness, brightness,brightness);  
  }
}

void SimulCloudRendererBase::InitFromCfg(ParamEntryPar entry)
{
  DoAssert(cloudInterface);
  if (!cloudInterface)
    return;

  DoAssert(cloudKeyframer);
  if (!cloudKeyframer)
    return;

  DoAssert(helper);
  if (!helper)
    return;

  RString fileName = entry >> "noiseTexture";
  CreateNoiseTexture(fileName);

  int gridWidth = entry >> "cloudGridWidth";
  cloudInterface->SetGridWidth(gridWidth);

  int gridLength = entry >> "cloudGridLength";
  cloudInterface->SetGridLength(gridLength);

  int gridHeight = entry >> "cloudGridHeight";
  cloudInterface->SetGridHeight(gridHeight);


  cloudInterface->SetCloudWidth(entry >> "cloudWidth");
  cloudInterface->SetCloudLength(entry >> "cloudLength");

  cloudInterface->SetWrap(entry >> "wrapClouds");

  int noiseResolution = entry >> "noiseResolution";
  cloudInterface->SetNoiseResolution(noiseResolution);
  cloudInterface->SetNoisePeriod(entry >> "noisePeriod");

  cloudInterface->SetOpticalDensity(entry >> "opticalDensity");

  cloudInterface->SetAlphaSharpness(entry >> "alphaSharpness");
  cloudInterface->SetSelfShadowScale(entry >> "selfShadowScale");
  cloudInterface->SetMieAsymmetry(entry >> "mieAsymmetry");
  cloudInterface->SetMinimumLightElevationDegrees(entry >> "minimumLightElevationDegrees");

  int elevSteps = entry >> "helperGridElevationSteps";
  int azSteps = entry >> "helperGridAzimuthSteps";

  helper->SetGrid(elevSteps, azSteps);
  helper->SetCurvedEarth(entry >> "helperCurvedEarth");
  helper->SetEffectiveEarthRadius(entry >> "helperEffectiveEarthRadius");
  helper->SetAdjustCurvature(entry >> "helperAdjustCurvature");
  helper->SetNearCloudFade(entry >> "helperNearCloudFade");

	_baseChurn = entry >> "helperChurn";
  helper->SetChurn(_baseChurn);

  int numLayers = entry >> "helperNumLayers";
  if (GEngine)
    numLayers = GEngine->GetSWGeometryLayersCount();
  helper->Initialize(numLayers,entry >> "helperMaxDistance");

  _windSpeedCoef = entry >> "windSpeedCoef";
  _directLightCoef = entry >> "directLightCoef";
  _indirectLightCoef = entry >> "indirectLightCoef";

  _maxFadeDistanceMetres = entry >> "fadeMaxDistanceKm";
  _maxFadeDistanceMetres *= 1000;

  _fogStart = entry >> "fogStart";
  _fogEnd = entry >> "fogEnd";
}

void SimulCloudRendererBase::InitKeyframes(int numKeyframes, simul::clouds::CloudKeyframer::Keyframe& keyframe)
{
  DoAssert(cloudKeyframer);
  if (!cloudKeyframer)
    return;
  Assert(numKeyframes > 0);
  if (numKeyframes <= 0)
    return;

  cloudKeyframer->SetUniformKeyframes(numKeyframes);
  for(int i=0;i<cloudKeyframer->GetNumKeyframes();i++)
  {
    simul::clouds::CloudKeyframer::Keyframe *k=(simul::clouds::CloudKeyframer::Keyframe *)cloudKeyframer->GetKeyframe(i);
    Assert(k);
    if (!k)
      continue;

    ApplyKeyframe(*k, keyframe);
  }

  cloudKeyframer->RecalculateOffsets();
} 

void SimulCloudRendererBase::ApplyKeyframe(simul::clouds::CloudKeyframer::Keyframe& dest, const simul::clouds::CloudKeyframer::Keyframe& src)
{
  dest.wind_speed = src.wind_speed ;
  dest.wind_direction = src.wind_direction ;
  dest.precipitation = src.precipitation ;
  dest.cloudiness = src.cloudiness ;
  dest.cloud_base_km = src.cloud_base_km ;
  dest.cloud_height_km = src.cloud_height_km ;
  dest.direct_light = src.direct_light ;
  dest.indirect_light = src.indirect_light ;
  dest.ambient_light = src.ambient_light ;
  dest.octaves = src.octaves ;
  dest.persistence = src.persistence ;
  dest.fractal_amplitude = src.fractal_amplitude ;
  dest.fractal_wavelength = src.fractal_wavelength ;
  dest.extinction = src.extinction ;
  dest.rain_to_snow = src.rain_to_snow ;
  dest.diffusivity = src.diffusivity ;
  dest.lightning = src.lightning;
}

int SimulCloudRendererBase::FillVertices(SimulCloudRendererBase* cloudRenderer, void* verticesToFill, int maxVertices, int vertexSize,
                                        const Matrix4& cameraMatrix, const Matrix4& cameraProjMatrix, 
                                        bool cubemap, bool useCpuFade, bool enableLightning)
{
  PROFILE_SCOPE_EX(swFillV, SW);

  static float lastFillTime = -1.0f;

  DoAssert(verticesToFill);
  if (!verticesToFill)
    return 0;

  DoAssert(cloudRenderer);
  if (!cloudRenderer)
    return 0;

  simul::clouds::CloudInterface * cloudInterface = cloudRenderer->GetCloudInterface();
  DoAssert(cloudInterface);
  if (!cloudInterface)
    return 0;

  simul::math::Vector3 wind_offset=cloudInterface->GetWindOffset();
  Swap(wind_offset.y,wind_offset.z); //in arma we use Y as verticl axis

  simul::math::Vector3 view_dir	(cameraMatrix.Direction()[0],cameraMatrix.Direction()[1],cameraMatrix.Direction()[2]);
  simul::math::Vector3 up(cameraMatrix.DirectionUp()[0], cameraMatrix.DirectionUp()[1], cameraMatrix.DirectionUp()[2]);

  /// update helper
  simul::clouds::CloudGeometryHelper* helper = cloudRenderer->GetHelperPtr();
  DoAssert(helper);
  if (!helper)
    return 0;

  simul::math::Vector3 viewPos;
  viewPos.x = cameraMatrix.Position()[0];
  viewPos.y = cameraMatrix.Position()[1];
  viewPos.z = cameraMatrix.Position()[2];
  viewPos.w = 1.0f;

  simul::sky::BaseSkyInterface* skyInterface = cloudRenderer->GetSkyInterface();
  DoAssert(skyInterface);
  if (!skyInterface)
    return 0;

  simul::clouds::CloudKeyframer* cloudKeyframer = cloudRenderer->GetCloudKeyframer();
  DoAssert(cloudKeyframer);
  if (!cloudKeyframer)
    return 0;

  float curTime=skyInterface->GetTime();
  float deltaT =(curTime-lastFillTime)*cloudKeyframer->GetTimeFactor();
  if(lastFillTime <= 0.0f || lastFillTime > curTime)
    deltaT = 0.0f;

  lastFillTime=curTime;

  helper->Update(viewPos,wind_offset,view_dir,up,deltaT,cubemap);

  simul::sky::float4 view_km=viewPos;
  view_km*=0.001f;

  float base_alt_km=0.001f*(cloudInterface->GetCloudBaseZ());//+.5f*cloudInterface->GetCloudHeight());

  // Get the sunlight at this altitude:
  simul::sky::float4 sunlight=skyInterface->GetLocalIrradiance(base_alt_km);

  /// make geometry
  float tan_half_fov_vertical=1.0f / cameraProjMatrix(1,1);
  float tan_half_fov_horizontal=1.0f / cameraProjMatrix(0,0);

  helper->SetFrustum(tan_half_fov_horizontal,tan_half_fov_vertical);
  helper->MakeGeometry(cloudInterface,enableLightning);

  if(useCpuFade)
    helper->CalcInscatterFactors(skyInterface);

  simul::math::Vector3 pos;
  int v=0;
  static std::vector<simul::sky::float4> light_colours;
  unsigned grid_x,el_grid;
  helper->GetGrid(el_grid,grid_x);
  light_colours.resize(el_grid+1);


  Vertex_t* vertices = 0;
  CPUFadeVertex_t* cpu_fade_vertices=0;

  if(useCpuFade)
  {
    cpu_fade_vertices = (CPUFadeVertex_t*)verticesToFill;
    DoAssert(sizeof(CPUFadeVertex_t) == vertexSize);
    if (sizeof(CPUFadeVertex_t) != vertexSize)
      return 0;
  }
  else
  {
    vertices = (Vertex_t*)verticesToFill;
    DoAssert(sizeof(Vertex_t) == vertexSize);
    if (sizeof(Vertex_t) != vertexSize)
      return 0;
  }

  for(std::vector<simul::clouds::CloudGeometryHelper::RealtimeSlice*>::const_iterator i=helper->GetSlices().begin();
    i!=helper->GetSlices().end();i++)
  {
    helper->MakeLayerGeometry(cloudInterface,*i);
    const std::vector<int> &quad_strip_vertices=helper->GetQuadStripIndices();
    size_t qs_vert=0;
    float fade=(*i)->fadeIn;
    bool start=true;
    if((*i)->quad_strips.size())
      for(int j=(*i)->elev_start;j<=(*i)->elev_end;j++)
      {
        float j_interp=(float)j/(float)el_grid;
        float sine=(2.f*j_interp-1.f);
        float alt_km=floatMin(floatMax(0.f,view_km.y+sine*0.001f*(*i)->distance),0.001f*(cloudInterface->GetCloudBaseZ()+cloudInterface->GetCloudHeight()));
        light_colours[j]=skyInterface->GetLocalIrradiance(alt_km);
      }
      for(std::vector<const simul::clouds::CloudGeometryHelper::QuadStrip*>::const_iterator j=(*i)->quad_strips.begin();
        j!=(*i)->quad_strips.end();j++)
      {
        bool l=0;
        for(size_t k=0;k<(*j)->num_vertices;k++,qs_vert++,l++,v++)
        {
          const simul::clouds::CloudGeometryHelper::Vertex &V=helper->GetVertices()[quad_strip_vertices[qs_vert]];
          pos.Define(V.x,V.y,V.z);
          simul::math::Vector3 tex_pos(V.cloud_tex_x,V.cloud_tex_y,V.cloud_tex_z);

          if(v>=maxVertices)
          {
            break;
          }
          if(start)
          {
            Vertex_t *startvertex=NULL;
            if(useCpuFade)
              startvertex=&cpu_fade_vertices[v];
            else
              startvertex=&vertices[v];
            
						startvertex->position=pos;

						//this was left undefined, don't know if it's correct or not
						startvertex->texCoords=tex_pos;
						startvertex->texCoordsNoise.x=V.noise_tex_x;
						startvertex->texCoordsNoise.y=V.noise_tex_y;
						startvertex->layerFade=fade;

            v++;
            start=false;
          }
          if(v>=maxVertices)
          {
            break;
          }

          Vertex_t *vertex=NULL;
          if(useCpuFade)
            vertex=&cpu_fade_vertices[v];
          else
            vertex=&vertices[v];
          vertex->position=pos;
          vertex->texCoords=tex_pos;
          vertex->texCoordsNoise.x=V.noise_tex_x;
          vertex->texCoordsNoise.y=V.noise_tex_y;
          vertex->layerFade=fade;

          // LUKE TEST
          const simul::sky::float4 &light_c=light_colours[V.grid_y];
          vertex->sunlightColour.x=light_c.x;
          vertex->sunlightColour.y=light_c.y;
          vertex->sunlightColour.z=light_c.z;

          //vertex->sunlightColour.x=sunlight.x;
          //vertex->sunlightColour.y=sunlight.y;
          //vertex->sunlightColour.z=sunlight.z;
          if(useCpuFade)
          {
            simul::sky::float4 loss			=helper->GetLoss(*i,V);
            simul::sky::float4 inscatter	=helper->GetInscatter(*i,V);
            CPUFadeVertex_t *cpu_vertex=&cpu_fade_vertices[v];
            cpu_vertex->loss.x=loss.x;
            cpu_vertex->loss.y=loss.y;
            cpu_vertex->loss.z=loss.z;
            cpu_vertex->inscatter.x=inscatter.x;
            cpu_vertex->inscatter.y=inscatter.y;
            cpu_vertex->inscatter.z=inscatter.z;
          }
        }
      }
      if(v>=maxVertices)
      {
        break;
      }
      Vertex_t *vertex=NULL;
      if(useCpuFade)
        vertex=&cpu_fade_vertices[v];
      else
        vertex=&vertices[v];
      vertex->position=pos;
      v++;
  }

  return v;
}

#endif //_ARMA3_SIMUL_WEATHER
