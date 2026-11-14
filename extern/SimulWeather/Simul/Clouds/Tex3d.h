#ifndef SIMULCLOUDS_TEXTURE3D_H
#define SIMULCLOUDS_TEXTURE3D_H

#include "Simul/Clouds/Export.h"
SIMUL_CLOUDS_EXPORT_CLASS Texture3D
{
	float *tex;
	int X;
	int Y;
	int Z;
	bool WrapX;
	bool WrapY;
	bool WrapZ;
	bool Border;
	float Edge;
	int texsize;
	float GetBorder(int i,int j,int k);
public:
	Texture3D();
	~Texture3D();
	float Get(float u,float v,float w);
	float *Setup(int x,int y,int z,bool wrapx,bool wrapy,bool wrapz,float edge=-1.f);
};
#endif
