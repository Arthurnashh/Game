#include "precomp.h"
#include "Simul/Math/Float3.h"
#include <math.h>
using namespace simul::math;

namespace simul
{
	namespace math
	{
		float3 Exp(const float3 &c)
		{
			float3 r;
			r.x=exp(c.x);
			r.y=exp(c.y);
			r.z=exp(c.z);
			return r;
		}
		float float3::length(const float3 &r)
		{
			return sqrt(r.x*r.x+r.y*r.y+r.z*r.z);
		}
	}
}
