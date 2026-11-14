#ifndef SIMUL_CLOUDS_TEXTUREGENERATOR_H
#define SIMUL_CLOUDS_TEXTUREGENERATOR_H

#include "Simul/Clouds/Export.h"

namespace simul
{
	namespace math
	{
		class Vector3;
	}
	namespace graph
	{
		namespace meta
		{
			class Node;
		}
		namespace standardnodes
		{
			class ShowProgressInterface;
		}
	}
	namespace clouds
	{
		class CloudInterface;
		class LightningRenderInterface;
		//! This namespace provides functions to fill in linear textures with data from a cloud volume.
		namespace TextureGenerator
		{
			//! Textures are assumed to be 32 bits per texel. SetBits determines which bits will be filled.
			//! For a noise texture, \p x, \p y, and \p z are the 3 elements of the noise value to be returned, and \p w is unused.
			//! For a cloud texture, density is \p x, directional light brightness \p y, secondary light \p z, and ambient light \p w.
			//! \p Bytes_per_texel is the number of bytes per texel, e.g. 2 for a 16 bit texture, 4 for a 32 bit texture etc.
			//! \p bigendian specifies whether this platform stores numbers in reverse byte-order. The Xbox 360 and PS3 both do this,
			//! while the PC does not.
			void SIMUL_CLOUDS_EXPORT_FN SetBits(unsigned density,unsigned brightness,unsigned secondary,unsigned ambient,
		unsigned Bytes_per_texel,bool bigendian);
			//! For a 3D cloud texture, fill the specified number of \p texels, starting at the \p index texel, where \p image_ptr is the start
			//! of the texture in memory. If \p y_vertical is set, the height value is used for the y direction, otherwise,
			//! height is z.
			bool SIMUL_CLOUDS_EXPORT_FN PartialCloudTextureFill(simul::clouds::CloudInterface *ci,
					simul::graph::standardnodes::ShowProgressInterface *progress,
					unsigned index,
					unsigned texels,
					unsigned char *image_ptr,
					bool y_vertical,unsigned modified_height=0,bool blank_upper_and_lower_layers=true);
			//! For a 3D cloud texture, fill only density for the specified number of \p texels, starting at the \p index texel, where \p image_ptr is the start
			//! of the texture in memory.
			bool SIMUL_CLOUDS_EXPORT_FN PartialCloudTextureFillDensityOnly(simul::clouds::CloudInterface *ci,
					simul::graph::standardnodes::ShowProgressInterface *progress,
					unsigned index,
					unsigned texels,
					unsigned char *image_ptr,
					bool y_vertical,unsigned modified_height=0,bool blank_upper_and_lower_layers=true);
			//! For a 2D cloud texture, fill the specified number of \p texels, starting at the \p index texel, where \p image_ptr is the start
			//! of the texture in memory. If \p y_vertical is set, the height value is used for the y direction, otherwise,
			//! height is z.
			bool SIMUL_CLOUDS_EXPORT_FN Partial2DCloudTextureFill(simul::clouds::CloudInterface *ci,
					simul::graph::standardnodes::ShowProgressInterface *progress,
					unsigned index,
					unsigned texels,
					unsigned char *image_ptr);
			//!
			bool SIMUL_CLOUDS_EXPORT_FN PartialMake3DLightningTexture(
				simul::clouds::LightningRenderInterface *lri,unsigned source_index,
				unsigned tex_width,unsigned tex_length,unsigned tex_height,
				const simul::math::Vector3 &X1,const simul::math::Vector3 &X2,
				unsigned index,
				unsigned texels,
				unsigned char *image_ptr,
				bool use_threading=false);
			//! Fill a texture with light values due to lightning.
			bool SIMUL_CLOUDS_EXPORT_FN Make3DLightningTexture(
				simul::clouds::LightningRenderInterface *lri,
				unsigned tex_width,unsigned tex_length,unsigned tex_height,
				const simul::math::Vector3 &X1,const simul::math::Vector3 &X2,
				unsigned char *image_ptr,
				simul::graph::standardnodes::ShowProgressInterface *progress=0,
				simul::graph::meta::Node *sender=0,
				bool y_vertical=false);
			//! Create a 3D Perlin noise texture of size \p texSize, with a specified number of noise \p octaves, at memory location \p image_ptr.
			void SIMUL_CLOUDS_EXPORT_FN Make3DNoiseTexture(unsigned char *image_ptr,unsigned texSize,unsigned octaves,
				simul::graph::standardnodes::ShowProgressInterface *progress=0,simul::graph::meta::Node *sender=0);
			//! Create a 2D Perlin noise texture of size \p texSize, with a specified number of noise \p octaves, at memory location \p image_ptr.
			void SIMUL_CLOUDS_EXPORT_FN Make2DNoiseTexture(unsigned char *image_ptr,unsigned texSize,unsigned noiseSize,unsigned octaves,float persistence=0.5f,
				simul::graph::standardnodes::ShowProgressInterface *progress=0,simul::graph::meta::Node *sender=0);
			//! Create a 2D noise texture of size \p texSize, at memory location \p image_ptr.
			void SIMUL_CLOUDS_EXPORT_FN Make2DRandomTexture(unsigned char *image_ptr,unsigned texSize);
			void SIMUL_CLOUDS_EXPORT_FN Make2DCirrusNoiseTexture(unsigned char *image_ptr,unsigned texSize,unsigned noiseSize,unsigned octaves);
		}
	}
}
#endif
