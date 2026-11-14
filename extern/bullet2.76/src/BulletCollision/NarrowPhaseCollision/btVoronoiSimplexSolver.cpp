
/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
	
	Elsevier CDROM license agreements grants nonexclusive license to use the software
	for any purpose, commercial or non-commercial as long as the following credit is included
	identifying the original source of the software:

	Parts of the source are "from the book Real-Time Collision Detection by
	Christer Ericson, published by Morgan Kaufmann Publishers,
	(c) 2005 Elsevier Inc."
		
*/


#include "btVoronoiSimplexSolver.h"

#define VERTA  0
#define VERTB  1
#define VERTC  2
#define VERTD  3

#define CATCH_DEGENERATE_TETRAHEDRON 1

void	btVoronoiSimplexSolver::reduceVertices (btUsageBitfield usedVerts)
{
	int currentVertices = m_numVertices;
	for(int n = 3; n >= 0; n--)
	{
		if ((currentVertices >= (n + 1)) && (usedVerts & (1 << n)) == 0)
		{
			currentVertices--;
			m_simplexVectorW[n] = m_simplexVectorW[currentVertices];
			m_simplexPointsP[n] = m_simplexPointsP[currentVertices];
			m_simplexPointsQ[n] = m_simplexPointsQ[currentVertices];
		}
	}
	m_numVertices = currentVertices;
}

//clear the simplex, remove all the vertices
void btVoronoiSimplexSolver::reset()
{
	m_cachedValidClosest = false;
	m_numVertices = 0;
	m_needsUpdate = true;
	m_lastW = btVector3(btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT));
	m_cachedBC.reset();
}

inline void closestPtPointTriangle(const btVector3& p, const btVector3& a, const btVector3& b, const btVector3& c,btSubSimplexClosestResult& result)
{
	// Check if P in vertex region outside A
	btVector3 ab = b - a;
	btVector3 ac = c - a;
	btVector3 ap = p - a;
	btScalar d1 = ab.dot(ap);
	btScalar d2 = ac.dot(ap);
	if (d1 <= btScalar(0.0) && d2 <= btScalar(0.0)) 
	{
		result.m_closestPointOnSimplex = a;
		result.m_usedVertices = UBF_USED_VERTEX_A;
		result.setBarycentricCoordinates(1,0,0);
		return;
	}

	// Check if P in vertex region outside B
	btVector3 bp = p - b;
	btScalar d3 = ab.dot(bp);
	btScalar d4 = ac.dot(bp);
	if (d3 >= btScalar(0.0) && d4 <= d3) 
	{
		result.m_closestPointOnSimplex = b;
		result.m_usedVertices = UBF_USED_VERTEX_B;
		result.setBarycentricCoordinates(0,1,0);
		return;
	}
	// Check if P in edge region of AB, if so return projection of P onto AB
	btScalar vc = d1*d4 - d3*d2;
	if (vc <= btScalar(0.0) && d1 >= btScalar(0.0) && d3 <= btScalar(0.0)) {
		btScalar v = d1 / (d1 - d3);
		result.m_closestPointOnSimplex = a + v * ab;
		result.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_B;
		result.setBarycentricCoordinates(1-v,v,0);
		return;
	}

	// Check if P in vertex region outside C
	btVector3 cp = p - c;
	btScalar d5 = ab.dot(cp);
	btScalar d6 = ac.dot(cp);
	if (d6 >= btScalar(0.0) && d5 <= d6) 
	{
		result.m_closestPointOnSimplex = c;
		result.m_usedVertices = UBF_USED_VERTEX_C;
		result.setBarycentricCoordinates(0,0,1);
		return;
	}

	// Check if P in edge region of AC, if so return projection of P onto AC
	btScalar vb = d5*d2 - d1*d6;
	if (vb <= btScalar(0.0) && d2 >= btScalar(0.0) && d6 <= btScalar(0.0)) {
		btScalar w = d2 / (d2 - d6);
		result.m_closestPointOnSimplex = a + w * ac;
		result.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_C;
		result.setBarycentricCoordinates(1-w,0,w);
		return;
	}

	// Check if P in edge region of BC, if so return projection of P onto BC
	btScalar va = d3*d6 - d5*d4;
	if (va <= btScalar(0.0) && (d4 - d3) >= btScalar(0.0) && (d5 - d6) >= btScalar(0.0)) {
		btScalar w = (d4 - d3) / ((d4 - d3) + (d5 - d6));

		result.m_closestPointOnSimplex = b + w * (c - b);
		result.m_usedVertices = UBF_USED_VERTEX_B|UBF_USED_VERTEX_C;
		result.setBarycentricCoordinates(0,1-w,w);
		return;	
	}

	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	btScalar denom = btScalar(1.0) / (va + vb + vc);
	btScalar v = vb * denom;
	btScalar w = vc * denom;

	result.m_closestPointOnSimplex = a + ab * v + ac * w;
	result.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_B|UBF_USED_VERTEX_C;
	result.setBarycentricCoordinates(1-v-w,v,w);
}

btScalar closestPtPointDistTriangle(const btVector3& p, const btVector3& a, const btVector3& b, const btVector3& c,btSubSimplexClosestResult& result)
{
	// Check if P in vertex region outside A
	btVector3 ab = b - a;
	btVector3 ac = c - a;
	btVector3 ap = p - a;
	btScalar d1 = ab.dot(ap);
	btScalar d2 = ac.dot(ap);
	if (d1 <= btScalar(0.0) && d2 <= btScalar(0.0)) 
	{
		result.m_closestPointOnSimplex = a;
		result.m_usedVertices = UBF_USED_VERTEX_A;
		result.setBarycentricCoordinates(1,0,0);
		return (a - p).length2();
	}

	// Check if P in vertex region outside B
	btVector3 bp = p - b;
	btScalar d3 = ab.dot(bp);
	btScalar d4 = ac.dot(bp);
	if (d3 >= btScalar(0.0) && d4 <= d3) 
	{
		result.m_closestPointOnSimplex = b;
		result.m_usedVertices = UBF_USED_VERTEX_B;
		result.setBarycentricCoordinates(0,1,0);
		return (b - p).length2();
	}
	// Check if P in edge region of AB, if so return projection of P onto AB
	btScalar vc = d1*d4 - d3*d2;
	if (vc <= btScalar(0.0) && d1 >= btScalar(0.0) && d3 <= btScalar(0.0)) {
		btScalar v = d1 / (d1 - d3);
		btVector3 closestPointOnSimplex = a + v * ab;
		result.m_closestPointOnSimplex = closestPointOnSimplex;
		result.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_B;
		result.setBarycentricCoordinates(1-v,v,0);
		return (closestPointOnSimplex - p).length2();
	}

	// Check if P in vertex region outside C
	btVector3 cp = p - c;
	btScalar d5 = ab.dot(cp);
	btScalar d6 = ac.dot(cp);
	if (d6 >= btScalar(0.0) && d5 <= d6) 
	{
		result.m_closestPointOnSimplex = c;
		result.m_usedVertices = UBF_USED_VERTEX_C;
		result.setBarycentricCoordinates(0,0,1);
		return (c - p).length2();
	}

	// Check if P in edge region of AC, if so return projection of P onto AC
	btScalar vb = d5*d2 - d1*d6;
	if (vb <= btScalar(0.0) && d2 >= btScalar(0.0) && d6 <= btScalar(0.0)) {
		btScalar w = d2 / (d2 - d6);
		btVector3 closestPointOnSimplex = a + w * ac;
		result.m_closestPointOnSimplex = closestPointOnSimplex;
		result.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_C;
		result.setBarycentricCoordinates(1-w,0,w);
		return (closestPointOnSimplex - p).length2();
	}

	// Check if P in edge region of BC, if so return projection of P onto BC
	btScalar va = d3*d6 - d5*d4;
	if (va <= btScalar(0.0) && (d4 - d3) >= btScalar(0.0) && (d5 - d6) >= btScalar(0.0)) {
		btScalar w = (d4 - d3) / ((d4 - d3) + (d5 - d6));

		btVector3 closestPointOnSimplex = b + w * (c - b);
		result.m_closestPointOnSimplex = closestPointOnSimplex;
		result.m_usedVertices = UBF_USED_VERTEX_B|UBF_USED_VERTEX_C;
		result.setBarycentricCoordinates(0,1-w,w);
		return (closestPointOnSimplex - p).length2();
	}

	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	btScalar denom = btScalar(1.0) / (va + vb + vc);
	btScalar v = vb * denom;
	btScalar w = vc * denom;

	btVector3 closestPointOnSimplex = a + ab * v + ac * w;
	result.m_closestPointOnSimplex = closestPointOnSimplex;
	result.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_B|UBF_USED_VERTEX_C;
	result.setBarycentricCoordinates(1-v-w,v,w);
	return (closestPointOnSimplex - p).length2();
}

bool	btVoronoiSimplexSolver::updateClosestVectorAndPoints()
{
	if (m_needsUpdate)
	{
		m_cachedBC.reset();

		m_needsUpdate = false;

		switch (numVertices())
		{
		case 0:
				m_cachedValidClosest = false;
				break;
		case 1:
			{
				m_cachedP1 = m_simplexPointsP[0];
				m_cachedP2 = m_simplexPointsQ[0];
				m_cachedV = m_cachedP1-m_cachedP2; //== m_simplexVectorW[0]
				m_cachedBC.reset();
				m_cachedBC.setBarycentricCoordinates(btScalar(1.),btScalar(0.),btScalar(0.),btScalar(0.));
				m_cachedValidClosest = m_cachedBC.isValid();
				break;
			};
		case 2:
			{
			//closest point origin from line segment
					const btVector3& from = m_simplexVectorW[0];
					const btVector3& to = m_simplexVectorW[1];
					btVector3 nearest;

#ifdef BT_PS3PPU_VEC
					btVector3 p = btZeroVec();
#else
					btVector3 p (btScalar(0.),btScalar(0.),btScalar(0.));
#endif
					btVector3 diff = p - from;
					btVector3 v = to - from;
					btScalar t = v.dot(diff);
					
					if (t > 0) {
						btScalar dotVV = v.dot(v);
						if (t < dotVV) {
							t /= dotVV;
							diff -= t*v;
							m_cachedBC.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_B;
						} else {
							t = 1;
							diff -= v;
							//reduce to 1 point
							m_cachedBC.m_usedVertices = UBF_USED_VERTEX_B;
						}
					} else
					{
						t = 0;
						//reduce to 1 point
						m_cachedBC.m_usedVertices = UBF_USED_VERTEX_A;
					}
					m_cachedBC.setBarycentricCoordinates(1-t,t);
					nearest = from + t*v;

					m_cachedP1 = m_simplexPointsP[0] + t * (m_simplexPointsP[1] - m_simplexPointsP[0]);
					m_cachedP2 = m_simplexPointsQ[0] + t * (m_simplexPointsQ[1] - m_simplexPointsQ[0]);
					m_cachedV = m_cachedP1 - m_cachedP2;
					
					reduceVertices(m_cachedBC.m_usedVertices);

					m_cachedValidClosest = m_cachedBC.isValid();
					break;
			}
		case 3: 
			{ 
				//closest point origin from triangle 
#ifdef BT_PS3PPU_VEC
					btVector3 p = btZeroVec();
#else
					btVector3 p (btScalar(0.),btScalar(0.),btScalar(0.));
#endif
				const btVector3& a = m_simplexVectorW[0]; 
				const btVector3& b = m_simplexVectorW[1]; 
				const btVector3& c = m_simplexVectorW[2]; 

				closestPtPointTriangle(p,a,b,c,m_cachedBC); 
				m_cachedP1 = m_simplexPointsP[0] * m_cachedBC.m_barycentricCoords[0] + 
				m_simplexPointsP[1] * m_cachedBC.m_barycentricCoords[1] + 
				m_simplexPointsP[2] * m_cachedBC.m_barycentricCoords[2]; 

				m_cachedP2 = m_simplexPointsQ[0] * m_cachedBC.m_barycentricCoords[0] + 
				m_simplexPointsQ[1] * m_cachedBC.m_barycentricCoords[1] + 
				m_simplexPointsQ[2] * m_cachedBC.m_barycentricCoords[2]; 

				m_cachedV = m_cachedP1-m_cachedP2; 

				reduceVertices (m_cachedBC.m_usedVertices); 
				m_cachedValidClosest = m_cachedBC.isValid(); 

				break; 
			}
		case 4:
			{

				
				btVector3 p (btScalar(0.),btScalar(0.),btScalar(0.));
				
				const btVector3& a = m_simplexVectorW[0];
				const btVector3& b = m_simplexVectorW[1];
				const btVector3& c = m_simplexVectorW[2];
				const btVector3& d = m_simplexVectorW[3];

				bool hasSeperation = closestPtPointTetrahedron(p,a,b,c,d,m_cachedBC);

				if (hasSeperation)
				{

					m_cachedP1 = m_simplexPointsP[0] * m_cachedBC.m_barycentricCoords[0] +
						m_simplexPointsP[1] * m_cachedBC.m_barycentricCoords[1] +
						m_simplexPointsP[2] * m_cachedBC.m_barycentricCoords[2] +
						m_simplexPointsP[3] * m_cachedBC.m_barycentricCoords[3];

					m_cachedP2 = m_simplexPointsQ[0] * m_cachedBC.m_barycentricCoords[0] +
						m_simplexPointsQ[1] * m_cachedBC.m_barycentricCoords[1] +
						m_simplexPointsQ[2] * m_cachedBC.m_barycentricCoords[2] +
						m_simplexPointsQ[3] * m_cachedBC.m_barycentricCoords[3];

					m_cachedV = m_cachedP1-m_cachedP2;
					reduceVertices (m_cachedBC.m_usedVertices);
				} else
				{
//					printf("sub distance got penetration\n");

					if (m_cachedBC.m_degenerate)
					{
						m_cachedValidClosest = false;
					} else
					{
						m_cachedValidClosest = true;
						//degenerate case == false, penetration = true + zero
						m_cachedV.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
					}
					break;
				}

				m_cachedValidClosest = m_cachedBC.isValid();

				//closest point origin from tetrahedron
				break;
			}
		default:
			{
				m_cachedValidClosest = false;
			}
		};
	}

	return m_cachedValidClosest;

}

//return/calculate the closest vertex
bool btVoronoiSimplexSolver::closest(btVector3& v)
{
	bool succes = updateClosestVectorAndPoints();
	v = m_cachedV;
	return succes;
}



btScalar btVoronoiSimplexSolver::maxVertex()
{
	int i, numverts = numVertices();
	btScalar maxV = btScalar(0.);
	for (i=0;i<numverts;i++)
	{
		btScalar curLen2 = m_simplexVectorW[i].length2();
		if (maxV < curLen2)
			maxV = curLen2;
	}
	return maxV;
}



	//return the current simplex
int btVoronoiSimplexSolver::getSimplex(btVector3 *pBuf, btVector3 *qBuf, btVector3 *yBuf) const
{
	int i;
	for (i=0;i<numVertices();i++)
	{
		yBuf[i] = m_simplexVectorW[i];
		pBuf[i] = m_simplexPointsP[i];
		qBuf[i] = m_simplexPointsQ[i];
	}
	return numVertices();
}




bool btVoronoiSimplexSolver::inSimplex(const btVector3& w)
{
	bool found = false;
	int i, numverts = numVertices();
	//btScalar maxV = btScalar(0.);
	
	//w is in the current (reduced) simplex
	for (i=0;i<numverts;i++)
	{
#ifdef BT_USE_EQUAL_VERTEX_THRESHOLD
		if ( m_simplexVectorW[i].distance2(w) <= m_equalVertexThreshold)
#else
		if (m_simplexVectorW[i] == w)
#endif
			found = true;
	}

	//check in case lastW is already removed
	if (w == m_lastW)
		return true;
    	
	return found;
}

void btVoronoiSimplexSolver::backup_closest(btVector3& v) 
{
	v = m_cachedV;
}


bool btVoronoiSimplexSolver::emptySimplex() const 
{
	return (numVertices() == 0);

}

void btVoronoiSimplexSolver::compute_points(btVector3& p1, btVector3& p2) 
{
	updateClosestVectorAndPoints();
	p1 = m_cachedP1;
	p2 = m_cachedP2;

}





/// Test if point p and d lie on opposite sides of plane through abc
inline int pointOutsideOfPlane(const btVector3& p, const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& d)
{
	btVector3 normal = (b-a).cross(c-a);

	btScalar signp = (p - a).dot(normal); // [AP AB AC]
	btScalar signd = (d - a).dot(normal); // [AD AB AC]

#ifdef CATCH_DEGENERATE_TETRAHEDRON
#ifdef BT_USE_DOUBLE_PRECISION
	if (signd * signd < (btScalar(1e-8) * btScalar(1e-8)))
	{
		return -1;
	}
#else
	if (signd * signd < (btScalar(1e-4) * btScalar(1e-4)))
	{
//		printf("affine dependent/degenerate\n");//
		return -1;
	}
#endif

#endif
	// Points on opposite sides if expression signs are opposite
    return signp * signd < btScalar(0.);
}

#ifdef BT_PS3PPU_VEC
vec_float4 VectorCross(vec_float4 v1, vec_float4 v2)
{
	vec_float4 vec1021 = vec_vmrghw(vec_sld(v1, v1, 4), v1);
	vec_float4 vec1201a = vec_vmrghw(vec1021, vec_sld(vec1021, vec1021, 8));

	vec1021 = vec_vmrghw(vec_sld(v2, v2, 4), v2);
	vec_float4 vec1201b = vec_vmrghw(vec1021, vec_sld(vec1021, vec1021, 8));

	return vec_sub(vec_madd(vec1201a, vec_sld(vec1201b, vec1201b, 4), btZeroVec()), vec_madd(vec1201b, vec_sld(vec1201a, vec1201a, 4), btZeroVec()));
}

inline vec_int4 pointOutsideOfPlane(vec_float4 p, vec_float4 a, vec_float4 b, vec_float4 c, vec_float4 d, vec_float4 treshold)
{
	//btVector3::btHitPrint(94);
	vec_float4 normal = VectorCross(vec_sub(b, a), vec_sub(c, a));

	vec_float4 signp = vec_dot3(vec_sub(p, a), normal); // [AP AB AC]
	vec_float4 signd = vec_dot3(vec_sub(d, a), normal); // [AD AB AC]

	vec_float4 signSq = vec_madd(signd, signd, btZeroVec());
	
	vector bool deg = vec_cmpgt(treshold, signSq);

	vec_float4 signPD = vec_madd(signp, signd, btZeroVec());

	vector bool opposite = vec_cmpgt(btZeroVec(), signPD);

	vec_int4 result = vec_and((vec_int4)opposite, vec_splats(1));
	// Points on opposite sides if expression signs are opposite
	//return signPD.x < btScalar(0.);
	return vec_sel((vec_int4)deg, result, vec_splats((unsigned int)-1));
}

#elif defined(BT_USE_VMX128)
__vector4 VectorLoad(const float* ptr)
{
	btAssert(ptr);
	btAssert(((intptr_t)ptr & 0xf) == 0);

	return __lvx(ptr, 0);
}
void VectorStore(float* ptr, __vector4 val)
{
	btAssert(ptr);
	btAssert(((intptr_t)ptr & 0xf) == 0);
	__stvx(val, ptr, 0);
}

__vector4 VectorCross(__vector4 v1, __vector4 v2)
{
__vector4 crossV1;
__vector4 crossV2;
__vector4 crossR;

	crossV1 = __vpermwi(v1, 0x63);
	crossV2 = __vpermwi(v2, 0x87);
	crossR = __vmulfp(crossV1, crossV2);
	crossV1 = __vpermwi(v1, 0x87);
	crossV2 = __vpermwi(v2, 0x63);
	crossR = __vnmsubfp(crossV1, crossV2, crossR);
	return crossR;
}

inline __vector4 pointOutsideOfPlane(__vector4 p, __vector4 a, __vector4 b, __vector4 c, __vector4 d, __vector4 treshold)
{
	__vector4 normal = VectorCross(__vsubfp(b, a), __vsubfp(c, a));

	__vector4 signp = __vmsum3fp(__vsubfp(p, a), normal); // [AP AB AC]
	__vector4 signd = __vmsum3fp(__vsubfp(d, a), normal); // [AD AB AC]

	__vector4 signSq = __vmulfp(signd, signd);
	
	__vector4 deg = __vcmpgtfp(treshold, signSq);

/*	if (signSq.x < (btScalar(1e-4) * btScalar(1e-4)))
	{
		//		printf("affine dependent/degenerate\n");//
		return -1;
	}*/

	__vector4 signPD = __vmulfp(signp, signd);

	__vector4 opposite = __vcmpgtfp(__vspltisw(0), signPD);

	__vector4 result = __vand(opposite, __vspltisw(1));
	// Points on opposite sides if expression signs are opposite
	//return signPD.x < btScalar(0.);
	return __vsel(deg, result, __vspltisw(-1));
}
#endif

#define XM_CRMASK_CR6       0x000000F0
#define XM_CRMASK_CR6TRUE   0x00000080
#define XM_CRMASK_CR6FALSE  0x00000020
#define XM_CRMASK_CR6BOUNDS XM_CRMASK_CR6FALSE

#define XMComparisonAnyTrue(CR)  (((CR) & XM_CRMASK_CR6FALSE) != XM_CRMASK_CR6FALSE)
#define XMComparisonAllTrue(CR)  (((CR) & XM_CRMASK_CR6TRUE) == XM_CRMASK_CR6TRUE)
#define XMComparisonAnyFalse(CR)  (((CR) & XM_CRMASK_CR6TRUE) != XM_CRMASK_CR6TRUE)
#define XMComparisonAllFalse(CR)  (((CR) & XM_CRMASK_CR6FALSE) == XM_CRMASK_CR6FALSE)

bool	btVoronoiSimplexSolver::closestPtPointTetrahedron(const btVector3& p, const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& d, btSubSimplexClosestResult& finalResult)
{
	btSubSimplexClosestResult tempResult;

	// Start out assuming point inside all halfspaces, so closest to itself
	finalResult.m_closestPointOnSimplex = p;
	finalResult.m_usedVertices = UBF_USED_VERTEX_A|UBF_USED_VERTEX_B|UBF_USED_VERTEX_C|UBF_USED_VERTEX_D;

#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(95);
	vec_float4 threshold = {1e-4 * 1e-4, 1e-4 * 1e-4, 1e-4 * 1e-4, 1e-4 * 1e-4};
	vec_float4 vp = p.get128();
	vec_float4 va = a.get128();
	vec_float4 vb = b.get128();
	vec_float4 vc = c.get128();
	vec_float4 vd = d.get128();

	vec_int4 result;
	__declspec(align(16)) static const int mask[] = {0xffffffff, 0x00000000, 0x00000000, 0x00000000};
	vec_int4 vmask0 = vec_lvlx(0, (const vec_int4*)mask);

	result = vec_andc(pointOutsideOfPlane(vp, va, vb, vc, vd, threshold), vmask0);
	vmask0 = vec_sld(vmask0, vmask0, 12);
	result = vec_or(vec_and(pointOutsideOfPlane(vp, va, vc, vd, vb, threshold), vmask0), result);
	vmask0 = vec_sld(vmask0, vmask0, 12);
	result = vec_or(vec_and(pointOutsideOfPlane(vp, va, vd, vb, vc, threshold), vmask0), result);
	vmask0 = vec_sld(vmask0, vmask0, 12);
	result = vec_or(vec_and(pointOutsideOfPlane(vp, vb, vd, vc, va, threshold), vmask0), result);

	vec_uint4 eqTest = (vec_uint4)vec_cmpeq(result, vec_splats(-1));
	eqTest = vec_or(eqTest, vec_sld(eqTest, eqTest, 8));
	eqTest = vec_or(eqTest, vec_sld(eqTest, eqTest, 4));

	if(eqTest[0])
	{
		finalResult.m_degenerate = true;
		return false;
	}

	eqTest = (vec_uint4)vec_cmpeq(result, vec_splats(0));
	eqTest = vec_and(eqTest, vec_sld(eqTest, eqTest, 8));
	eqTest = vec_and(eqTest, vec_sld(eqTest, eqTest, 4));
	if(eqTest[0])
	{
		return false;
	}


	__declspec(align(16)) union
	{
		int pointOutside[4];
		struct 
		{
			int pointOutsideABC;
			int pointOutsideACD;
			int pointOutsideADB;
			int pointOutsideBDC;
		};
	};
	
	assert(((uintptr_t)pointOutside & 0xf)  == 0);
	vec_stvlx(result, 0, (vec_int4*)pointOutside);
#elif defined(BT_USE_VMX128)
	__vector4 threshold = {1e-4 * 1e-4, 1e-4 * 1e-4, 1e-4 * 1e-4, 1e-4 * 1e-4};
	__vector4 vp = VectorLoad(p.m_floats);
	__vector4 va = VectorLoad(a.m_floats);
	__vector4 vb = VectorLoad(b.m_floats);
	__vector4 vc = VectorLoad(c.m_floats);
	__vector4 vd = VectorLoad(d.m_floats);

	__vector4 result;
	__declspec(align(16)) static const int mask[] = {0xffffffff, 0x00000000, 0x00000000, 0x00000000};
	__vector4 vmask0 = VectorLoad((const float*)mask);

	result = __vandc(pointOutsideOfPlane(vp, va, vb, vc, vd, threshold), vmask0);
	vmask0 = __vpermwi(vmask0, VPERMWI_CONST(1, 0, 1, 1));
	result = __vor(__vand(pointOutsideOfPlane(vp, va, vc, vd, vb, threshold), vmask0), result);
	vmask0 = __vpermwi(vmask0, VPERMWI_CONST(0, 2, 1, 0));
	result = __vor(__vand(pointOutsideOfPlane(vp, va, vd, vb, vc, threshold), vmask0), result);
	vmask0 = __vpermwi(vmask0, VPERMWI_CONST(0, 1, 3, 2));
	result = __vor(__vand(pointOutsideOfPlane(vp, vb, vd, vc, va, threshold), vmask0), result);

	unsigned int CR;
	__vcmpequwR(result, __vspltisw(-1), &CR); 

	if(XMComparisonAnyTrue(CR))
	{
		finalResult.m_degenerate = true;
		return false;
	}

	__vcmpequwR(result, __vspltisw(0), &CR); 
	if (XMComparisonAllTrue(CR))
	{
		return false;
	}


	__declspec(align(16)) union
	{
		int pointOutside[4];
		struct 
		{
			int pointOutsideABC;
			int pointOutsideACD;
			int pointOutsideADB;
			int pointOutsideBDC;
		};
	};

	VectorStore((float*)pointOutside, result);

#else
	int pointOutsideABC = pointOutsideOfPlane(p, a, b, c, d);
	int pointOutsideACD = pointOutsideOfPlane(p, a, c, d, b);
	int pointOutsideADB = pointOutsideOfPlane(p, a, d, b, c);
	int pointOutsideBDC = pointOutsideOfPlane(p, b, d, c, a);


	if (pointOutsideABC < 0 || pointOutsideACD < 0 || pointOutsideADB < 0 || pointOutsideBDC < 0)
	{
		finalResult.m_degenerate = true;
		return false;
	}

	if (!pointOutsideABC  && !pointOutsideACD && !pointOutsideADB && !pointOutsideBDC)
	{
		return false;
	}
#endif
	
	btScalar bestSqDist = FLT_MAX;
	// If point outside face abc then compute closest point on abc
	if (pointOutsideABC) 
	{
		btScalar sqDist = closestPtPointDistTriangle(p, a, b, c,tempResult);
		// Update best closest point if (squared) distance is less than current best
		if (sqDist < bestSqDist) {
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = tempResult.m_closestPointOnSimplex;
			//convert result bitmask!
			//finalResult.m_usedVertices.reset();
			//finalResult.m_usedVertices.usedVertexA = tempResult.m_usedVertices.usedVertexA;
			//finalResult.m_usedVertices.usedVertexB = tempResult.m_usedVertices.usedVertexB;
			//finalResult.m_usedVertices.usedVertexC = tempResult.m_usedVertices.usedVertexC;
			finalResult.m_usedVertices = (tempResult.m_usedVertices & (UBF_USED_VERTEX_A|UBF_USED_VERTEX_B|UBF_USED_VERTEX_C));
			finalResult.setBarycentricCoordinates(
					tempResult.m_barycentricCoords[VERTA],
					tempResult.m_barycentricCoords[VERTB],
					tempResult.m_barycentricCoords[VERTC],
					0
			);
		}
	}

	// Repeat test for face acd
	if (pointOutsideACD) 
	{
		//convert result bitmask!

		btScalar sqDist = closestPtPointDistTriangle(p, a, c, d,tempResult);
		if (sqDist < bestSqDist) 
		{
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = tempResult.m_closestPointOnSimplex;
			//finalResult.m_usedVertices.reset();
			//finalResult.m_usedVertices.usedVertexA = tempResult.m_usedVertices.usedVertexA;
			//finalResult.m_usedVertices.usedVertexC = tempResult.m_usedVertices.usedVertexB;
			//finalResult.m_usedVertices.usedVertexD = tempResult.m_usedVertices.usedVertexC;
			unsigned int used = tempResult.m_usedVertices;
			finalResult.m_usedVertices = (used & UBF_USED_VERTEX_A) | ((used & (UBF_USED_VERTEX_B|UBF_USED_VERTEX_C)) << 1);

			finalResult.setBarycentricCoordinates(
					tempResult.m_barycentricCoords[VERTA],
					0,
					tempResult.m_barycentricCoords[VERTB],
					tempResult.m_barycentricCoords[VERTC]
			);
		}
	}
	// Repeat test for face adb


	if (pointOutsideADB)
	{
		//convert result bitmask!
		btScalar sqDist = closestPtPointDistTriangle(p, a, d, b,tempResult);;
		if (sqDist < bestSqDist) 
		{
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = tempResult.m_closestPointOnSimplex;
			//finalResult.m_usedVertices.reset();
			//finalResult.m_usedVertices.usedVertexA = tempResult.m_usedVertices.usedVertexA;
			//finalResult.m_usedVertices.usedVertexB = tempResult.m_usedVertices.usedVertexC;
			//finalResult.m_usedVertices.usedVertexD = tempResult.m_usedVertices.usedVertexB;
			unsigned int used = tempResult.m_usedVertices;
			finalResult.m_usedVertices = (used & UBF_USED_VERTEX_A) | ((used & UBF_USED_VERTEX_C) >> 1) | ((used & UBF_USED_VERTEX_B) << 2);

			finalResult.setBarycentricCoordinates(
					tempResult.m_barycentricCoords[VERTA],
					tempResult.m_barycentricCoords[VERTC],
					0,
					tempResult.m_barycentricCoords[VERTB]
			);
		}
	}

	// Repeat test for face bdc
	if (pointOutsideBDC)
	{
		//convert result bitmask!
		btScalar sqDist = closestPtPointDistTriangle(p, b, d, c,tempResult);
		if (sqDist < bestSqDist) 
		{
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = tempResult.m_closestPointOnSimplex;
			//finalResult.m_usedVertices.reset();
			//finalResult.m_usedVertices.usedVertexB = tempResult.m_usedVertices.usedVertexA;
			//finalResult.m_usedVertices.usedVertexC = tempResult.m_usedVertices.usedVertexC;
			//finalResult.m_usedVertices.usedVertexD = tempResult.m_usedVertices.usedVertexB;
			unsigned int used = tempResult.m_usedVertices;
			finalResult.m_usedVertices = ((used & UBF_USED_VERTEX_A) << 1) | (used & UBF_USED_VERTEX_C) | ((used & UBF_USED_VERTEX_B) << 2);

			finalResult.setBarycentricCoordinates(
					0,
					tempResult.m_barycentricCoords[VERTA],
					tempResult.m_barycentricCoords[VERTC],
					tempResult.m_barycentricCoords[VERTB]
			);
		}
	}

	//help! we ended up full !

	if (finalResult.m_usedVertices == (UBF_USED_VERTEX_A|UBF_USED_VERTEX_B|UBF_USED_VERTEX_C|UBF_USED_VERTEX_D)) 
	{
		return true;
	}

	return true;
}

