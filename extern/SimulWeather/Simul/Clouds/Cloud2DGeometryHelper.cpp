#include "precomp.h"

#include "Simul/Clouds/Cloud2DGeometryHelper.h"
#include "Simul/Clouds/CloudInterface.h"
#include "Simul/Sky/SkyInterface.h"
#include "Simul/Sky/FadeTableInterface.h"
#include "Simul/Math/Pi.h"
#include <algorithm>
// For memset:
#include <string.h>
using namespace simul::clouds;
using namespace simul::math;

Cloud2DGeometryHelper::RealtimeSequence::~RealtimeSequence()
{
	for(size_t i=0;i<realtimeSlices.size();i++)
	{
		delete realtimeSlices[i];
	}
	realtimeSlices.clear();
}

Cloud2DGeometryHelper::RealtimeSlice::RealtimeSlice(float d,float x,float y)
{
	distance=d;
	fadeIn=thickness=0;
	noise_tex_x=x;
	noise_tex_y=y;
	elev_start=elev_end=0;
}

Cloud2DGeometryHelper::RealtimeSlice::~RealtimeSlice()
{
	quad_strips.clear();
}

namespace simul
{
	namespace clouds
	{
		bool pred(const simul::clouds::Cloud2DGeometryHelper::RealtimeSlice *S1,
									const simul::clouds::Cloud2DGeometryHelper::RealtimeSlice *S2)
		{
			static bool reverse=false;
			bool res=(S1->distance>S2->distance);
			if(reverse)
				res=!res;
			return res;
		}
	}
}

/*static float saturate(float f)
{
	if(f<0)
		return 0.f;
	if(f>1.f)
		return 1.f;
	return f;^
}
*/
Cloud2DGeometryHelper::Cloud2DGeometryHelper()
		:inscatter_angular_mult(NULL)
		,frustum_tan_half_horiz_fov(0)
		,frustum_tan_half_vert_fov(0)
		,active_slices(0)
		,noise_2d_angle(0)
		,noise_2d_aniso(1.f)
		,automatic_detail(true)
		,DetailFadeStrength(1.f)
		,AdjustCurvature(true)
		,nearCloudFade(0.5f)
		,dH(1.f)
		,no_frustum_limit(false)
{
	noise_random.Seed(13561);
	Churn=0;
	curved_earth=true;
	y_vertical=false;
	effective_world_radius=6378000.f;
	realtimeForwardOffset=0;
	MaxLayers=1;
	centre_z_offset=0;
	noise_coords=NULL;
	last_view_pos.Define(0,0,0);
	el_grid=0;
	az_grid=0;
	frustum_sine_elev_min=0;
	frustum_sine_elev_max=0;
	az_grid_start=0;
	az_grid_end=0;
	update_frame=0;
	last_update=NULL;
	max_cloud_distance=20000.f;
	el_min=0;
	el_max=0;
	last_view_pos.Zero();
	last_wind_offset.Zero();
	view_dir.Zero();
	up_dir.Zero();
	SetGrid(24,18);
}

Cloud2DGeometryHelper::~Cloud2DGeometryHelper()
{
	delete [] noise_coords;
	delete [] last_update;
	
	elevations.clear();
	sin_elevation.clear();
	cos_elevation.clear();
	sin_azimuth.clear();
	cos_azimuth.clear();
			
	realtimeSlices.clear();
	ClearRealtimeSequences();
	vertices.clear();
	quadStrips.clear();
	quad_strip_indices.clear();

	inscatter_angular_mult.clear();
	
	losses_2d.clear();
	inscatter_factors_2d.clear();
}

void Cloud2DGeometryHelper::SetNoFrustumLimit(bool nf)
{
	no_frustum_limit=nf;
}

void Cloud2DGeometryHelper::ClearRealtimeSequences()
{
	for(size_t i=0;i<realtimeSequences.size();i++)
	{
		delete realtimeSequences[i];
	}
	realtimeSequences.clear();
}

void Cloud2DGeometryHelper::Initialize(unsigned max_num_layers,float max_dist)
{
	max_cloud_distance=max_dist;
	MaxLayers=max_num_layers;
	SetGrid(MaxLayers,az_grid);
	quadStrips.resize((el_grid+1));
	vertices.resize((el_grid+1)*az_grid);
	ClearRealtimeSequences();
	realtimeSlices.clear();
	if(max_num_layers)
	{
		// num_seq=L.
		// N * 2^L=max_num_layers
		// so N=max_num_layers/(2^L);
		// so L=log2(max_num_layers/N)
		static int num_seq=5;
		int Ns=std::max((int)1,(int)((float)max_num_layers/pow(2.f,num_seq-1)));
		int slices_in_sequence=(int)(Ns);
		Ns=slices_in_sequence;

		int total_slices=0;
		float current_dist=max_dist;
		float offset=max_dist/(float)slices_in_sequence;
		if(slices_in_sequence>(int)max_num_layers)
			slices_in_sequence=max_num_layers;
		static float f=1.f;
		for(int i=0;i<num_seq+1&&total_slices<(int)max_num_layers;i++)
		{
			float interval=current_dist/(float)slices_in_sequence;
			if(slices_in_sequence>(int)max_num_layers-total_slices)
				slices_in_sequence=(int)max_num_layers-total_slices;
			if(slices_in_sequence<=0)
				break;
			realtimeSequences.push_back(new RealtimeSequence());
			RealtimeSequence *seq=realtimeSequences.back();

			seq->interval=interval;
			seq->startOffset=offset;
			seq->distance=offset;

			seq->numSlices=slices_in_sequence;
			total_slices+=seq->numSlices;
			//current_dist*=0.5f;
			if(i)
				Ns*=2;
			slices_in_sequence=Ns;
			offset*=0.5f;
		}
		for(size_t i=0;i<realtimeSequences.size();i++)
		{
			RealtimeSequence *seq=realtimeSequences[i];
			for(int j=0;j<(int)seq->numSlices;j++)
			{
				seq->realtimeSlices.push_back(new RealtimeSlice(seq->distance+j*seq->interval,
					f*max_cloud_distance*noise_random.FRand(),
					f*max_cloud_distance*noise_random.FRand()));
			}
		}
		for(int i=0;i<total_slices;i++)
			realtimeSlices.push_back(NULL);
	}
	first_frame=true;
}

void Cloud2DGeometryHelper::SetNearCloudFade(float f)
{
	nearCloudFade=f;
}

void Cloud2DGeometryHelper::SetChurn(float c)
{
	Churn=c;
}

void Cloud2DGeometryHelper::Update(const Vector3 &view_pos,
								 const Vector3 &wind_offset,
								 const simul::math::Vector3 &view,
								 const simul::math::Vector3 &up,
								 float ,bool )
{
	//float churn=Churn*dt;
	Vector3 vp=view_pos;
	Vector3 wo=wind_offset;
	up_dir=up;
	view_dir=view;

	if(y_vertical)
	{
		std::swap(vp.y,vp.z);
		std::swap(wo.y,wo.z);
		std::swap(up_dir.y,up_dir.z);
		std::swap(view_dir.y,view_dir.z);
	}

	Vector3 view_change=vp-wo-last_view_pos+last_wind_offset;
	last_wind_offset=wo;
	float forward_change=view_dir*view_change;
	if(first_frame)
	{
		first_frame=false;
	}
	else
		realtimeForwardOffset+=forward_change;
	last_view_pos=vp;

	float nearest_cloud=0.f;
	if(last_view_pos.z>H2)
		nearest_cloud=last_view_pos.z-H2;
	else if(last_view_pos.z<H1)
		nearest_cloud=H1-last_view_pos.z;
}

const simul::geometry::SimulOrientation &Cloud2DGeometryHelper::GetNoiseOrientation() const
{
	return noise_o;
}

float Cloud2DGeometryHelper::GetOffset() const
{
	return realtimeForwardOffset;
}

void Cloud2DGeometryHelper::SetFrustum(float x,float y)
{
	frustum_tan_half_horiz_fov=x;
	frustum_tan_half_vert_fov=y;
}

void Cloud2DGeometryHelper::UpdateFrustum()
{
	float elev_min=0;
	float elev_max=0;
	float az_min=0;
	float az_max=0;
	simul::geometry::SimulOrientation Or;
	Or.DefineFromYZ(up_dir,view_dir);// define an orientation with z in the look direction, y up and x left!!!
	float view_azimuth=atan2(view_dir.y,view_dir.x);
	// Unit Frustum:
	simul::math::Vector3 unit_frustum_local[4];
	simul::math::Vector3 &loc=unit_frustum_local[0];
	loc.x=frustum_tan_half_horiz_fov;
	loc.y=frustum_tan_half_vert_fov;
	loc.z=1.f;
	for(int i=1;i<4;i++)
	{
		simul::math::Vector3 &loc_i=unit_frustum_local[i];
		loc_i=loc;
		if(i==1||i==3)
			loc_i.x*=-1.f;
		if(i>1)
			loc_i.y*=-1.f;
	}

	// In world elevation, find the min and max of the view frustum.
	float h_max=-1e23f;
	float h_min=1e23f;
	az_min=1e23f;
	az_max=-1e23f;
	
	for(int j=0;j<4;j++)
	{
		simul::math::Vector3 frustum_global;
		Or.LocalToGlobalDirection(frustum_global,unit_frustum_local[j]);
		
		{
			h_min=std::min(h_min,frustum_global.z);
			h_max=std::max(h_max,frustum_global.z);
		}
		frustum_global.Normalize();
		{
			h_min=std::min(h_min,frustum_global.z);
			h_max=std::max(h_max,frustum_global.z);
		}
		float azimuth=atan2(frustum_global.y,frustum_global.x);
		float from_centre=azimuth-view_azimuth;
		while(from_centre>=pi)
			from_centre-=2.f*pi;
		while(from_centre<-pi)
			from_centre+=2.f*pi;
		if(from_centre<az_min)
			az_min=from_centre;
		if(from_centre>az_max)
			az_max=from_centre;
	}
	h_min=std::max(-1.f,h_min);
	h_max=std::min(1.f,h_max);
	h_max=std::max(-1.f,h_max);
	h_min=std::min(1.f,h_min);
	frustum_sine_elev_min=h_min;
	frustum_sine_elev_max=h_max;
	elev_min=asin(frustum_sine_elev_min);
	elev_max=asin(frustum_sine_elev_max);
	bool vertical_in_frustum			=false;
	bool negative_vertical_in_frustum	=false;
	az_min+=view_azimuth;
	az_max+=view_azimuth;
	simul::math::Vector3 vertical(0,0,1.f);

	simul::math::Vector3 vertical_in_local;
	Or.GlobalToLocalDirection(vertical_in_local,vertical);
	float yz=0;
	yz=vertical_in_local.y;
	if(vertical_in_local.x>-frustum_tan_half_horiz_fov&&
		vertical_in_local.x<frustum_tan_half_horiz_fov)
	{
		if(yz>-frustum_tan_half_vert_fov&&
			yz<frustum_tan_half_vert_fov)
		{
			vertical_in_frustum=true;
			az_min=0;
			az_max=2.f*pi-0.0001f;
			frustum_sine_elev_max=1.f;
		}
	}
	else if(-vertical_in_local.x>-frustum_tan_half_horiz_fov&&
		-vertical_in_local.x<frustum_tan_half_horiz_fov)
	{
		if(-yz>-frustum_tan_half_vert_fov&&
			-yz<frustum_tan_half_vert_fov)
		{
			negative_vertical_in_frustum=true;
			az_min=0;
			az_max=2.f*pi-0.0001f;
			frustum_sine_elev_min=-1.f;
		}
	}
	while(az_min<0)
	{
		az_min+=2.f*pi;
		az_max+=2.f*pi;
	}
	az_grid_start=(unsigned)(az_grid*az_min/(2.f*pi));
	az_grid_end=1+(unsigned)(az_grid*az_max/(2.f*pi));
	if(no_frustum_limit)
	{
		az_grid_start=0;
		az_grid_end=az_grid;
		frustum_sine_elev_min=-1.f;
		frustum_sine_elev_max=1.f;
	}
}

void Cloud2DGeometryHelper::Update2DNoiseCoords()
{
	// build the noise texture co-ordinates that correspond to the cloud grid:
	bool reset=false;
	if(!last_update)
	{
		last_update=new int[(el_grid+1)*az_grid];
		reset=true;
	}
	if(reset)
	{
		memset(last_update,0,(el_grid+1)*az_grid*sizeof(int));
		update_frame=10;
	}
	update_frame++;

	for(unsigned j=0;j<el_grid+1;j++)
	{
		float j_interp=(float)(j)/(float)el_grid;
		float z=-1.f+2.f*j_interp;
		float elevation=asin(z);
		if(z<frustum_sine_elev_min-pi/(float)el_grid)
			continue;
		if(z>frustum_sine_elev_max+pi/(float)el_grid)
			continue;
		float cos_el=cos(elevation);
		for(unsigned k=az_grid_start;k<az_grid_end+1;k++)
		{
			unsigned kk=k%az_grid;
			int &last_update_frame=last_update[j*az_grid+kk];

			float k_interp=(float)(kk)/(float)az_grid;
			float azimuth=2.f*pi*k_interp;
			simul::math::Vector3 pos;
			pos.x=cos_el*cos(azimuth);
			pos.y=cos_el*sin(azimuth);
			pos.z=z;
			
			simul::math::Vector3 noise_pos;
			noise_o.GlobalToLocalPosition(noise_pos,pos);
			noise_coords[2*(j*az_grid+kk)+0]=atan2(noise_pos.x,noise_pos.z);
			noise_coords[2*(j*az_grid+kk)+1]=atan2(noise_pos.y,noise_pos.z);
			
			last_update_frame=update_frame;
		}
	}
}
const std::vector<Cloud2DGeometryHelper::RealtimeSlice*> &Cloud2DGeometryHelper::GetSlices() const
{
	return realtimeSlices;
}

const std::vector<Cloud2DGeometryHelper::Vertex> &Cloud2DGeometryHelper::GetVertices() const
{
	return vertices;
}

const std::vector<Cloud2DGeometryHelper::QuadStrip> &Cloud2DGeometryHelper::GetQuadStrips() const
{
	return quadStrips;
}

const std::vector<int> &Cloud2DGeometryHelper::GetQuadStripIndices() const
{
	return quad_strip_indices;
}

void Cloud2DGeometryHelper::SetCurvedEarth(bool c)
{
	curved_earth=c;
}

void Cloud2DGeometryHelper::SetYVertical(bool c)
{
	y_vertical=c;
}

void Cloud2DGeometryHelper::SetEffectiveEarthRadius(float r)
{
	effective_world_radius=r;
}

float Cloud2DGeometryHelper::GetHorizonElevation() const
{
	if(!effective_world_radius)
		return 0.f;
	float altitude=last_view_pos.z;
	float horizon_elev=-acos(effective_world_radius/(effective_world_radius+altitude));
	return horizon_elev;
}

float Cloud2DGeometryHelper::GetHorizonDistance() const
{
	if(!effective_world_radius)
		return 1e23f;
	float altitude=last_view_pos.z;
	float horizon_elev=-GetHorizonElevation();
	float horizon_distance=(effective_world_radius+altitude)*sin(horizon_elev);
	return horizon_distance;
}

float Cloud2DGeometryHelper::GetEffectiveEarthRadius(float cloud_height) const
{
	float R=1e9f;
	if(cloud_height>0)
		R=(max_cloud_distance*max_cloud_distance-cloud_height*cloud_height)/(2*cloud_height);

	if((effective_world_radius&&(!AdjustCurvature||effective_world_radius<R)))
		return effective_world_radius;
	if(R>1e9f)
		R=1e9f;
	if(R<100.f)
		R=100.f;
	return R;
}

float Cloud2DGeometryHelper::GetVerticalShiftDueToCurvature(float dist,float z) const
{
	float R=GetEffectiveEarthRadius(z);
	return GetVerticalShiftDueToCurvature(dist,z,R);
}

float Cloud2DGeometryHelper::GetVerticalShiftDueToCurvature(float dist,float z,float R) const
{
	float Rh=R+z;
	float vertical_shift=0;
	if(curved_earth)
		vertical_shift=sqrt(Rh*Rh-dist*dist)-Rh;
	return vertical_shift;
}

void Cloud2DGeometryHelper::GetRenderHeights(const CloudInterface *ci,float &H1,float &H2) const
{
	math::Vector3 X1,X2;
	ci->GetExtents(X1,X2);
	H1=X1.z;
	H2=X2.z;
	unsigned tex_width,tex_height,tex_length;
	ci->GetGrid(tex_width,tex_length,tex_height);
	float one_texel_height=(X2.z-X1.z)/(float)tex_height;
	H1+=one_texel_height;
	H2-=one_texel_height;
	// FractalOffsetScale is the magnitude of the difference between most +ve and most -ve offset.
	// So 1/2 of this in either direction:
	H1-=0.5f*ci->GetFractalOffsetScale();
	H2+=0.5f*ci->GetFractalOffsetScale();
}
/*
void Cloud2DGeometryHelper::GetHeights(const CloudInterface *ci,float &H1,float &H2) const
{
	GetRenderHeights(ci,H1,H2);
	H1-=last_view_pos.z;
	H2-=last_view_pos.z;
}*/

void Cloud2DGeometryHelper::MakeGeometry(const CloudInterface *ci,bool ,float ,bool cap)
{
	Cap=cap;
	UpdateFrustum();
	Update2DNoiseCoords();
	Make2DGeometry(ci);
}

void Cloud2DGeometryHelper::PrepareLayerGeometry(Cloud2DGeometryHelper::RealtimeSlice *RS)										
{
	float dist=RS->distance;
	RS->quad_strips.resize(0);
	RS->elev_start=RS->elev_end=el_min;
	RS->index_end=RS->index_start=0;
	if(RS->fadeIn==0)
		return;

	Vector3 view_km=last_view_pos;
	// convert metres to km:
	view_km*=0.001f;
	// How thick is this layer, optically speaking?
	// The min and max elevations for the frustum:
	float sine_elev_min=frustum_sine_elev_min;
	float sine_elev_max=frustum_sine_elev_max;
	float vertical_shift=GetVerticalShiftDueToCurvature(dist,X1.z,R);

	float h1=H1-last_view_pos.z+vertical_shift;
	float h2=H2-last_view_pos.z+vertical_shift;
	
	float c=sqrt(dist*dist+centre_z_offset*centre_z_offset);
	// But only the elevation range where there are clouds - between relative heights h1 and h2
	if(fabs(centre_z_offset+h1)<c)
	{
		float offset_elev_min_cloud=asin((centre_z_offset+h1)/c);
		float sine_elev_min_cloud=sin(atan2(h1,c*cos(offset_elev_min_cloud)));
		sine_elev_min=std::max(sine_elev_min,sine_elev_min_cloud);
	}
	// question - is the minimum elevation for this layer below the horizon?
	// if so, just use the horizon value.
	if(dist>horizon_distance)
	{
		sine_elev_min=std::max(sine_elev_min,sine_horizon_elev);
	}
	if(!Cap)
	if(fabs(centre_z_offset+h2)<(centre_z_offset+dist))
	{
		float offset_elev_max_cloud=asin((centre_z_offset+h2)/c);
		float sine_elev_max_cloud=sin(atan2(h2,c*cos(offset_elev_max_cloud)));
		sine_elev_max=std::min(sine_elev_max,sine_elev_max_cloud);
	}
	if(fabs(h1)>dist&&fabs(h2)>dist&&h1*h2>0)
		return;
	if(h1>h2)
		return;
	active_slices++;
	if(sine_elev_max-sine_elev_min<0)
		return;
	unsigned el_grid_start=(unsigned)(.5f*(sine_elev_min+1.f)*(float)el_grid);
	unsigned el_grid_end=1+(unsigned)(.5f*(sine_elev_max+1.f)*(float)el_grid);
	el_grid_end=std::min(el_grid,el_grid_end);
	if(el_grid_start<el_min)
		el_min=el_grid_start;
	if(el_grid_end>el_max)
		el_max=el_grid_end;
	RS->elev_start=el_min;
	RS->elev_end=el_max;
	if(RS->elev_start==RS->elev_end)
	{
		RS->index_start	=2*el_min*(az_grid+1);
		RS->index_end	=RS->index_start;
	}
	else
	{
		RS->index_start	=2*el_min*(az_grid+1);
		RS->index_end	=2*el_max*(az_grid+1)-1;
	}
	if(RS->index_start<RS->index_end&&RS->index_end<(int)(quad_strip_indices.size()))
	{
		RS->min_index	=quad_strip_indices[RS->index_start];
		RS->max_index	=quad_strip_indices[RS->index_end];
	}

}

void Cloud2DGeometryHelper::GenerateSphereVertices()
{
	vertices.resize((el_grid+1)*(az_grid));
	quad_strip_indices.resize((el_grid*2)*(az_grid+1));
	unsigned vertex=0;
	unsigned index=0;
	for(unsigned i=0;i<=el_grid;i++)
	{
		float sine_elevation=-1.f+2.f*((float)(i)/(float)el_grid);
		float z=sine_elevation;
		float elevation=asin(sine_elevation);
		float horizontal_radius=cos(elevation);
		for(unsigned j=0;j<az_grid;j++)
		{
			Vertex &V=vertices[vertex++];
			unsigned jj=(j%az_grid);
			float j_interp=(float)(jj)/(float)(az_grid);
			float azimuth=2.f*pi*j_interp;
			V.grid_x=jj;
			V.grid_y=i;
			V.x=horizontal_radius*cos(azimuth);
			V.y=horizontal_radius*sin(azimuth);
			V.z=z;
			if(y_vertical)
				std::swap(V.y,V.z);
		}
	}
	for(unsigned i=0;i<el_grid;i++)
	{
		for(unsigned j=0;j<=az_grid;j++)
		{
			unsigned jj=(j%az_grid);
			quad_strip_indices[index++]=(i*az_grid)+jj;
			quad_strip_indices[index++]=((i+1)*az_grid)+jj;
		}
	}
}

void Cloud2DGeometryHelper::UpdateLayerGeometry(const CloudInterface *ci,Cloud2DGeometryHelper::RealtimeSlice *RS)										
{
	float dist=RS->distance;
	if(RS->fadeIn==0)
		return;
	Vector3 view_km=last_view_pos;
	// convert metres to km:
	view_km*=0.001f;
	// How thick is this layer, optically speaking?
	// The min and max elevations for the frustum:
	float sine_elev_min=frustum_sine_elev_min;
	float sine_elev_max=frustum_sine_elev_max;
	float vertical_shift=GetVerticalShiftDueToCurvature(dist,X1.z,R);
	float h1=H1-last_view_pos.z+vertical_shift;
	float h2=H2-last_view_pos.z+vertical_shift;
	float c=sqrt(dist*dist+centre_z_offset*centre_z_offset);
	// But only the elevation range where there are clouds - between relative heights h1 and h2
#if 1
	if(fabs(h1)<dist)
	{
		float elev_min_cloud=asin((h1)/dist);
		float sine_elev_min_cloud=sin(atan2(h1,dist*cos(elev_min_cloud)));
		sine_elev_min=std::max(sine_elev_min,sine_elev_min_cloud);
	}
#else
	if(fabs(centre_z_offset+h1)<c)
	{
		float offset_elev_min_cloud=asin((h1-centre_z_offset)/c);
		float sine_elev_min_cloud=sin(atan2(h1-centre_z_offset,c*cos(offset_elev_min_cloud)));
		sine_elev_min=std::max(sine_elev_min,sine_elev_min_cloud);
	}
#endif
	// question - is the minimum elevation for this layer below the horizon?
	// if so, just use the horizon value.
	if(dist>horizon_distance)
	{
		sine_elev_min=std::max(sine_elev_min,sine_horizon_elev);
	}
	if(!Cap)
#if 1
	if(fabs(h2)<dist)
	{
		float elev_max_cloud=asin((h2)/dist);
		float sine_elev_max_cloud=sin(atan2(h2,dist*cos(elev_max_cloud)));
		sine_elev_max=std::min(sine_elev_max,sine_elev_max_cloud);
	}
#else
	if(fabs(centre_z_offset+h2)<c)
	{
		float offset_elev_max_cloud=asin((h2-centre_z_offset)/c);
		float sine_elev_max_cloud=sin(atan2(h2-centre_z_offset,c*cos(offset_elev_max_cloud)));
		sine_elev_max=std::min(sine_elev_max,sine_elev_max_cloud);
	}
#endif
	if(fabs(h1)>dist&&fabs(h2)>dist&&h1*h2>0)
		return;
	if(h1>h2)
		return;
	if(sine_elev_max-sine_elev_min<0)
		return;
	static float do_offset_scale=0.f;
	if(do_offset_scale)
	{
		centre_z_offset=do_offset_scale*(-h1-h2)*0.5f;
	}
	unsigned el_grid_start=(unsigned)(.5f*(sine_elev_min+1.f)*(float)el_grid);
	unsigned el_grid_end=1+(unsigned)(.5f*(sine_elev_max+1.f)*(float)el_grid);
	el_grid_end=std::min(el_grid,el_grid_end);
	if(el_grid_start<el_min)
		el_min=el_grid_start;
	if(el_grid_end>el_max)
		el_max=el_grid_end;

	float tex_scale=dist/ci->GetFractalRepeatLength();
	float tex_offset_x=RS->noise_tex_x/ci->GetFractalRepeatLength();
	float tex_offset_y=RS->noise_tex_y/ci->GetFractalRepeatLength();


	for(unsigned j=el_grid_start;j<=el_grid_end;j++)
	{
		float sine_elevation=sin_elevation[j];
		float z=dist*sine_elevation;
		//float elevation=elevations[j];
		float horizontal_radius=dist*cos_elevation[j];
		float zc=centre_z_offset*-sine_elevation;
		float modified_radius=zc+.5f*sqrt(zc*zc+4*(c*c-centre_z_offset*centre_z_offset));
		z*=modified_radius/dist;
		z+=last_view_pos.z;
		horizontal_radius*=modified_radius/dist;
	
		// make vertices:
		for(unsigned k=az_grid_start;k<az_grid_end+1;k++)
		{
			unsigned kk=(k%az_grid);
			size_t vertex=j*az_grid+kk;
			Vertex &V=vertices[vertex];
			V.grid_x=kk;
			V.grid_y=j;
			//V.x=last_view_pos.x+horizontal_radius*cos_azimuth[kk];
			//V.y=last_view_pos.y+horizontal_radius*sin_azimuth[kk];
			//V.z=z;
			V.cloud_tex_x=(last_view_pos.x+dist*V.x-offset_X1.x)/DX.x;
			if(y_vertical)
			{
				V.cloud_tex_y=(last_view_pos.y+dist*V.z-offset_X1.y)/DX.y;
				V.cloud_tex_z=(last_view_pos.z+dist*V.y-offset_X1.z-vertical_shift)/DX.z;
			}
			else
			{
				V.cloud_tex_y=(last_view_pos.y+dist*V.y-offset_X1.y)/DX.y;
				V.cloud_tex_z=(last_view_pos.z+dist*V.z-offset_X1.z-vertical_shift)/DX.z;
			}
			simul::math::Vector3 noise_texc1;
			V.noise_tex_x=noise_coords[2*(j*az_grid+kk)+0]*tex_scale+tex_offset_x;
			V.noise_tex_y=noise_coords[2*(j*az_grid+kk)+1]*tex_scale+tex_offset_y;
		}
	}
}
void Cloud2DGeometryHelper::MakeLayerGeometry(const CloudInterface *ci,Cloud2DGeometryHelper::RealtimeSlice *RS)										
{
	size_t vertex=0;
	size_t strip=0;
	vertices.resize(0);
	quadStrips.resize(0);
	float dist=RS->distance;
	RS->quad_strips.resize(0);
	quad_strip_indices.resize(0);
	if(RS->fadeIn==0)
		return;
	Vector3 view_km=last_view_pos;
	// convert metres to km:
	view_km*=0.001f;
	// How thick is this layer, optically speaking?
	// The min and max elevations for the frustum:
	float sine_elev_min=frustum_sine_elev_min;
	float sine_elev_max=frustum_sine_elev_max;
	float vertical_shift=GetVerticalShiftDueToCurvature(dist,X1.z,R);
	float h1=H1-last_view_pos.z+vertical_shift;
	float h2=H2-last_view_pos.z+vertical_shift;
	float c=sqrt(dist*dist+centre_z_offset*centre_z_offset);
	// But only the elevation range where there are clouds - between relative heights h1 and h2
#if 1
	if(fabs(h1)<dist)
	{
		float elev_min_cloud=asin((h1)/dist);
		float sine_elev_min_cloud=sin(atan2(h1,dist*cos(elev_min_cloud)));
		sine_elev_min=std::max(sine_elev_min,sine_elev_min_cloud);
	}
#else
	if(fabs(centre_z_offset+h1)<c)
	{
		float offset_elev_min_cloud=asin((h1-centre_z_offset)/c);
		float sine_elev_min_cloud=sin(atan2(h1-centre_z_offset,c*cos(offset_elev_min_cloud)));
		sine_elev_min=std::max(sine_elev_min,sine_elev_min_cloud);
	}
#endif
	// question - is the minimum elevation for this layer below the horizon?
	// if so, just use the horizon value.
	if(dist>horizon_distance)
	{
		sine_elev_min=std::max(sine_elev_min,sine_horizon_elev);
	}
	if(!Cap)
#if 1
	if(fabs(h2)<dist)
	{
		float elev_max_cloud=asin((h2)/dist);
		float sine_elev_max_cloud=sin(atan2(h2,dist*cos(elev_max_cloud)));
		sine_elev_max=std::min(sine_elev_max,sine_elev_max_cloud);
	}
#else
	if(fabs(centre_z_offset+h2)<c)
	{
		float offset_elev_max_cloud=asin((h2-centre_z_offset)/c);
		float sine_elev_max_cloud=sin(atan2(h2-centre_z_offset,c*cos(offset_elev_max_cloud)));
		sine_elev_max=std::min(sine_elev_max,sine_elev_max_cloud);
	}
#endif
	if(fabs(h1)>dist&&fabs(h2)>dist&&h1*h2>0)
		return;
	if(h1>h2)
		return;
	if(sine_elev_max-sine_elev_min<0)
		return;
	static float do_offset_scale=0.f;
	if(do_offset_scale)
	{
		centre_z_offset=do_offset_scale*(-h1-h2)*0.5f;
	}
	unsigned el_grid_start=(unsigned)(.5f*(sine_elev_min+1.f)*(float)el_grid);
	unsigned el_grid_end=1+(unsigned)(.5f*(sine_elev_max+1.f)*(float)el_grid);
	el_grid_end=std::min(el_grid,el_grid_end);
	if(el_grid_start<el_min)
		el_min=el_grid_start;
	if(el_grid_end>el_max)
		el_max=el_grid_end;
	vertices.resize((el_grid_end-el_grid_start+1)*(az_grid_end-az_grid_start+1));
	quadStrips.resize(el_grid_end-el_grid_start);

	float tex_scale=dist/ci->GetFractalRepeatLength();
	float tex_offset_x=RS->noise_tex_x/ci->GetFractalRepeatLength();
	float tex_offset_y=RS->noise_tex_y/ci->GetFractalRepeatLength();

	for(unsigned j=el_grid_start;j<=el_grid_end;j++)
	{
		float sine_elevation=sin_elevation[j];
		float z=dist*sine_elevation;
		float elevation=elevations[j];
		float horizontal_radius=dist*cos_elevation[j];
		float zc=centre_z_offset*-sine_elevation;
		float modified_radius=zc+.5f*sqrt(zc*zc+4*(c*c-centre_z_offset*centre_z_offset));
		z*=modified_radius/dist;
		z+=last_view_pos.z;
		horizontal_radius*=modified_radius/dist;
	
		int start_vertex=(int)vertex;
		// make vertices:
		for(unsigned k=az_grid_start;k<az_grid_end+1;k++)
		{
			Vertex &V=vertices[vertex++];
			unsigned kk=(k%az_grid);
			V.grid_x=kk;
			V.grid_y=j;
			assert((int)V.grid_y<=RS->elev_end);
			V.x=last_view_pos.x+horizontal_radius*cos_azimuth[kk];
			V.y=last_view_pos.y+horizontal_radius*sin_azimuth[kk];
			V.z=z;
			V.cloud_tex_x=(V.x-offset_X1.x)/DX.x;
			V.cloud_tex_y=(V.y-offset_X1.y)/DX.y;
			V.cloud_tex_z=(V.z-offset_X1.z-vertical_shift)/DX.z;
			if(y_vertical)
				std::swap(V.y,V.z);
			simul::math::Vector3 noise_texc1;
			V.noise_tex_x=noise_coords[2*(j*az_grid+kk)+0]*tex_scale+tex_offset_x;
			V.noise_tex_y=noise_coords[2*(j*az_grid+kk)+1]*tex_scale+tex_offset_y;
		}
		// make quad strip
		if(j<el_grid_end)
		{
			QuadStrip &quad_strip=quadStrips[strip++];
			quad_strip.num_vertices=0;
			quad_strip.distance1=dist;
			quad_strip.distance2=dist;
			quad_strip.elevation1=elevation;
			quad_strip.elevation2=elevations[j+1];
			unsigned num_verts=az_grid_end-az_grid_start+1;
			if(!(j%2))
			{
				for(unsigned k=0;k<num_verts;k++)
				{
					quad_strip_indices.push_back(start_vertex+k);
					quad_strip_indices.push_back(start_vertex+k+(num_verts));
					quad_strip.num_vertices+=2;
				}
			}
			else
			{
				for(unsigned k=0;k<num_verts;k++)
				{
					quad_strip_indices.push_back(start_vertex+num_verts-1-k);
					quad_strip_indices.push_back(start_vertex+num_verts-1-k+num_verts);
					quad_strip.num_vertices+=2;
				}
			}
			RS->quad_strips.push_back(&quad_strip);
		}
	}
}

void Cloud2DGeometryHelper::Set2DNoiseTexturing(float angle,float anisotropy,float scale)
{
	noise_2d_angle=angle;
	noise_2d_aniso=anisotropy;
	noise_2d_scale=scale;
}

void Cloud2DGeometryHelper::Make2DGeometry(const CloudInterface *ci)
{
	unsigned NumLayers=MaxLayers;
	math::Vector3 X1,X2,DX;
	ci->GetExtents(X1,X2);
	DX=X2-X1;
	float altitude_z=X1.z;
	math::Vector3 temp_X1=X1;
	temp_X1+=last_wind_offset;
	int v=0;
	vertices.resize((NumLayers+1)*az_grid);
	float R=GetEffectiveEarthRadius(X1.z);
	float Rh=R+X1.z;
	quadStrips.resize(NumLayers-1);
	float last_z=altitude_z;
	float last_r=0;
	float anisotropy=noise_2d_aniso;
	float x_anis=noise_2d_scale/anisotropy;
	float y_anis=noise_2d_scale*anisotropy;
	quad_strip_indices.resize(0);
	for(unsigned i=0;i<NumLayers;i++)
	{
		float r1=(float)i/(float)NumLayers*max_cloud_distance;
		float vertical_shift=sqrt(Rh*Rh-r1*r1)-Rh;
		float z1=altitude_z;
		for(unsigned j=0;j<az_grid;j++,v++)
		{
			Vertex &vertex=vertices[v];
			float azimuth=(float)j/(float)az_grid*2.f*pi;
			vertex.x=last_view_pos.x+r1*sin(azimuth);
			vertex.y=last_view_pos.y+r1*cos(azimuth);
			vertex.z=z1+vertical_shift;
			vertex.cloud_tex_x=(vertex.x-temp_X1.x)/DX.x;
			vertex.cloud_tex_y=(vertex.y-temp_X1.y)/DX.y;
			vertex.cloud_tex_z=z1;
			float tex_x=(vertex.x-temp_X1.x)/ci->GetFractalRepeatLength();
			float tex_y=(vertex.y-temp_X1.y)/ci->GetFractalRepeatLength();
			vertex.noise_tex_x=x_anis*( tex_x*cos(noise_2d_angle)+tex_y*sin(noise_2d_angle));
			vertex.noise_tex_y=y_anis*(-tex_x*sin(noise_2d_angle)+tex_y*cos(noise_2d_angle));
			vertex.grid_x=j;
			vertex.grid_y=i;
			if(y_vertical)
				std::swap(vertex.y,vertex.z);
		}
		if(i>0)
		{
			QuadStrip &quad_strip=quadStrips[i-1];
			quad_strip.num_vertices=0;
			//Q.vertices.reserve((az_grid+1)*2);
			//Q.vertices.resize(0);
			quad_strip.distance1=(float)(i-1)/(float)NumLayers*max_cloud_distance;
			quad_strip.distance2=(float)(i  )/(float)NumLayers*max_cloud_distance;
			quad_strip.elevation1=atan2(last_z,last_r);
			quad_strip.elevation2=atan2(z1+vertical_shift,r1);
			for(unsigned j=0;j<az_grid+1;j++)
			{
				quad_strip_indices.push_back((i-1)*(az_grid)+(j%az_grid));
				quad_strip_indices.push_back((i  )*(az_grid)+(j%az_grid));
				quad_strip.num_vertices+=2;
			}
		}
		last_z=z1+vertical_shift;
		last_r=r1;
	}
}

void Cloud2DGeometryHelper::GetGrid(unsigned &el,unsigned &az) const
{
	el=el_grid;
	az=az_grid;
}
void Cloud2DGeometryHelper::SetGrid(unsigned el,unsigned az)
{
	if(el_grid==el&&az_grid==az)
		return;
	el_grid=MaxLayers;
	az_grid=az;
	delete [] noise_coords;
	delete [] last_update;
	elevations.resize(el_grid+1);
	sin_elevation.resize(el_grid+1);
	cos_elevation.resize(el_grid+1);
	sin_azimuth.resize(az_grid);
	cos_azimuth.resize(az_grid);
	if(az_grid)
	{
		noise_coords=new float[(el_grid+1)*az_grid*2];
		last_update=new int[(el_grid+1)*az_grid];
		inscatter_angular_mult.resize((el_grid+1)*az_grid);
	}
	else
	{
		last_update=NULL;
		noise_coords=NULL;
	}
	quadStrips.resize(el_grid);
	vertices.resize((el_grid+1)*az_grid);
	for(size_t i=0;i<=el_grid;i++)
	{
		float i_interp=(float)i/(float)el_grid;
		float z=-1.f+2.f*i_interp;
		sin_elevation[i]=z;
		float elevation=asin(z);
		cos_elevation[i]=cos(elevation);
		elevations[i]=elevation;
	}
	for(size_t i=0;i<az_grid;i++)
	{
		float i_interp=(float)(i)/(float)(az_grid);
		float azimuth=2.f*pi*i_interp;
		sin_azimuth[i]=sin(azimuth);
		cos_azimuth[i]=cos(azimuth);
	}
}
void Cloud2DGeometryHelper::GetCurrentGrid(unsigned &el_start,unsigned &el_end,unsigned &az_start,unsigned &az_end) const
{
	az_start=az_grid_start;
	az_end=az_grid_end;
	el_start=el_min;
	el_end=el_max;
	el_end=std::min(el_grid,el_end);		
}

float Cloud2DGeometryHelper::GetMaxCloudDistance() const
{
	return max_cloud_distance;
}


void Cloud2DGeometryHelper::CalcInscatterFactors(simul::clouds::CloudInterface *ci,simul::sky::BaseSkyInterface *skyInterface,
											   bool )
{
	if(!realtimeSlices.size())
		return;
	if(!skyInterface)
		return;
	simul::sky::float4 view_km=last_view_pos.FloatPointer(0);
	// convert metres to km:
	view_km*=0.001f;
	simul::sky::float4 sun_dir=skyInterface->GetDirectionToLight();
	static float scale=0.001f;
	// distances:
	math::Vector3 X1,X2;
	ci->GetExtents(X1,X2);
	float altitude_z=X1.z;
	float R=GetEffectiveEarthRadius(X1.z);
	float Rh=R+X1.z;
	losses_2d.resize(MaxLayers);
	inscatter_factors_2d.resize(MaxLayers);
	for(unsigned i=0;i<MaxLayers;i++)
	{
		float horiz_dist=(float)i/(float)MaxLayers*max_cloud_distance;
		float vertical_shift=sqrt(Rh*Rh-horiz_dist*horiz_dist)-Rh;
		float z1=altitude_z+vertical_shift;
		float elev=atan2(z1-last_view_pos.z,horiz_dist);
		float dist=sqrt(horiz_dist*horiz_dist+pow((z1-last_view_pos.z),2.f));
		losses_2d[i]			=skyInterface->GetIsotropicColourLossFactor	(view_km.z,elev,0,scale*dist);
		inscatter_factors_2d[i]	=skyInterface->GetIsotropicInscatterFactor	(view_km.z,elev,0,scale*dist,false,0);
	}
	Rh=R+cloudbase_altitude;
	for(unsigned i=0;i<MaxLayers;i++)
	{
		simul::math::Vector3 pos;
		float r1=(float)i/(float)MaxLayers*max_cloud_distance;
		float vertical_shift=sqrt(Rh*Rh-r1*r1)-Rh;
		pos.z=cloudbase_altitude+vertical_shift-last_view_pos.z;
		float elev=atan2(pos.z,r1);
		simul::sky::float4 inscatter_factor;
		inscatter_factor=skyInterface->GetIsotropicInscatterFactor(view_km.z,elev,0,0.001f*r1,false,0);
		for(unsigned j=0;j<az_grid;j++)
		{
			float azimuth=(float)j/(float)az_grid*2.f*pi;
			pos.x=r1*sin(azimuth);
			pos.y=r1*cos(azimuth);
			simul::math::Vector3 dir=pos-last_view_pos;
			dir.Normalize();
			float cos0=simul::sky::float4::dot(sun_dir,dir.FloatPointer(0));
			inscatter_angular_mult[i*az_grid+j]=skyInterface->GetInscatterAngularMultiplier(cos0,inscatter_factor.w,view_km.z);
		}
	}
}

simul::sky::float4 Cloud2DGeometryHelper::GetInscatter(const Cloud2DGeometryHelper::Vertex &V)
{
	return inscatter_factors_2d[V.grid_y]*inscatter_angular_mult[V.grid_y*az_grid+V.grid_x];
}

simul::sky::float4 Cloud2DGeometryHelper::GetLoss(const Cloud2DGeometryHelper::Vertex &V)
{
	return losses_2d[V.grid_y];
}

simul::sky::float4 Cloud2DGeometryHelper::GetInscatter(const simul::sky::float4 &inscatter_factor,const Cloud2DGeometryHelper::Vertex &V)
{
	return inscatter_factor*inscatter_angular_mult[V.grid_y*az_grid+V.grid_x];
}

simul::sky::float4 Cloud2DGeometryHelper::GetInscatterFactor(const Cloud2DGeometryHelper::Vertex &V)
{
	return inscatter_factors_2d[V.grid_y];
}

simul::sky::float4 Cloud2DGeometryHelper::GetFractalScales(const CloudInterface *ci) const
{
	simul::math::Vector3 X1,X2;
	ci->GetExtents(X1,X2);
	simul::math::Vector3 DX=X2-X1;
	simul::sky::float4 fractal_scales(	ci->GetFractalOffsetScale()/DX.x,
										ci->GetFractalOffsetScale()/DX.y,
										ci->GetFractalOffsetScale()/DX.z,0);
	return fractal_scales;
}


float Cloud2DGeometryHelper::GetCoverage(const CloudInterface *ci) const
{
	float coverage=0;
	float H1,H2;
	GetRenderHeights(ci,H1,H2);
	for(std::vector<RealtimeSlice*>::const_iterator i=realtimeSlices.begin();i!=realtimeSlices.end();i++)
	{
		if(!(*i))
			continue;
		if((*i)->fadeIn==0)
			continue;
		float dist=(*i)->distance;
		float a2=(H2-last_view_pos.z)/dist;
		float a1=(H1-last_view_pos.z)/dist;
		if(a2>1.f)
			a2=1.f;
		if(a1<-1.f)
			a1=-1.f;
		if(a1>1.f||a2<-1.f)
			continue;
		float arc=asin(a2)-asin(a1);
		float this_slice_coverage=2.f*pi*arc;
		coverage+=this_slice_coverage;
	}
	return coverage;
}

unsigned Cloud2DGeometryHelper::GetNumActiveLayers() const
{
	return active_slices;
}

unsigned Cloud2DGeometryHelper::GetMemoryUsage() const
{
	size_t memory=sizeof(*this);
	memory+=sizeof(RealtimeSequence)*realtimeSequences.size();
	memory+=sizeof(RealtimeSlice*)*realtimeSlices.size();
	memory+=sizeof(Vertex)*vertices.size();
	memory+=sizeof(QuadStrip)*quadStrips.size();
	memory+=sizeof(float)*(el_grid+1)*az_grid*2;
	memory+=sizeof(simul::sky::float4)*inscatter_angular_mult.size();
	memory+=sizeof(simul::sky::float4)*losses_2d.size();
	memory+=sizeof(simul::sky::float4)*inscatter_factors_2d.size();
	return (unsigned)memory;
}

void Cloud2DGeometryHelper::SetDetailFadeStrength(float dfs)
{
	DetailFadeStrength=dfs;
}

float Cloud2DGeometryHelper::GetDetailFadeStrength() const
{
	return DetailFadeStrength;
}
