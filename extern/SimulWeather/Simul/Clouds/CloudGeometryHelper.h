#ifndef SIMULCLOUDS_CLOUDGEOMETRYHELPER_H
#define SIMULCLOUDS_CLOUDGEOMETRYHELPER_H
#include "Simul/Base/Referenced.h"
#include "Simul/Clouds/Export.h"
#include "Simul/Math/Vector3.h"
#include "Simul/Math/RandomNumberGenerator.h"
#include "Simul/Graph/StandardNodes/MemoryUsageInterface.h"
#include "Simul/Geometry/Orientation.h"
#include "Simul/Sky/Float4.h"
#include <vector>
#ifdef _MSC_VER
	
	#pragma warning(push)
	// C4251 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
	#pragma warning(disable:4251)
#endif
namespace simul
{
	namespace sky
	{
		class BaseSkyInterface;
		class FadeTableInterface;
	}
	namespace clouds
	{
		class CloudInterface;
		//! A helper class that generates a list of distances for rendering the cloud volume in slices.
		//! The distances go in realtimeSlices, and are listed starting with the furthest.

		//! 
		SIMUL_CLOUDS_EXPORT_CLASS CloudGeometryHelper
			:public simul::base::Referenced, public simul::graph::standardnodes::MemoryUsageInterface
		{
		public:
			CloudGeometryHelper();
			~CloudGeometryHelper();
			struct Vertex
			{
				float x,y,z;
				float cloud_tex_x,cloud_tex_y,cloud_tex_z;
				float noise_tex_x,noise_tex_y;
				unsigned grid_x,grid_y;
			};
			struct QuadStrip
			{
				float distance1,distance2;
				float elevation1,elevation2;
				unsigned num_vertices;
			};
			struct RealtimeSlice
			{
				RealtimeSlice(float d,float x,float y);
				~RealtimeSlice();
				float distance;
				float fadeIn;
				// the centre of the view is at these tex_coords in the 2D noise texture:
				float noise_tex_x,noise_tex_y;
				float noise_tex_scale;
				float thickness;
				//! The elevation index where this slice begins.
				int elev_start;
				int elev_end;
				int index_start;
				int index_end;
				int min_index;
				int max_index;

				std::vector<simul::sky::float4> losses;
				std::vector<simul::sky::float4> inscatter_factors;
				std::vector<const QuadStrip *> quad_strips;
			};
			void SetNoFrustumLimit(bool nf);
			//! Set up this CloudGeometryHelper to track \a max_num_layers (spherical shells). The furthest layer should be at distance \a max_dist.
			//! This function need only be called once for a given CloudGeometryHelper, unless the input values need to change.
			void Initialize(unsigned max_num_layers,float max_dist);
			unsigned GetMaxLayers() const{return MaxLayers;}
			//! Set a value, usually between 0 and 1, that determines how close cloud layers must be to the camera before starting to fade out.
			//! This value is a multiple for the distance between cloud layers at the nearest point.
			void SetNearCloudFade(float f);
			//! Set the texture churn - how much the texture churns per frame - zero gives a static appearance,
			//! values up to 1 and over make the texture animate.
			void SetChurn(float c);
			//! Update the viewing direction and up direction only, useful for situations where the clouds must be rendered multiple times from the same position.
			void UpdateViewingAngle(const simul::math::Vector3 &view,
											 const simul::math::Vector3 &up);
			//! Convert a world XYZ to a texture coordinate.
			const simul::math::Vector3 &WorldCoordToTexCoord(const CloudInterface *ci,const simul::math::Vector3 &X);
			//! Call this function once per frame with updated view position \a view_pos, the direction the camera is pointed \a view and
			//! the camera \a up direction.
			//! From this information the helper calculates the values for all the slices.
			void Update(const simul::math::Vector3 &view_pos,
						const simul::math::Vector3 &wind_offset,
						const simul::math::Vector3 &view,
						const simul::math::Vector3 &up,
						float dt=0.f,bool cubemap=false);
			const simul::geometry::SimulOrientation &GetNoiseOrientation() const;
			//! Get the forward motion offset - not usually required.
			float GetOffset() const;
			//! Set the width and height of the viewing frustum by sending the tangent of half the horizontal field of view;
			//! and the tangent of half the vertical field of view.
			void SetFrustum(float frustum_tan_half_horiz_fov,float frustum_tan_half_vert_fov);
			//! Call once per frame before using the vertex data.
			void Update2DNoiseCoords();
			//! If you want to use Simul Sky for cloud distance fading, call this once per frame after MakeGeometry.
			void CalcInscatterFactors(simul::sky::BaseSkyInterface *si,bool incremental=false);
			//! The amount of colour loss at this vertex.
			simul::sky::float4 GetLoss(const RealtimeSlice* RS,const Vertex &V);
			//! If  want to use Simul Sky for cloud distance fading, call this for each vertex (with a pointer to the slice that owns it) to
			//! get the colour that should be scattered into the cloud colour due to distance.
			simul::sky::float4 GetInscatter(const RealtimeSlice* RS,const Vertex &V);
			//! Get the inscatter factor - this is the anisotropic vector which must then be multiplied by the isotropic Beta factor
			//! to get the inscatter.
			simul::sky::float4 GetInscatterFactor(const CloudGeometryHelper::RealtimeSlice* RS,const CloudGeometryHelper::Vertex &V);
			//! Get the colour that should be scattered into the cloud colour due to distance.
			simul::sky::float4 GetInscatter(const simul::sky::float4 &inscatter_factor,const CloudGeometryHelper::Vertex &V);
			//! Get the slices - the cloud geometry layers, ordered from back to front. Use this to get the data per fram
			//! for rendering.
			const std::vector<RealtimeSlice*> &GetSlices() const;
			//! Get the list of QuadStrips. For 3D Geometry it is not necessary to use this function - instead get the list of QuadStrips
			//! from each RealtimeSlice. Use this for 2D cloud layers only.
			const std::vector<QuadStrip> &GetQuadStrips() const;
			const std::vector<int> &GetQuadStripIndices() const;
			
			//! Get the vertex list - these are indexed by the QuadStrip struct.
			const std::vector<Vertex> &GetVertices() const;
			//! Get the effective shift due to the Earth's curvature at distance dist from the viewer, and altitude z.
			float GetVerticalShiftDueToCurvature(float dist,float z) const;
			//! Make the cloud geometry - call once per frame to build the list of RealtimeSlice structs. Pass the pointer
			//! to the cloud object in \param ci . If you need the geometry to extend to ground level, set \param to_ground to true and
			//! specify the ground height in \param base_height . If you want the geometry to cover the whole sky instead of being
			//! clipped by the cloud volume, set \param cap to true (this may impact performance and is not usually needed).
			void MakeGeometry(const CloudInterface *ci,bool to_ground=false,float base_height=0,bool cap=false);
			void MakeGeometry(const CloudInterface *ci,bool to_ground,float base_height,bool cap,float altitude);
			//! Set the elevation/azimuth grid sizes.
			void SetGrid(unsigned el,unsigned az);
			//! Get the elevation and azimuth grid sizes.
			void GetGrid(unsigned &el,unsigned &az) const;
			//! Get the range of the elevation/azimuth grid currently in the viewing frustum.
			//! If needed, call this after MakeGeometry in the frame, so it will be up-to-date.
			void GetCurrentGrid(unsigned &el_start,unsigned &el_end,unsigned &az_start,unsigned &az_end) const;
			//! Specify whether to curve the cloud volume over the Earth's surface, based on the effective radius.
			void SetCurvedEarth(bool);
			//! Specify whether to consider the Y-axis as being the vertical axis, or the Z-axis (default).
			void SetYVertical(bool c);
			//! Set the effective radius of the Earth in metres, for curving the cloud volumes. The actual radius is ~6378000 metres, but
			//! smaller values produce more curvature. If set to zero, the radius will be calculated to make the clouds go to the horizon
			//! at the maximum cloud distance.
			void SetEffectiveEarthRadius(float);
			//! Get the effective radius of the Earth in metres used for curving the cloud volumes.
			//! If radius is automatically calculated using SetEffectiveEarthRadius(0), this function returns the calculated value, not zero.
			float GetEffectiveEarthRadius(float cloud_height) const;
			//! Get the current maximum distance set for cloud layers.
			float GetMaxCloudDistance() const;
			//! Get the multipliers for the noise lookup.
			simul::sky::float4 GetFractalScales(const CloudInterface *ci) const;
			//! Enable or disable the Manual Layers functionality. When enabled, cloud slices are set manually.
			void SetManualLayers(bool val);
			bool GetManualLayers() const;
			void SetManualSlice(unsigned index,float dist,float fadeIn=1.f);
			//! Coverage is an estimate of the amount of pixel-drawing these clouds will require.
			//! It is calculated as the sum of the vertical angular span of the layers.
			float GetCoverage(const CloudInterface *ci) const;
			unsigned GetNumActiveLayers() const;
			//! Get the effective shift due to the Earth's curvature at distance dist from the viewer, and altitude z.
			float GetVerticalShiftDueToCurvature(float dist,float z,float R) const;
			//! Get the amount of memory this class instance uses, in bytes.
			unsigned GetMemoryUsage() const;
			//! Rebuild geometry to correspond to a particular slice.
			void MakeLayerGeometry(const CloudInterface *ci,RealtimeSlice *RS);
			//! Update the existing sphere geometry, without rebuilding.
			void UpdateLayerGeometry(const CloudInterface *ci,RealtimeSlice *RS);
			//! Get the true minimum and maximum altitudes for rendering. At no height below H1 can you look down and see clouds;
			//! at no height above H2 can you look up and see clouds.
			void GetRenderHeights(const CloudInterface *ci,float &H1,float &H2) const;
			//! Test for whether the given altitude is above any part of the cloud layer.
			bool IsAltitudeAboveCloudBase(float alt_km) const;
			//! We may choose to render using a single sphere of vertices, scaling it using shader inputs. If so,
			//! the number of vertices is given by this function.
			void GenerateSphereVertices();

			//! How strongly to fade cloud level-of-detail with respect to distance, default is 1.0
			void SetDetailFadeStrength(float dfs);
			//! How strongly to fade cloud level-of-detail with respect to distance, default is 1.0
			float GetDetailFadeStrength() const;

			void EnablePrecalculatedGamma(float g);
			void DisablePrecalculatedGamma();
			void SetAdjustCurvature(bool a)
			{
				AdjustCurvature=a;
			}
		private:
			void Make2DGeometry(const CloudInterface *ci);
			void UpdateFrustum();
			float GetHorizonElevation() const;
			float GetHorizonDistance() const;
			void ClearRealtimeSequences();
			bool use_precalculated_gamma;
			float gamma;
			bool AdjustCurvature;
			float DetailFadeStrength;
			void PrepareLayerGeometry(RealtimeSlice *RS,float altitude);
			simul::math::RandomNumberGenerator noise_random;
			std::vector<float> elevations;
			std::vector<float> sin_elevation;
			std::vector<float> cos_elevation;
			std::vector<float> sin_azimuth;
			std::vector<float> cos_azimuth;
			simul::geometry::SimulOrientation noise_o;
			simul::math::Vector3 last_view_pos;
			simul::math::Vector3 last_wind_offset;
			simul::math::Vector3 view_dir;
			simul::math::Vector3 up_dir;
			friend bool pred(const RealtimeSlice *S1,const RealtimeSlice *S2);
			struct RealtimeSequence	// a sequence of slices
			{
				~RealtimeSequence();
				float distance;
				float endFade;
				float startOffset;
				float interval;
				unsigned numSlices;
				std::vector<RealtimeSlice*> realtimeSlices;
			};
			std::vector<RealtimeSequence*> realtimeSequences;
			std::vector<RealtimeSlice*> realtimeSlices;
			bool first_frame;
			float realtimeForwardOffset;
			std::vector<Vertex> vertices;
			std::vector<QuadStrip> quadStrips;
			std::vector<int> quad_strip_indices;
			int *last_update;
			int update_frame;
			unsigned MaxLayers;
			float frustum_sine_elev_min,frustum_sine_elev_max;
			unsigned el_min;
			unsigned el_max;
			unsigned az_grid_start;
			unsigned az_grid_end;
			unsigned el_grid;
			unsigned az_grid;
			unsigned active_slices;
			bool no_frustum_limit;
			bool curved_earth;
			float cloudbase_altitude;
			float cloud_max_altitude;
			float dH;
			float effective_world_radius;
			float max_cloud_distance;
			float centre_z_offset;
			float *noise_coords;
			bool y_vertical;
			bool automatic_detail;
			float nearCloudFade;
			std::vector<simul::sky::float4> inscatter_angular_mult;
			float Churn;
			float frustum_tan_half_horiz_fov;
			float frustum_tan_half_vert_fov;
			bool manual_layers;

			float H1,H2;
			simul::math::Vector3 X1,X2,DX,offset_X1;
			float sine_horizon_elev;
			float horizon_distance;
			float R;
			bool Cap;
		};
	}
}
#ifdef _MSC_VER
	#pragma warning(pop)
#endif
#endif
