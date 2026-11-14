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

#ifndef ENF_HEIGHTFIELD_TERRAIN_SHAPE_H
#define ENF_HEIGHTFIELD_TERRAIN_SHAPE_H

#include "BulletCollision/CollisionShapes/btConcaveShape.h"

class Scenery;

namespace enf
{
	//class Terrain;
	class BulletWorld;
}
class btMaterial;
///btHeightfieldTerrainShape simulates a 2D heightfield terrain
/**
  The caller is responsible for maintaining the heightfield array; this
  class does not make a copy.

  The heightfield can be dynamic so long as the min/max height values
  capture the extremes (heights must always be in that range).

  The local origin of the heightfield is assumed to be the exact
  center (as determined by width and length and height, with each
  axis multiplied by the localScaling).

  Whatever the caller specifies as minHeight and maxHeight will be honored.
  The class will not inspect the heightfield to discover the actual minimum
  or maximum heights.  These values are used to determine the heightfield's
  axis-aligned bounding box, multiplied by localScaling.

  For usage and testing see the TerrainDemo.
 */
class HeightfieldTerrainShape : public btConcaveShape
{
protected:
	btVector3	m_localAabbMin;
	btVector3	m_localAabbMax;
	btVector3	m_localOrigin;

	///terrain data
	const Scenery*	m_terrain;
	enf::BulletWorld*		m_owner;

	int m_heightWidth;
	int m_heightLength;
	btScalar	m_minHeight;
	btScalar	m_maxHeight;
	btScalar m_width;
	btScalar m_length;
	btScalar m_heightScale;
	btVector3	m_localScaling;

	inline void quantizeWithClamp(int* imin, int* imax, const btVector3& mins, const btVector3& maxs) const;

	/// protected initialization
	/**
	  Handles the work of constructors so that public constructors can be
	  backwards-compatible without a lot of copy/paste.
	 */
	void initialize(const Scenery* terr, enf::BulletWorld* mw, 
						 btScalar heightScale,
						 btScalar minHeight, btScalar maxHeight);

public:
	/// preferred constructor
	/**
	  This constructor supports a range of heightfield
	  data types, and allows for a non-zero minimum height value.
	  heightScale is needed for any integer-based heightfield data types.
	 */
	HeightfieldTerrainShape(const Scenery* terr, enf::BulletWorld* mw,
	                          btScalar heightScale,
	                          btScalar minHeight, btScalar maxHeight
	                          );

	virtual ~HeightfieldTerrainShape();

	virtual btScalar getLineIntersection(const btCollisionObject* obj, const btTransform& colObjWorldTransform, const btVector3& from, const btVector3& to, btVector3& contactNormal, const btMaterial*& contactMaterial) const;

	virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const;

	virtual void	processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const;

	virtual void	calculateLocalInertia(btScalar mass,btVector3& inertia) const;

	virtual void	setLocalScaling(const btVector3& scaling);
	
	virtual const btVector3& getLocalScaling() const;
	
	//debugging
	virtual const char*	getName()const {return "HEIGHTFIELD";}

};

#endif //ENF_HEIGHTFIELD_TERRAIN_SHAPE_H
