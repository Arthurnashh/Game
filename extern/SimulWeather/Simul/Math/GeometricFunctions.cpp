#include "precomp.h"
#include "GeometricFunctions.h"
#include "Simul/Math/Vector3.h"

namespace simul
{
	namespace math
	{
void SupportPoint(float *PointsX,float *PointsY,float *PointsZ,const Vector3 &D,int NumPoints,Vector3 &X1,int &i1)
{
	i1=-1;
	float furthest=-1e15f;
	const float &Dx=D(0);
	const float &Dy=D(1);
	const float &Dz=D(2);
	for(int i=0;i<NumPoints;i++)
	{
		float extent=PointsX[i]*Dx+PointsY[i]*Dy+PointsZ[i]*Dz;
		if(extent>furthest)
		{
			furthest=extent;
			i1=i;
		}
	}
	X1(0)=PointsX[i1];
	X1(1)=PointsY[i1];
	X1(2)=PointsZ[i1];
}
	}
}
