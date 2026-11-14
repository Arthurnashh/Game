#include "Cutout.h"
#include <float.h>
#include <assert.h>
using namespace simul;
using namespace math;
using namespace terrain;

Cutout::Cutout()
{
}

Cutout::~Cutout()
{
	vertices.clear();
}

size_t Cutout::NumVertices() const
{
	return vertices.size();
}

void Cutout::Clear()
{
	vertices.clear();
}

void Cutout::GetCorners(float2 &min_corner,float2 &max_corner) const
{
	min_corner.Set(1e26f,1e26f);
	max_corner.Set(-1e26f,-1e26f);
	for(size_t i=0;i<vertices.size();i++)
	{
		min_corner=simul::math::float2::Min(min_corner,vertices[i].pos);
		max_corner=simul::math::float2::Max(max_corner,vertices[i].pos);
	}
}

void Cutout::unuse() const
{
	for(size_t i=0;i<vertices.size();i++)
	{
		vertices[i].used=false;
	}
}

void Cutout::addvertex(const simul::math::float2 &v,float h)
{
	vertices.push_back(Vert());
	vertices.back().used=false;
	vertices.back().pos=v;
	vertices.back().height=h;
}
void Cutout::RemoveVertex(size_t v)
{
	vertices.erase(vertices.begin()+v);
}

/*
divide the polygon into quadrants centered on the test point, as in Figure 4.
Start at the first vertex in the polygon and set a counter to 0. Anytime an edge
crosses from one quadrant to the next, add one to the counter if it crosses
clockwise around the test point and subtract one if it crosses counter-clockwise.
If the edge crosses diagonally across two quadrants, you need to determine which
side of the test point it crossed, and then either add or subtract 2. 
if the counter is equal to 4 or –4, the test point is inside the polygon.
*/
bool Cutout::IsInside(const float2 &point) const
{
	int counter=0;
	for(size_t i=0;i<vertices.size();i++)
	{
		const float2 &p1=vertex((int)i).pos;
		const float2 &p2=vertex((int)i+1).pos;
		// origin of x1 and x2 is the test point
		float2 x1=p1-point;
		float2 x2=p2-point;
		// can't cope with zeroes! so nudge them:
		if(x1.x==0.f)
			x1.x=1e-7f;
		if(x1.y==0.f)
			x1.y=1e-7f;
		if(x2.x==0.f)
			x2.x=1e-7f;
		if(x2.y==0.f)
			x2.y=1e-7f;
		// if x1 and x2 are in the same quadrant, continue:
		if(x1.x*x2.x>=0&&x1.y*x2.y>=0)
			continue;
		// if we don't cross the x-axis, we are crossing the y-axis:
		if(x1.y*x2.y>0)
		{
			if(x1.y*x1.x<0)
				counter++;
			else
				counter--;
			continue;
		}
		// if we don't cross the y-axis, we are crossing the x-axis
		else if(x1.x*x2.x>0)
		{
			// if x>0 and crossing from -ve y axis, or x<0 and crossing from +ve y, it's anti-clockwise.
			if(x1.y*x1.x>0)
				counter++;
			else
				counter--;
			continue;
		}
		// so we're crossing both axes!
		// clockwise or anticlockwise? -ve cross-product means clockwise.
		float2 dx=x2-x1;
		x1/=float2::length(x1);
		dx/=float2::length(dx);
		float cp=float2::cross(x1,dx);
		if(cp<0)
			counter+=2;
		else
			counter-=2;
	}
	return (counter==4);
}

int Cutout::GetCrossover(const float2 &x1,const float2 &x2,float2 &intersection,int &edge) const
{
	float2 dx=x2-x1;
	float xmin=1e26f;
	int result=0;
	for(size_t i=0;i<vertices.size();i++)
	{
		if(vertex((int)i).used)
			continue;
		float2 p1=vertex((int)i).pos-x1;
		float2 p2=vertex((int)i+1).pos-x1;
		float2 dp=p2-p1;
		// i=a dx = p1 + b dp;
		// x dx2= p1.dx + p dp.dx;

		// x (dx.dp)=p1.dp + p dp2
		// p = (x (dx.dp) - p1.dp)/dp2

		// x = (p1 + p dp).dx / dx2
		//   = (p1 + (x (dx.dp) - p1.dp) dp /dp2).dx/dx2
		// x = (p1 - (p1.dp) dp/dp2).dx/dx2 + x (dx.dp) dp.dx/(dp2 dx2)
		// x(1-(dx.dp)^2/(dp2 dx2))= (p1 - (p1.dp) dp/dp2).dx/dx2

		// a = (p1.dx + b dp.dx) /dx2
		// a(dx.dp)= p1.dp + b dp.dp;
		// (p1.dx + b dp.dx)/dx2= p1.dp + b dp.dp
		// p1.dx/dx2 - p1.dp = b (dp2 - dp.dx/dx2)
		// b = (p1.dx/dx2-p1.dp)/(dp2 - dp.dx/dx2)
		// b = (p1.(dx/dx2-dp))/(dp2 - dp.dx/dx2)
		// b = p1.(dx-dx2 dp)/(dp2 dx2- dp.dx)
		float dxdp=float2::dot(dx,dp);
		float dx2=float2::dot(dx,dx);
		float dp2=float2::dot(dp,dp);
		float p1dp=float2::dot(p1,dp);
		float p1dx=float2::dot(p1,dx);

		float x=(p1dx*dp2-dxdp*p1dp)/(dx2*dp2-(dxdp*dxdp));
		float p = (x *dxdp-p1dp)/dp2;

		if(x<0||x>1.f||x>xmin)
			continue;
		if(!_finite(x))
			continue;
/*
		float p = float2::dot(p1,dx-dx2*dp)/(dx2*dp2-float2::dot(dp,dx));*/
		if(p<0||p>1.f)
			continue;
		if(!_finite(p))
			continue;
	//	float x = float2::dot(p1+p*dp,dx)/dx2;
		intersection=x1+x*dx;
		xmin=x;
		result=float2::cross(dx,dp)>0?1:-1;
		edge=(int)i;
	}
	return result;
}

size_t Cutout::FindEar() const
{
	for(int i=0;i<(int)vertices.size();i++)
	{
		const float2 &x=vertex(i).pos;
		const float2 &p1=vertex(i-1).pos;
		const float2 &p2=vertex(i+1).pos;
		float2 p2p1=p1-p2;
		float2 p1x=x-p1;
		float2 xp2=p2-x;
		bool ok=true;
		for(int j=0;j<(int)vertices.size();j++)
		{
			if(j==i||j==(int)loopindex(i-1)||j==(int)loopindex(i+1))
			{
				continue;
			}
			const float2 &y=vertices[j].pos;
			float2 p1y=y-p1;
			float2 xy=y-x;
			float2 p2y=y-p2;
			if(float2::cross(p1y,p1x)>0&&float2::cross(xy,xp2)>0&&
				float2::cross(p2y,p2p1)>0)
			{
				ok=false;
				break;
			}
		}
		if(ok)
			return i;
	}
	return 0xFFFFFFFF;
}

float Cutout::GetHeightAt(int edge,float2 &intersection) const
{
	const float2 &x1=vertex(edge).pos;
	const float2 &x2=vertex(edge+1).pos;
	float2 dx=x2-x1;
	float2 xi=intersection-x1;
	float l=float2::length(dx);
	float along=float2::dot(xi,dx)/(l*l);
	assert(along>=0.f&&along<=1.f);
	float h=vertex(edge).height*(1.f-along)+vertex(edge+1).height*along;
	return h;
}