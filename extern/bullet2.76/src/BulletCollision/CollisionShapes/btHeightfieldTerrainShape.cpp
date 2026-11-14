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
3. This notice may not be remov ed or altered from any source distribution.
*/

#include "btHeightfieldTerrainShape.h"
#include "btConcaveShape.h"
#include "../CollisionDispatch/btConvexConcaveCollisionAlgorithm.h"

#include "LinearMath/btTransformUtil.h"

btHeightfieldTerrainShape::btHeightfieldTerrainShape
(
int heightStickWidth, int heightStickLength, void* heightfieldData,
btScalar heightScale, btScalar minHeight, btScalar maxHeight,int upAxis,
PHY_ScalarType hdt, bool flipQuadEdges
)
{
	initialize(heightStickWidth, heightStickLength, heightfieldData,
	           heightScale, minHeight, maxHeight, upAxis, hdt,
	           flipQuadEdges);
}



void btHeightfieldTerrainShape::initialize
(
int heightStickWidth, int heightStickLength, void* heightfieldData,
btScalar heightScale, btScalar minHeight, btScalar maxHeight, int upAxis,
PHY_ScalarType hdt, bool flipQuadEdges
)
{
	// validation
	btAssert(heightStickWidth > 1 && "bad width");
	btAssert(heightStickLength > 1 && "bad length");
	btAssert(heightfieldData && "null heightfield data");
	// btAssert(heightScale) -- do we care?  Trust caller here
	btAssert(minHeight <= maxHeight && "bad min/max height");
	btAssert(upAxis == 2 && "bad upAxis--should be in range [0,2]");
	btAssert(hdt != PHY_UCHAR || hdt != PHY_FLOAT || hdt != PHY_SHORT &&
	    "Bad height data type enum");

	// initialize member variables
	m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
	m_heightStickWidth = heightStickWidth;
	m_heightStickLength = heightStickLength;
	m_minHeight = minHeight;
	m_maxHeight = maxHeight;
	m_width = (btScalar) (heightStickWidth - 1);
	m_length = (btScalar) (heightStickLength - 1);
	m_heightScale = heightScale;
	m_heightfieldDataUnknown = heightfieldData;
	m_heightDataType = hdt;
	m_flipQuadEdges = flipQuadEdges;
	m_useDiamondSubdivision = false;
	m_upAxis = upAxis;
	m_localScaling.setValue(btScalar(1.), btScalar(1.), btScalar(1.));

	// determine min/max axis-aligned bounding box (aabb) values
	m_localAabbMin.setValue(0, 0, m_minHeight);
	m_localAabbMax.setValue(m_width, m_length, m_maxHeight);

	// remember origin (defined as exact middle of aabb)
	m_localOrigin = btScalar(0.5) * (m_localAabbMin + m_localAabbMax);
}



btHeightfieldTerrainShape::~btHeightfieldTerrainShape()
{
}



void btHeightfieldTerrainShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
{
	btVector3 halfExtents = (m_localAabbMax-m_localAabbMin)* m_localScaling * btScalar(0.5);
	btVector3 localOrigin = m_localOrigin * m_localScaling;

	btMatrix3x3 abs_b = t.getBasis().absolute();  
	btVector3 center = t(localOrigin);
	float margin = getMargin();
#ifdef BT_PS3PPU_VEC 
	//btVector3::btHitPrint(79);
	btVector3 extent = btVector3(vec_add(vec_dot3mx(abs_b.get128ptr(), halfExtents.mVec), vec_float4(margin)));
#else
	btVector3 extent = btVector3(abs_b[0].dot(halfExtents) + margin,
		abs_b[1].dot(halfExtents) + margin,
		abs_b[2].dot(halfExtents) + margin);
#endif
	aabbMin = center - extent;
	aabbMax = center + extent;
}


/// given input vector, return quantized version
/**
  This routine is basically determining the gridpoint indices for a given
  input vector, answering the question: "which gridpoint is closest to the
  provided point?".

  "with clamp" means that we restrict the point to be in the heightfield's
  axis-aligned bounding box.
 */
inline void btHeightfieldTerrainShape::quantizeWithClamp(int* imin, int* imax, const btVector3& mins, const btVector3& maxs) const
{
	// expand the min/max quantized values
	// this is to catch the case where the input aabb falls between grid points!
	imin[0] = btMax((int)(BT_FLOOR(mins.getX() - 0.1f)), 0);
	imin[1] = btMax((int)(BT_FLOOR(mins.getY() - 0.1f)), 0);
	imax[0] = btMin((int)(BT_CEIL(maxs.getX() + 0.1f)), m_heightStickWidth - 1);
	imax[1] = btMin((int)(BT_CEIL(maxs.getY() + 0.1f)), m_heightStickLength - 1);
}


/// process all triangles within the provided axis-aligned bounding box
/**
  basic algorithm:
    - convert input aabb to local coordinates (scale down and shift for local origin)
    - convert input aabb to a range of heightfield grid points (quantize)
    - iterate over all triangles in that subset of the grid
 */
void	btHeightfieldTerrainShape::processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const
{
	// scale down the input aabb's so they are in local (non-scaled) coordinates
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(101);
	btVector3	localAabbMin = aabbMin / m_localScaling;
	btVector3	localAabbMax = aabbMax / m_localScaling;
#else
	btVector3	localAabbMin = aabbMin*btVector3(1.f/m_localScaling[0],1.f/m_localScaling[1],1.f/m_localScaling[2]);
	btVector3	localAabbMax = aabbMax*btVector3(1.f/m_localScaling[0],1.f/m_localScaling[1],1.f/m_localScaling[2]);
#endif
	//quantize the aabbMin and aabbMax, and adjust the start/end ranges
	int	quantizedAabbMin[2];
	int	quantizedAabbMax[2];
	quantizeWithClamp(quantizedAabbMin, quantizedAabbMax, localAabbMin,localAabbMax);

	int startX = quantizedAabbMin[0];
	int endX = quantizedAabbMax[0];
	int startJ = quantizedAabbMin[1];
	int endJ = quantizedAabbMax[1];

	static btAlignedObjectArray<btCollisionTriangle> triangles;
	triangles.resize(0);

	unsigned short* hdata = m_heightfieldDataUShort + m_heightStickWidth * startJ;
	btScalar xScale = m_localScaling.getX();
	btScalar yScale = m_localScaling.getY();
	btScalar zScale = m_localScaling.getZ() * m_heightScale;
	btScalar xOff = btScalar(startX) * xScale;
	btScalar yOff = btScalar(startJ) * yScale;
	btScalar minZ = localAabbMin.getZ();

	for(int j=startJ; j<endJ; j++)
	{
		btScalar xf = xOff;

		btScalar h00 = btScalar(((int)hdata[startX] - 32768)) * zScale;
		btScalar h01 = btScalar(((int)hdata[startX + m_heightStickWidth] - 32768)) * zScale;

		for(int x=startX+1; x<=endX; x++)
		{
			btScalar h10 = btScalar(((int)hdata[x] - 32768)) * zScale;
			btScalar h11 = btScalar(((int)hdata[x + m_heightStickWidth] - 32768)) * zScale;

			if(minZ <= h00 || minZ <= h01 || minZ <= h10 || minZ <= h11)
			{
				btVector3 p00 = btVector3(xf, yOff, h00);
				btVector3 p01 = btVector3(xf, yOff + yScale, h01);
				btVector3 p10 = btVector3(xf + xScale, yOff, h10);
				btVector3 p11 = btVector3(xf + xScale, yOff + yScale, h11);
				triangles.push_back(btCollisionTriangle(p00, p01, p10, (x << 16) | j, getMaterial()));
				triangles.push_back(btCollisionTriangle(p10, p01, p11, (x << 16) | j, getMaterial()));
			}
			xf += xScale;
			h00 = h10;
			h01 = h11;
		}
		hdata += m_heightStickWidth;
		yOff += yScale;
	}

	if(triangles.size() > 0)
		callback->processTriangles(triangles);
}

void	btHeightfieldTerrainShape::calculateLocalInertia(btScalar ,btVector3& inertia) const
{
	//moving concave objects not supported
	
	inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
}

void	btHeightfieldTerrainShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}
const btVector3& btHeightfieldTerrainShape::getLocalScaling() const
{
	return m_localScaling;
}
