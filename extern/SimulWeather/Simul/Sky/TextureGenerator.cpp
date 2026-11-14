#include "precomp.h"

#include "Simul/Sky/TextureGenerator.h"
#include "Simul/Sky/SkyInterface.h"
#include "Simul/Sky/Sky.h"
#include "Simul/Sky/Float4.h"
#define pi (3.1415926536f)
#include <math.h>

#if defined(WIN32)||defined(WIN64)
	#if defined(_DLL) && !defined(XENON)&&!defined(_DEBUG)
		#define SIMUL_USE_TBB
	#endif
#endif

#ifdef SIMUL_USE_TBB
	#define  TBB_DO_THREADING_TOOLS 1
	#include "tbb/parallel_for.h" 
	#include "tbb/blocked_range.h"
#endif

using namespace simul::sky;
static bool fix_distances=false;
union uif
{
	unsigned int	i;
	float			f;
};
static bool table=false;
static unsigned short eLut[1 << 9];
void initELut()
{
    for (int i = 0; i < 0x100; i++)
    {
		int e = (i & 0x0ff) - (127 - 15);
		if (e <= 0 || e >= 30)
		{
			eLut[i]         = 0;
			eLut[i | 0x100] = 0;
		}
		else
		{
			eLut[i]         = (unsigned short)(e << 10);
			eLut[i | 0x100] = (unsigned short)((e << 10) | 0x8000);
		}
    }
}

static float max_distance_km=1e4f;

static void Convert(unsigned char *&current_ptr,simul::sky::float4 &coeff,simul::sky::TextureGenerator::ColourFormat format,
					bool use_floats,bool use_float16,unsigned bytes_per_texel)
{
	if(format==simul::sky::TextureGenerator::BGRA)
		std::swap(coeff.x,coeff.z);
	if(use_float16)
	{
		short *short_ptr=(short*)(current_ptr);
		short_ptr[0]	=simul::sky::TextureGenerator::ToFloat16(coeff.x);
		short_ptr[1]	=simul::sky::TextureGenerator::ToFloat16(coeff.y);
		short_ptr[2]	=simul::sky::TextureGenerator::ToFloat16(coeff.z);
		short_ptr[3]	=simul::sky::TextureGenerator::ToFloat16(coeff.w);
	}
	else if(use_floats)
	{
		float *float_ptr=(float*)(current_ptr);
		float_ptr[0]	=coeff.x;
		float_ptr[1]	=coeff.y;
		float_ptr[2]	=coeff.z;
		float_ptr[3]	=coeff.w;
	}
	else
	{
		if(coeff.x>1.f)
			coeff.x=1.f;
		if(coeff.y>1.f)
			coeff.y=1.f;
		if(coeff.z>1.f)
			coeff.z=1.f;
		if(coeff.w>1.f)
			coeff.w=1.f;
		unsigned char *uc_ptr=(unsigned char *)(current_ptr);
		uc_ptr[0]	=(unsigned char)(255.f*coeff.x);
		uc_ptr[1]	=(unsigned char)(255.f*coeff.y);
		uc_ptr[2]	=(unsigned char)(255.f*coeff.z);
		uc_ptr[3]	=(unsigned char)(255.f*coeff.w);
	}
	current_ptr+=bytes_per_texel;
}

namespace simul
{
	namespace sky
	{
		namespace TextureGenerator
		{
			class DoFade
			{ 
				const SkyInterface *skyInterface;
				unsigned bytes_per_texel;
				unsigned char *inscptr;
				unsigned char *lossptr;
				unsigned char *skyptr;
				unsigned tex_width;
				unsigned tex_height;
				float altitude_km;
				float max_dist_km;
				ColourFormat format;
				bool use_float16;
				bool use_floats;
				bool do_inscatter;
				bool earth_test;
			public: 
	#ifdef SIMUL_USE_TBB
				void operator()( const tbb::blocked_range<size_t>& r ) const
	#else
				void operator()( unsigned begin,unsigned end ) const
	#endif
				{
	#ifdef SIMUL_USE_TBB
					unsigned begin=(unsigned)r.begin();
					unsigned end=(unsigned)r.end();
	#endif
					begin*=tex_width;
					end*=tex_width;
					unsigned char *insc_ptr=&(((unsigned char*)inscptr)[bytes_per_texel*begin]);
					unsigned char *loss_ptr=&(((unsigned char*)lossptr)[bytes_per_texel*begin]);
					unsigned sky_idx=begin/tex_width;
					unsigned char *sky_ptr=&(((unsigned char*)skyptr)[bytes_per_texel*sky_idx]);
					unsigned last_i=0xFFFFFFFF;
					float sine=0.f;
					float elevation=0.f;
					float current_max_dist_km=0.f;
					for(unsigned k=begin;k<end;++k )
					{
static int steps=0;
						unsigned i=(k)/tex_width;
						unsigned j=(k)-(i*tex_width);
						if(i!=last_i)
						{
							sine=1.f-(((float)i)/(float)(tex_height-1))*2.f;
							sine=std::max(sine,-1.f);
							sine=std::min(sine,1.f);
							elevation=asin(sine);
						// Add 100m to altitude for distance limit
							current_max_dist_km=skyInterface->GetDistanceToEarth(elevation,altitude_km+1.f);
							if(current_max_dist_km<0.f)
								current_max_dist_km=max_dist_km;
							else
								current_max_dist_km=0.7f*current_max_dist_km+0.3f*max_dist_km;
							if(current_max_dist_km<1.f)
								current_max_dist_km=1.f;
							//if(current_max_dist_km>max_dist_km)
								current_max_dist_km=max_dist_km;
						}
						float dist=pow((((float)j)/(float)(tex_width-1)),2.f)*current_max_dist_km;
						float prev_dist=pow(std::max(0.f,((float)j-1.f)/(float)(tex_width-1)),2.f)*current_max_dist_km;
						if(fix_distances)
							dist=max_distance_km;

						float4 insc,loss;
						float4 previous_loss(1.f,1.f,1.f,1.f);
						float4 previous_insc(0,0,0,0);
						if(j!=0)
						{
							float4 *float4_ptr=(float4*)(insc_ptr-16);
							previous_insc=*float4_ptr;
							previous_loss=*((float4*)(loss_ptr-16));
						}

		float4 skylight=skyInterface->GetAmbientLight(2.f);
						insc			=skyInterface->GetIsotropicInscatterFactor(altitude_km,elevation,prev_dist,dist,earth_test,steps);
						//loss			=skyInterface->GetIsotropicColourLossFactor(altitude_km,elevation,0,prev_dist,false,steps);

						loss			=skyInterface->GetIsotropicColourLossFactor(altitude_km,elevation,prev_dist,dist,false,steps);
						loss			*=previous_loss;
						Convert(loss_ptr,loss,format,use_floats,use_float16,bytes_per_texel);

						insc			*=previous_loss;
						insc.w			*=(dist-prev_dist);
						previous_insc.w	*=prev_dist;
						insc			+=previous_insc;
						if(dist>0)
							insc.w		/=dist;
						//insc=skyInterface->GetIsotropicInscatterFactor(altitude_km,elevation,0,dist,earth_test,steps);
						Convert(insc_ptr,insc,format,use_floats,use_float16,bytes_per_texel);

						// after the last value, fill in the sky as an effective next step.
						if(sky_ptr&&j==tex_width-1)
						{
							previous_insc=insc;
							//loss		=skyInterface->GetIsotropicColourLossFactor(altitude_km,elevation,0,dist,false,steps);
							insc		=skyInterface->GetIsotropicInscatterFactor(altitude_km,elevation,dist,1e3f,earth_test,steps);
							insc		*=loss;
							insc		+=previous_insc;
							Convert(sky_ptr,insc,format,use_floats,use_float16,bytes_per_texel);
						}
						last_i=i;
					}
				}
				DoFade(	const SkyInterface *s,
						unsigned w,
						unsigned h,
						unsigned char *l,
						unsigned char *p,
						unsigned char *sk,
						float alt_km,
						float maxdistkm,
						bool do_earth_test,
						ColourFormat fmt,
						bool usefloat16,
						bool usefloats,
						unsigned bpt)
					: skyInterface(s)
					, tex_width(w)
					, tex_height(h)
					, lossptr(l)
					, inscptr(p)
					, skyptr(sk)
					, altitude_km(alt_km)
					, max_dist_km(maxdistkm)
					, earth_test(do_earth_test)
					, format(fmt)
					, use_float16(usefloat16)
					, use_floats(usefloats)
					, bytes_per_texel(bpt)
				{
				} 
			};
		}
	}
}



short simul::sky::TextureGenerator::ToFloat16(float f)
{
	short _h=0;
	if (f == 0)
	{
		_h = 0;
		return _h;
	}
	else
	{
		uif x;
		x.f = f;
		register int e = (x.i >> 23) & 0x000001ff;
		e = eLut[e];
		if (e)
		{
			_h = (short)(e + (((x.i & 0x007fffff) + 0x00001000) >> 13));
			return _h;
		}
		else
		{
			unsigned int i = (x.i);
			register int s =  (i >> 16) & 0x00008000;
			register int e = ((i >> 23) & 0x000000ff) - (127 - 15);
			register int m =   i        & 0x007fffff;
			if (e <= 0)
			{
				if (e < -10)
				{
					_h=(short)s;
					return _h;
				}
				m = m | 0x00800000;
				int t = 14 - e;
				int a = (1 << (t - 1)) - 1;
				int b = (m >> t) & 1;
				m = (m + a + b) >> t;

				_h=(short)(s | m);
				return _h;
			}
			else if (e == 0xff - (127 - 15))
			{
				if (m == 0)
				{
					_h=(short)(s | 0x7c00);
					return _h;
				}
				else
				{
					m >>= 13;
					_h=(short)(s | 0x7c00 | m | (m == 0));
					return _h;
				}
			}
			else
			{
				m = m + 0x00000fff + ((m >> 13) & 1);
				if (m & 0x00800000)
				{
					m =  0;		// overflow in significand,
					e += 1;		// adjust exponent
				}
				if (e > 30)
				{
					// Cause a hardware floating point overflow;
					_h= (short)(s | 0x7c00);
					return _h;
				}
				_h= (short)(s | (e << 10) | (m >> 13));
				return _h;
			}
		}
	}
}


bool simul::sky::TextureGenerator::Make1DSkyImage(const simul::sky::SkyInterface *sky,int texSize,void *filtered_ptr,float4 &mean_colour,
					float exposure,float cloud_cover,int filter,ColourFormat format,bool use_floats,float altitude_km)
{
	if(!sky)
		return false;
	//sky->TimeStep(0);
//    const float PI = pi;
	float* output_colour=NULL;
	unsigned bytes_per_texel=4;
	if(use_floats)
	{
		output_colour=(float*)filtered_ptr;
		bytes_per_texel=16;
	}
	else
		output_colour=new float[4*texSize];
    float* ptr = output_colour;
	float vertex_dir[4];
	mean_colour[0]=0;
	mean_colour[1]=0;
	mean_colour[2]=0;
//	static float default_secondary_scattering=0.f;
	static float angle_from_sun=pi/2.f;
	float azimuth=sky->GetAzimuthToSunRadians()+angle_from_sun;
	float total_area=0;
    for(int i = 0; i < texSize; i++)
    {
		float sine=1.f-((float)i/(float)texSize)*2.f;
		float elevation=asin(-sine);

		vertex_dir[0]=-sin(azimuth)*cos(elevation);
		vertex_dir[1]= cos(azimuth)*cos(elevation);
		vertex_dir[2]= sin(elevation);

		vertex_dir[3]=0;
		float4 this_colour;
		this_colour=sky->GetSkyColour(vertex_dir,altitude_km);
		//0.27f, 0.67f, 0.06f,
		float avg=(0.27f*this_colour[0]+0.67f*this_colour[1]+0.06f*this_colour[2]);
		this_colour[0]=(1.f-cloud_cover)*this_colour[0]+cloud_cover*avg;// ADDING clouds - because clouds are bright!
		this_colour[1]=(1.f-cloud_cover)*this_colour[1]+cloud_cover*avg;
		this_colour[2]=(1.f-cloud_cover)*this_colour[2]+cloud_cover*avg;
		//this_colour[2]+=default_secondary_scattering*sky->GetDensity();
		this_colour[0]*=exposure;
		this_colour[1]*=exposure;
		this_colour[2]*=exposure;
        *ptr++ = this_colour[0];
        *ptr++ = this_colour[1];
        *ptr++ = this_colour[2];
        *ptr++ = 1;

		float area=sqrt(1.f-sine*sine);
		mean_colour[0]+=this_colour[0]*area;
		mean_colour[1]+=this_colour[1]*area;
		mean_colour[2]+=this_colour[2]*area;
		total_area+=area;
    }
	mean_colour[0]/=total_area;
	mean_colour[1]/=total_area;
	mean_colour[2]/=total_area;

	// Now filter the image:
//	int filterSize=texSize;

	for(int f=0;f<filter;f++)
	{
		for(int i=0;i<texSize;i++)
		{
			int i1=i-1;
			if(i1<0)
				i1=-i1;
			int i2=i+1;
			if(i2>=texSize)
				i2=2*texSize-i2-2;
			float sine =1.f-((float)i /(float)texSize)*2.f;// from -1 to 1
			float sine1=1.f-((float)i1/(float)texSize)*2.f;// from -1 to 1
			float sine2=1.f-((float)i2/(float)texSize)*2.f;// from -1 to 1
			float area =sqrt(1.f-sine *sine );
			float area1=sqrt(1.f-sine1*sine1);
			float area2=sqrt(1.f-sine2*sine2);
			for(int j=0;j<4;j++)
			{
				output_colour[i*4+j]*=area;
				output_colour[i*4+j]+=output_colour[i1*4+j]*area1;
				output_colour[i*4+j]+=output_colour[i2*4+j]*area2;
				output_colour[i*4+j]/=(area+area1+area2);
			}
		}
	}
	if(!use_floats)
	{
		unsigned char *uchar_ptr=(unsigned char *)filtered_ptr;
		for(int i=0;i<texSize;i++)
		{
//			float cosine=1.f-((float)i/(float)texSize)*2.f;
//			float elevation=acos(-cosine);
//			int colour_index=i;//(int)(elevation*(float)texSize/PI);		// index in the output colours
			float r=0;
			float g=0;
			float b=0;

			r=output_colour[i*4];
			g=output_colour[i*4+1];
			b=output_colour[i*4+2];
			r=std::min(r,1.f);
			g=std::min(g,1.f);
			b=std::min(b,1.f);
			if(format==BGRA)
			{
				*uchar_ptr++ = (unsigned char)(255.f*b);
				*uchar_ptr++ = (unsigned char)(255.f*g);
				*uchar_ptr++ = (unsigned char)(255.f*r);
			}
			else
			{
				*uchar_ptr++ = (unsigned char)(255.f*r);
				*uchar_ptr++ = (unsigned char)(255.f*g);
				*uchar_ptr++ = (unsigned char)(255.f*b);
			}
			*uchar_ptr++ = 255;
		}
		delete [] output_colour;
	}
	else
	{
		for(int i=0;i<texSize*4;i+=4)
		{
			if(format==BGRA)
			{
				std::swap(output_colour[i+2],output_colour[i+0]);
			}
		}
	}
	return true;        
}

bool simul::sky::TextureGenerator::Make1DDensityImage(const simul::sky::Sky *sky,int texSize,void *filtered_ptr,
					ColourFormat format,bool use_floats)
{
	// for altitudes from 0 to 64km, fill in the ozone scattering
	if(!sky)
		return false;
	float* output_colour=NULL;
	if(use_floats)
		output_colour=(float*)filtered_ptr;
	else
		output_colour=new float[4*texSize];
    float4 *ptr = (float4*)(output_colour);
    for(int i = 0; i < texSize; i++)
    {
		float altitude_km=sky->GetAtmosphereThickness()*(float)i/(float)texSize;
		ptr->Set(0,0,0,0);
		if(i!=texSize-1)	// MUST go to zero as we clamp values to this.
		{
			ptr->x=sky->GetDensityFactorAtAltitude(altitude_km);
			ptr->y=sky->GetOzoneAtAltitude(altitude_km);
			ptr->z=sky->GetHazeFactorAtAltitude(altitude_km);
		}
		ptr++;
	}
	
	if(!use_floats)
	{
		unsigned char *uchar_ptr=(unsigned char *)filtered_ptr;
		for(int i=0;i<texSize;i++)
		{
			float r=0;
			float g=0;
			float b=0;

			r=output_colour[i*4];
			g=output_colour[i*4+1];
			b=output_colour[i*4+2];
			r=std::min(r,1.f);
			g=std::min(g,1.f);
			b=std::min(b,1.f);
			if(format==BGRA)
			{
				*uchar_ptr++ = (unsigned char)(255.f*b);
				*uchar_ptr++ = (unsigned char)(255.f*g);
				*uchar_ptr++ = (unsigned char)(255.f*r);
			}
			else
			{
				*uchar_ptr++ = (unsigned char)(255.f*r);
				*uchar_ptr++ = (unsigned char)(255.f*g);
				*uchar_ptr++ = (unsigned char)(255.f*b);
			}
			*uchar_ptr++ = 255;
		}
		delete [] output_colour;
	}
	else
	{
		for(int i=0;i<texSize*4;i+=4)
			if(format==BGRA)
				std::swap(output_colour[i+2],output_colour[i+0]);
	}
	return true;
}

bool simul::sky::TextureGenerator::Make2DSkyLookupTexture(const simul::sky::OpticalLengthInterface *sky,
	float PlanetRadius,float SunRadiusArcMinutes,unsigned tex_height,unsigned lookup_tex_width,
	void *image_ptr,ColourFormat format,bool use_floats,bool do_scales,float max_alt,bool use_float16)
{
	if(!sky)
		return false;
	size_t bytes_per_texel=4;
	if(use_float16)
	{
		bytes_per_texel=2;
		if(!table)
			initELut();
		table=true;
	}
	static float opt_length_scale=0.5f;
	static float dist_scale=0.3f;
	static float alt_mult=255.f/max_alt;
	unsigned char *char_ptr=(unsigned char *)image_ptr;
	for(unsigned i=0;i<tex_height;i++)
	{
		float altitude=(float)i/((float)tex_height-1.f)*max_alt;
		for(unsigned j=0;j<lookup_tex_width;j++)
		{
			float x=(float)j/((float)lookup_tex_width-1.f);
			float elevation=(x*2.f-1.f)*pi/2.f;
			float opt_len=0;
			float average_alt=0;
			float dist=0;
			opt_len=sky->GetOpticalLength(elevation,altitude);
			float ozone_opt_len=sky->GetOzoneOpticalLength(elevation,altitude);
			float visibility=1.f;

			float horizon_elevation=-acos(PlanetRadius/(PlanetRadius+altitude));
			
			float sun_arc_radius=SunRadiusArcMinutes*3.1412659f/180.f/60.f;// radius = 1/4 of a degree
			
			if(elevation-horizon_elevation<-sun_arc_radius)
				visibility=0.f;
			else if(elevation-horizon_elevation<sun_arc_radius)
			{
				float angular_difference=sun_arc_radius+horizon_elevation-elevation;
				visibility=1.f-angular_difference/(2.f*sun_arc_radius);
				visibility=std::max(0.f,visibility);
			}
			float haze=sky->GetHazeFactorOpticalLength(elevation,altitude);
			if(do_scales)
			{
				if(!use_floats)
				{
					opt_len*=opt_length_scale;
					ozone_opt_len*=opt_length_scale;
					dist*=dist_scale;
					average_alt*=alt_mult;
				}
			}
			bool swap_rb=(format==BGRA);
			if(use_floats)
			{
				if(!use_float16)
				{
					float *float_ptr=(float*)(image_ptr);
					float_ptr[i*4*lookup_tex_width+j*4+2*(swap_rb)]	=opt_len;
					float_ptr[i*4*lookup_tex_width+j*4+1]			=visibility;
					float_ptr[i*4*lookup_tex_width+j*4+2*(!swap_rb)]=haze;
					float_ptr[i*4*lookup_tex_width+j*4+3]			=ozone_opt_len;
				}
				else
				{
					short *short_ptr=(short*)(char_ptr);
					short_ptr[0]	=ToFloat16(opt_len);
					short_ptr[1]	=ToFloat16(visibility);
					short_ptr[2]	=ToFloat16(haze);
					short_ptr[3]	=ToFloat16(ozone_opt_len);
					char_ptr+=bytes_per_texel;

				}
			}
			else
			{
				int int_opt_len=(int)opt_len;
				int int_haze=(int)(haze);
				int int_visibility=(int)(255.f*visibility);
				if(int_visibility>255)
					int_visibility=255;
				if(ozone_opt_len>255)
					ozone_opt_len=255;
				if(int_opt_len>255)
					int_opt_len=255;
				if(int_haze>255)
					int_haze=255;
				char_ptr[i*4*lookup_tex_width+j*4+2*(swap_rb)]=(unsigned char)int_opt_len;
				char_ptr[i*4*lookup_tex_width+j*4+1]=(unsigned char)int_visibility;
				char_ptr[i*4*lookup_tex_width+j*4+2*(!swap_rb)]=(unsigned char)int_haze;
				char_ptr[i*4*lookup_tex_width+j*4+3]=(unsigned char)ozone_opt_len;//int_dist;
			}
		}
	}
	return true;
}

bool simul::sky::TextureGenerator::Make2DSkyImage(const simul::sky::SkyInterface *sky,
	unsigned tex_height,unsigned tex_width,
	void *image_ptr,ColourFormat format,float exposure,bool use_floats,float altitude_km)
{
	float* output_colour=0;
	if(use_floats)
		output_colour=(float*)image_ptr;
	else
		output_colour=new float[4*tex_height*tex_width];
    float4* this_colour=(float4*)output_colour;
	float vertex_dir[4];
	for(unsigned i=0;i<tex_height;i++)
	{
		float sine=-1.f+((float)i/(float)(tex_height-1))*2.f;
		sine=std::max(sine,-1.f);
		sine=std::min(sine,1.f);
		float elevation=asin(-sine);
		for(unsigned j=0;j<tex_width;j++)
		{
			float x=(float)j/((float)tex_width);

			float azimuth=x*pi*2.f;

			vertex_dir[0]=-sin(azimuth)*cos(elevation);
			vertex_dir[1]= cos(azimuth)*cos(elevation);
			vertex_dir[2]= sin(elevation);

			vertex_dir[3]=0;
			*this_colour=sky->GetSkyColour(vertex_dir,altitude_km);
			(*this_colour)*=exposure;
			if(format==BGRA)
				std::swap(this_colour->x,this_colour->z);
			this_colour++;
		}
	}
	if(!use_floats)
	{
		unsigned char *char_ptr=(unsigned char *)image_ptr;
		for(unsigned i=0;i<tex_height*tex_width*4;i++)
		{
			int ival=(int)(255.f*output_colour[i]);
			if(ival>255)
				ival=255;
			unsigned char c=(unsigned char)ival;
			char_ptr[i]=c;
		}
		delete output_colour;
	}
	return true;
}

bool simul::sky::TextureGenerator::Make2DFastSky(const simul::sky::SkyInterface *sky,
	unsigned tex_height,unsigned tex_width,
	void *image_ptr,ColourFormat format,float exposure,float altitude_km,
				unsigned index,unsigned texels,bool use_floats,unsigned bytes_per_texel,
				float radius)
{
	bool use_float16=(use_floats&&bytes_per_texel==8);
	if(use_float16)
	{
		if(!table)
			initELut();
		table=true;
	}
	unsigned char *ptr=&(((unsigned char*)image_ptr)[bytes_per_texel*index]);
	float4 vertex_dir;
	float4 view_pos_km(0,0,altitude_km,0);

	unsigned i=index/tex_width;
	unsigned j=index-(i*tex_width);
	float sine=1.f-((float)i/(float)(tex_height-1))*2.f;
	sine=std::max(sine,-1.f);
	sine=std::min(sine,1.f);
	float elevation=asin(sine);

	if(!texels)
		texels=tex_width*tex_height;
	for(unsigned k=0;k<texels;k++,j++)
	{
		if(j>=tex_width)
		{
			j=0;
			i++;
			sine=1.f-((float)i/(float)(tex_height-1))*2.f;
			sine=std::max(sine,-1.f);
			sine=std::min(sine,1.f);
			elevation=asin(sine);
		}
		float4 this_colour;
		float x=(float)j/((float)tex_width);
		float azimuth=x*pi*2.f;
		vertex_dir.x=-sin(azimuth)*cos(elevation);
		vertex_dir.y= cos(azimuth)*cos(elevation);
		vertex_dir.z= sin(elevation);
		vertex_dir.w=0;
		float4 temp_view_pos_km=view_pos_km;
		float4 diff=vertex_dir;
		vertex_dir*=1000.f;
		if(radius)
		{
			diff*=radius;
			temp_view_pos_km+=diff;
		}
		this_colour=sky->GetIsotropicInscatterFactor(temp_view_pos_km.z,elevation,max_distance_km,true);

		//this_colour=sky->GetInscatterFactor(temp_view_pos,view_pos_km,true);
		this_colour*=exposure;
		if(format==BGRA)
			std::swap(this_colour.x,this_colour.z);
		if(use_float16)
		{
			short *short_ptr=(short*)(ptr);
			short_ptr[0]	=ToFloat16(this_colour.x);
			short_ptr[1]	=ToFloat16(this_colour.y);
			short_ptr[2]	=ToFloat16(this_colour.z);
			short_ptr[3]	=ToFloat16(this_colour.w);
		}
		else if(use_floats)
		{
			float *float_ptr=(float*)(ptr);
			float_ptr[0]	=this_colour.x;
			float_ptr[1]	=this_colour.y;
			float_ptr[2]	=this_colour.z;
			float_ptr[3]	=this_colour.w;
		}
		else
		{
			float4 *f4ptr=(float4*)(ptr);
			*f4ptr=this_colour;
		}
		ptr+=bytes_per_texel;
	}
	return true;
}

unsigned simul::sky::TextureGenerator::Get1DSkyChecksum(const simul::sky::SkyInterface *sky,float )
{
	unsigned checksum=sky->GetAtmosphericPropertiesChecksum();
	//checksum+=*reinterpret_cast<const unsigned*>(&alt_km);
	return checksum;
}

bool simul::sky::TextureGenerator::Make1DSunlightTexture(const simul::sky::SkyInterface *sky,
				const std::vector<float> &altitudes_km,
				void *image_ptr,ColourFormat format,float exposure,
				unsigned index,unsigned texels,unsigned bytes_per_texel)
{
	bool use_floats=true;
	bool use_float16=false;
	if(bytes_per_texel==8||bytes_per_texel==6)
		use_float16=true;
	else if(bytes_per_texel==16||bytes_per_texel==12)
		use_float16=false;
	else if(bytes_per_texel==4)
		use_floats=false;
	else
		throw simul::sky::TextureGenerator::BadParameter();

	if(use_float16)
	{
		if(!table)
			initELut();
		table=true;
	}
	unsigned char *ptr=&(((unsigned char*)image_ptr)[bytes_per_texel*index]);
	unsigned tex_width=(unsigned)altitudes_km.size();
	if(!texels)
		texels=tex_width;
	if(index>=tex_width)
		return true;
	if(index+texels>=tex_width)
		texels=tex_width-index;
	for(unsigned i=0;i<texels;i++)
	{
		float alt_km=altitudes_km[i];
		float4 this_colour=sky->GetLocalIrradiance(alt_km);
		this_colour.x*=exposure;
		this_colour.y*=exposure;
		this_colour.z*=exposure;
		if(format==BGRA)
			std::swap(this_colour.x,this_colour.z);
		if(use_float16)
		{
			short *short_ptr=(short*)(ptr);
			short_ptr[0]	=ToFloat16(this_colour.x);
			short_ptr[1]	=ToFloat16(this_colour.y);
			short_ptr[2]	=ToFloat16(this_colour.z);
			short_ptr[3]	=ToFloat16(this_colour.w);
		}
		else if(use_floats)
		{
			float *float_ptr=(float*)(ptr);
			float_ptr[0]	=this_colour.x;
			float_ptr[1]	=this_colour.y;
			float_ptr[2]	=this_colour.z;
			float_ptr[3]	=this_colour.w;
		}
		else
		{
			unsigned char *uc_ptr=(unsigned char *)(ptr);
			uc_ptr[0]	=(unsigned char)(255.f*this_colour.x);
			uc_ptr[1]	=(unsigned char)(255.f*this_colour.y);
			uc_ptr[2]	=(unsigned char)(255.f*this_colour.z);
			uc_ptr[3]	=(unsigned char)(255.f*this_colour.w);
		}
		ptr+=bytes_per_texel;
	}
	return true;
}

bool simul::sky::TextureGenerator::Make1DSkyTexture(const simul::sky::SkyInterface *sky,
				unsigned tex_width,
				void *image_ptr,ColourFormat format,float exposure,float altitude_km,
				unsigned index,unsigned texels,unsigned bytes_per_texel,
				float radius,bool earth_test,float )
{
	bool use_floats=true;
	bool use_float16=false;
	static int steps=0;
	if(bytes_per_texel==8||bytes_per_texel==6)
		use_float16=true;
	else if(bytes_per_texel==16||bytes_per_texel==12)
		use_float16=false;
	else if(bytes_per_texel==4)
		use_floats=false;
	else
		throw simul::sky::TextureGenerator::BadParameter();

	if(use_float16)
	{
		if(!table)
			initELut();
		table=true;
	}
	unsigned char *ptr=&(((unsigned char*)image_ptr)[bytes_per_texel*index]);
	float4 vertex_dir;
	float4 view_pos_km(0,0,altitude_km,0);

	unsigned j=index;
	float sine=-1.f+(((float)j-1.f)/(float)(tex_width-1))*2.f;
	sine=std::max(sine,-1.f);
	sine=std::min(sine,1.f);
	float elevation=asin(-sine);

	if(!texels)
		texels=tex_width;
	if(index>=tex_width)
		return true;
	if(index+texels>=tex_width)
		texels=tex_width-index;
	for(unsigned k=0;k<texels;k++,j++)
	{
		sine=1.f-(((float)j)/(float)(tex_width-1))*2.f;
		sine=std::max(sine,-1.f);
		sine=std::min(sine,1.f);
		elevation=asin(sine);
		if(j>=tex_width)
			j=0;
		float4 this_colour;
		float temp_view_alt_km=view_pos_km.z;
		if(radius)
		{
			temp_view_alt_km+=radius*sin(elevation);
		}
		this_colour=sky->GetIsotropicInscatterFactor(temp_view_alt_km,elevation,0.f,max_distance_km,earth_test,steps);


		this_colour.x*=exposure;
		this_colour.y*=exposure;
		this_colour.z*=exposure;
		if(format==BGRA)
			std::swap(this_colour.x,this_colour.z);
		if(use_float16)
		{
			short *short_ptr=(short*)(ptr);
			short_ptr[0]	=ToFloat16(this_colour.x);
			short_ptr[1]	=ToFloat16(this_colour.y);
			short_ptr[2]	=ToFloat16(this_colour.z);
			short_ptr[3]	=ToFloat16(this_colour.w);
		}
		else if(use_floats)
		{
			float *float_ptr=(float*)(ptr);
			float_ptr[0]	=this_colour.x;
			float_ptr[1]	=this_colour.y;
			float_ptr[2]	=this_colour.z;
			float_ptr[3]	=this_colour.w;
		}
		else
		{
			if(this_colour.x>1.f)
				this_colour.x=1.f;
			if(this_colour.y>1.f)
				this_colour.y=1.f;
			if(this_colour.z>1.f)
				this_colour.z=1.f;
			if(this_colour.w>1.f)
				this_colour.w=1.f;
			unsigned char *uc_ptr=(unsigned char *)(ptr);
			uc_ptr[0]	=(unsigned char)(255.f*this_colour.x);
			uc_ptr[1]	=(unsigned char)(255.f*this_colour.y);
			uc_ptr[2]	=(unsigned char)(255.f*this_colour.z);
			uc_ptr[3]	=(unsigned char)(255.f*this_colour.w);
		}
		ptr+=bytes_per_texel;
	}
	return true;
}

bool simul::sky::TextureGenerator::Make2DLossAndInscatterTextures(const simul::sky::SkyInterface *sky,
				unsigned tex_height,unsigned tex_width,
				void *loss_ptr,void *insc_ptr,void *sky_ptr,ColourFormat format,float altitude_km,float max_dist_km,
				unsigned index,unsigned texels,unsigned bytes_per_texel,bool earth_test,bool use_tbb)
{
	static bool force_infinite=false;
	bool use_floats=true;
	bool use_float16=false;
	if(bytes_per_texel==8||bytes_per_texel==6)
		use_float16=true;
	else if(bytes_per_texel==16||bytes_per_texel==12)
		use_float16=false;
	else if(bytes_per_texel==4)
		use_floats=false;
	else
		throw simul::sky::TextureGenerator::BadParameter();

	if(use_float16)
	{
		if(!table)
			initELut();
		table=true;
	}

	float4 vertex_dir;

	unsigned size=tex_height*tex_width;
	if(!texels)
		texels=size;
	if(index>=size)
		return true;
	if(index+texels>=size)
		texels=size-index;
	unsigned block_index=(index/tex_width);
	unsigned block_texels=(texels/tex_width);
	if(tex_width*(block_index+block_texels)<index+texels)
		block_texels++;
	if(block_texels<=0)
		return true;
	DoFade dl(sky,tex_width,tex_height,(unsigned char*)loss_ptr,(unsigned char*)insc_ptr,(unsigned char*)sky_ptr,altitude_km,max_dist_km,earth_test,format,use_float16,use_floats,bytes_per_texel);
#ifdef SIMUL_USE_TBB
	if(use_tbb)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(block_index,block_index+block_texels),dl,tbb::auto_partitioner());
	}
	else
	{
		dl(tbb::blocked_range<size_t>(block_index,block_index+block_texels)); 
	}
#else
		use_tbb;
		dl(block_index,block_index+block_texels); 
#endif
	return true;
}
