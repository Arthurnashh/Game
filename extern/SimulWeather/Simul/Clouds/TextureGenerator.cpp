#include "precomp.h"

#include "Simul/Clouds/TextureGenerator.h"
#include "Simul/Clouds/CloudInterface.h"
#include "Simul/Clouds/LightningRenderInterface.h"
#include "Simul/Math/Noise3D.h"
#include "Simul/Math/Noise2D.h"
#include "Simul/Graph/StandardNodes/ShowProgressInterface.h"
#include "Simul/Math/Vector3.h"
#define pi (3.1415926536f)
#include <math.h>
#if defined(_MSC_VER)
	#if defined(_DLL) && !defined(XENON)&&!defined(_DEBUG)
		#define SIMUL_USE_TBB
	#endif
	#pragma warning(push)
	#pragma warning(disable:4512)
#endif



#ifdef SIMUL_USE_TBB
	#define  TBB_DO_THREADING_TOOLS 1
	#include "tbb/parallel_for.h" 
	#include "tbb/blocked_range.h"
#endif

using namespace simul::clouds;
unsigned channel_mult[4]={255,255,255,255};
unsigned channel_shift[4]={16,0,8,24};
unsigned total_bits=0;
size_t bytes_per_texel=4;
bool big_endian=false;

namespace simul
{
	namespace clouds
	{
		namespace TextureGenerator
		{
			bool PartialCloudTextureFill(simul::clouds::CloudInterface *ci
					,simul::graph::standardnodes::ShowProgressInterface *progress
					,unsigned index
					,unsigned texels
					,unsigned char *image_ptr
					,bool y_vertical
					,bool force_2d
					,unsigned
					,bool);
			bool PartialCloudTextureFillDensityOnly(simul::clouds::CloudInterface *ci
					,simul::graph::standardnodes::ShowProgressInterface *progress
					,unsigned index
					,unsigned texels
					,unsigned char *image_ptr
					,bool y_vertical
					,bool force_2d
					,unsigned
					,bool);
		}
	}
}

void GetMultAndShift(unsigned bits,unsigned &mult,unsigned &shift)
{
	if(!bits)
		return;
	unsigned temp_bits=bits;
	shift=0;
	while(!(temp_bits&1))
	{
		temp_bits=temp_bits>>1;
		shift++;
	}
	mult=temp_bits;
}

void simul::clouds::TextureGenerator::SetBits(unsigned density,unsigned brightness,unsigned secondary,unsigned ambient,
		unsigned Bytes_per_texel,bool bigendian)
{
	big_endian=bigendian;
	GetMultAndShift(density		,channel_mult[0]	,channel_shift[0]);
	GetMultAndShift(brightness	,channel_mult[1]	,channel_shift[1]);
	GetMultAndShift(secondary	,channel_mult[2]	,channel_shift[2]);
	GetMultAndShift(ambient		,channel_mult[3]	,channel_shift[3]);
	total_bits=~(density|brightness|secondary|ambient);
	bytes_per_texel=Bytes_per_texel;
}

static float lerp(float l,float h,float f)
{
	return l+((h-l)*f);
}

namespace simul
{
	namespace clouds
	{
		class FillTexels
		{ 
			simul::clouds::CloudInterface *ci;
			simul::graph::meta::Node *progress_sender;
			simul::graph::standardnodes::ShowProgressInterface *progress;
			unsigned char *image;
			bool y_vertical;
			bool force_2d;
			bool blank_upper_and_lower_layers;
			unsigned modified_height;
		public: 
#ifdef SIMUL_USE_TBB
			void operator()( const tbb::blocked_range<size_t>& r ) const
			{
				const size_t &begin=r.begin();
				const size_t &end=r.end();
#else
			void operator()( size_t begin,size_t end ) const
			{
#endif
				unsigned char *image_ptr=image;
				simul::math::Vector3 X1,X2;
				ci->GetExtents(X1,X2);
				unsigned tex_width,tex_length,tex_height,grid_height;
				ci->GetGrid(tex_width,tex_length,grid_height);
				int height_offset=0;
				tex_height=grid_height;
				if(modified_height&&modified_height>tex_height)
				{
					height_offset=tex_height-modified_height+1;
					tex_height=modified_height;
				}
				if(force_2d)
					tex_height=1;
				if(y_vertical)
					std::swap(tex_height,tex_length);
				image_ptr+=begin*bytes_per_texel;
				size_t total_size=tex_width*tex_length*tex_height;
				if(begin>=total_size)
					return;
				for(unsigned b=(unsigned)begin;b!=(unsigned)end;++b )
				{
					int i=b/(tex_width*tex_length);
					unsigned j=(b-i*(tex_width*tex_length))/tex_width;
					unsigned k=(b-i*(tex_width*tex_length))-j*tex_width;
					if(progress&&!begin&&!j)
					{
						progress->SetProgress(progress_sender,(float)i/(tex_height-1.f));
					}
					float amb_f,bri_f,sec_f,den_f;
					ci->GetGridValues(den_f,bri_f,amb_f,sec_f,k,j,height_offset+i);
					if(height_offset!=0)
					{
						if(i==(int)tex_height-1)
							bri_f=sec_f=1.f;
						if(i==0)
							bri_f=sec_f=0;
					}

					unsigned amb=(unsigned)((float)channel_mult[3]*(amb_f));
					unsigned br=(unsigned)((float)channel_mult[1]*bri_f);
					unsigned sec=(unsigned)((float)channel_mult[2]*sec_f);
					unsigned dens=0;
					if(!blank_upper_and_lower_layers||((!y_vertical&&(i>0&&i+height_offset<(int)grid_height-1))
						||(y_vertical&&(j>0&&j+height_offset<grid_height-1))))
						dens=(unsigned)((float)channel_mult[0]*den_f); // we don't want anything at the edges, it's glitchy!
					if(dens>channel_mult[0])
						dens=channel_mult[0];
					if(amb>channel_mult[3])
						amb=channel_mult[3];
					if(br>channel_mult[1])
						br=channel_mult[1];
					if(sec>channel_mult[2])
						sec=channel_mult[2];
					unsigned u=(dens<<channel_shift[0]);
					u|=(amb<<channel_shift[3]);
					u|=(sec<<channel_shift[2]);
					u|=(br<<channel_shift[1]);
					unsigned char *src_ptr=(unsigned char *)&u;
					unsigned char *dst_ptr=image_ptr;
					if(big_endian)
					{
						src_ptr+=4-bytes_per_texel;
					}
					for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
						*dst_ptr=*src_ptr;
					image_ptr+=bytes_per_texel;
				}
			} 
			FillTexels( simul::clouds::CloudInterface *c
					,simul::graph::standardnodes::ShowProgressInterface *p
					,unsigned char *ip,
					bool yv,bool f2d,unsigned mh,bool bl)
				: ci(c)
				, progress(p)
				, image(ip)
				, y_vertical(yv)
				, force_2d(f2d)
				, modified_height(mh)
				, blank_upper_and_lower_layers(bl)
			{
				progress_sender=dynamic_cast<simul::graph::meta::Node *>(ci);
			}
		};

		
		class FillTexelsDensityOnly
		{ 
			simul::clouds::CloudInterface *ci;
			simul::graph::meta::Node *progress_sender;
			simul::graph::standardnodes::ShowProgressInterface *progress;
			unsigned char *image;
			bool y_vertical;
			bool force_2d;
			bool blank_upper_and_lower_layers;
			unsigned modified_height;
		public: 
#ifdef SIMUL_USE_TBB
			void operator()( const tbb::blocked_range<size_t>& r ) const
			{
				const size_t &begin=r.begin();
				const size_t &end=r.end();
#else
			void operator()( size_t begin,size_t end ) const
			{
#endif
				unsigned char *image_ptr=image;
				simul::math::Vector3 X1,X2;
				ci->GetExtents(X1,X2);
				unsigned tex_width,tex_length,tex_height,grid_height;
				ci->GetGrid(tex_width,tex_length,grid_height);
				int height_offset=0;
				tex_height=grid_height;
				if(modified_height&&modified_height>tex_height)
				{
					height_offset=tex_height-modified_height+1;
					tex_height=modified_height;
				}
				if(force_2d)
					tex_height=1;
				if(y_vertical)
					std::swap(tex_height,tex_length);
				image_ptr+=begin*bytes_per_texel;
				size_t total_size=tex_width*tex_length*tex_height;
				if(begin>=total_size)
					return;
				for(unsigned b=(unsigned)begin;b!=(unsigned)end;++b )
				{
					int i=b/(tex_width*tex_length);
					unsigned j=(b-i*(tex_width*tex_length))/tex_width;
					unsigned k=(b-i*(tex_width*tex_length))-j*tex_width;
					if(progress&&!begin&&!j)
					{
						progress->SetProgress(progress_sender,(float)i/(tex_height-1.f));
					}
					float den_f=ci->GetCloud(k,j,height_offset+i);

					unsigned dens=0;
					if(!blank_upper_and_lower_layers||((!y_vertical&&(i>0&&i+height_offset<(int)grid_height-1))
						||(y_vertical&&(j>0&&j+height_offset<grid_height-1))))
						dens=(unsigned)((float)channel_mult[0]*den_f); // we don't want anything at the edges, it's glitchy!
					if(dens>channel_mult[0])
						dens=channel_mult[0];
					unsigned u=(dens<<channel_shift[0]);
					unsigned char *src_ptr=(unsigned char *)&u;
					unsigned char *dst_ptr=image_ptr;
					if(big_endian)
					{
						src_ptr+=4-bytes_per_texel;
					}
					for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
						*dst_ptr=*src_ptr;
					image_ptr+=bytes_per_texel;
				}
			} 
			FillTexelsDensityOnly( simul::clouds::CloudInterface *c
					,simul::graph::standardnodes::ShowProgressInterface *p
					,unsigned char *ip,
					bool yv,bool f2d,unsigned mh,bool bl)
				: ci(c)
				, progress(p)
				,progress_sender(NULL)
				, image(ip)
				, y_vertical(yv)
				, force_2d(f2d)
				, modified_height(mh)
				, blank_upper_and_lower_layers(bl)
			{
				if(progress)
					progress_sender=dynamic_cast<simul::graph::meta::Node *>(ci);
			}
		};
	}
}
bool simul::clouds::TextureGenerator::PartialCloudTextureFill(
					simul::clouds::CloudInterface *ci
					,simul::graph::standardnodes::ShowProgressInterface *progress
					,unsigned index
					,unsigned texels
					,unsigned char *image_ptr
					,bool y_vertical
					,bool force_2d
					,unsigned modified_height
					,bool blank_upper_and_lower_layers)
{
	simul::math::Vector3 X1,X2;
	ci->GetExtents(X1,X2);
	unsigned tex_width,tex_length,tex_height,grid_height;
	ci->GetGrid(tex_width,tex_length,grid_height);
	int height_offset=0;
	tex_height=grid_height;
	if(modified_height&&modified_height>tex_height)
	{
		height_offset=tex_height-modified_height+1;
		tex_height=modified_height;
	}
	if(force_2d)
		tex_height=1;
	if(y_vertical)
		std::swap(tex_height,tex_length);
	unsigned total_size=tex_width*tex_length*tex_height;
	if(index>=total_size)
		return true;
	if(texels>total_size)
		texels=total_size;
	if(index+texels>total_size)
		texels=total_size-index;
#ifdef SIMUL_USE_TBB
	if(ci->GetUseTbb())
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(index,index+texels),
			FillTexels(ci,progress,image_ptr,y_vertical,force_2d,modified_height,blank_upper_and_lower_layers),
			tbb::auto_partitioner()); 
	}
	else
	{
		FillTexels ft(ci,progress,image_ptr,y_vertical,force_2d,modified_height,blank_upper_and_lower_layers);
		tbb::blocked_range<size_t> b(index,index+texels);
		ft(b);
	}
#else
	FillTexels ft(ci,progress,image_ptr,y_vertical,force_2d,modified_height,blank_upper_and_lower_layers);
	ft(index,index+texels);
#endif
	return true;
}
bool simul::clouds::TextureGenerator::PartialCloudTextureFillDensityOnly(
					simul::clouds::CloudInterface *ci
					,simul::graph::standardnodes::ShowProgressInterface *progress
					,unsigned index
					,unsigned texels
					,unsigned char *image_ptr
					,bool y_vertical
					,bool force_2d
					,unsigned modified_height
					,bool blank_upper_and_lower_layers)
{
	simul::math::Vector3 X1,X2;
	ci->GetExtents(X1,X2);
	unsigned tex_width,tex_length,tex_height,grid_height;
	ci->GetGrid(tex_width,tex_length,grid_height);
	int height_offset=0;
	tex_height=grid_height;
	if(modified_height&&modified_height>tex_height)
	{
		height_offset=tex_height-modified_height+1;
		tex_height=modified_height;
	}
	if(force_2d)
		tex_height=1;
	if(y_vertical)
		std::swap(tex_height,tex_length);
	unsigned total_size=tex_width*tex_length*tex_height;
	if(index>=total_size)
		return true;
	if(texels>total_size)
		texels=total_size;
	if(index+texels>total_size)
		texels=total_size-index;
#ifdef SIMUL_USE_TBB
	if(ci->GetUseTbb())
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(index,index+texels),
			FillTexelsDensityOnly(ci,progress,image_ptr,y_vertical,force_2d,modified_height,blank_upper_and_lower_layers),
			tbb::auto_partitioner()); 
	}
	else
	{
		FillTexelsDensityOnly ft(ci,progress,image_ptr,y_vertical,force_2d,modified_height,blank_upper_and_lower_layers);
		tbb::blocked_range<size_t> b(index,index+texels);
		ft(b);
	}
#else
	FillTexelsDensityOnly ft(ci,progress,image_ptr,y_vertical,force_2d,modified_height,blank_upper_and_lower_layers);
	ft(index,index+texels);
#endif
	return true;
}

bool simul::clouds::TextureGenerator::PartialCloudTextureFill(simul::clouds::CloudInterface *ci,
					simul::graph::standardnodes::ShowProgressInterface *progress,
					unsigned index,
					unsigned texels,
					unsigned char *image_ptr,
					bool y_vertical,unsigned modified_height,bool blank_upper_and_lower_layers)
{
	return PartialCloudTextureFill(ci,progress,index,texels,image_ptr,y_vertical,false,modified_height,blank_upper_and_lower_layers);
}


bool simul::clouds::TextureGenerator::PartialCloudTextureFillDensityOnly(simul::clouds::CloudInterface *ci,
					simul::graph::standardnodes::ShowProgressInterface *progress,
					unsigned index,
					unsigned texels,
					unsigned char *image_ptr,
					bool y_vertical,unsigned modified_height,bool blank_upper_and_lower_layers)
{
	return PartialCloudTextureFillDensityOnly(ci,progress,index,texels,image_ptr,y_vertical,false,modified_height,blank_upper_and_lower_layers);
}
bool simul::clouds::TextureGenerator::Partial2DCloudTextureFill(simul::clouds::CloudInterface *ci,
					simul::graph::standardnodes::ShowProgressInterface *progress,
					unsigned index,
					unsigned texels,
					unsigned char *image_ptr)
{
	simul::math::Vector3 X1;
	simul::math::Vector3 X2;
	ci->GetExtents(X1,X2);
	unsigned tex_width,tex_length,tex_height,grid_height;
	simul::graph::meta::Node *progress_sender=dynamic_cast<simul::graph::meta::Node*>(ci);
	ci->GetGrid(tex_width,tex_length,grid_height);
	tex_height=1;
	simul::math::Vector3 X(0,0,0);
	float thickness=(X2.z-X1.z)*0.001f*ci->GetOpticalDensity();
	image_ptr+=index*bytes_per_texel;
	size_t total_size=tex_width*tex_length;
	if(index>=total_size)
		return true;
	if(index+texels>total_size)
		texels=total_size-index;
	if(texels>total_size)
		return true;
	for(unsigned a=0;a<texels;a++)
	{
		unsigned b=a+index;
		unsigned j=b/tex_width;
		unsigned k=b-j*tex_width;
		if(!k||!a)
		{
			float y_interp=(float)j/(float)(tex_length);
			X.y=lerp(X1.y,X2.y,y_interp);
			if(progress)
				progress->SetProgress(progress_sender,y_interp);
		}
		X.x=lerp(X1.x,X2.x,(float)k/(float)(tex_width));
		float inv_opac=1.f;
		static float ff=0.f;
		float amb_f=ff,bri_f=ff,sec_f=ff,den_f=0.f;
		for(unsigned i=grid_height-1;i<grid_height;i--)
		{
			float ii=1.f/(float)grid_height;
			float b,a,s;
			float opacity;
			ci->GetGridValues(opacity,b,a,s,k,j,i);
			float ext=1.f-exp(-ii*opacity*thickness);
			inv_opac	*=1.f-ext;
			ext=1.f-exp(-ii*thickness);
			amb_f		*=1.f-ext;
			bri_f		*=1.f-ext;
			sec_f		*=1.f-ext;
			den_f		*=1.f-ext;
			den_f+=opacity;
			amb_f+=a;
			bri_f+=b;
			sec_f+=s;
		}
		static bool div=false;
		if(div&&inv_opac<1.f)
		{
			amb_f		/=(1.f-inv_opac);
			bri_f		/=(1.f-inv_opac);
			sec_f		/=(1.f-inv_opac);
			den_f		/=(1.f-inv_opac);
		}
		unsigned amb=(unsigned)((float)channel_mult[3]*amb_f);
		unsigned br=(unsigned)((float)channel_mult[1]*bri_f);
		unsigned sec=(unsigned)((float)channel_mult[2]*sec_f);
		unsigned dens=(unsigned)((float)channel_mult[0]*(den_f));
		if(dens>channel_mult[0])
			dens=channel_mult[0];
		if(amb>channel_mult[3])
			amb=channel_mult[3];
		if(br>channel_mult[1])
			br=channel_mult[1];
		if(sec>channel_mult[2])
			sec=channel_mult[2];
		unsigned u=(dens<<channel_shift[0]);
		u|=(amb<<channel_shift[3]);
		u|=(sec<<channel_shift[2]);
		u|=(br<<channel_shift[1]);
		unsigned char *src_ptr=(unsigned char *)&u;
		unsigned char *dst_ptr=image_ptr;
		if(big_endian)
		{
			src_ptr+=4-bytes_per_texel;
		}
		for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
			*dst_ptr=*src_ptr;
		image_ptr+=bytes_per_texel;
	}
	return true;
}



namespace simul
{
	namespace clouds
	{
		class FillLightningTexels
		{
			simul::clouds::LightningRenderInterface *lri;
			unsigned char *image;
			unsigned source_index;
			unsigned tex_width,tex_length,tex_height;
			const simul::math::Vector3 &X1;
			const simul::math::Vector3 &X2;
			unsigned index;
			unsigned texels;
			unsigned char *image_ptr;
		public: 
#ifdef SIMUL_USE_TBB
			void operator()( const tbb::blocked_range<size_t>& r ) const
			{
				const unsigned &begin=r.begin();
				const unsigned &end=r.end();
#else
			void operator()(unsigned begin,unsigned end) const
			{ 
#endif
				unsigned char *image_ptr=image;
				if(source_index>=lri->GetNumLightSources())
					return;
				simul::math::Vector3 X(0,0,0);
				size_t total_size=tex_width*tex_length*tex_height;
				if(index>=total_size)
					return;
				image_ptr+=begin*bytes_per_texel;
				if(begin>=total_size)
					return;
				for(unsigned b=begin;b!=end;++b )
				{
					unsigned a=(b-begin);
					unsigned i=b/(tex_width*tex_length);
					unsigned j=(b-i*(tex_width*tex_length))/tex_width;
					unsigned k=(b-i*(tex_width*tex_length))-j*tex_width;
					if(!k||!a)
					{
						if(!j||!a)
						{
							X.z=lerp(X1.z,X2.z,((float)i+0.5f)/(float)(tex_height));
						}
						X.y=lerp(X1.y,X2.y,((float)j+0.5f)/(float)(tex_length));
					}
					X.x=lerp(X1.x,X2.x,((float)k+0.5f)/(float)(tex_width));
					float light=lri->GetLightAt(source_index,X);
					unsigned x=(unsigned)(channel_mult[source_index]);
					x=x<<channel_shift[source_index];
					x=~x;
					unsigned uval=(unsigned)((float)channel_mult[source_index]	*light);
					unsigned u=0;
					u=(uval<<channel_shift[source_index]);
					unsigned char *mask_ptr=(unsigned char *)&x;
					unsigned char *src_ptr=(unsigned char *)&u;
					unsigned char *dst_ptr=image_ptr;
					if(big_endian)
					{
						src_ptr+=4-bytes_per_texel;
					}
					for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++,mask_ptr++)
					{
						*dst_ptr&=(*mask_ptr);
						*dst_ptr|=(*src_ptr);
					}
					image_ptr+=bytes_per_texel;
				}
			} 
			FillLightningTexels(simul::clouds::LightningRenderInterface *lr,
					unsigned char *im,
					unsigned si,
					unsigned w,
					unsigned l,
					unsigned h,
					const simul::math::Vector3 &x1,
					const simul::math::Vector3 &x2,
					unsigned in,
					unsigned tx)
				: 
					lri(lr),
					image(im),
					source_index(si),
					tex_width(w),
					tex_length(l),
					tex_height(h),
					X1(x1),
					X2(x2),
					index(in),
					texels(tx)
			{} 
		};
	}
}

bool simul::clouds::TextureGenerator::PartialMake3DLightningTexture(
	simul::clouds::LightningRenderInterface *lri,unsigned source_index,
	unsigned tex_width,unsigned tex_length,unsigned tex_height,
	const simul::math::Vector3 &X1,const simul::math::Vector3 &X2,
	unsigned index,
	unsigned texels,
	unsigned char *image_ptr,
#ifdef SIMUL_USE_TBB
	bool use_threading
#else
	bool
#endif
	)
{
	if(source_index>=lri->GetNumLightSources())
		return true;
	simul::math::Vector3 X(0,0,0);
	unsigned total_size=tex_width*tex_length*tex_height;
	if(index>=total_size)
		return true;
	if(texels>total_size)
		texels=total_size;
	if(index+texels>total_size)
		texels=total_size-index;
#ifdef SIMUL_USE_TBB
	if(use_threading)
	{
		tbb::parallel_for(tbb::blocked_range<size_t>(index,index+texels),
			FillLightningTexels(lri,image_ptr,
					source_index,
					tex_width,tex_length,tex_height,
					X1,
					X2,
					index,
					texels),
			tbb::auto_partitioner()); 
	}
	else
#endif
	{
		FillLightningTexels ft(lri,image_ptr,
					source_index,
					tex_width,tex_length,tex_height,
					X1,
					X2,
					index,
					texels);
#ifdef SIMUL_USE_TBB
		tbb::blocked_range<size_t> b(index,index+texels);
		ft(b);
#else
		ft(index,index+texels);
#endif
	}
	index+=texels;
	return true;
}
bool simul::clouds::TextureGenerator::Make3DLightningTexture(
	simul::clouds::LightningRenderInterface *lri,
	unsigned tex_width,unsigned tex_length,unsigned tex_height,
	const simul::math::Vector3 &X1,const simul::math::Vector3 &X2,
	unsigned char *image_ptr,
	simul::graph::standardnodes::ShowProgressInterface *progress,
	simul::graph::meta::Node *sender,
	bool y_vertical)
{
	simul::math::Vector3 X(0,0,0);
	unsigned index=0;
	if(y_vertical)
		std::swap(tex_height,tex_length);
	if(progress)
		progress->SetText(sender,"Making 3D Cloud Texture");
	for(unsigned i=0;i<tex_height;i++)
	{
		float interp_i=(float)i/(float)(tex_height);
		if(progress)
		{
			progress->SetProgress(sender,interp_i);
			if(progress->GetCancel(sender))
				break;
		}
		if(!y_vertical)
			X.z=lerp(X1.z,X2.z,interp_i);
		else
			X.y=lerp(X1.y,X2.y,interp_i);
		for(unsigned j=0;j<tex_length;j++)
		{
			if(!y_vertical)
				X.y=lerp(X1.y,X2.y,(float)j/(float)(tex_length));
			else
				X.z=lerp(X1.z,X2.z,(float)j/(float)(tex_length));
			for(unsigned k=0;k<tex_width;k++,index++)
			{
				X.x=lerp(X1.x,X2.x,(float)k/(float)(tex_width));
				float light[4];
				for(unsigned l=0;l<4;l++)
				{
					if(l<lri->GetNumLightSources())
						light[l]=lri->GetLightAt(l,X);
					else
						light[l]=0;
				}

				unsigned r=(unsigned)((float)channel_mult[0]	*light[0]);
				unsigned g=(unsigned)((float)channel_mult[1]	*light[1]);
				unsigned b=(unsigned)((float)channel_mult[2]	*light[2]);
				unsigned a=(unsigned)((float)channel_mult[3]	*light[3]);

				unsigned u;
				u =(r<<channel_shift[0]);
				u|=(g<<channel_shift[1]);
				u|=(b<<channel_shift[2]);
				u|=(a<<channel_shift[3]);

				unsigned char *src_ptr=(unsigned char *)&u;
				unsigned char *dst_ptr=image_ptr;
				if(big_endian)
				{
					src_ptr+=4-bytes_per_texel;
				}
				for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
					*dst_ptr=*src_ptr;
				image_ptr+=bytes_per_texel;
			}
		}
	}
	if(progress)
		progress->SetProgressComplete(sender);
	return true;
}

void simul::clouds::TextureGenerator::Make3DNoiseTexture(unsigned char *image_ptr,unsigned texSize,unsigned octaves,
														 simul::graph::standardnodes::ShowProgressInterface *progress,
														simul::graph::meta::Node *sender)
{
	simul::math::Noise3D noise1;
	simul::math::Noise3D noise2;
	simul::math::Noise3D noise3;
	noise1.Setup(texSize,136,1);
	noise2.Setup(texSize,12346,1);
	noise3.Setup(texSize,12,1);
	if(progress)
		progress->SetText(sender,"Making 3D Noise Texture");
	for (unsigned i = 0; i < texSize; ++i)
	{
		if(progress)
			progress->SetProgress(sender,(float)i/(float)texSize);
		for (unsigned j = 0; j < texSize; ++j)
		{
			for (unsigned k = 0; k < texSize; ++k)
			{
				float ii=((float)i+.5f)/(float)texSize;
				float jj=((float)j+.5f)/(float)texSize;
				float kk=((float)k+.5f)/(float)texSize;

				float x=0,y=0,z=0;
				float scale=.5f,total=0;

				for(unsigned l=0;l<octaves;l++)
				{
					float rnd1=noise1.PerlinNoise3D(ii,jj,kk);
					float rnd2=noise2.PerlinNoise3D(ii,jj,kk);
					float rnd3=noise3.PerlinNoise3D(ii,jj,kk);

					float r=1.f-pow(rnd1,4.f);
					float az=rnd2*2*3.1415926536f;
					float el=asin(rnd3*2.f-1.f);
					x+=scale*(r*sin(az)*cos(el));	//from 0 to 1;
					y+=scale*(r*cos(az)*cos(el));	//FRand()*2.f-1.f;
					z+=scale*(r*sin(el));			//FRand()*2.f-1.f;
					ii*=2.f;
					jj*=2.f;
					kk*=2.f;
					total+=scale;
					scale*=.5f;
				}
				x/=total;
				y/=total;
				z/=total;
				x+=1.f;
				y+=1.f;
				z+=1.f;
				x*=0.5f;
				y*=0.5f;
				z*=0.5f;

				unsigned r=(unsigned)((float)channel_mult[0]*x);
				unsigned g=(unsigned)((float)channel_mult[1]*y);
				unsigned b=(unsigned)((float)channel_mult[2]*z);

				unsigned u;
				u =(r<<channel_shift[0]);
				u|=(g<<channel_shift[1]);
				u|=(b<<channel_shift[2]);

				unsigned char *src_ptr=(unsigned char *)&u;
				unsigned char *dst_ptr=image_ptr;
				if(big_endian)
				{
					src_ptr+=4-bytes_per_texel;
				}
				for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
					*dst_ptr=*src_ptr;
				image_ptr+=bytes_per_texel;
			}
		}
	}
	if(progress)
		progress->SetProgressComplete(sender);
}

void simul::clouds::TextureGenerator::Make2DRandomTexture(unsigned char *image_ptr,unsigned texSize)
{
	simul::math::Noise2D noise1;
	simul::math::Noise2D noise2;
	simul::math::Noise2D noise3;
	simul::math::Noise2D noise4;
	noise1.Setup(texSize,61,1,1.f);
	noise2.Setup(texSize,691,1,1.f);
	noise3.Setup(texSize,7241,1,1.f);
	noise4.Setup(texSize,45161,1,1.f);

	for (int i = 0; i <(int)texSize; ++i)
	{
		for (int j = 0; j <(int)texSize; ++j)
		{
//			float ii=((float)i+.5f)/(float)texSize;
//			float jj=((float)j+.5f)/(float)texSize;

			float x=noise1.PerlinNoise2D(i,j);
			float y=noise2.PerlinNoise2D(i,j);
			float z=noise3.PerlinNoise2D(i,j);
			float w=noise4.PerlinNoise2D(i,j);

			unsigned r=(unsigned)((float)channel_mult[0]*x);
			unsigned g=(unsigned)((float)channel_mult[1]*y);
			unsigned b=(unsigned)((float)channel_mult[2]*z);
			unsigned a=(unsigned)((float)channel_mult[3]*w);

			unsigned u;
			u =(r<<channel_shift[0]);
			u|=(g<<channel_shift[1]);
			u|=(b<<channel_shift[2]);
			u|=(a<<channel_shift[3]);

			unsigned char *src_ptr=(unsigned char *)&u;
			unsigned char *dst_ptr=image_ptr;
			if(big_endian)
			{
				src_ptr+=4-bytes_per_texel;
			}
			for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
				*dst_ptr=*src_ptr;
			image_ptr+=bytes_per_texel;
		}
	}
}
void simul::clouds::TextureGenerator::Make2DNoiseTexture(unsigned char *image_ptr,
							unsigned texSize,unsigned noiseSize,unsigned octaves,float persistence,
							simul::graph::standardnodes::ShowProgressInterface *progress,
							simul::graph::meta::Node *sender)
{
	static float avg_z;
	avg_z=0.f;
	int n=0;
	simul::math::Noise2D noise1;
	simul::math::Noise2D noise2;
	simul::math::Noise2D noise3;
	noise1.Setup(noiseSize,36	,1,persistence);
	noise2.Setup(noiseSize,2346,1,persistence);
	noise3.Setup(noiseSize,2	,1,persistence);

	if(progress)
		progress->SetText(sender,"Making 2D Noise Texture");
	for (unsigned i = 0; i < texSize; ++i)
	{
		if(progress)
			progress->SetProgress(sender,(float)i/(float)texSize);
		for (unsigned j = 0; j < texSize; ++j)
		{
			float ii=((float)i+.5f)/(float)texSize;
			float jj=((float)j+.5f)/(float)texSize;

			float x=0,y=0,z=0;
			float scale=.5f,total=0;

			for(unsigned l=0;l<octaves;l++)
			{
				float rnd1=noise1.PerlinNoise2D(ii,jj);
				float rnd2=noise2.PerlinNoise2D(ii,jj);
				float rnd3=noise3.PerlinNoise2D(ii,jj);

				float r=1.f-pow(rnd1,4.f);
				float az=rnd2*2*3.1415926536f;
				float el=asin(rnd3*2.f-1.f);
				x+=scale*(r*sin(az)*cos(el));	//from 0 to 1;
				y+=scale*(r*cos(az)*cos(el));	//FRand()*2.f-1.f;
				z+=scale*(r*sin(el));			//FRand()*2.f-1.f;
				ii*=2.f;
				jj*=2.f;
				total+=scale;
				scale*=persistence;
			}
			x/=total;
			y/=total;
			z/=total;
			x+=1.f;
			y+=1.f;
			z+=1.f;
			x*=0.5f;
			y*=0.5f;
			z*=0.5f;
avg_z+=z;
n++;

			unsigned r=(unsigned)((float)channel_mult[0]*x);
			unsigned g=(unsigned)((float)channel_mult[1]*y);
			unsigned b=(unsigned)((float)channel_mult[2]*z);

			unsigned u;
			u =(r<<channel_shift[0]);
			u|=(g<<channel_shift[1]);
			u|=(b<<channel_shift[2]);

			unsigned char *src_ptr=(unsigned char *)&u;
			unsigned char *dst_ptr=image_ptr;
			if(big_endian)
			{
				src_ptr+=4-bytes_per_texel;
			}
			for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
				*dst_ptr=*src_ptr;
			image_ptr+=bytes_per_texel;
		}
	}
	avg_z/=(float)n;
	if(progress)
		progress->SetProgressComplete(sender);
}

void simul::clouds::TextureGenerator::Make2DCirrusNoiseTexture(unsigned char *image_ptr,
							unsigned texSize,unsigned noiseSize,unsigned octaves)
{
	simul::math::Noise2D noise1;
	simul::math::Noise2D noise2;
	simul::math::Noise2D noise3;
	noise1.Setup(noiseSize,136,1);
	noise2.Setup(noiseSize,12346,1);
	noise3.Setup(noiseSize,12,1);

	for (unsigned i = 0; i < texSize; ++i)
	{
		for (unsigned j = 0; j < texSize; ++j)
		{
			float ii=((float)i+.5f)/(float)texSize;
			float jj=((float)j+.5f)/(float)texSize;

			float x=0,y=0,z=0;
			float scale=.5f,total=0;

			for(unsigned l=0;l<octaves;l++)
			{
				float rnd1=noise1.PerlinNoise2D(ii,jj);
				float rnd2=noise2.PerlinNoise2D(ii,jj);
				float rnd3=noise3.PerlinNoise2D(ii,jj);

				float r=1.f-pow(rnd1,4.f);
				float az=rnd2*2*3.1415926536f;
				float el=asin(rnd3*2.f-1.f);
				x+=scale*(r*sin(az)*cos(el));	//from 0 to 1;
				y+=scale*r*cos(az)*cos(el);		//FRand()*2.f-1.f;
				z+=scale*r*sin(el);				//FRand()*2.f-1.f;
				ii*=2.f;
				jj*=2.f;
				total+=scale;
				scale*=.5f;
			}
			x/=total;
			y/=total;
			z/=total;
			x+=1.f;
			y+=1.f;
			z+=1.f;
			static float ff=2.f;
			static float cc=.5f;
			float offset=cc*sqrt(ff*ff-x*x)/ff;
			y*=1.f-offset;
			y+=cc-offset;
			x*=0.5f;
			y*=0.5f;
			z*=0.5f;

			unsigned r=(unsigned)((float)channel_mult[0]*x);
			unsigned g=(unsigned)((float)  channel_mult[1]*y);
			unsigned b=(unsigned)((float) channel_mult[2]*z);

			unsigned u;
			u =(r<<channel_shift[0]);
			u|=(g<<channel_shift[1]);
			u|=(b<<channel_shift[2]);

			unsigned char *src_ptr=(unsigned char *)&u;
			unsigned char *dst_ptr=image_ptr;
			if(big_endian)
			{
				src_ptr+=4-bytes_per_texel;
			}
			for(size_t l=0;l<bytes_per_texel;l++,dst_ptr++,src_ptr++)
				*dst_ptr=*src_ptr;
			image_ptr+=bytes_per_texel;
		}
	}
}

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif