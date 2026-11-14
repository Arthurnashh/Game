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
*/
//#include "precomp.h"
#include "physics/enf_btHeightfieldTerrainShape.h"
#include "BulletCollision/CollisionShapes/btConcaveShape.h"
#include "BulletCollision/CollisionDispatch/btConvexConcaveCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"

#include "LinearMath/btTransformUtil.h"

//#include "terrain/enf_terrain.h"
#include "lib\scenery.hpp"
//#include "terrain/enf_terrainblock.h"
//#include "baseworld/enf_worldphysics.h"
#include "physics/enf_physicsbulletinternal.h"
//#include "enf_shapes.h"

HeightfieldTerrainShape::HeightfieldTerrainShape
(
const Scenery* terr, enf::BulletWorld* mw, 
btScalar heightScale, btScalar minHeight, btScalar maxHeight
)
{
	initialize(terr, mw, heightScale, minHeight, maxHeight);
}



void HeightfieldTerrainShape::initialize
(
const Scenery* terr, 
enf::BulletWorld* mw, 
btScalar heightScale, btScalar minHeight, btScalar maxHeight
)
{
	// validation
	btAssert(terr != NULL);
	btAssert(mw);
	// btAssert(heightScale) -- do we care?  Trust caller here
	btAssert(minHeight <= maxHeight && "bad min/max height");

	m_shapeType = TERRAIN_SHAPE_PROXYTYPE;
	m_terrain = terr;
	m_owner = mw;
  int heightStickWidth = terr->_data.GetXRange();//terr->GetGridWidth();
	int heightStickLength = terr->_data.GetYRange();//GetGridLength();

	// initialize member variables
	m_heightWidth = heightStickWidth;
	m_heightLength = heightStickLength;
	m_minHeight = minHeight;
	m_maxHeight = maxHeight;
	m_width = (btScalar) (heightStickWidth - 1);
	m_length = (btScalar) (heightStickLength - 1);
	m_heightScale = heightScale;
	m_localScaling.setValue(btScalar(1.), btScalar(1.), btScalar(1.));

	// determine min/max axis-aligned bounding box (aabb) values
	m_localAabbMin.setValue(0, m_minHeight, 0);
	m_localAabbMax.setValue(m_length, m_maxHeight, m_width);

	// remember origin (defined as exact middle of aabb)
	m_localOrigin = btScalar(0.5) * (m_localAabbMin + m_localAabbMax);
}



HeightfieldTerrainShape::~HeightfieldTerrainShape()
{
}



void HeightfieldTerrainShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
{
	btVector3 halfExtents = (m_localAabbMax-m_localAabbMin)* m_localScaling * btScalar(0.5);
	btVector3 localOrigin = m_localOrigin * m_localScaling;
	btVector3 center = t(localOrigin);

  aabbMin = center - halfExtents;
	aabbMax = center + halfExtents;
}


/// given input vector, return quantized version
/**
  This routine is basically determining the gridpoint indices for a given
  input vector, answering the question: "which gridpoint is closest to the
  provided point?".

  "with clamp" means that we restrict the point to be in the heightfield's
  axis-aligned bounding box.
 */
inline void HeightfieldTerrainShape::quantizeWithClamp(int* imin, int* imax, const btVector3& mins, const btVector3& maxs) const
{
	// expand the min/max quantized values
	// this is to catch the case where the input aabb falls between grid points!
	imin[0] = btMax((int)(floor(mins.getX() - 0.1f)), 0);
	imin[1] = btMax((int)(floor(mins.getZ() - 0.1f)), 0);
	imax[0] = btMin((int)(ceil(maxs.getX() + 0.1f)), m_heightWidth - 1);
	imax[1] = btMin((int)(ceil(maxs.getZ() + 0.1f)), m_heightLength - 1);
}


/// process all triangles within the provided axis-aligned bounding box
/**
  basic algorithm:
    - convert input aabb to local coordinates (scale down and shift for local origin)
    - convert input aabb to a range of heightfield grid points (quantize)
    - iterate over all triangles in that subset of the grid
 */

/*static IntersectedTriangle sTri;
IntersectedTriangle* HeightfieldTerrainShape::GetLastIntersectedTriangle()
{
  return &sTri;
}
*/
btScalar HeightfieldTerrainShape::getLineIntersection(const btCollisionObject* obj, const btTransform& colObjWorldTransform, const btVector3& start, const btVector3& end, btVector3& contactNormal, const btMaterial*& contactMaterial) const
{
  Vector3 tstart	= enf::BulletWorld::btVector2Point(start);
	Vector3 tend	= enf::BulletWorld::btVector2Point(end);

	Scenery* terrain = (Scenery*)obj->getUserPointer();
	if(terrain == NULL)
		return btScalar(-2);

  //clip tp terrain
	const float terreps = 1.0f/32.0f;
	if (terrain->GetWorldClipRect().TestPointEps(tstart.X(), tstart.Z(), terreps) != enf::CLIP_INSIDE || terrain->GetWorldClipRect().TestPointEps(tend.X(), tend.Z(), terreps) != enf::CLIP_INSIDE)
	{
		return btScalar(-1);
	}

  //convert to terrain coordinates
	//enf::Vector3 tstart = terrain->WorldToTerrCoord(wstart);
	//enf::Vector3 tend	= terrain->WorldToTerrCoord(wend);
  tstart.Set(0) = tstart.X() / m_terrain->_terrainGrid;
  tstart.Set(2) = tstart.Z() / m_terrain->_terrainGrid;
  tend.Set(0) = tend.X() / m_terrain->_terrainGrid;
  tend.Set(2) = tend.Z() / m_terrain->_terrainGrid;

  int terrsx = enf::enf_truncint(tstart.X());
	int terrsz = enf::enf_truncint(tstart.Z());
	int terrex = enf::enf_truncint(tend.X());
	int terrez = enf::enf_truncint(tend.Z());

	IntersectedTriangle triangle;

	//simple test, for one exact quad, when the coordinates do not differ
	if (terrsx != terrex || terrsz != terrez)
	{
		//if (m_pTerrain->IsPointVisibleTC(tstart, tend, triangle, true, true))
		if (!terrain->TestIntersectionTTT(tstart, tend, tend - tstart, triangle, true))
		{
			return btScalar(-1);
		}
	}
	else
	{
		//test intersection
		if (terrain->TestTerrainQuadSimple(tstart, tend, triangle, terrsx, terrsz, 0.001f) == INTERSECTION_NONE)
		{
			return btScalar(-1);
		}
	}


	btScalar frac = triangle.TriangleRatio;

	//extra geometry data -> material
#if _ENABLE_PHYSICS
 	TexMaterial* tmat = terrain->ClippedTextureInfo(triangle.Intersection.x, triangle.Intersection.y)._mat;
	if(tmat && tmat->GetPhysMaterial())
		contactMaterial = ((enf::BulletPhysicsMaterial*)(tmat->GetPhysMaterial()))->m_pBtMaterial;
	else
		contactMaterial = enf::g_pBulletManager->FindMaterial("material/default")->m_pBtMaterial;
#endif

	//get normal
	//heighscale must 1, then following code will work, otherwise it must be done in world coords
	//Assert(fabs(terrain->GetHeightScale() - 1.0f) < 0.001);

	//in terrain coords, this works!
	Vector3 vec1 = triangle.V0 - triangle.V1;
	Vector3 vec2 = triangle.V0 - triangle.V2;
  Vector3 normal = vec2.CrossProduct(vec1);
	normal.Set(1) = terrain->GetUnitScale();

	normal.Normalize();
	Assert(normal.Y() > 0.0f);
			

	contactNormal = enf::BulletWorld::Vector2btVector(normal);

  	return frac;
}

void	HeightfieldTerrainShape::processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const
{
  	// scale down the input aabb's so they are in local (non-scaled) coordinates
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(1);
	vec_float4 reScale = m_localScaling.re();
	btVector3	localAabbMin = vec_madd(aabbMin.mVec, reScale, btZeroVec());
	btVector3	localAabbMax = vec_madd(aabbMax.mVec, reScale, btZeroVec());
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

	//enf::Shape::CreateBox(enf::BulletWorld::btVector2Point(aabbMin), enf::BulletWorld::btVector2Point(aabbMax), 0x3fff1f1f, enf::SS_ONCE|enf::SS_TRANSP);
	if(true)
	{
    int startBlockX = enf::enf_min(startX, (int)m_terrain->_data.GetXRange() - 1);
		int endBlockX = enf::enf_min(endX, (int)m_terrain->_data.GetXRange() - 1);
		int startBlockZ = enf::enf_min(startJ, (int)m_terrain->_data.GetYRange() - 1);
		int endBlockZ = enf::enf_min(endJ, (int)m_terrain->_data.GetYRange() - 1);
		bool intersects = false;
		float minY = (aabbMin.getY() - m_collisionMargin) * enf::MEQ_WORLDSCALE;
		for(int x = startBlockX; x <= endBlockX; x++)
		{
			for(int z = startBlockZ; z <= endBlockZ; z++)
			{
				float h = m_terrain->GetData(x, z);
				//enf::Shape::CreateBox(wbounds.GetMins(), wbounds.GetMaxs(), 0x3f7f7f7f, enf::SS_ONCE|enf::SS_TRANSP);
				if(h >= minY)
				{
					intersects = true;
					break;
				}
			}
		}
		if(!intersects)
			return;
	}

	static btAlignedObjectArray<btCollisionTriangle> triangles;
	triangles.resize(0);

	//const enf::TerrainHeight* hdata = m_terrain->GetHeightMap() + m_heightWidth * startJ;
	btScalar xScale = m_localScaling.getX();
	btScalar yScale = m_localScaling.getY() * m_heightScale;
	btScalar zScale = m_localScaling.getZ(); 
	btScalar xOff = btScalar(startX) * xScale;
	btScalar zOff = btScalar(startJ) * zScale;
	btScalar minY = localAabbMin.getY();
	const btMaterial* mat = getMaterial();
	const enf::BulletPhysicsMaterial*const*	bmaterials = ((enf::BulletManager*)(m_owner->GetManager()))->GetBaseMaterials();

  for(int j=startJ; j<endJ; j++)
	{
		btScalar xf = xOff;

		btScalar h00 = btScalar(m_terrain->GetData(startX, j)) * yScale;
    btScalar h01 = btScalar(m_terrain->GetData(startX, j+1)) * yScale;
		
		for(int x=startX+1; x<=endX; x++)
		{
			btScalar h10 = btScalar(m_terrain->GetData(x, j)) * yScale;
			btScalar h11 = btScalar(m_terrain->GetData(x, j+1)) * yScale;

			if(minY <= h00 || minY <= h01 || minY <= h10 || minY <= h11)
			{
				btVector3 p00 = btVector3(xf, h00, zOff);
				btVector3 p01 = btVector3(xf, h01, zOff + zScale);
				btVector3 p10 = btVector3(xf + xScale, h10, zOff);
				btVector3 p11 = btVector3(xf + xScale, h11, zOff + zScale);

#if _ENABLE_PHYSICS
				TexMaterial* tmat = m_terrain->ClippedTextureInfo(x, j)._mat;
				if(tmat && tmat->GetPhysMaterial())
					mat = ((enf::BulletPhysicsMaterial*)(tmat->GetPhysMaterial()))->m_pBtMaterial;
				else
					mat = enf::g_pBulletManager->FindMaterial("material/default")->m_pBtMaterial;
#endif

        triangles.push_back(btCollisionTriangle(p11, p10, p00, (x << 16) | j, mat));
				triangles.push_back(btCollisionTriangle(p00, p11, p01, (x << 16) | j, mat));
    	}
			xf += xScale;
			h00 = h10;
			h01 = h11;
		}
		//hdata += m_heightWidth;
		zOff += zScale;
	}

	if(triangles.size() > 0)
		callback->processTriangles(triangles);
}

void	HeightfieldTerrainShape::calculateLocalInertia(btScalar ,btVector3& inertia) const
{
	//moving concave objects not supported
	inertia = btZeroVec();
}

void	HeightfieldTerrainShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}
const btVector3& HeightfieldTerrainShape::getLocalScaling() const
{
	return m_localScaling;
}
