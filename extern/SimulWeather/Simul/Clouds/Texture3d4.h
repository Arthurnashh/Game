#ifndef SIMULCLOUDS_Texture3D4_H
#define SIMULCLOUDS_Texture3D4_H
#include "Simul/Sky/Float4.h"
#include "Simul/Clouds/Export.h"
SIMUL_CLOUDS_EXPORT_CLASS Texture3D4
{
	simul::sky::float4 *tex;
	int X;
	int Y;
	int Z;
	bool WrapX;
	bool WrapY;
	bool WrapZ;
	bool Border;
	int texsize;
	simul::sky::float4 Edge;
public:
	Texture3D4();
	~Texture3D4();
	simul::sky::float4 Get(float u,float v,float w);
	simul::sky::float4 *Setup(int x,int y,int z,bool wrapx,bool wrapy,bool wrapz,simul::sky::float4 edge=simul::sky::float4(0.f,0.f,0.f,0.f));
};
#endif
