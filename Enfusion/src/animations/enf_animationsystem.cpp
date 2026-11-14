//-----------------------------------------------------------------------------
// File: enf_animationsystem.cpp
//
// Desc: Animation system class
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
//#include "engine/enf_engine.h"
#include "animations/enf_animationsystem.h"
#include "resource/enf_resourcemanager.h"
#include "animations/enf_animation.h"
//#include "animations/enf_animset.h"
#include "tinyxml/tinyxml.h"
#include "enf_main.h"
//#include "enf_scriptapi.h"
//#include "enf_eparser.h"
//#include "BaseWorld/enf_entity.h"

namespace enf
{
AnimationSystem* g_pAnimationSystem = NULL;

//-----------------------------------------------------------------------------
/*
bool ScriptAnimHandler::LinkActions(Entity* ent, const AnimInfo* info, uint* links)
{
	Funct* fn;

	for(uint n = 0; n < info->GetAnimHooks().GetCardinality(); n++)
	{
		AnimHook& hook = info->GetAnimHooks()[n];

		fn = ent->GetClass()->GetMethod((uint)hook.m_pAction);

		if(fn == NULL || fn->GetName() != hook.m_strAction.cstr())
		{
			hook.m_pAction = (void*)ent->GetClass()->GetMethodIndex(hook.m_strAction.cstr());
		}

		links[n] = (uint)hook.m_pAction;
	}
	return true;
}

//-----------------------------------------------------------------------------
bool ScriptAnimHandler::HandleAction(Entity* ent, uint link, const AnimHook& hook)
{
	if(link >= ent->GetClass()->GetNumMethods())
		return false;

	if(hook.m_pParam)
	{		
		Funct *f = ent->GetClass()->GetMethod(link);
		const Variable* parm = (const Variable*)hook.m_pParam;

		if(f->GetNumParameters() >= 1 && f->GetVariable(0)->GetVType() == parm->GetVType())
		{
			if(parm->GetVType() == VARTYPE_STRING)
				Module->GetContext()->CopyString(parm->String, f->GetVariable(0), &f->GetVariable(0)->String);
			else
				f->GetVariable(0)->Int = parm->Int;
		}
	}

	Module->IExecute(Module->GetContext()->GetCurrThread(), link + 1, ent, ent->GetClass());
	return true;
}

//-----------------------------------------------------------------------------
bool ScriptAnimHandler::SetActionParam(const AnimAction* action, AnimHook& hook)
{
	if(action->m_strHandler != "script" || hook.m_strParam.IsEmpty())
		return false;

	hook.m_pParam = Module->GetVariable(hook.m_strParam.cstr());

	if(hook.m_pParam == NULL)
	{
		TToken token;
		token.Text = hook.m_strParam.cstr();
		token.Spaces = 0;
		switch(action->m_ParmType)
		{
		case VARTYPE_FLOAT:
			token.Type = TT_NUMBER;
			hook.m_pParam = Module->AddConstant(Module->GetContext()->m_vtFloatType, &token);
			break;

		case VARTYPE_INT:
			token.Type = TT_NUMBER;
			hook.m_pParam = Module->AddConstant(Module->GetContext()->m_vtIntType, &token);
			break;

		case VARTYPE_STRING:
			token.Type = TT_STRING;
			hook.m_pParam = Module->AddConstant(Module->GetContext()->m_vtStringType, &token);
			break;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
uint AnimInfo::AddAnimHook(const AnimsetDef* def, float frame, uint actionindex, CStr param)
{
	AnimHook ahook;
	if(!def->SetupAnimHook(ahook, frame, actionindex, param))
		return INDEX_NOT_FOUND;
	uint index;
	m_Hooks.Insert(ahook, &index);
	return index;
}

//-----------------------------------------------------------------------------
void AnimInfo::SetFPS(float fps)
{
	m_fFPS = fps;
}

//-----------------------------------------------------------------------------
float AnimInfo::GetFPS() const
{
	return m_fFPS;
}

//-----------------------------------------------------------------------------
void AnimInfo::SetBlendIn(float blendin)
{
	m_fBlendIn = blendin;
}

//-----------------------------------------------------------------------------
float AnimInfo::GetBlendIn() const
{
	return m_fBlendIn;
}

//-----------------------------------------------------------------------------
void AnimInfo::SetBlendOut(float blendout)
{
	m_fBlendOut = blendout;
}

//-----------------------------------------------------------------------------
float AnimInfo::GetBlendOut() const
{
	return m_fBlendOut;
}

//-----------------------------------------------------------------------------
void AnimInfo::RemoveAttribute(uint attrid)
{
	int index = m_Attributes.GetIndexOf(attrid);
	if(index != -1)
		m_Attributes.RemoveElement(index);
}

//-----------------------------------------------------------------------------
bool AnimInfo::IsAttributeSet(uint attrid) const
{
	return (m_Attributes.GetIndexOf(AnimAttrValue(attrid)) != -1);
}

//-----------------------------------------------------------------------------
CStr AnimInfo::GetAttributeValue(uint attrid) const
{
	int index = m_Attributes.GetIndexOf(attrid);
	if(index != -1)
		return m_Attributes[index].m_strValue.c_str();

	return NULL;
}

//-----------------------------------------------------------------------------
void AnimInfo::SetAttributeValue(uint attrid, CStr value)
{
	int index = m_Attributes.GetIndexOf(AnimAttrValue(attrid));
	if(index < 0)
		m_Attributes.Insert(AnimAttrValue(attrid, value.cstr()));
	else
		m_Attributes[index].m_strValue = value.cstr();
}

//-----------------------------------------------------------------------------
int ENF_CDECL _CompareHooks(const void *a, const void *b)
{
AnimHook *h1, *h2;

	h1 = (AnimHook*)a;
	h2 = (AnimHook*)b;

	return (int)(h1->m_fFrame - h2->m_fFrame);
}

//-----------------------------------------------------------------------------
void AnimInfo::SetAnimHooks(const Array<AnimHook>& animhooks)
{
	m_Hooks.Reserve(animhooks.GetCardinality());
	for(uint n = 0; n < animhooks.GetCardinality(); n++)
		m_Hooks.Insert(animhooks[n]);
	enf_assert(animhooks.GetCardinality() == m_Hooks.GetCardinality());
}
*/
//-----------------------------------------------------------------------------
bool BoneGroupDef::IsFull() const
{
	for(uint n = 0; n < Animation::MAX_CHANNELS; n++)
	{
		if(!IsSetBit(n))
			return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
BoneGroupDef::BoneGroupDef(): m_strName(NULL)
{
MemStats::m_iAnims += sizeof(*this);
}

//-----------------------------------------------------------------------------
BoneGroupDef::BoneGroupDef(CStr name)
{
m_strName = name.Static();
MemStats::m_iAnims += sizeof(*this);
}

//-----------------------------------------------------------------------------
BoneGroupDef::~BoneGroupDef()
{
MemStats::m_iAnims -= sizeof(*this);
}

/*
//-----------------------------------------------------------------------------
bool AnimationSystem::InsertBoneGroup(BoneGroupDef* def)
{
	if(!m_BoneGroups.Insert(def))
		return false;

	m_BoneGroupsHashList.Insert(def->GetName().cstr(), def);
	return true;
}

//-----------------------------------------------------------------------------
bool AnimationSystem::LoadBoneGroups(CStr name)
{
	TiXmlDocument doc;

	if(doc.LoadFile(name.cstr()))
	{
		for(TiXmlElement* elem = doc.RootElement()->FirstChildElement(); elem; elem = elem->NextSiblingElement())
		{
			if(CStr("bonegroup") == elem->Value())
			{
				CStr name = elem->Attribute("name");
				enf_assert(!name.IsEmpty());
				if(name.IsEmpty())
					continue;

				if(m_BoneGroupsHashList.Find(name.cstr()))
				{
					enf::LogF(LL_ERROR, "BoneGroup '%s' already exists", name.cstr());
					continue;
				}

				BoneGroupDef* def = ENF_NEW BoneGroupDef(name);

				for(TiXmlElement* bones = elem->FirstChildElement("bone"); bones; bones = bones->NextSiblingElement("bone"))
				{
					CStr name = bones->Attribute("name");
					if(!name.IsEmpty())
					{
						uint bindex = this->GetBoneExternIndex(name);
						if(bindex == INDEX_NOT_FOUND)
						{
							enf::LogF(LL_ERROR, "Bone '%s' in bonegroup '%s' is not defined", name.cstr(), def->GetName().cstr());
							continue;
						}
						if(bones->Attribute("weight").ToFloat() > 0.5f)
							def->SetBit(enf_min(bindex, Animation::MAX_CHANNELS));
					}
				}

				InsertBoneGroup(def);
			}
		}
	}
	return true;
}
*/
//-----------------------------------------------------------------------------
const BoneGroupDef* AnimationSystem::GetBoneGroup(CStr name) const
{
	//return m_BoneGroupsHashList.Find(name.cstr());
	return NULL;
}

//-----------------------------------------------------------------------------
AnimationSystem::AnimationSystem()
{
	//special builtin bonegroup with all weights = 1.0
/*
	BoneGroupDef* def = ENF_NEW BoneGroupDef("all");
	def->SetAll();
	InsertBoneGroup(def);
*/
	MemStats::m_iAnims += sizeof(*this);
}

//-----------------------------------------------------------------------------
AnimationSystem::~AnimationSystem()
{
//we cannot delete them, as they could be static
//	for(uint n = 0; n < m_ActionHandlers.GetCardinality(); n++)
//		delete m_ActionHandlers[n];

	ForEach<DeleteItem>(m_SkeletonDefs);
	m_SkeletonDefs.Clear();

	UnloadAnimsetDefinitions();
	UnloadBoneGroups();
	MemStats::m_iAnims -= sizeof(*this);
}

//-----------------------------------------------------------------------------
void AnimationSystem::UnloadAnimsetDefinitions()
{
//	ForEach<ReleaseItem>(m_AnimsetDefs);
//	m_AnimsetDefs.Clear();
//	m_AnimsetDefHashList.Clear();
}

/*
//-----------------------------------------------------------------------------
AnimsetDef* AnimationSystem::GetAnimsetDef(CStr name)
{
	AnimsetDef* def = m_AnimsetDefHashList.Find(name.cstr());
	if(def) def->AddRef();
	return def;
}
*/
//-----------------------------------------------------------------------------
void AnimationSystem::BoneDef::Cleanup()
{
	BoneRef* next;
	for(BoneRef* ref = Refs; ref; ref = next)
	{
		next = ref->Next;
		delete ref;
	}
	Refs = NULL;
}

//-----------------------------------------------------------------------------
AnimationSystem::BoneDef::~BoneDef()
{
	BoneRef* next;
	for(BoneRef* ref = Refs; ref; ref = next)
	{
		next = ref->Next;
		delete ref;
	}
}

//-----------------------------------------------------------------------------
void AnimationSystem::BoneDef::AddRef(SkeletonDef* def, uint boneindex)
{
	BoneRef* ref = ENF_NEW BoneRef;
	ref->Next = Refs;
	Refs = ref;
	ref->BoneIndex = boneindex;
	ref->Skeleton = def;
}

//-----------------------------------------------------------------------------
SkeletonDef::SkeletonDef(CStr name)
{
	m_strName = name.Static();
	memset(m_iBoneMapping, 0xff, sizeof(m_iBoneMapping));
	MemStats::m_iAnims += sizeof(*this);
}

//-----------------------------------------------------------------------------
SkeletonDef::~SkeletonDef()
{
	MemStats::m_iAnims -= sizeof(*this);
}

//-----------------------------------------------------------------------------
bool SkeletonDef::HasBone(CStr bname) const
{
	for(uint n = 0; n < m_Bones.GetCardinality(); n++)
	{
		if(bname.cmpi(m_Bones[n].Name) == 0)
			return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
uint SkeletonDef::AddBone(CStr bname)
{
	return m_Bones.Insert(SkeletonDef::Bone(bname.cstr(), INDEX_NOT_FOUND));
}

//-----------------------------------------------------------------------------
bool SkeletonDef::SetBoneExternIndex(uint boneindex, uint index)
{
	enf_assert(boneindex < MAX_BONES);
	enf_assert(index < MAX_BONES);
	
	if(m_iBoneMapping[index] != INDEX_NOT_FOUND)
		return false;

	m_Bones[boneindex].Index = index;
	m_iBoneMapping[index] = boneindex;
	return true;
}

//-----------------------------------------------------------------------------
uint AnimationSystem::GetBoneIndex(CStr bonename) const
{
	return m_BoneMapping.GetIndexOf(BoneDef(bonename));
}

//-----------------------------------------------------------------------------
uint AnimationSystem::GetBoneExternIndex(CStr bonename) const
{
	int b = m_BoneMapping.GetIndexOf(BoneDef(bonename));
	if(b == -1)
		return INDEX_NOT_FOUND;

	return m_BoneMapping[b].Index;
}

//-----------------------------------------------------------------------------
const char* AnimationSystem::GetBoneName(CStr bonename) const
{
	int b = m_BoneMapping.GetIndexOf(BoneDef(bonename));
	if(b == -1)
		return NULL;

	return m_BoneMapping[b].Name.cstr();
}

//-----------------------------------------------------------------------------
AnimationSystem::BoneDef* AnimationSystem::FindOrCreateBone(CStr bonename)
{
	uint b = m_BoneMapping.GetIndexOf(BoneDef(bonename));
	if(b == -1)
		m_BoneMapping.Insert(BoneDef(bonename), &b);

	return &m_BoneMapping[b];
}

//-----------------------------------------------------------------------------
bool AnimationSystem::LoadPivots(CStr name)
{
	TiXmlDocument doc;

	doc.RegisterTag("pivots");
	doc.RegisterTag("pivot");
	doc.RegisterTag("name");
	doc.RegisterTag("index");

	if(!doc.LoadFile(name.cstr()))
	{
		enf::LogF(LL_ERROR, "Can't load %s", name.cstr());
		if(doc.Error())
		{
			enf::LogF(LL_ERROR, "%s, line %d, column %d", doc.ErrorDesc(), doc.ErrorRow(), doc.ErrorCol());
		}
		return false;
	}

	for(TiXmlElement* elem = doc.RootElement()->FirstChildElement("pivot"); elem; elem = elem->NextSiblingElement("pivot"))
	{
		CStr name = elem->Attribute("name");
		if(name.IsEmpty())
			continue;

		CStr sindex = elem->Attribute("index");

		int index = sindex.IsEmpty() ? 0 : sindex.ToInt();
		if(index < 0 || index >= 256)
		{
			enf::LogF(LL_ERROR, "Pivot's '%s' index (%d) is out of range", name.cstr(), index);
			continue;
		}

		m_Pivots.Insert(MakePair(name.Static(), (uint)index));

	}
	return true;
}

//-----------------------------------------------------------------------------
uint AnimationSystem::GetPivotIndex(CStr pivotname) const
{
	int index = m_Pivots.GetIndexOf(MakePair(pivotname, uint(0)));
	if(index != -1)
		return m_Pivots[index].Data;

	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------------
bool AnimationSystem::LoadSkeletons(CStr name)
{
	TiXmlDocument doc;

	doc.RegisterTag("skeleton");
	doc.RegisterTag("bone");
	doc.RegisterTag("name");
	doc.RegisterTag("index");

	if(!doc.LoadFile(name.cstr()))
	{
		enf::LogF(LL_ERROR, "Can't load %s", name.cstr());
		if(doc.Error())
		{
			enf::LogF(LL_ERROR, "%s, line %d, column %d", doc.ErrorDesc(), doc.ErrorRow(), doc.ErrorCol());
		}
		return false;
	}

	for(TiXmlElement* elem = doc.RootElement()->FirstChildElement("skeleton"); elem; elem = elem->NextSiblingElement("skeleton"))
	{
		CStr name = elem->Attribute("name");
		if(name.IsEmpty())
			continue;

		SkeletonDef* sdef = ENF_NEW SkeletonDef(name);
		
		TiXmlElement* bones[MAX_BONES];
		uint numbones = 0;

		for(TiXmlElement* bone = elem->FirstChildElement("bone"); bone; bone = bone->NextSiblingElement("bone"))
			bones[numbones++] = bone;

		sdef->m_Bones.Reserve(numbones);

		for(uint ib = 0; ib < numbones; ib++)
		{
			CStr bname = bones[ib]->Attribute("name");

			if(bname.IsEmpty())
				continue;

			CStr pivotname = bones[ib]->Attribute("pivot");

			SString<64> bnamelow(bname);

			bname = CStr(bnamelow.lower()).Static();

			if(sdef->HasBone(bname))
			{
				enf::LogF(LL_ERROR, "Skeleton %s has duplicite bone %s", name.cstr(), bname.cstr());
				continue;
			}

			BoneDef* bdef = FindOrCreateBone(bname);

			if(!pivotname.IsEmpty())
			{
/*				const Variable* pvar = g_pScript->GetVariable(pivotname.cstr());
				if(pvar)
				{
					if(pvar->GetVType() == VARTYPE_INT)
						bdef->PivotIndex = pvar->Int;
				}
				else
				{
					uint index = GetPivotIndex(pivotname);
					if(index == INDEX_NOT_FOUND)
						bdef->PivotIndex = pivotname.ToInt();
					else
						bdef->PivotIndex = index;

				}
*/
			}

			CStr indexstr = bones[ib]->Attribute("index");
			if(!indexstr.IsNull())
			{
				uint index = indexstr.ToInt();

				if(bdef->Index != INDEX_NOT_FOUND)
				{
					if(bdef->Index != index)
					{
						enf::LogF(LL_ERROR, "Skeleton '%s' is trying to map bone '%s' to index '%d', but it was already mapped to '%d'",
							name.cstr(), bname.cstr(), index, bdef->Index);
						continue;
					}
				}
				else
				{
					//we have defined index, set it into global mapping
					bdef->Index = index;
				}
			}

			uint iindex = sdef->AddBone(bname);
			bdef->AddRef(sdef, iindex);
		}
		m_SkeletonDefs.Insert(sdef);
	}

	//1st pass. Set hardcoded indices
	for(uint b = 0; b < m_BoneMapping.GetCardinality(); b++)
	{
		uint index = m_BoneMapping[b].Index;
		if(index == INDEX_NOT_FOUND)
			continue;
		//set the index into all SkeletonDef
		for(BoneRef* ref = m_BoneMapping[b].Refs; ref; ref = ref->Next)
		{
			if(ref->Skeleton->m_iBoneMapping[index] != INDEX_NOT_FOUND)
			{
				enf::LogF(LL_ERROR, "Skeleton '%s' is trying to map bone '%s' to index '%d', but it was already used by '%s'",
					ref->Skeleton->m_strName.cstr(),
					m_BoneMapping[b].Name.cstr(),
					index,
					ref->Skeleton->m_Bones[ref->Skeleton->m_iBoneMapping[index]].Name);
				continue;
			}
			ref->Skeleton->m_Bones[ref->BoneIndex].Index = index;
			ref->Skeleton->m_iBoneMapping[index] = ref->BoneIndex;
		}
	}

	//2nd pass. Find free indices for the rest
	for(uint b = 0; b < m_BoneMapping.GetCardinality(); b++)
	{
		if(m_BoneMapping[b].Index != INDEX_NOT_FOUND)
			continue;

		//pick the first free index
		uint index;
		for(index = 0; index < MAX_BONES; index++)
		{
			BoneRef* ref;
			for(ref = m_BoneMapping[b].Refs; ref; ref = ref->Next)
			{
				//occupied. Get out!
				if(ref->Skeleton->m_iBoneMapping[index] != INDEX_NOT_FOUND)
					break;
			}

			//it's free
			if(ref == NULL)
				break;
		}

		if(index == MAX_BONES)
		{
			enf::LogF(LL_ERROR, "Cannot found free index for bone '%s'", m_BoneMapping[b].Name.cstr());
			continue;
		}

		//set global mapping index
		m_BoneMapping[b].Index = index;

		//set the index into all SkeletonDef
		for(BoneRef* ref = m_BoneMapping[b].Refs; ref; ref = ref->Next)
		{
			enf_assert(ref->Skeleton->m_iBoneMapping[index] == INDEX_NOT_FOUND);
			ref->Skeleton->m_Bones[ref->BoneIndex].Index = index;
			ref->Skeleton->m_iBoneMapping[index] = ref->BoneIndex;
		}
	}

	//every index should be set. Copy them into script variables
/*	const Array<Variable*>& vars = g_pScript->GetGlobalVariables();
	for(uint v = 0; v < vars.GetCardinality(); v++)
	{
		const char* name = vars[v]->GetName();

		if(vars[v]->GetVType() != VARTYPE_INT ||
			name[0] != 'B' || name[1] != '_')
			continue;

		SString<64> bnamelow(name + 2);

		name = g_pStatic->FindString(bnamelow.lower());
		
		if(name == NULL)
			continue;

		uint bindex = GetBoneExternIndex(name);

		if(bindex == INDEX_NOT_FOUND)
			continue;
		
		if(vars[v]->Spec & VSPEC_CONST)
		{
			enf::LogF(LL_ERROR, "Cannot set bone index variable '%s'", vars[v]->GetName());
		}
		vars[v]->Int = bindex;
	}
	*/
	return true;
}

//-----------------------------------------------------------------------------
bool AnimationSystem::LoadAnimsetDefinitions(CStr name)
{
/*
	TiXmlDocument doc;

	if(doc.LoadFile(name.cstr()))
	{
		for(TiXmlElement* elem = doc.RootElement()->FirstChildElement(); elem; elem = elem->NextSiblingElement())
		{
			if(CStr("setdef") == elem->Value())
			{
				CStr parentname = elem->Attribute("parent");
				AnimsetDef* parent = NULL;
				if(!parentname.IsEmpty())
				{
					parent = GetAnimsetDef(parentname);
				}
				AnimsetDef* def = ENF_NEW AnimsetDef(elem->Attribute("name"), parent);

				def->Load(elem);

				m_AnimsetDefs.Insert(def);
				m_AnimsetDefHashList.Insert(def->GetName().cstr(), def);

				SAFE_RELEASE(parent);
			}
		}
	}
*/
	return true;
}

//-----------------------------------------------------------------------------
void AnimationSystem::AddActionHandler(AnimActionHandler* handler)
{
	m_ActionHandlers.Insert(handler);
}

//-----------------------------------------------------------------------------
void AnimationSystem::RemoveActionHandler(AnimActionHandler* handler)
{
	m_ActionHandlers.Remove(handler);
}

//-----------------------------------------------------------------------------
void AnimationSystem::LinkActions(Entity* ent, const AnimInfo* animinfo, uint* links)
{
	for(uint n = 0; n < m_ActionHandlers.GetCardinality(); n++)
	{
		m_ActionHandlers[n]->LinkActions(ent, animinfo, links);
	}
}

//-----------------------------------------------------------------------------
void AnimationSystem::SetActionParam(const AnimAction* action, AnimHook& hook)
{
	for(uint n = 0; n < m_ActionHandlers.GetCardinality(); n++)
	{
		if(m_ActionHandlers[n]->SetActionParam(action, hook))
			break;
	}
}

//-----------------------------------------------------------------------------
void AnimationSystem::HandleAction(Entity* ent, uint link, const AnimHook& hook)
{
	for(uint n = 0; n < m_ActionHandlers.GetCardinality(); n++)
	{
		if(m_ActionHandlers[n]->HandleAction(ent, link, hook))
			break;
	}
}

//-----------------------------------------------------------------------------
void AnimationSystem::UnloadBoneGroups()
{
	ForEach<DeleteItem>(m_BoneGroups);
	m_BoneGroups.Clear();
	m_BoneGroupsHashList.Clear();
}

}