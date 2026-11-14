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

#ifndef COLLISION_OBJECT_H
#define COLLISION_OBJECT_H

#include "LinearMath/btTransform.h"

//island management, m_activationState1
#define ACTIVE_TAG 1
#define ISLAND_SLEEPING 2
#define WANTS_DEACTIVATION 3
#define DISABLE_DEACTIVATION 4
#define DISABLE_SIMULATION 5

struct	btBroadphaseProxy;
class	btCollisionShape;
struct btCollisionShapeData;
#include "LinearMath/btMotionState.h"
#include "LinearMath/btAlignedAllocator.h"
#include "LinearMath/btAlignedObjectArray.h"

typedef btAlignedObjectArray<class btCollisionObject*> btCollisionObjectArray;

#ifdef BT_USE_DOUBLE_PRECISION
#define btCollisionObjectData btCollisionObjectDoubleData
#define btCollisionObjectDataName "btCollisionObjectDoubleData"
#else
#define btCollisionObjectData btCollisionObjectFloatData
#define btCollisionObjectDataName "btCollisionObjectFloatData"
#endif

enum MaterialOp
{
	MO_MULTIPLY,
	MO_ADD,
	MO_REPLACE
};

ATTRIBUTE_ALIGNED16(class)	btMaterial
{
protected:
	friend class btCollisionObject;

	btVector3		m_anisotropicFriction;
	btScalar			m_friction;
	btScalar			m_restitution;
	MaterialOp		m_op;
	void*				m_userObjectPointer;
	bool				m_hasAnisotropicFriction;
	bool				m_isSingleSided;

	struct Pair
	{
		const btMaterial* key;
		const btMaterial* data;
		Pair(): key(NULL), data(NULL) {}
		Pair(const btMaterial* k, const btMaterial* d): key(k), data(d) {}
		bool operator<(const Pair& other) const {
			return key < other.key;
		}
		bool operator>(const Pair& other) const {
			return key > other.key;
		}
		bool operator==(const Pair& other) const {
			return key == other.key;
		}
	};
	btAlignedObjectArray<Pair> m_combiners;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

 #ifdef BT_PS3PPU_VEC
	btMaterial(): m_anisotropicFriction(1.f), m_hasAnisotropicFriction(false), m_friction(btScalar(0.5f)), m_restitution(btScalar(0.0f)), m_op(MO_MULTIPLY), m_userObjectPointer(0), m_isSingleSided(false)
	{
	}

	btMaterial(btScalar friction, btScalar restitution): m_anisotropicFriction(1.f), m_hasAnisotropicFriction(false), m_friction(friction), m_restitution(restitution), m_op(MO_MULTIPLY), m_userObjectPointer(0), m_isSingleSided(false)
	{
	}
#else
	btMaterial(): m_anisotropicFriction(1.f,1.f,1.f), m_hasAnisotropicFriction(false), m_friction(btScalar(0.5f)), m_restitution(btScalar(0.0f)), m_op(MO_MULTIPLY), m_userObjectPointer(0), m_isSingleSided(false)
	{
	}

	btMaterial(btScalar friction, btScalar restitution): m_anisotropicFriction(1.f,1.f,1.f), m_hasAnisotropicFriction(false), m_friction(friction), m_restitution(restitution), m_op(MO_MULTIPLY), m_userObjectPointer(0), m_isSingleSided(false)
	{
	}
#endif
	void addCombiner(const btMaterial* other, const btMaterial* combination);
	const btMaterial* findCombiner(const btMaterial* other) const;
	bool hasCombiners() const{
		return m_combiners.size() > 0;
	}
	bool hasAnisotropicFriction() const
	{
		return m_hasAnisotropicFriction;
	}
	const btVector3& getAnisotropicFriction() const
	{
		return m_anisotropicFriction;
	}
	void	setAnisotropicFriction(const btVector3& anisotropicFriction)
	{
		m_anisotropicFriction = anisotropicFriction;
		m_hasAnisotropicFriction = (anisotropicFriction[0]!=1.f) || (anisotropicFriction[1]!=1.f) || (anisotropicFriction[2]!=1.f);
	}
	void	setRestitution(btScalar rest)
	{
		m_restitution = rest;
	}

	MaterialOp getOperation() const { return m_op; }
	void setOperation(MaterialOp op) { m_op = op; }
	btScalar	getRestitution() const
	{
		return m_restitution;
	}
	void	setFriction(btScalar frict)
	{
		m_friction = frict;
	}
	btScalar	getFriction() const
	{
		return m_friction;
	}

	///users can point to their objects, userPointer is not used by Bullet
	void*	getUserPointer() const
	{
		return m_userObjectPointer;
	}
	
	///users can point to their objects, userPointer is not used by Bullet
	void	setUserPointer(void* userPointer)
	{
		m_userObjectPointer = userPointer;
	}

	void setSingleSided(bool ss)
	{
		m_isSingleSided = ss;
	}
	bool isSingleSided() const
	{
		return m_isSingleSided;
	}
};

/// btCollisionObject can be used to manage collision detection objects. 
/// btCollisionObject maintains all information that is needed for a collision detection: Shape, Transform and AABB proxy.
/// They can be added to the btCollisionWorld.
ATTRIBUTE_ALIGNED16(class)	btCollisionObject
{

protected:

	btTransform	m_worldTransform;

	///m_interpolationWorldTransform is used for CCD and interpolation
	///it can be either previous or future (predicted) transform
	btTransform	m_interpolationWorldTransform;
	
	btScalar	m_contactProcessingThreshold;	

	btBroadphaseProxy*		m_broadphaseHandle;
	btCollisionShape*		m_collisionShape;
	
	///m_rootCollisionShape is temporarily used to store the original collision shape
	///The m_collisionShape might be temporarily replaced by a child collision shape during collision detection purposes
	///If it is NULL, the m_collisionShape is not temporarily replaced.
	btCollisionShape*		m_rootCollisionShape;
	unsigned int			m_interactionLayer;

	int				m_collisionFlags;

	int				m_islandTag1;
	//int				m_companionId;

	int				m_activationState1;
	btScalar			m_deactivationTime;
  btScalar      m_lifeTime;

	//const btMaterial*	m_material;

	///m_internalType is reserved to distinguish Bullet's btCollisionObject, btRigidBody, btSoftBody, btGhostObject etc.
	///do not assign your own m_internalType unless you write a new dynamics object class.
	int				m_internalType;

	///users can point to their objects, m_userPointer is not used by Bullet, see setUserPointer/getUserPointer
	void*			m_userObjectPointer;

	///time of impact calculation
	btScalar		m_hitFraction; 
	
	///Swept sphere radius (0.0 by default), see btConvexConvexAlgorithm::
	btScalar		m_ccdSweptSphereRadius;

	/// Don't do continuous collision detection if the motion (in one step) is less then m_ccdMotionThreshold
	btScalar		m_ccdMotionThreshold;
	
	/// If some object should have elaborate collision filtering by sub-classes
	short			m_checkCollideWith;

	short			m_interactionBlockCount;

	virtual bool	checkCollideWithOverride(btCollisionObject* /* co */)
	{
		return true;
	}

public:

	BT_DECLARE_ALIGNED_ALLOCATOR();

	enum CollisionFlags
	{
		CF_STATIC_OBJECT= 1,
		CF_KINEMATIC_OBJECT= 2,
		CF_NO_CONTACT_RESPONSE = 4,
		CF_CUSTOM_MATERIAL_CALLBACK = 8,//this allows per-triangle material (friction/restitution)
		CF_CHARACTER_OBJECT = 16,
		CF_DISABLE_VISUALIZE_OBJECT = 32, //disable debug drawing
		CF_DISABLE_SPU_COLLISION_PROCESSING = 64,//disable parallel/SPU processing
		CF_NO_COLLISION_RESPONSE = 128,
		CF_NO_GRAVITY = 256,
		CF_NO_CONTACT = 512,
	};

	enum	CollisionObjectTypes
	{
		CO_COLLISION_OBJECT =1,
		CO_RIGID_BODY,
		///CO_GHOST_OBJECT keeps track of all objects overlapping its AABB and that pass its collision filter
		///It is useful for collision sensors, explosion objects, character controller etc.
		CO_GHOST_OBJECT,
		CO_SOFT_BODY,
		CO_HF_FLUID
	};

	int getInteractionBlockCount() const
	{
		return m_interactionBlockCount;
	}
	SIMD_FORCE_INLINE bool mergesSimulationIslands() const
	{
		///static objects, kinematic and object without contact response don't merge islands
		return  ((m_collisionFlags & (CF_STATIC_OBJECT | CF_KINEMATIC_OBJECT | CF_NO_CONTACT_RESPONSE) )==0);
	}


	///the constraint solver can discard solving contacts, if the distance is above this threshold. 0 by default.
	///Note that using contacts with positive distance can improve stability. It increases, however, the chance of colliding with degerate contacts, such as 'interior' triangle edges
	void	setContactProcessingThreshold( btScalar contactProcessingThreshold)
	{
		m_contactProcessingThreshold = contactProcessingThreshold;
	}
	btScalar	getContactProcessingThreshold() const
	{
		return m_contactProcessingThreshold;
	}

	SIMD_FORCE_INLINE bool		isStaticObject() const {
		return (m_collisionFlags & CF_STATIC_OBJECT) != 0;
	}

	SIMD_FORCE_INLINE bool		isKinematicObject() const
	{
		return (m_collisionFlags & CF_KINEMATIC_OBJECT) != 0;
	}

	SIMD_FORCE_INLINE bool		isStaticOrKinematicObject() const
	{
		return (m_collisionFlags & (CF_KINEMATIC_OBJECT | CF_STATIC_OBJECT)) != 0 ;
	}

	SIMD_FORCE_INLINE bool		hasContactResponse() const {
		return (m_collisionFlags & CF_NO_CONTACT_RESPONSE)==0;
	}

	
	btCollisionObject();

	void addInteractionBlock()
	{
		m_interactionBlockCount++;
	}
	void removeInteractionBlock()
	{
		btAssert(m_interactionBlockCount > 0);
		m_interactionBlockCount--;
	}
	bool hasInteractionBlocks() const
	{
		return m_interactionBlockCount > 0;
	}

	virtual ~btCollisionObject();

	virtual void	setCollisionShape(btCollisionShape* collisionShape);

	SIMD_FORCE_INLINE const btCollisionShape*	getCollisionShape() const
	{
		return m_collisionShape;
	}

	SIMD_FORCE_INLINE btCollisionShape*	getCollisionShape()
	{
		return m_collisionShape;
	}

	SIMD_FORCE_INLINE unsigned int getInteractionLayer() const
	{
		return m_interactionLayer;
	}

	SIMD_FORCE_INLINE void setInteractionLayer(unsigned int layer)
	{
		m_interactionLayer = layer;
	}

	
	SIMD_FORCE_INLINE const btCollisionShape*	getRootCollisionShape() const
	{
		return m_rootCollisionShape;
	}

	SIMD_FORCE_INLINE btCollisionShape*	getRootCollisionShape()
	{
		return m_rootCollisionShape;
	}

	///Avoid using this internal API call
	///internalSetTemporaryCollisionShape is used to temporary replace the actual collision shape by a child collision shape.
	void	internalSetTemporaryCollisionShape(btCollisionShape* collisionShape)
	{
		m_collisionShape = collisionShape;
	}

	SIMD_FORCE_INLINE	int	getActivationState() const { return m_activationState1;}
	
	void setActivationState(int newState);

  void setLifeTime(btScalar time)
  {
    m_lifeTime = time;
  }

  btScalar getLifeTime() const
  {
    return m_lifeTime;
  }

	void	setDeactivationTime(btScalar time)
	{
		m_deactivationTime = time;
	}
	btScalar	getDeactivationTime() const
	{
		return m_deactivationTime;
	}

	void forceActivationState(int newState);

	void	activate(bool forceActivation = false);

	SIMD_FORCE_INLINE bool isActive() const
	{
		return ((getActivationState() != ISLAND_SLEEPING) && (getActivationState() != DISABLE_SIMULATION));
	}

	const btMaterial* getMaterial() const;

	///reserved for Bullet internal usage
	int	getInternalType() const
	{
		return m_internalType;
	}

	btTransform&	getWorldTransform()
	{
		return m_worldTransform;
	}

	const btTransform&	getWorldTransform() const
	{
		return m_worldTransform;
	}

	void	setWorldTransform(const btTransform& worldTrans)
	{
		m_worldTransform = worldTrans;
	}

	const btVector3&     getCenterOfMassPosition() const { 
		return m_worldTransform.getOrigin(); 
	}
	btQuaternion getOrientation() const;
	
	const btTransform&  getCenterOfMassTransform() const { 
		return m_worldTransform; 
	}

	SIMD_FORCE_INLINE btBroadphaseProxy*	getBroadphaseHandle()
	{
		return m_broadphaseHandle;
	}

	SIMD_FORCE_INLINE const btBroadphaseProxy*	getBroadphaseHandle() const
	{
		return m_broadphaseHandle;
	}

	void	setBroadphaseHandle(btBroadphaseProxy* handle)
	{
		m_broadphaseHandle = handle;
	}


	const btTransform&	getInterpolationWorldTransform() const
	{
		return m_interpolationWorldTransform;
	}

	btTransform&	getInterpolationWorldTransform()
	{
		return m_interpolationWorldTransform;
	}

	void	setInterpolationWorldTransform(const btTransform&	trans)
	{
		m_interpolationWorldTransform = trans;
	}

	SIMD_FORCE_INLINE int getIslandTag() const
	{
		return	m_islandTag1;
	}

	void	setIslandTag(int tag)
	{
		m_islandTag1 = tag;
	}
/*
	SIMD_FORCE_INLINE int getCompanionId() const
	{
		return	m_companionId;
	}

	void	setCompanionId(int id)
	{
		m_companionId = id;
	}
*/
	SIMD_FORCE_INLINE btScalar			getHitFraction() const
	{
		return m_hitFraction; 
	}

	void	setHitFraction(btScalar hitFraction)
	{
		m_hitFraction = hitFraction;
	}

	
	SIMD_FORCE_INLINE int	getCollisionFlags() const
	{
		return m_collisionFlags;
	}

	void	setCollisionFlags(int flags)
	{
		m_collisionFlags = flags;
	}
	
	///Swept sphere radius (0.0 by default), see btConvexConvexAlgorithm::
	btScalar			getCcdSweptSphereRadius() const
	{
		return m_ccdSweptSphereRadius;
	}

	///Swept sphere radius (0.0 by default), see btConvexConvexAlgorithm::
	void	setCcdSweptSphereRadius(btScalar radius)
	{
		m_ccdSweptSphereRadius = radius;
	}

	btScalar 	getCcdMotionThreshold() const
	{
		return m_ccdMotionThreshold;
	}

	btScalar 	getCcdSquareMotionThreshold() const
	{
		return m_ccdMotionThreshold*m_ccdMotionThreshold;
	}



	/// Don't do continuous collision detection if the motion (in one step) is less then m_ccdMotionThreshold
	void	setCcdMotionThreshold(btScalar ccdMotionThreshold)
	{
		m_ccdMotionThreshold = ccdMotionThreshold;
	}

	///users can point to their objects, userPointer is not used by Bullet
	void*	getUserPointer() const
	{
		return m_userObjectPointer;
	}
	
	///users can point to their objects, userPointer is not used by Bullet
	void	setUserPointer(void* userPointer)
	{
		m_userObjectPointer = userPointer;
	}


	inline bool checkCollideWith(btCollisionObject* co)
	{
		if (m_checkCollideWith)
			return checkCollideWithOverride(co);

		return true;
	}

	virtual	int	calculateSerializeBufferSize()	const;

	///fills the dataBuffer and returns the struct name (and 0 on failure)
	virtual	const char*	serialize(void* dataBuffer, class btSerializer* serializer) const;

	virtual void serializeSingleObject(class btSerializer* serializer) const;

};

ATTRIBUTE_ALIGNED16(class) btInteractionBlock
{
public:

	struct Feature
	{
		btCollisionObject* m_object;
		const btCollisionShape*	m_shape;

		Feature(): m_object(0), m_shape(0) {}
	};

	btInteractionBlock(btCollisionObject* object1, btCollisionObject* object2);
	btInteractionBlock(btCollisionObject* object1, const btCollisionShape* shape1, btCollisionObject* object2, const btCollisionShape* shape2);

	unsigned int getHashCode() const;
		
	inline bool isEqual(const btInteractionBlock& other) const
	{
		return m_feature1.m_object == other.m_feature1.m_object &&
				m_feature1.m_shape == other.m_feature1.m_shape &&
				m_feature2.m_object == other.m_feature2.m_object &&
				m_feature2.m_shape == other.m_feature2.m_shape;
	}

	inline const Feature& getFeature1() const
	{
		return m_feature1;
	}

	inline const Feature& getFeature2() const
	{
		return m_feature2;
	}

protected:
	Feature m_feature1;
	Feature m_feature2;
};


///do not change those serialization structures, it requires an updated sBulletDNAstr/sBulletDNAstr64
struct	btCollisionObjectDoubleData
{
	void					*m_broadphaseHandle;
	void					*m_collisionShape;
	btCollisionShapeData	*m_rootCollisionShape;
	char					*m_name;

	btTransformDoubleData	m_worldTransform;
	btTransformDoubleData	m_interpolationWorldTransform;
	btVector3DoubleData		m_interpolationLinearVelocity;
	btVector3DoubleData		m_interpolationAngularVelocity;
	btVector3DoubleData		m_anisotropicFriction;
	double					m_contactProcessingThreshold;	
	double					m_deactivationTime;
  double          m_lifeTime;
	double					m_friction;
	double					m_restitution;
	double					m_hitFraction; 
	double					m_ccdSweptSphereRadius;
	double					m_ccdMotionThreshold;

	int						m_hasAnisotropicFriction;
	int						m_collisionFlags;
	int						m_islandTag1;
	int						m_companionId;
	int						m_activationState1;
	int						m_internalType;
	int						m_checkCollideWith;

	char	m_padding[4];
};

///do not change those serialization structures, it requires an updated sBulletDNAstr/sBulletDNAstr64
struct	btCollisionObjectFloatData
{
	void					*m_broadphaseHandle;
	void					*m_collisionShape;
	btCollisionShapeData	*m_rootCollisionShape;
	char					*m_name;

	btTransformFloatData	m_worldTransform;
	btTransformFloatData	m_interpolationWorldTransform;
	btVector3FloatData		m_interpolationLinearVelocity;
	btVector3FloatData		m_interpolationAngularVelocity;
	btVector3FloatData		m_anisotropicFriction;
	float					m_contactProcessingThreshold;	
	float					m_deactivationTime;
  float         m_lifeTime;
	float					m_friction;
	float					m_restitution;
	float					m_hitFraction; 
	float					m_ccdSweptSphereRadius;
	float					m_ccdMotionThreshold;

	int						m_hasAnisotropicFriction;
	int						m_collisionFlags;
	int						m_islandTag1;
	int						m_companionId;
	int						m_activationState1;
	int						m_internalType;
	int						m_checkCollideWith;
};



SIMD_FORCE_INLINE	int	btCollisionObject::calculateSerializeBufferSize() const
{
	return sizeof(btCollisionObjectData);
}



#endif //COLLISION_OBJECT_H
