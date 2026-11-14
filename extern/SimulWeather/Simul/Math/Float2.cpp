#include "precomp.h"
#include "Simul/Math/Float2.h"
#include <math.h>
using namespace simul::math;

namespace simul
{
	namespace sky
	{
		float2 Exp(const float2 &c)
		{
			float2 r;
			r.x=exp(c.x);
			r.y=exp(c.y);
			return r;
		}
	}
}


float simul::math::float2::length(const float2 &u)
{
	float size2=u.x*u.x+u.y*u.y;
	return sqrt(size2);
}
