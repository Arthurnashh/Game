/*
Copyright (c) 2003-2006 Gino van den Bergen / Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/ 



#include "btGeometryUtil.h"


/*
  Make sure this dummy function never changes so that it
  can be used by probes that are checking whether the
  library is actually installed.
*/
extern "C"
{	
	void btBulletMathProbe ();

	void btBulletMathProbe () {}
}

bool	btGeometryUtil::isPointInsidePlanes(const btAlignedObjectArray<btVector3>& planeEquations, const btVector3& point, btScalar	margin)
{
	int numbrushes = planeEquations.size();
	for (int i=0;i<numbrushes;i++)
	{
		const btVector3& N1 = planeEquations[i];
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(164);
		vec_float4 tmp = vec_madd(N1.mVec, vec_sel(point.mVec, btOne, btSelVec3), btZeroVec());
		tmp = vec_add(tmp, vec_sld(tmp, tmp, 8));
		if(vec_any_gt(vec_sel(btZeroVec(), vec_sub(vec_add(tmp, vec_sld(tmp, tmp, 4) ), vec_float4(margin)), btSelVec0), btZeroVec()))
			return false;
#else
		btScalar dist = btScalar(N1.dot(point))+btScalar(N1[3])-margin;
		if (dist>btScalar(0.))
		{
			return false;
		}
#endif
	}
	return true;
		
}


bool	btGeometryUtil::areVerticesBehindPlane(const btVector3& planeNormal, const btAlignedObjectArray<btVector3>& vertices, btScalar	margin)
{
	int numvertices = vertices.size();
	for (int i=0;i<numvertices;i++)
	{
		const btVector3& N1 = vertices[i];
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(199);
		vec_float4 tmp = vec_madd(planeNormal.mVec, vec_sel(N1.mVec, btOne, btSelVec3), btZeroVec());
		tmp = vec_add(tmp, vec_sld(tmp, tmp, 8));
		if(vec_any_gt(vec_sel(btZeroVec(), vec_sub(vec_add(tmp, vec_sld(tmp, tmp, 4) ), vec_float4(margin)), btSelVec0), btZeroVec()))
			return false;
#else
		btScalar dist = btScalar(planeNormal.dot(N1))+btScalar(planeNormal[3])-margin;
		if (dist>btScalar(0.))
		{
			return false;
		}
#endif
	}
	return true;
}

bool notExist(const btVector3& planeEquation,const btAlignedObjectArray<btVector3>& planeEquations);

bool notExist(const btVector3& planeEquation,const btAlignedObjectArray<btVector3>& planeEquations)
{
	int numbrushes = planeEquations.size();
	for (int i=0;i<numbrushes;i++)
	{
		const btVector3& N1 = planeEquations[i];
		if (planeEquation.dot(N1) > btScalar(0.999))
		{
			return false;
		} 
	}
	return true;
}

void	btGeometryUtil::getPlaneEquationsFromVertices(btAlignedObjectArray<btVector3>& vertices, btAlignedObjectArray<btVector3>& planeEquationsOut )
{
		const int numvertices = vertices.size();
	// brute force:
	for (int i=0;i<numvertices;i++)
	{
		const btVector3& N1 = vertices[i];
		

		for (int j=i+1;j<numvertices;j++)
		{
			const btVector3& N2 = vertices[j];
				
			for (int k=j+1;k<numvertices;k++)
			{

				const btVector3& N3 = vertices[k];

				btVector3 planeEquation,edge0,edge1;
				edge0 = N2-N1;
				edge1 = N3-N1;
				btScalar normalSign = btScalar(1.);
				for (int ww=0;ww<2;ww++)
				{
					planeEquation = normalSign * edge0.cross(edge1);
					if (planeEquation.length2() > btScalar(0.0001))
					{
						planeEquation.normalize();
						if (notExist(planeEquation,planeEquationsOut))
						{
#ifdef BT_PS3PPU_VEC
							//btVector3::btHitPrint(0);
							//dot with result in w and then select
							vec_float4 tmp = vec_madd(planeEquation.mVec, N1.mVec, btZeroVec());
							vec_float4 result = vec_add(tmp, vec_sld(tmp, tmp, 4));
							result = vec_sld(tmp, vec_add(result, vec_sld(tmp, tmp, 8)), 4); //res in w (first sld param doesnt really matter)
							planeEquation = vec_sel(planeEquation.mVec, vec_sub(btZeroVec(), result), btSelVec3);
#else
							planeEquation.setW(-planeEquation.dot(N1));
#endif
							
								//check if inside, and replace supportingVertexOut if needed
								if (areVerticesBehindPlane(planeEquation,vertices,btScalar(0.01)))
								{
									planeEquationsOut.push_back(planeEquation);
								}
						}
					}
					normalSign = btScalar(-1.);
				}
			
			}
		}
	}

}

void	btGeometryUtil::getVerticesFromPlaneEquations(const btAlignedObjectArray<btVector3>& planeEquations , btAlignedObjectArray<btVector3>& verticesOut )
{
	const int numbrushes = planeEquations.size();
	// brute force:
	for (int i=0;i<numbrushes;i++)
	{
		const btVector3& N1 = planeEquations[i];
		

		for (int j=i+1;j<numbrushes;j++)
		{
			const btVector3& N2 = planeEquations[j];
				
			for (int k=j+1;k<numbrushes;k++)
			{

				const btVector3& N3 = planeEquations[k];

				btVector3 n2n3; n2n3 = N2.cross(N3);
				btVector3 n3n1; n3n1 = N3.cross(N1);
				btVector3 n1n2; n1n2 = N1.cross(N2);
				
				if ( ( n2n3.length2() > btScalar(0.0001) ) &&
					 ( n3n1.length2() > btScalar(0.0001) ) &&
					 ( n1n2.length2() > btScalar(0.0001) ) )
				{
					//point P out of 3 plane equations:

					//	d1 ( N2 * N3 ) + d2 ( N3 * N1 ) + d3 ( N1 * N2 )  
					//P =  -------------------------------------------------------------------------  
					//   N1 . ( N2 * N3 )  

					btScalar quotient = (N1.dot(n2n3));
					if (btFabs(quotient) > btScalar(0.000001))
					{
						quotient = btScalar(-1.) / quotient;
						n2n3 *= N1[3];
						n3n1 *= N2[3];
						n1n2 *= N3[3];
						btVector3 potentialVertex = n2n3;
						potentialVertex += n3n1;
						potentialVertex += n1n2;
						potentialVertex *= quotient;

						//check if inside, and replace supportingVertexOut if needed
						if (isPointInsidePlanes(planeEquations,potentialVertex,btScalar(0.01)))
						{
							verticesOut.push_back(potentialVertex);
						}
					}
				}
			}
		}
	}
}

