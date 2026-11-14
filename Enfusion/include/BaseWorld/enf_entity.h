//-----------------------------------------------------------------------------
// File: enf_entity.h
//
// Desc: 
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ENTITY_H
#define ENF_ENTITY_H

#include "enf_config.h"
#include "common/enf_mathlib.h"
#include "script/enf_scriptvariable.h"
#include "engine/enf_event.h"

#ifdef ENF_WB
	#include "enf_entitywb.h"
#endif

class GameEditorAPI;

namespace enf
{

using namespace enf;

/** @defgroup Entity Entity management
@{*/

class BaseWorld;
class Entity;
class EntitySource;
class PhysicsController;
class VObjectInstance;
class VObject;
struct SceneTreeLink;
class Shape;
class SerializeOutputAdapter;
class SerializeInputAdapter;
class EventHandler;
class Contact;
struct LightInteraction;
class Texture;
class ModelContext;
struct WindInfo;
class RTTextureWidget;

const float TICK_PERIOD = 1.0f / 15.0f;

//-----------------------------------------------------------------------
/*!
Various Entity flags
*/
//-----------------------------------------------------------------------
enum EntityFlags
{
/*!
Entity is visible. Is rendered if there is some attached object
and event EV_VISIBLE is invoked, when event mask is set.
*/
	TFL_VISIBLE				= BITF(0),


	TFL_SOLID				= BITF(1),	///<Is collidable by various trace methods
	TFL_TRIGGER				= BITF(2),	///<Is not collidable, but invokes touch events.

/*!
Used by tracing methods. When tracing with TRACE_PASSTRANSLUCENT,
then this entity is ignored. Used for glass for example.
*/
	TFL_TRANSLUCENT		= BITF(7),

//!Used by tracing methods. Traceable only with flag TRACE_WATER
	TFL_WATER				= BITF(8),

/*!
Tells that this entity has to be actively updated by engine, her
EV_FRAME has to be called etc.
*/
	TFL_ACTIVE				= BITF(9),

/*!
Tells that this entity will represent mostly static object, so
we can use more precise but slower methods for scene-tree linking.
Also it tells to scene tree that he can count with this entity as
tree-split hint.
*/
	TFL_STATIC				= BITF(10),

/*!
Used by visibility algorithms. This entity is used as occluder
when overlapping area portals. Used by doors for example.
*/
	TFL_AREASPLITTER		= BITF(11),

/*!
Used by visibility algorithms together with TFL_AREASPLITTER.
When set, overlapped areaportal is occluded. Used by doors for
example.
*/
	TFL_AREACLOSED			= BITF(12),

//!Flags for custom usage and filterings.
	TFL_USER1				= BITF(13),
	TFL_USER2				= BITF(14),
	TFL_USER3				= BITF(15),
	TFL_USER4				= BITF(16),
	TFL_USER5				= BITF(17),
	TFL_USER6				= BITF(19),

	TFL_TRANSPARENT_OCCLUDER = BITF(20),
//! When set, Entity calls GetUserBoundBox() virtual method, instead of the standard way
	TFL_USER_BOUNDBOX		= BITF(21),

//!Private flag. Set when Entity have no ortho-normalized matrix
	TFL_UNNORMALIZED		= BITF(26),

//!Private flag. Forces to update AABB, nevertheless it fit's into old one
	TFL_UPDATE_BOUNDS		= BITF(27),

//!Private flag. Set when entity has update attached object.
	TFL_UPDATE_MDL			= BITF(28),

/*!
Private flag. Set when entity has chaged position, orientation or just
have to relink its position in scene tree.
*/
	TFL_UPDATE_LINK		= BITF(29),

//!Private flag. Set when entity has changed something and should be updated.
	TFL_UPDATE				= BITF(30),

//! Private flag. Set when entity is going to be deleted upon UpdateEntities.
	TFL_REMOVE				= BITF(31),

//!Private flag. Mask of all private flags.
	TFL_PRIVATE_FLAGS = TFL_UPDATE_MDL|TFL_UPDATE_LINK|TFL_UPDATE|TFL_REMOVE
};


//-----------------------------------------------------------------------
/*!
Various Entity events
*/
//-----------------------------------------------------------------------
enum EntityEvents
{
	EV_TOUCH			= BITF(0),

	EV_INIT			= BITF(3),
	EV_EXTRA			= BITF(4),
	EV_NOTVISIBLE	= BITF(5),
	EV_FRAME			= BITF(6),
	EV_VISIBLE		= BITF(7),
	EV_POSTFRAME	= BITF(8),

	EV_WORLDPROCESS= BITF(9), ///<only world entity

	EV_FLASHLIGHT	= BITF(10),

	EV_ANIMEND		= BITF(14),	///<animation reached last frame
	EV_ANIMBLEND	= BITF(16),	///<blend just ended

	EV_SOUNDEND		= BITF(19),
	EV_USER			= BITF(20),	///<21,22,23

	EV_TICK			= BITF(26),
	EV_SIMULATE		= BITF(27),
	EV_POSTSIMULATE= BITF(28),

	EV_JOINTBREAK	= BITF(29),
	EV_PHYSICSMOVE	= BITF(30),
	EV_CONTACT		= BITF(31),

	EV_ALL			= 0xffffffff
};

//-----------------------------------------------------------------------
#define FOREACH_ENTITY(a,b) \
	{ \
		CLink *foreach_link, *foreach_start; \
		foreach_start = (a); \
		for( foreach_link = foreach_start->Next; foreach_link != foreach_start; foreach_link = foreach_link->Next) \
		{ \
			b = Entity::FromEntityChain(foreach_link);

#define END_FOREACH \
		} \
	}


//-----------------------------------------------------------------------
struct EntityHierarchy
{
	ENF_DECLARE_SMALLOBJECT(EntityHierarchy);

	Entity*					m_pParent;	///<if we are child
	uint						m_iPivot;	///<which pivot we are using
	Entity*					m_pSibling;	///<sibling sitting on this pivot
	Entity*					m_pChildren;
	Matrix43					m_Matrix;
	EntityHierarchy(): m_pParent(NULL), m_iPivot(0), m_pSibling(NULL), m_pChildren(NULL), m_Matrix(true)
	{
	}
};


//-----------------------------------------------------------------------
class PUBLIC_API Entity: public Instance
{
protected:
/*!
Dummy destructor. Entity must never be deleted directly, but by Entity::Delete method!
*/
	virtual ~Entity();

private:

	friend class BaseWorld;
	friend class PhysicsController;

	CLink								m_liChain;			///<Entity chain
	uint								m_iFlags;			///<See enf::EntityFlags
	uint								m_iEventMask;		///<See enf::EntityEvents

	uint								m_iTraceCount;		///<For visibility/collision checks
	uint								m_iRenderFrame;	///<Set to BaseWorld::m_iFrameCount when visible
	VObjectInstance*		m_pObjectInstance;///<Pointer to VObject instance
	
	AAB									m_BBox;				///<Maximized bound box in world coordinates
	enf::Matrix43				m_Matrix;			///<positional matrix

	BaseWorld*					m_pWorld;			///<The BaseWorld we belong to
	SceneTreeLink*			m_pLink;				///<Pointer to scene-tree native link structure

	uint								m_iID;				///<ID from EntityData
	EntityHierarchy*		m_pHierarchy;		///<Description of hierarchy, if any

	PhysicsController*	m_pPhysics;			///<Physics controller

	uint r_Update(const Matrix43 *pvp);
	int _ThrowEvent(Entity* actor, uint eventmask, void* extra);

public:

#ifndef ENF_DOXYGEN
	void* operator new(size_t size, char const * file, int line);
	void* operator new[](size_t size, char const * file, int line);
	void* operator new(size_t size);
	void* operator new[](size_t size);
	void operator delete(void* ptr, char const * file, int line);
	void operator delete[](void* ptr, char const * file, int line);
	void operator delete(void* ptr);
	void operator delete[](void* ptr);
	ENF_INLINE void* operator new(size_t size, void *p){ return p;}
	ENF_INLINE void* operator new[](size_t size, void *p){ return p;}
	ENF_INLINE void operator delete(void* ptr, void* p) {  }
	ENF_INLINE void operator delete[](void* ptr, void* p) {  }
#endif

	/*!
	Returns type-name of object. Default implementation returns name
	of associated script class. When scripting is not used, you
	should re-implement this.
	*/
	virtual const char* GetTypeName() const;

	/*!
	Returns name of object. Default implementation returns content
	of _name variable of script class. When scripting is not used, you
	should re-implement this.
	*/
	virtual const char* GetName() const;

	/*!
	Set unique identifier. This is usually copied
	from EntitySource. This ID is used when adding
	into BaseWorld and Entity can be found by BaseWorld::FindEntityByID
	then.
	\param id
	unique ID
	*/
	void NMETHOD SetID(uint id);

	/*!
	Get unique identifier. See enf::Entity::SetID
	\return
	ID of entity
	*/
	uint NMETHOD GetID() const
	{
		return m_iID;
	}
	
	//!Removes all associated script threads
	void KillThreads();

	/** \name Orientation methods
		Setting and getting of entity orientation
	*/
	//@{

	/*!
	Immediately gets orientation matrix of entity. It doesn't update entity
	before return when matrix is dirty.
	\return
	Current orientation matrix
	*/
	ENF_INLINE const Matrix43& IGetMatrix() const
	{
		return m_Matrix;
	}

	/*!
	Immediately gets normalized orientation matrix of entity.
	It doesn't update entity before return when matrix is dirty.
	\return
	Current orientation matrix
	*/
	ENF_INLINE Matrix43 IGetNormalizedMatrix() const
	{
		if(m_iFlags & TFL_UNNORMALIZED)
		{
			Matrix43 mat = m_Matrix;
			mat.Normalize();
			return mat;
		}
		else
			return m_Matrix;
	}

	/*!
	Gets orientation matrix of entity. When we don't have parent, it returns
	global matrix
	\return
	Current local orientation matrix
	*/
	ENF_INLINE const Matrix43& GetLocalMatrix() const
	{
		return GetParent() ? m_pHierarchy->m_Matrix : m_Matrix;
	}

	/*!
	Immediately gets translation of entity. It doesn't update entity
	before return when translation is dirty.
	\return
	Current translation
	*/
	ENF_INLINE const Vector3& IGetTranslation() const
	{
		return m_Matrix.GetTranslation();
	}

	/*!
	Immediately sets orientation matrix of entity. It doesn't set
	dirty flags, nor pass matrix to physics engine. It's used internaly
	by physics engine.
	\param mat
	New orientation matrix
	*/
	void ISetMatrix(const Matrix43& mat);

	/*!
	Changes orientation of entity.
	\param mat
	Matrix containing orientation and translation.
	\return
	True when matrix has been updated, false otherwise.
	*/
	ebool SetMatrix(const Matrix43& mat);

	/*!
	Changes position of entity. 
	\param org
	Vector containing position.
	\return
	True when position has been updated, false otherwise (may happen when position is the same
	as previous one).
	*/
	ebool SetOrigin(const Vector3& org);
	
	ENF_INLINE ebool SetOrigin(float x, float y, float z)
	{
		return SetOrigin(Vector3(x, y, z));
	}

	/*!
	Immediately sets position of entity. It doesn't set
	dirty flags, nor pass position to physics engine. It's used internaly
	by physics engine.
	\param org
	New position matrix
	*/
	void ISetOrigin(const Vector3& org);

	/*!
	Gets position of entity
	\return
	Position
	*/
	ENF_INLINE const Vector3& GetOrigin() const
	{
		return m_Matrix.GetTranslation();
	}

	/*!
	Changes orientation of entity.
	\param angles
	Vector containing angles.
	\return
	True when orientation has been updated, false otherwise (may happen when orientation is the same
	as previous one).
	*/
	ebool SetAngles(const Vector3& angles);

	/*!
	Changes orientation of entity.
	\param pitch Pitch angle
	\param yaw Yaw angle
	\param roll Roll angle
	\return
	True when orientation has been updated, false otherwise (may happen when orientation is the same
	as previous one).
	*/
	ENF_INLINE ebool SetAngles(float pitch, float yaw, float roll)
	{
		return SetAngles(Vector3(pitch, yaw, roll));
	}

	/*!
	Gets orientation of entity
	\return Euler angles
	*/
	Vector3 GetAngles() const;

	//!returns roll
	float GetRollAngle() const;

	//!returns pitch
	float GetPitchAngle() const;

	//!returns yaw
	float GetYawAngle() const;

	/*!
	Gets local orientation of entity. When there is no parent,
	it returns global orientation
	\return
	Euler angles
	*/
	Vector3 GetLocalAngles() const;

	/*!
	Transforms vector from entity space to parent(world) space
	\param vec
	Vector to be transformed
	\return
	Vector transformed to parent space
	*/
	ENF_INLINE Vector3 VectorToParent(const Vector3& vec) const
	{
		return m_Matrix.TransformVector(vec);
	}
	Vector3 NMETHOD _VectorToParent(const Vector3& vec) const
	{
		return m_Matrix.TransformVector(vec);
	}

	/*!
	Transforms position from entity space to parent(world) space
	\param coord
	Position to be transformed
	\return
	Position transformed to parent space
	*/
	ENF_INLINE Vector3 CoordToParent(const Vector3& coord) const
	{
		return m_Matrix.TransformCoord(coord);
	}
	Vector3 NMETHOD _CoordToParent(const Vector3& coord) const
	{
		return m_Matrix.TransformCoord(coord);
	}

	/*!
	Transforms vector from parent(world) space to local(entity) space
	\param vec
	Vector to be transformed
	\return
	Vector transformed to local space
	*/
	ENF_INLINE Vector3 VectorToLocal(const Vector3& vec) const
	{
		if(m_iFlags & TFL_UNNORMALIZED)
			return m_Matrix.GetInverse().TransformVector(vec);
		else
			return m_Matrix.InvTransformVector(vec);
	}
	
	Vector3 NMETHOD _VectorToLocal(const Vector3& vec) const
	{
		return VectorToLocal(vec);
	}

	/*!
	Transforms position from parent(world) space to local(entity) space
	\param coord
	Position to be transformed
	\return
	Position transformed to local space
	*/
	ENF_INLINE Vector3 CoordToLocal(const Vector3& coord) const
	{
		if(m_iFlags & TFL_UNNORMALIZED)
			return m_Matrix.GetInverse().TransformCoord(coord);
		else
			return m_Matrix.InvTransformCoord(coord);
	}
	Vector3 NMETHOD _CoordToLocal(const Vector3& coord) const
	{
		return CoordToLocal(coord);
	}

	//@}

	/** \name Hierarchy methods
		Scene hierarchy management
	*/
	//@{

	/*!
	Returns parent of this entity (entity on upper level
	of hierarchy).
	\return
	Pointer to parent entity, or NULL, when we are not child.
	*/
	ENF_INLINE Entity* GetParent() const
	{
		return m_pHierarchy ? m_pHierarchy->m_pParent : NULL;
	}

	ENF_INLINE uint GetParentPivot() const
	{
		return m_pHierarchy ? m_pHierarchy->m_iPivot : INDEX_NOT_FOUND;
	}

	/*!
	Returns next sibling of this entity (next entity on this level
	of hierarchy).
	\return
	Pointer to sibling entity, or NULL
	*/
	ENF_INLINE Entity* GetSibling() const
	{
		return m_pHierarchy ? m_pHierarchy->m_pSibling : NULL;
	}

	/*!
	Returns first child from this level of hierarchy
	if there is any.
	\return
	Pointer to child entity, or NULL
	*/
	ENF_INLINE Entity* GetChildren() const
	{
		return m_pHierarchy ? m_pHierarchy->m_pChildren : NULL;
	}

	/*!
	Removes child entity from hierarchy.
	\param ent
	Pointer to child entity we want to remove.
	\param keeptransform
	When set to true, Entity is kept on her world position. Otherwise it's local transform is used as world-space one.
	\return
	True if it was removed, false when this entity is not our child.
	*/
	ebool RemoveChild(Entity *ent, bool keeptransform = false);

	/*!
	Adds child entity to this entity. 
	\param child
	Pointer to entity which become our child
	\param pivot
	ID of pivot the child will be attached to. The pivot must exists in VObject set to this entity.
	When it is not, child is attached directly to origin of this entity.
	\return
	True when entity has been attached. False otherwise
	*/
	ebool AddChild(Entity *child, uint pivot);

	/*!
	Tells whether some entity belongs to the same hierarchy.
	In fact, we are searching if top parent entity of both entites is the same.
	\param ent
	Entity we are asking for
	\return
	True when the entity belongs to the same hierarchy, false otherwise.
	*/
	ebool IsHierarchy(const Entity *ent) const;

	/*!
	Completely removes from parent and child entities
	*/
	void RemoveFromHierarchy();

	//@}


	//!Wen called, the Entity is excluded from consequent TraceMove/TraceLine
	void FilterNextTrace();

	ENF_INLINE bool SetTraceCount(uint tracecount)
	{
		if(m_iTraceCount != tracecount)
		{
			m_iTraceCount = tracecount;
			return true;
		}
		return false;
	}

	ENF_INLINE bool SetRenderFrame(uint renderframe)
	{
		if(m_iRenderFrame != renderframe)
		{
			m_iRenderFrame = renderframe;
			return true;
		}
		return false;
	}

	ENF_INLINE uint GetRenderFrame() const
	{
		return m_iRenderFrame;
	}

	uint GetNumFramesSinceRendered() const;

	ENF_INLINE bool IsRendered(uint renderframe) const
	{
		return (m_iRenderFrame == renderframe);
	}

	ENF_INLINE SceneTreeLink* GetSceneTreeLink() const
	{
		return m_pLink;
	}

	ENF_INLINE void SetSceneTreeLink(SceneTreeLink* link)
	{
		m_pLink = link;
	}

	virtual void Serialize(SerializeOutputAdapter* );
	virtual void Serialize(SerializeInputAdapter* );

	static Entity* FromEntityChain(const CLink* l);

	/*!
	Gets current AABB box.
	\return
	Bounding box.
	*/
	ENF_INLINE const AAB& GetBBox() const
	{
		return m_BBox;
	}
	virtual AAB GetUserBoundBox() const;

	//!Returns tight bounding box
	AAB ComputeBoundBox() const;

	/*!
	Gets the BaseWorld we belong to.
	\return
	Pointer to owner BaseWorld.
	*/
	ENF_INLINE BaseWorld* GetWorld() const
	{
		return m_pWorld;
	}

	/*!
	Gets object instance pointer.
	\return
	VObjectInstance pointer or NULL when not present.
	*/
	ENF_INLINE VObjectInstance* GetInstance() const
	{
		return m_pObjectInstance;
	}

	/*!
	Changes visual object represented by this entity
	\param object Visual object, or NULL
	\param options Various options. Depends on object type. Or NULL
	\return
	VObjectInstance pointer or NULL when not present.
	*/
	VObjectInstance* NMETHOD SetObject(VObject* object, const char* options = NULL);

	/*!
	Gets VObject pointer if is attached to the entity
	\return
	VObject pointer or NULL when not present.
	*/
	VObject* NMETHOD GetVObject() const;

	/*!
	Updates bounding box of this entity. It's done
	automatically during physics update, but we can
	do it manually, when we need actual box just in time.
	*/
	bool UpdateBBox();
   uint Update();
	
	/*!
	Deletes entity. In fact it doesn't delete it right away but rather it cleans it up,
	calls destructors and let the world to delete it later when it is really safe.
	*/
	void PrepareForDelete();

	/*!
	Tells whether is this entity just point sized.
	\return
	True if it's point, false otherwise.
	*/
	ENF_INLINE ebool IsPointSize() const
	{
		return m_BBox.GetSize().IsZero();
	}

	void DirtyFlags(uint flags = 0);

	/*!
	Sets one or more of specified flags
	\param flags
	One or more flags to be set. See enf::EntityFlags
	\param recursively
	When true, the flags will be recursively applied to all children
	\return
	Current state of bitmask. See enf::EntityFlags
	*/
	uint SetFlags(uint flags, ebool recursively = false);

	/*!
	Clears one or more of specified flags
	\param flags
	One or more flags to be cleared. See: enf::EntityFlags
	\param recursively
	When true, the flags will be recursively cleared on all children
	\return
	These bits that were set before and now are cleared.
	*/
	uint ClearFlags(uint flags, ebool recursively = false);

	/*!
	Returns currently set flags.
	\return
	One or more set flags. See: enf::EntityFlags
	*/
	ENF_INLINE uint GetFlags() const
	{
		return m_iFlags;
	}
	
	/*!
	Test if one or more of specified flags are set
	\param flags
	One or more flags. See: enf::EntityFlags
	\return
	True if is set, false otherwise.
	*/
	ENF_INLINE bool IsFlagSet(uint flags) const
	{
		return ((m_iFlags & flags) != 0);
	}

	/*!
	Gets current eventmask. Entity accepts only events which has set bits in eventmask.
	\return
	Bitmask of events the entity accepts
	*/
	ENF_INLINE uint GetEventMask() const
	{
		return m_iEventMask;
	}

	/*!
	Sets eventmask. Entity accepts only events which has set bits in eventmask.
	Bits are or'ed with existing bitmask. See enf::EntityEvents
	\return
	Bitmask of events the entity accepts (result of mask|GetEventMask())
	*/
	uint SetEventMask(uint mask);

	/*!
	Clears bitmask. Entity accepts only events which has set bits in eventmask.
	Only bits set in the mask are cleared. See enf::EntityEvents
	\param mask
	Mask of those bits, which will be cleared
	\return
	These bits that were set before and now are cleared.
	*/
	uint ClearEventMask(uint mask);

	/*!
	Sends event to the default event handler. See enf::BaseWorld::SetEntityEventHandler
	\param actor
	Pointer to entity what caused event (for example the toucher during EV_TOUCH event)
	\param eventmask
	bitmask of event See enf::EntityEvents
	\param extra
	extra pointer. Used for event related data
	*/
	ENF_INLINE int ThrowEvent(Entity* actor, uint eventmask, void* extra)
	{
		if((m_iEventMask & eventmask) == 0)
			return 0;
		return _ThrowEvent(actor, eventmask, extra);
	}

	/*!
	Tells whether object set to this entity casts shadows.
	\return
	True when does, false otherwise
	*/
	ebool IsShadowCaster() const;

	/*!
	Gets physics controller attached to this entity
	\return
	Pointer to attached physics controller, or NULL
	*/
	ENF_INLINE PhysicsController* GetPhysics() const
	{
		return m_pPhysics;
	}

	/*!
	Sets physics controller. If there is one set, it's destroyed.
	\param pc
	Pointer to physics controller, or NULL
	*/
	void SetPhysicsController(PhysicsController* pc);

	//! Returns associated render target texture 
	virtual Texture* GetRTTexture(uint index) const;
	virtual void SetRTTexture(RTTextureWidget* rtwidget);

	virtual void PrepareInstanceInfo(const WindInfo& windInfo, float globpower, float zdelta, float invzdelta, float freq, uint windEmitors, ModelContext& mc) {}
	virtual void PrepareInstanceInfo(ModelContext& mc) {}

	/*!
	Sets eventmask EV_TICK and attaches internal timer with defined period.
	EOnTick() is called by this period
	\param period The period of timer
	*/
	void NMETHOD SetTick(float period);
	
	/*!
	Clears eventmask EV_TICK and removes internal timer
	*/
	void NMETHOD RemoveTick();

	//!Returns period or -1 when there is not any.
	bool GetTick(float& period, float& actualTime);
	void SetTickEx(float period, float actualTime);

	/** \name Event methods
		Most common entity events. They might be reimplemented.
	*/
	//@{

	/*!
	Event when touched by other entity
	\param other
	Entity who touched us
	\param extra
	Bitmask of touch types TODO
	*/
	virtual void EOnTouch(Entity* other, int extra) {}

	/*!
	Event when we touched ground
	\param other
	Entity which represents ground
	\param surfparms
	Surface flags of surface we are standing on
	*/
	virtual void EOnGround(Entity* other, uint surfparms) {}

	/*!
	Event after entity is allocated and initialized.
	\param other
	Pointer to world entity
	\param extra
	Number of entity
	*/
	virtual void EOnInit(Entity* other, int extra) {}

	/*!
	Extra event of various functional extensions. ATM it's used
	by Trigger when some insider is leaving
	\param other
	Other actor of event
	\param extra
	Extra value of event
	*/
	virtual void EOnExtra(Entity* other, int extra) {}

	/*!
	Event when we are out of visibility
	\param other
	World entity
	\param extra
	Frame number
	*/
	virtual void EOnNotVisible(Entity* other, int extra) {}

	/*!
	Event when we are visible
	\param other
	World entity
	\param extra
	Frame number
	*/
	virtual void EOnVisible(Entity* other, int extra) {}

	/*!
	Event every frame
	\param other
	World entity
	\param extra
	Frame number
	*/
	virtual void EOnFrame(Entity* other, int extra) {}

	/*!
	Even after physics update
	\param other
	World entity
	\param extra
	Frame number
	*/
	virtual void EOnPostFrame(Entity* other, int extra) {}

	/*!
	Event called only on world entity on begin of frame
	\param other
	World entity
	\param extra
	Frame number
	*/
	virtual void EOnWorldProcess(Entity* other, int extra) {}

	/*!
	Event when entity is enlighten by DynamicLight marked by LFL_FLASHLIGHT
	\param other
	Owner of the DynamicLight
	\param extra
	Not used ATM
	*/
	virtual void EOnFlashlight(Entity* other, int extra) {}

	/*!
	Event when animation is finished on this Entity
	\param other
	Pointer to us
	\param extra
	Number of animation channel
	*/
	virtual void EOnAnimEnd(Entity* other, int extra) {}

	/*!
	Event when animation blending is finnished on this Entity
	\param other
	Pointer to us
	\param extra
	Number of animation channel
	*/
	virtual void EOnAnimBlend(Entity* other, int extra) {}

	/*!
	Event when sound playing is finished on this entity
	\param other
	Pointer to us
	\param extra
	Handle returned by XSoundSystem::Play
	*/
	virtual void EOnSoundEnd(Entity* other, int extra) {}

	/*!
	Event every TICK_PERIOD period of time
	\param other
	World Entity
	\param extra
	Not used ATM
	*/
	virtual void EOnTick(Entity* other, int extra) {}

	/*!
	Event after simulated by physics engine (once per frame)
	\param other
	World Entity
	\param timeslice
	Time slice of simulation step
	*/
	virtual void EOnPostSimulate(Entity* other, float timeslice) {}

	/*!
	Event before simulated by physics engine (called from sub-iterations!
	\param other
	World Entity
	\param timeslice
	Time slice of simulation step
	*/
	virtual void EOnSimulate(Entity* other, float timeslice) {}

	/*!
	Event when joint attached to RigidBody of this entity is broken
	\param other
	Other Entity attached to the joint
	\param extra
	Not used ATM
	*/
	virtual void EOnJointBreak(Entity* other, int extra) {}
	/*!
	Event when physics engine has moved with this Entity
	\param other
	World Entity
	\param extra
	Not used ATM
	*/
	virtual void EOnPhysicsMove(Entity* other, int extra) {}

	/*!
	Event when physics engine registered contact with other RigidBody
	\param other
	Other Entity who contacted us
	\param contact
	Structure describing the contact
	*/
	virtual void EOnContact(Entity* other, const Contact* contact) {}

	/*!
	Any other event
	\param other
	Other entity what caused event
	\param event
	What event is it
	\param extra
	Extra data about event
	*/
	virtual void EOnEvent(Entity* other, uint event, int extra) {}
	//@}

	/*!
	Constructor used by ScriptContext::New for Class defined
	entities.
	\param src
	Source data. If some script variables matches, they are initialized
	*/
	Entity(const EntitySource* src);
	void InitScriptVariables(const EntitySource* src);

	/*!
	Constructor for pure C++ defined entities
	*/
	Entity();

	#ifdef ENF_WB
	/** \name Workbench methods
	*/
	//@{
		virtual void WB_MakeVisible(const EntitySource* src, bool visible);	//<in some situations editor need show or hide entity
		virtual void WB_MakeSolid(const EntitySource* src, bool solid);	//<in some situations editor need make entity solid (raycastable) or no
		virtual void WB_SetOrientation(const Vector3& pos, const Vector3& angles, const EntitySource* src);	//<potreba zmenit orientaciu
		virtual void WB_OnBeforeKeysChanged(const EntitySource* src, const Array<CStr>& keys){};
		virtual void WB_OnKeysChanged(const EntitySource* src, const Array<CStr>& keys){};
		virtual Entity* WB_GetEditableOwner( void ) { return NULL; }	//<editor pri selektovani naraycastoval alebo inak ziskal tuto entitu a potrebuje vediet ci patri nejakej inej entite ktora editovatelna je. ked ano, vratit pointer ne toho ownera
		virtual void WB_SetEditableOwner( Entity* pOwner ) {}	//<editor pri selektovani naraycastoval alebo inak ziskal tuto entitu a potrebuje vediet ci patri nejakej inej entite ktora editovatelna je. ked ano, vratit pointer ne toho ownera
		virtual bool WB_CanDelete(const EntitySource* EntSrc){return true;}
		virtual void WB_GetAutoDeleteEnts(Array<EntitySource*>& ents){}
		virtual bool WB_CanRename(const EntitySource* EntSrc, enf::SString<>* reason){return true;}												//<rename is not possible if false
		virtual bool WB_CanCopy(const EntitySource* EntSrc){return true;}
		virtual bool WB_IsVisibleVolume(const EntitySource* src);	//<entity must return true, if have any logical visible volume
		virtual void WB_GetBoundBox(AAB& BBox, const EntitySource* src);													//<editor potrebuje BBox za ucelom zobrazenia vizualizeru a pod
		virtual void WB_GetCBoundBox(AAB& BBox, const EntitySource* src);
		virtual void WB_SetExtraVisualiser(EntityVisualizerType type, const EntitySource* src);		//<space to create optional extra visualizer in editor
		virtual void WB_GetContextMenuItems(Array<UIMenuItem>& items){}	//<opportunity to append items into entity context menu
		GameEditorAPI* WB_GetEditorAPI(bool CheckForNull = true);	//this returns API with editor edit actions
		virtual bool WB_CanMovePhysically(const EntitySource* src){return false;}
		virtual bool WB_EnablePhysics(const EntitySource* src, bool physics){ return false;}	//prepare to edit position/orientation by physical simulation. return true, if physics was enabled before

		//editor events
		virtual void WB_AfterWorldUpdate(){}	//<called only if entity is registered for receiving workbench events using GameEditorAPI::RegisterEventsEntity()
		virtual void WB_OnContextMenu(const Array<UIMenuItem>& items, int index){}		//<result index to items returned by WB_GetContextMenuItems()
		virtual void WB_OnKeyDown(int KeyCode){}
		virtual void WB_OnCreate(EntitySource* src){}			//<created from editor
		virtual void WB_OnDelete(const EntitySource* src){}			//<before delete. called before OnDelete() and allways before physical delete of entity!
		virtual void WB_OnRename(const EntitySource* src, const char* OldName){}		//<after rename
	//@}
	#endif
};

//-----------------------------------------------------------------------
/*!
Event used for reporting Entity events. By default it's routed to EnforceScriptContext,
but might be redirected to custom EventHandler by BaseWorld::SetEntityEventHandler
*/
//-----------------------------------------------------------------------
class EntityEvent: public Event
{
protected:
	Entity*	m_pEntity;
	Entity*	m_pOther;
	uint		m_iEventMask;
	void*		m_pExtra;

public:
	uint GetType() const
	{
		return EntityEventTypeID;
	}

	virtual ebool HasType(uint type) const
	{
		if(type == EntityEventTypeID)
			return true;

		return Event::HasType(type);
	}

	ENF_INLINE Entity* GetEntity() const
	{
		return m_pEntity;
	}

	ENF_INLINE Entity* GetOther() const
	{
		return m_pOther;
	}

	ENF_INLINE uint GetEventMask() const
	{
		return m_iEventMask;
	}

	ENF_INLINE void* GetExtra() const
	{
		return m_pExtra;
	}

	EntityEvent(Entity* ent, Entity* other, uint event, void* extra)
	{
		m_pEntity = ent;
		m_pOther = other;
		m_iEventMask = event;
		m_pExtra = extra;
	}
};
//@}
}

#endif //ENF_ENTITY_H