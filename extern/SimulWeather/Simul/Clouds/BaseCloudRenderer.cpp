#include <algorithm>
#include "precomp.h"

#include "BaseCloudRenderer.h"
#include "Simul/Sky/SkyInterface.h"
using namespace simul;
using namespace clouds;

float min_dist=180000.f;
float max_dist=320000.f;

BaseCloudRenderer::BaseCloudRenderer(bool thundercloud)
	:skyInterface(NULL)
	,cloudInterface(NULL)
	,cloudKeyframer(NULL)
	,cloud_tex_width_x(0)
	,cloud_tex_length_y(0)
	,cloud_tex_depth_z(0)
	,precip_strength(1.f)
	,altitude_tex_coord(0.f)
	,sun_occlusion(0.f)
	,Detail(1.f)
	,thunder_volume(0.f)
	,fade_interp(0.f)
	,last_time(0.f)
	,fade_mode(FRAGMENT)
	,enable_lightning(false)
	,noise_texture_frequency(8)
	,texture_octaves(7)
	,texture_persistence(0.79f)
	,noise_texture_size(1024)
{
	for(int i=0;i>16;i++)
	{
		LightToDensityTransform[i]=0;
	}
	for(int i=0;i>3;i++)
	{
		light_gridsizes[i]=0;
	}
	for(int i=0;i>4;i++)
	{
		light_extinctions[i]=0;
	}

	cam_pos.x=cam_pos.y=cam_pos.z=0;
	illumination_texel_index[0]=illumination_texel_index[1]=illumination_texel_index[2]=illumination_texel_index[3]=0;
	if(thundercloud)
		cloudNode=new simul::clouds::ThunderCloudNode;
	else
		cloudNode=new simul::clouds::FastCloudNode;
	
	AddChild(cloudNode.get());
	cloudNode->SetLightExtend(0.5f);
	cloudInterface=cloudNode.get();
	lightningRenderInterface=dynamic_cast<simul::clouds::LightningRenderInterface*>(cloudNode.get());

	cloudInterface->SetWrap(true);

	cloudInterface->SetGridLength(128);
	cloudInterface->SetGridWidth(128);
	cloudInterface->SetGridHeight(16);

	cloudInterface->SetAlphaSharpness(0.15f);

	cloudInterface->SetCloudBaseZ(3300.f);
	cloudInterface->SetCloudWidth(60000.f);
	cloudInterface->SetCloudLength(60000.f);
	cloudInterface->SetCloudHeight(6000.f);
	// How many days for the time-based noise signal to repeat:
	cloudInterface->SetNoisePeriod(4.f);

	cloudInterface->SetHumidity(0);

	cloudInterface->SetNoiseResolution(8);
	cloudInterface->SetNoiseOctaves(3);
	cloudInterface->SetNoisePersistence(.55f);

	cloudKeyframer=new simul::clouds::CloudKeyframer(cloudInterface,lightningRenderInterface);
	helper=new simul::clouds::CloudGeometryHelper();
	helper->Initialize((unsigned)(120.f*Detail),min_dist+(max_dist-min_dist)*Detail);

	helper->SetGrid(24,15);
	helper->SetCurvedEarth(true);
	helper->SetAdjustCurvature(false);
// Try to use Threading Building Blocks?
#ifdef _MSC_VER
	cloudInterface->SetUseTbb(true);
#else
	cloudInterface->SetUseTbb(false);
#endif
}

BaseCloudRenderer::~BaseCloudRenderer()
{
}


void BaseCloudRenderer::SetCloudiness(float c)
{
	simul::clouds::CloudKeyframer::Keyframe *K=cloudKeyframer->GetNextModifiableKeyframe();
	if(K)
		K->cloudiness=c;
}

void BaseCloudRenderer::SetEnableStorms(bool s)
{
	enable_lightning=s;
	lightningRenderInterface->SetLightningEnabled(s);
}

void BaseCloudRenderer::SetNoiseTextureProperties(int size,int freq,int octaves,float persistence)
{
	if(	noise_texture_size==size&&
		noise_texture_frequency==freq&&
		texture_octaves==octaves&&
		texture_persistence==persistence)
		return;
	noise_texture_size=size;
	noise_texture_frequency=freq;
	texture_octaves=octaves;
	texture_persistence=persistence;
	CreateNoiseTexture(true);
}

void BaseCloudRenderer::DetailChanged()
{
	helper->Initialize((unsigned)(120.f*Detail),min_dist+(max_dist-min_dist)*Detail);
}

void BaseCloudRenderer::SetFadeMode(FadeMode f)
{
	fade_mode=f;
}

float BaseCloudRenderer::GetWindSpeed() const
{
	return cloudKeyframer->GetWindSpeed();
}

float BaseCloudRenderer::GetWindHeadingDegrees() const
{
	return cloudKeyframer->GetWindHeadingDegrees();
}

void BaseCloudRenderer::SetWind(float speed,float heading_degrees)
{
	cloudKeyframer->SetWindSpeed(speed);
	cloudKeyframer->SetWindHeadingDegrees(heading_degrees);
}

simul::clouds::CloudInterface *BaseCloudRenderer::GetCloudInterface()
{
	return cloudInterface;
}

void BaseCloudRenderer::SetSkyInterface(simul::sky::BaseSkyInterface *si)
{
	skyInterface=si;
	cloudKeyframer->SetSkyInterface(si);
}

simul::clouds::CloudKeyframer *BaseCloudRenderer::GetCloudKeyframer()
{
	return cloudKeyframer.get();
}

simul::clouds::LightningRenderInterface *BaseCloudRenderer::GetLightningRenderInterface()
{
	simul::clouds::LightningRenderInterface *lri=dynamic_cast<simul::clouds::LightningRenderInterface *>(cloudNode.get());
	return lri;
}

bool BaseCloudRenderer::IsLicenseOK() const
{
	return cloudNode->CheckLicense();
}

float BaseCloudRenderer::GetInterpolation() const
{
	return cloudKeyframer->GetInterpolation();
}

void BaseCloudRenderer::ForceRelight()
{
	cloudKeyframer->ForceRelight();
}

simul::sky::OvercastCallback *BaseCloudRenderer::GetOvercastCallback() const
{
	return cloudKeyframer.get();
}


float BaseCloudRenderer::GetSunOcclusion() const
{
	return sun_occlusion;
}
float BaseCloudRenderer::GetRainToSnow() const
{
	return cloudKeyframer->GetInterpolatedKeyframe().rain_to_snow;
}

void BaseCloudRenderer::SetCloudBaseKm(float h_km)
{
	for(int i=0;i<cloudKeyframer->GetNumKeyframes();i++)
	{
		simul::clouds::CloudKeyframer::Keyframe *K=static_cast<simul::clouds::CloudKeyframer::Keyframe *>(cloudKeyframer->GetKeyframe(i));
		K->cloud_base_km=h_km;
	}
}

float BaseCloudRenderer::GetPrecipitationIntensity(const float *pos) const
{
	float p=cloudKeyframer->GetPrecipitation();
	float below=((cloudInterface->GetCloudBaseZ()-pos[2])*cloudInterface->GetGridHeight()/cloudInterface->GetCloudHeight())+1.f;
	below=std::min(below,1.f);
	below=std::max(below,0.f);
	simul::math::Vector3 X(pos[0],pos[1],pos[2]);
	simul::math::Vector3 wind_offset=cloudInterface->GetWindOffset();
		X+=wind_offset;
	X.z=cloudInterface->GetCloudBaseZ()+cloudInterface->GetCloudHeight()/2.f;
	float dens=1.f;//cloudInterface->GetCloudDensity(X);
	return p*precip_strength*below*dens;
}

float BaseCloudRenderer::GetPrecipitationIntensity() const
{
	return GetPrecipitationIntensity(cam_pos);
}

// Save and load a sky sequence
std::ostream &BaseCloudRenderer::Save(std::ostream &os) const
{
	return cloudKeyframer->Save(os);
}

std::istream &BaseCloudRenderer::Load(std::istream &is) const
{
	return cloudKeyframer->Load(is);
}

void BaseCloudRenderer::New()
{
	cloudKeyframer->New();
}

simul::sky::OvercastCallback *BaseCloudRenderer::GetOvercastCallback()
{
	return cloudKeyframer.get();
}

float t[4];
float u[4];
#include "Simul/Clouds/TextureGenerator.h"
bool BaseCloudRenderer::UpdateIlluminationTexture(const float *view_pos,float dt)
{
	lightningRenderInterface->SetLightningCentreX(view_pos);

	// RGBA bit-shift is 12,8,4,0
	simul::clouds::TextureGenerator::SetBits((unsigned)255<<0,(unsigned)255<<0,(unsigned)255<<0,(unsigned)255<<0,1,false);
	unsigned w=64;
	unsigned l=64;
	unsigned h=8;
	static unsigned char lightning_cloud_tex_data[64*64*8];
	simul::math::Vector3 X1,X2,DX;
	DX.Define(lightningRenderInterface->GetLightningZoneSize(),
		lightningRenderInterface->GetLightningZoneSize(),
		cloudInterface->GetCloudBaseZ()+cloudInterface->GetCloudHeight());
	X1=lightningRenderInterface->GetLightningCentreX()-0.5f*DX;
	X2=lightningRenderInterface->GetLightningCentreX()+0.5f*DX;
	X1.z=0;
	unsigned max_texels=w*l*h;
	// lightning rate : strikes per second
	static float rr=1.7f;
	float lightning_rate=rr;
	for(unsigned i=0;i<4;i++)
	{
		if(lightningRenderInterface->IsSourceStarted(i))
		{
			u[i]=lightningRenderInterface->GetLightSourceProgress(i);
			if(u[i]<0.5f)
			{
				static float ff=0.05f;
				thunder_volume+=ff;
			}
			continue;
		}
		else
			u[i]=0;
		if(!lightningRenderInterface->CanStartSource(i))
			continue;
		t[i]=(float)illumination_texel_index[i]/(float)(max_texels);
		unsigned &index=illumination_texel_index[i];
		unsigned texels=(unsigned)(lightning_rate*dt/4.f*(float)max_texels);
		if(index<max_texels)
		{
			if(texels+index>max_texels)
				texels=max_texels-index;
			simul::clouds::TextureGenerator::PartialMake3DLightningTexture(	lightningRenderInterface,
																			i,
																			w,l,h,
																			X1,X2,
																			index,
																			texels,
																			lightning_cloud_tex_data,
																			cloudInterface->GetUseTbb());
			FillIlluminationSequentially(i,index,texels,lightning_cloud_tex_data);
		}
		if(enable_lightning&&index>=max_texels)
		{
			index=0;
			lightningRenderInterface->StartSource(i);
		}
	}
	return true;
}

void BaseCloudRenderer::Update(float dt)
{
	if(cloudKeyframer)
		cloudKeyframer->Update(skyInterface->GetTime());
	if(skyInterface)
	{
		simul::sky::float4 sun_dir=skyInterface->GetDirectionToLight();
//	calculate sun occlusion for any external classes that need it:
		sun_occlusion=1.f-cloudKeyframer->GetVisibility(cam_pos,sun_dir);
	}
	simul::graph::meta::TimeStepVisitor tsv;
	tsv.SetTimeStep(dt);
	cloudNode->Accept(tsv);
}



const float *BaseCloudRenderer::GetCloudScales() const
{
	static float s[3];
	s[0]=1.f/cloudInterface->GetCloudWidth();
	s[1]=1.f/cloudInterface->GetCloudLength();
	s[2]=1.f/cloudInterface->GetCloudHeight();
	return s;
}

const float *BaseCloudRenderer::GetCloudOffset() const
{
	static simul::math::Vector3 offset,X1,X2;
	offset=cloudInterface->GetWindOffset();
	cloudInterface->GetExtents(X1,X2);
	offset+=X1;
	return offset.FloatPointer(0);
}

bool BaseCloudRenderer::GetWrap() const
{
	return cloudInterface->GetWrap();
}
