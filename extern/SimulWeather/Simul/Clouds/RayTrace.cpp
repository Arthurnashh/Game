#include "precomp.h"

#include "Simul/Clouds/RayTrace.h"
#include "Simul/Clouds/Tex3d.h"
#include "Simul/Graph/Meta/Group.h"
#include "Simul/Graph/Meta/Resource.h"
#include "Simul/Clouds/CellularCloudNode.h"
#include "Simul/Graph/StandardNodes/ShowProgressInterface.h"
#include "Simul/Math/Pi.h"
#include "Simul/Sky/SkyNode.h"
//#include "Simul/Sky/StaticFadeTable.h"
#include "Simul/Sky/SkyKeyframer.h"

using namespace simul::sky;
using namespace simul::clouds;
using namespace simul::math;

#define MAX_NUM_LAYERS (4)
static int detail_multiplier=8;

static float saturate(float x)
{
	if(x>1.f)
		return 1.f;
	if(x<0.f)
		return 0.f;
	return x;
}

static float lerp(float l,float h,float f)
{
	return l+((h-l)*f);
}

static float FRand()
{
	return ((rand()%10000)/10000.f);
}
RayTrace::CloudAuxiliary::CloudAuxiliary()
{
	GenerationNumber=-45;
	RelightNumber=-45;
}
void RayTrace::CloudAuxiliary::Setup(CloudInterface *ci)
{
	cloudInterface=ci;
	assert(cloudInterface);
	simul::math::Vector3 X1,X2;
	cloudInterface->GetExtents(X1,X2);
	simul::math::Vector3 DX=X2-X1;
	// double height:
	DX.z*=2;
	inverseDimensions.Set(1.f/DX.x,1.f/DX.y,1.f/DX.z,0);
}

RayTrace::RayTrace()
	:skyInterface(NULL)
	,Detail(30)
	,Layers(0)
	,ground_fade_thickness(100.f)
	,GroundHeightKm(-1.f)
{
	skyKeyframer=NULL;
	InitializePropertiesDefinition();
	MakeVolumetricNoiseTexture(32);
	sec_beta=simul::sky::Sky::HenyeyGreenstein(0,0);
	UnlitUnderground=true;
	intermediate_steps=0;
	sec_beta;
	ChangedDetail();
}
RayTrace::~RayTrace()
{
	delete skyKeyframer;
	ClearClouds();
}

void RayTrace::SetSky(SkyInterface *si)
{
	skyInterface=si;
}

void RayTrace::ClearClouds()
{
	for(int i=0;i<(int)cloudAuxiliaries.size();i++)
	{
		delete cloudAuxiliaries[i];
	}
	cloudAuxiliaries.clear();
}

void RayTrace::AddCloud(CloudInterface *ci)
{
	for(int i=0;i<(int)cloudAuxiliaries.size();i++)
	{
		if(cloudAuxiliaries[i]->cloudInterface==ci)
		{
			return;
		}
		if(i>=MAX_NUM_LAYERS-1)
			return;
	}

	cloudAuxiliaries.push_back(new CloudAuxiliary());
	CloudAuxiliary *auxiliary=cloudAuxiliaries.back();
	auxiliary->Setup(ci);
}

void RayTrace::RemoveUnusedClouds()
{
	for(int i=0;i<(int)cloudAuxiliaries.size();i++)
	{
		if(!cloudAuxiliaries[i]->cloudInterface)
		{
			delete cloudAuxiliaries[i];
			cloudAuxiliaries.erase(cloudAuxiliaries.begin()+i);
			i--;
		}
	}
}
void RayTrace::ChangedDetail()
{
	if(rendering)
	{
		cancel=true;
	}
	else
	{
		Layers=Detail*detail_multiplier;
		helper.Initialize(Layers,GetMaxDistance());
	}
}

void RayTrace::RenderEnd()
{
	if(cancel)
	{
		Layers=Detail*detail_multiplier;
		helper.Initialize(Layers,GetMaxDistance());
	}
	rendering=false;
}

float RayTrace::GetMaxDistance() const
{
	return 400000.f;
}
void RayTrace::RenderBegin(const simul::sky::float4& view_pos_km,const simul::sky::float4& wind_offset,
						   const simul::sky::float4& view_dir,const simul::sky::float4& up_dir)
{
	cancel=true;
	exposure=pow(2.f,Exposure);
	cam_pos_km=(const float*)view_pos_km;
	cam_pos=cam_pos_km*1000.f;
  	// initializing stuff required by plugin prior to rendering
	ChangedDetail();
	helper.SetFrustum(2.5f,2.5f);
	helper.Update((const float *)view_pos_km,(const float *)wind_offset,(const float *)view_dir,(const float *)up_dir);

	if(!skyKeyframer)
		skyKeyframer=new SkyKeyframer(skyInterface);//,0,256,256,1024,1000.f);
	skyKeyframer->SetAltitudeKM(cam_pos_km.z);
//	skyKeyframer->UpdateTables();
	base_height=GroundHeightKm*1000.f;
	simul::math::Vector3 dir_to_sun(skyInterface->GetDirectionToSun());
	static bool slow_lighting=false;
	for(int i=0;i<(int)cloudAuxiliaries.size();i++)
	{
		CloudAuxiliary *auxiliary=(cloudAuxiliaries[i]);
		CloudInterface *cloudInterface=auxiliary->cloudInterface;

		float total_response=	cloudInterface->GetLightResponse()
								+cloudInterface->GetSecondaryLightResponse()
								;//+cloudInterface->GetAmbientLightResponse();
		if(!total_response)
			total_response=0.f;
		
		auxiliary->primary_proportion=cloudInterface->GetLightResponse()/total_response;
		auxiliary->secondary_proportion=cloudInterface->GetSecondaryLightResponse()/total_response;
		auxiliary->ambient_proportion=cloudInterface->GetAmbientLightResponse()/total_response;

		cloudInterface->Generate();
		cloudInterface->ReLight(dir_to_sun);

		int tex_width=cloudInterface->GetGridWidth();
		int tex_length=cloudInterface->GetGridLength();
		int tex_height=cloudInterface->GetGridHeight();
		Vector3 X,X1,X2;
		cloudInterface->GetExtents(X1,X2);
		if(auxiliary->GenerationNumber==cloudInterface->GetGenerationNumber()
			&&auxiliary->RelightNumber==cloudInterface->GetRelightNumber()
			&&auxiliary->X1==X1)
			continue;
		auxiliary->X1=X1;
		
		//float dh=(X2.z-X1.z)/(float)tex_height;
		float one_texel=1.f/(float)tex_height;
		float one_texel_height=one_texel*(X2.z-auxiliary->X1.z);
		int h=tex_height;
		while(auxiliary->X1.z>0&&h<h*4)
		{
			auxiliary->X1.z-=one_texel_height;
			tex_height++;
		}
		Vector3 DX=X2-auxiliary->X1;

		auxiliary->MaxTexCoordZ=1.f+one_texel+auxiliary->cloudInterface->GetFractalOffsetScale()/DX.z;
		
		auxiliary->GenerationNumber=cloudInterface->GetGenerationNumber();
		auxiliary->RelightNumber=cloudInterface->GetRelightNumber();

		float4 *cloud=auxiliary->cloud_tex.Setup(tex_width,tex_length,tex_height,true,true,false,float4(0,1,1,1));
		simul::graph::standardnodes::ShowProgressInterface *progress=
			simul::graph::meta::ResourceUser<simul::graph::standardnodes::ShowProgressInterface>::GetResourceInterface();

		if(progress)
			progress->SetText(this,"CloudWright Initializing...");
		int index=0;
	static float sec_mult=1.f/(4.f*pi);
	float extinction=cloudInterface->GetExtinction()*0.001f;
	float sec_extinction=extinction*sec_mult;
		for(int i=0;i<tex_height;i++)
		{
			if(progress)
				progress->SetProgress(this,(float)i/(float)(tex_height));
			int z=i+h-tex_height;
			X.z=lerp(auxiliary->X1.z,X2.z,(float)i/(float)(tex_height));
			for(int j=0;j<tex_length;j++)
			{
				X.y=lerp(auxiliary->X1.y,X2.y,(float)j/(float)(tex_length));
				for(int k=0;k<tex_width;k++,index++)
				{
					float dens,light,amb,sec;
					if(slow_lighting)
					{
						X.x=lerp(auxiliary->X1.x,X2.x,(float)k/(float)(tex_width));
						if(z>=0)
							dens=cloudInterface->GetCloudDensity(X);
						else
							dens=0;
						float opt_len=cloudInterface->GetOpticalPathLength(X,dir_to_sun);
						light=exp(-opt_len*extinction);
						sec=exp(-opt_len*sec_extinction);
						amb=cloudInterface->GetAmbient(X);

					}
					else
					{
						if(z>=0)
						{
							cloudInterface->GetGridValues(dens,light,amb,sec,k,j,z);
							if(z<=0||z>=h-1)
								dens=0;
						}
						else
						{
							X.x=lerp(auxiliary->X1.x,X2.x,(float)k/(float)(tex_width));
							dens=0;
							light=cloudInterface->GetLight(X);
							sec=cloudInterface->GetSecondaryLight(X);
							amb=cloudInterface->GetAmbient(X);
						}
					}
					cloud[index].x=dens;
					cloud[index].y=light;
					cloud[index].z=sec;
					cloud[index].w=amb;
				}
			}
		}
		if(progress)
			progress->SetProgressComplete(this);
	}
	float sun_radius=skyInterface->GetSunRadiusArcMinutes()*pi/180.f/60.f;
	sun_cosine=cos(sun_radius);
	cancel=false;
	rendering=true;
}

float4 RayTrace::GetNoise(const float4& pos,CloudInterface *cloudInterface)
{
	float c=1.f/(cloudInterface->GetFractalRepeatLength());
	float4 tex=c*pos;
	float scale=.5f;
	float4 noiseval(0,0,0,0);
	for(unsigned i=0;i<NoiseOctaves;i++)
	{
		noiseval+=noise_tex.Get(tex.x,tex.y,tex.z)*scale;
		scale*=NoisePersistence;
		tex*=2.f;
	}
	return noiseval;
}

void RayTrace::Trace(const float4& view_pos_km,const float4& pos_km,
						float4& final, float4& trans)
{
	if(!skyInterface)
		return;
	if(cancel)
		return;
	static float loss_factor=1.f;
	trans.Set(0,0,0,0);
	float4 end_pos_km=(const float*)pos_km;
	float4 dir=(const float*)(pos_km-view_pos_km);
	float dist_km=length(dir);
	dir/=length(dir);
	float elevation=asin(dir.z);
	float sun_cos=float4::dot(dir,skyInterface->GetDirectionToSun());
	if(sun_cos>sun_cosine)
		final+=skyInterface->GetSunIrradiance();
	if(!cloudAuxiliaries.size())
	{
		float4 loss=skyKeyframer->GetIsotropicColourLossFactor(elevation,0.f,dist_km,1e5f);
			//skyInterface->GetColourLossFactor(view_pos_km,pos_km,false);
		float4 insc=skyInterface->GetInscatter(view_pos_km,pos_km,UnlitUnderground);
		trans=loss;
		insc*=exposure;
		final+=insc;
		return;
	}
	float4 pure_insc=skyInterface->GetInscatter(view_pos_km,pos_km);
	std::vector<CloudGeometryHelper::RealtimeSlice*>::const_iterator i=helper.GetSlices().begin();

	if(i==helper.GetSlices().end())
		return;
	final/=exposure;
	float max_dist=1000.f*length(pos_km);

	//float4 loss=skyInterface->GetColourLossFactor(end_pos_km,pos_km);
	float4 loss=skyKeyframer->GetIsotropicColourLossFactor(elevation,dist_km,length(end_pos_km),1e5f);
	float4 insc=skyInterface->GetInscatter(end_pos_km,pos_km);
	trans=loss;
	final+=insc;

	float cos0=float4::dot(dir,skyInterface->GetDirectionToSun());
	simul::math::Vector3 X2;
	float4 lookup[MAX_NUM_LAYERS];
	bool in_tex[MAX_NUM_LAYERS];
	float R=helper.GetEffectiveEarthRadius(1000.f);
	if(cancel)
		return;
	float4 loss_this;
	if(i!=helper.GetSlices().end())
		loss_this=skyKeyframer->GetIsotropicColourLossFactor(elevation,0,(*i)->distance*0.001f,1e5f);
	for(i;i!=helper.GetSlices().end();i++)
	{
		if(cancel)
			return;
		float dist=(*i)->distance;
		if(dist>max_dist)
			continue;
		float wall_fade=(max_dist-dist)/(*i)->thickness;
		if(wall_fade>1.f)
			wall_fade=1.f;
		float next_dist=0;

		std::vector<CloudGeometryHelper::RealtimeSlice*>::const_iterator j=i;
		j++;
		if(j!=helper.GetSlices().end())
			next_dist=(*j)->distance;

		float steplength=dist-next_dist;

		float4 Pos=(const float*)(cam_pos+dir*dist);
		float ground_fade=(Pos.z-base_height)/ground_fade_thickness;
		ground_fade=saturate(ground_fade);

		float4 next_pos=(const float*)(cam_pos+dir*next_dist);

		float4 loss_next=skyKeyframer->GetIsotropicColourLossFactor(elevation,0,next_dist*0.001f,1e5f);
//float4 sky_loss1=skyInterface->GetColourLossFactor(0.001f*next_pos,0.001f*Pos,false,intermediate_steps);
float4 sky_loss=loss_this/loss_next;
		loss_this=loss_next;
		float4 sky_insc=skyInterface->GetInscatter(0.001f*next_pos,0.001f*Pos,UnlitUnderground);
		// for each cloud layer:

		float light=1.f;
		float sec=1.f;
		float amb=1.f;
		for(size_t j=0;j<cloudAuxiliaries.size();j++)
		{
			CloudAuxiliary *auxiliary=cloudAuxiliaries[j];
			float4 x1(auxiliary->X1.FloatPointer(0));
			x1.z+=helper.GetVerticalShiftDueToCurvature(dist,x1.z,R); 
			float4 texCoordDiffuse=(Pos-x1)*auxiliary->inverseDimensions;
			if(texCoordDiffuse.z<auxiliary->MaxTexCoordZ)
			{
				in_tex[j]=true;
				float4 noiseval=GetNoise(Pos,auxiliary->cloudInterface);
				float4 pos=Pos+auxiliary->cloudInterface->GetFractalOffsetScale()*noiseval;
				texCoordDiffuse=(pos-x1)*auxiliary->inverseDimensions;
				lookup[j]=auxiliary->cloud_tex.Get(texCoordDiffuse.x,texCoordDiffuse.y,texCoordDiffuse.z);
				light*=lookup[j].y;
				sec*=lookup[j].z;
				amb*=lookup[j].w;
			}
			else
				in_tex[j]=false;
		}
		
//		float light_loss=(1.f-light)*auxiliary->primary_proportion+(1.f-sec)*auxiliary->secondary_proportion
		//			+(1.f-amb)*(1.f-light);
		//		light_loss*=loss_factor;
				sky_insc*=1.f-(1.f-light)*loss_factor;//(1.f-light_loss)*ground_fade;
//				sky_insc*=loss_factor;


		float4 sunlight=skyInterface->GetSunlightColour(.001f*Pos);
		for(size_t j=0;j<cloudAuxiliaries.size();j++)
		{
			CloudAuxiliary *auxiliary=cloudAuxiliaries[j];
			float4 x1(auxiliary->X1.FloatPointer(0));
			x1.z+=helper.GetVerticalShiftDueToCurvature(dist,x1.z,R); 
			float4 texCoordDiffuse=(Pos-x1)*auxiliary->inverseDimensions;
			float4 noiseval=GetNoise(Pos,auxiliary->cloudInterface);
			float4 pos=Pos+auxiliary->cloudInterface->GetFractalOffsetScale()*noiseval;
			texCoordDiffuse=(pos-x1)*auxiliary->inverseDimensions;
			if(in_tex[j])
			{
				float dens=lookup[j].x;
				if(dens<auxiliary->cloudInterface->GetAlphaSharpness())
					dens=0.f;
				//dens=saturate((dens-auxiliary->cloudInterface->GetAlphaSharpness())/(1.f-auxiliary->cloudInterface->GetAlphaSharpness()));
				dens*=wall_fade*ground_fade;


		//sunlight*=light_scale*light.z+light.y*secondary_light_scale;
//		sunlight+=ambient.rgb*(1.f-density.z);



				dens*=auxiliary->cloudInterface->GetOpticalDensity()*steplength*0.001f;
				float Beta=simul::sky::Sky::HenyeyGreenstein(auxiliary->cloudInterface->GetMieAsymmetry(),cos0)
					*auxiliary->cloudInterface->GetLightResponse();
				float l=light*Beta;
				float s=sec*sec_beta*auxiliary->cloudInterface->GetSecondaryLightResponse();
				float a=amb*auxiliary->cloudInterface->GetAmbientLightResponse();
				float cloud_loss=exp(-dens);
				float opacity=1.f-cloud_loss;
				float4 ambient=skyInterface->GetSkyColour(float4(0,0,1.f,0),0.001f*Pos.z);
				float4 cloud_insc=opacity*((l+s)*sunlight+a*ambient);
				final*=cloud_loss;
				final+=cloud_insc;
			}
		}
		final*=sky_loss;
		final+=sky_insc;
	}
	final*=exposure;
}

void RayTrace::MakeVolumetricNoiseTexture(unsigned texSize)
{
	float4 *noise=noise_tex.Setup(texSize,texSize,texSize,true,true,true);
	noiseTextureSize=texSize;
	if(!noise)
		return;
	try
	{
		float4 *ptr = noise;
		for (unsigned i = 0; i < texSize; ++i)
		{
			for (unsigned j = 0; j < texSize; ++j)
			{
				for (unsigned k = 0; k < texSize; ++k)
				{
					float r=1.f-pow(FRand(),4.f);
					float az=FRand()*2*3.1415926536f;
					float el=asin(FRand()*2.f-1.f);
					float x=r*sin(az)*cos(el);
					float y=r*cos(az)*cos(el);
					float z=r*sin(el);
					(*ptr).x=x;
					(*ptr).y=y;
					(*ptr).z=z;
					(*ptr).w=0;
					ptr++;
				}
			}
		}
	}
	catch(...)
	{
	}
}
