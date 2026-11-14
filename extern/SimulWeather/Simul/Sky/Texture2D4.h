#ifndef SIMULSKY_TEXTURE2D4_H
#define SIMULSKY_TEXTURE2D4_H
#include "Simul/Sky/Float4.h"
#include "Simul/Sky/Export.h"
SIMUL_SKY_EXPORT_CLASS Texture2D4
{
	simul::sky::float4 *tex;
	int X;
	int Y;
	bool Wrap;
	bool Border;
	simul::sky::float4 Edge;
	int texsize;
public:
	Texture2D4();
	~Texture2D4();
	const Texture2D4 &operator=(const Texture2D4 &t);
	simul::sky::float4 Get(float u,float v);
	simul::sky::float4 *Setup(int x,int y,bool wrap,bool border,simul::sky::float4 edge=simul::sky::float4(0.f,0.f,0.f,0.f));
	simul::sky::float4 *GetDataPointer();
	void Zero();
};
#endif
