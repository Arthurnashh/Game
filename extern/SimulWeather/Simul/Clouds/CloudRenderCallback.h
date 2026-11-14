#ifndef SIMUL_CLOUDRENDERCALLBACK_H
#define SIMUL_CLOUDRENDERCALLBACK_H
namespace simul
{
	namespace clouds
	{
		//! A virtual callback class that simul::clouds::CloudKeyframer uses to fill in cloud textures. To make use of CloudKeyframer,
		//! you should derive a class from CloudRenderCallback, and implement its functions.
		class CloudRenderCallback
		{
		public:
//! Inform the derived class of the cloud volumetric texture size. All three cloud textures should be sized to these parameters.
			virtual void SetCloudTextureSize(unsigned width_x,unsigned length_y,unsigned depth_z)=0;
//! Inform the derived class that the texture \a texture_index, either 0, 1 or 2, should have \a num_texels texels
//! filled starting at \a texel_index, with the values in the array \a uint32_array, which contains
//! four times \a num_texels unsigned ints, or shorts - depending on the Use16Bit setting in the keyframer.
//! The first number is the value for \a texel_index, rather than the start of the whole table.
//! Each group of four represents the r,g,b and a of a colour value for the texture.
//! Only one of FillCloudTextureSequentially and FillCloudTextureAsBlocks need be implemented, depending on the graphics API.
//! Some API's permit transfer of data sequentially in memory to textures (e.g. DirectX 9), some expect cuboid blocks of
//! texels to be filled (e.g. OpenGL). The unwanted function should be implemented as a stub, with an assert() or some other
//! fail condition, to make sure it is never called.
			virtual void FillCloudTextureSequentially(int texture_index,int texel_index,int num_texels,const unsigned *uint32_array)=0;
//! Inform the derived class that the texture \a texture_index, either 0, 1 or 2, should fill a block of texels
//! starting at \a x, \a y, \a z, and \a w wide, \a l long, \a d deep, with the values in the array \a uint32_array, which contains
//! four times \a w * \a l * \a d unsigned ints, or shorts - depending on the Use16Bit setting in the keyframer.
//! The first number is the value at \a \a x, \a y, \a z, rather than the start of the whole table.
//! Each group of four represents the r,g,b and a of a colour value for the texture.
//! Only one of FillCloudTextureSequentially and FillCloudTextureAsBlocks need be implemented, depending on the graphics API.
//! Some API's permit transfer of data sequentially in memory to textures (e.g. DirectX 9), some expect cuboid blocks of
//! texels to be filled (e.g. OpenGL). The unwanted function should be implemented as a stub, with an assert() or some other
//! fail condition, to make sure it is never called.
			virtual void FillCloudTextureBlock(int texture_index,int x,int y,int z,int w,int l,int d,const unsigned *uint32_array)=0;
//! Inform the derived class that the textures should be cycled forward - i.e. 1 <- 2, 2 <- 3, and 3 <- 1.
//! This is done when interpolation has crossed over a step boundary.
			virtual void CycleTexturesForward()=0;

//! Inform the derived class of the grid size for lightning illuminating the clouds. This is an optional feature.
			virtual void SetIlluminationGridSize(unsigned width_x,unsigned length_y,unsigned depth_z)=0;
//! Fill the 3D lightning illumination texture (a grid of light values). This is an optional feature; if used, either this function
//! or FillIlluminationBlock should be implemented.
			virtual void FillIlluminationSequentially(int source_index,int texel_index,int num_texels,const unsigned char *uchar8_array)=0;
//! Fill the 3D lightning illumination texture (a grid of light values). This is an optional feature; if used, either this function
//! or FillIlluminationSequentially should be implemented.
			virtual void FillIlluminationBlock(int source_index,int x,int y,int z,int w,int l,int d,const unsigned char *uchar8_array)=0;
			//! Return true if the derived class can relight clouds using the GPU.
			virtual bool CanPerformGPULighting() const=0;
			//! If GPU lighting is supported, set the matrix to transform from light texture coordinate to density texture coordinate, the three sizes of the light grid, and the three light retention factors (direct, indirect and ambient).
			virtual void SetGPULightingParameters(const float *Matrix4x4LightToDensityTexcoords,const unsigned *light_grid,const float *lightspace_extinctions_float3)=0;
			//! If GPU lighting is supported, fully relight the target grid as an array of float4 structures.
			virtual void PerformFullGPURelight(int which_texture,float *target_direct_grid,float *target_indirect_grid)=0;
		};
	}
}
#endif