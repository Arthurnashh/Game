//-----------------------------------------------------------------------------
// File: enf_animationsystem.h
//
// Desc: Animation
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ANIMATIONSYSTEM_H
#define ENF_ANIMATIONSYSTEM_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "common/enf_hashmap.h"
#include "common/enf_bitfield64.h"
#include "engine/enf_module.h"

namespace enf
{

/** @addtogroup Animations
@{*/
class Entity;
class AnimsetDef;
class AnimationSystem;
class AnimInfo;
struct AnimSlot;
struct AnimHook;
struct AnimAction;
class ScriptModule;

extern PUBLIC_API AnimationSystem* g_pAnimationSystem;

ENF_INLINEFUNC AnimationSystem* GetAnimationSystem()
{
	return g_pAnimationSystem;
}

/*!
Array of bone weights for animation mixing.
For now there is just bit-mask, so weight
is 0/1, but it's almost ready for real
weights implementation.
Instances are created by AnimationSystem::LoadBoneGroups
and can be obtained by AnimationSystem::GetBoneGroup
*/
class PUBLIC_API BoneGroupDef: public BitField64<128>
{
protected:
	CStr	m_strName;

public:
	CStr GetName() const
	{
		return m_strName;
	}

	BoneGroupDef();
	BoneGroupDef(CStr name);
	~BoneGroupDef();

	ENF_INLINE bool IsZero() const
	{
		return BitField64<128>::IsZero();
	}

	bool IsFull() const;

	ENF_INLINE float GetWeight(uint bone) const
	{
		return IsSetBit(bone) ? 1.0f : 0.0f;
	}
};

/*!
Action handler for custom handling of animhook actions.
See AnimationSystem::AddActionHandler() AnimationSystem::RemoveActionHandler()
This class should be inherited, and methods HandleAction, LinkActions,
SetActionParam reimplemented.
*/
class PUBLIC_API AnimActionHandler
{
public:
	ENF_DECLARE_MMOBJECT(ActionHandler);

	/*!
	Provides handling of action.
	*/
	virtual bool HandleAction(Entity* ent, uint link, const AnimHook& hook) {return true;}

	/*!
	Provides dynamic linking of AnimHook::m_strAction->AnimHook::m_pAction
	when it's needed (i.e. links actions with script methods etc)
	*/
	virtual bool LinkActions(Entity* ent, const AnimInfo* info, uint* links) {return true;}

	/*!
	Provides handling of AnimHook::m_strParams -> AnimHook::m_pParams
	when it's needed (i.e. creates script variable for the value)
	*/
	virtual bool SetActionParam(const AnimAction* action, AnimHook& hook) {return true;}

	AnimActionHandler(const char* type):
		m_strType(type)
	{
	}
	virtual ~AnimActionHandler(){}

private:
	const char* m_strType;
};

/*!
AnimActionHandler that provides interface
between AnimationSystem and script core.
See AnimActionHandler
*/
class PUBLIC_API ScriptAnimHandler: public AnimActionHandler
{
protected:
	ScriptModule*	Module;

public:
	ScriptAnimHandler(ScriptModule* mod, const char* type): AnimActionHandler(type), Module(mod) {}
	bool LinkActions(Entity* ent, const AnimInfo* info, uint* links);
	bool HandleAction(Entity* ent, uint link, const AnimHook& hook);
	bool SetActionParam(const AnimAction* action, AnimHook& hook);
};


const uint MAX_BONES = 96;

/*!
Definition of skeleton. Created by AnimationSystem::LoadSkeletons.
Used internally for automatic bone mapping.
*/
class SkeletonDef
{
private:
friend class AnimationSystem;

	struct Bone
	{
		//!Name of bone
		const char*	Name;
		//!Extern index
		uint			Index;

		Bone():
			Name(NULL),
			Index(INDEX_NOT_FOUND)
		{
		}
		Bone(const char* bonename, uint index):
			Name(bonename),
			Index(index)
		{
		}
	};

	//!Array of bone definitions
	Array<Bone>		m_Bones;

	//!Name of skeleton def
	CStr				m_strName;

	//!Mapping from extern indexes to intern
	uint				m_iBoneMapping[MAX_BONES];

public:
	ENF_DECLARE_SMALLOBJECT(SkeletonDef);

	uint AddBone(CStr bonename);
	bool SetBoneExternIndex(uint boneindex, uint index);
	bool HasBone(CStr bonename) const;
	SkeletonDef(CStr name);
	~SkeletonDef();
};

/*!
Collection of all animation definitions
*/
class PUBLIC_API AnimationSystem: public Module
{
protected:
	Array<AnimsetDef*>	m_AnimsetDefs;
	HashMap<const char*, AnimsetDef*, StringHashFunc> m_AnimsetDefHashList;

	Set<BoneGroupDef*>	m_BoneGroups;
	HashMap<const char*, BoneGroupDef*, StringHashFunc> m_BoneGroupsHashList;

	Set<SkeletonDef*>	m_SkeletonDefs;
	
	/*!Used just for load time when we are searching for free
	extern indexes*/
	struct BoneRef
	{
		ENF_DECLARE_SMALLOBJECT(BoneRef);
		BoneRef*			Next;
		SkeletonDef*	Skeleton;
		uint				BoneIndex;
	};

	struct BoneDef
	{
		CStr			Name;
		uint			Index;
		uint			PivotIndex;
		BoneRef*		Refs;

		bool operator==(const BoneDef& def) const
		{
			return (Name.cmpi(def.Name) == 0);
		}
		bool operator<(const BoneDef& def) const
		{
			return (Name.cmpi(def.Name) < 0);
		}
		bool operator>(const BoneDef& def) const
		{
			return (Name.cmpi(def.Name) > 0);
		}
		BoneDef():
			Index(INDEX_NOT_FOUND),
			PivotIndex(INDEX_NOT_FOUND),
			Refs(NULL)
		{
		}
		BoneDef(CStr name, uint index = INDEX_NOT_FOUND):
			Name(name),
			Index(index),
			PivotIndex(INDEX_NOT_FOUND),
			Refs(NULL)
		{
		}
		//!Cleans references. We don't need them for runtime
		void Cleanup();
		
		//!Adds reference.
		void AddRef(SkeletonDef* def, uint boneindex);
		
		~BoneDef();
	};

	Set<Pair<CStr, uint> >	m_Pivots;

	Set<BoneDef>				m_BoneMapping;

	Set<AnimActionHandler*>	m_ActionHandlers;

	bool InsertBoneGroup(BoneGroupDef* def);

	void UnloadAnimsetDefinitions();
	~AnimationSystem();

public:
	ENF_DECLARE_ITEM(AnimationSystem, Module);

	void SetCurrent()
	{
		g_pAnimationSystem = this;
	}

	/*!
	Internal API for animators, like MeshObject-Skeleton
	*/
	void LinkActions(Entity* ent, const AnimInfo* animinfo, uint* links);

	/*!
	Internal API for AnimHook loaders
	*/
	void SetActionParam(const AnimAction* action, AnimHook& hook);

	/*!
	Internal API for animators, like MeshObject-Skeleton
	*/
	void HandleAction(Entity* ent, uint link, const AnimHook& hook);

	/*!
	Adds handler for anim-hook actions. When the handler is not
	removed, it's deleted upon AnimationSystem destruction
	\param handler The handler to be added
	*/
	void AddActionHandler(AnimActionHandler* handler);

	/*!
	Removes handler for anim-hook actions.
	\param handler The handler to be removed
	*/
	void RemoveActionHandler(AnimActionHandler* handler);

/** \name Animset defs
*/
//@{
/*!
Loads AnimsetDef definitions from XML file. Must
be loaded prior to Animsets!
\param name
Name of XML file with defs
*/
	bool LoadAnimsetDefinitions(CStr name);
	AnimsetDef* GetAnimsetDef(CStr name);
	
	ENF_INLINE const Array<AnimsetDef*>	GetAnimsetDefs() const
	{
		return m_AnimsetDefs;
	}

//@}

/** \name Bone groups
*/
//@{
/*!
Loads bonegroups definitions from XML file.
\param name
Name of XML file with defs
*/
	bool LoadBoneGroups(CStr name);
	void UnloadBoneGroups();

	const Set<BoneGroupDef*> GetBoneGroups() const
	{
		return m_BoneGroups;
	}

	const BoneGroupDef* GetBoneGroup(CStr name) const;
//@}

/** \name Skeletons
*/
//@{

	/*!
	Loads skeleton definitions from XML file.
	\param name
	Name of XML file with defs
	*/
	bool LoadSkeletons(CStr name);

	/*!
	Returns intern index to bone definition. It's for quick access
	of multiple parameters (extern index, name, etc)
	\param bonename
	Name of skeleton bone
	\return
	Internal index of bone, or INDEX_NOT_FOUND
	*/
	uint GetBoneIndex(CStr bonename) const;

	/*!
	Returns extern index for bone. This mapping comes from XML definition
	\param bonename
	Name of skeleton bone
	\return
	Index of bone, or INDEX_NOT_FOUND
	*/
	uint GetBoneExternIndex(CStr bonename) const;
	ENF_INLINE uint GetBoneExternIndex(uint index) const
	{
		return m_BoneMapping[index].Index;
	}

	/*!
	Returns pivot index of the bone. 
	\param index
	Internal index of bone
	\return
	Pivot index or INDEX_NOT_FOUND when bone is not pivot
	*/
	ENF_INLINE uint GetBonePivotIndex(uint index) const
	{
		return m_BoneMapping[index].PivotIndex;
	}
	
	/*!
	As bone name is not case sensitive, there might be many
	variants of the same name. That's we want to avoid. Here
	you can get const pointer to registered bone name.
	\param bonename
	Input name of bone
	\return
	Name of registered bone
	*/
	const char* GetBoneName(CStr bonename) const;
	ENF_INLINE const char* GetBoneName(uint index) const
	{
		return m_BoneMapping[index].Name.cstr();
	}

	/*!
	Gets registered bone, or creates new one when do not exists.
	\param bonename
	Input name of bone
	\return
	Definition of bone
	*/
	BoneDef* FindOrCreateBone(CStr bonename);
//@}

	bool LoadPivots(CStr name);
	uint GetPivotIndex(CStr pivotname) const;

	AnimationSystem();
};
//@}
}
#endif //ENF_ANIMATIONSYSTEM_H
