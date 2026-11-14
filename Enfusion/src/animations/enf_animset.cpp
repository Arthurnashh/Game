//-----------------------------------------------------------------------------
// File: enf_animset.cpp
//
// Desc: Animation set class
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "animations/enf_animset.h"
#include "animations/enf_animation.h"
#include "tinyxml/tinyxml.h"
#include "animations/enf_animationsystem.h"
#include "resource/enf_resourcemanager.h"
#include "common/enf_staticbuffer.h"

namespace enf
{

//-----------------------------------------------------------------------------
AnimRef::AnimRef(CStr name, Animation* anim):
	m_pAnim(anim)
{
#ifdef ENF_DEVELOPER
	m_strAnimName = name;
#endif
}

//-----------------------------------------------------------------------------
AnimsetDef::~AnimsetDef()
{
}

//-----------------------------------------------------------------------------
bool AnimsetDef::SetupAnimHook(AnimHook& animhook, float frame, uint actionindex, CStr param) const
{
	animhook.m_fFrame = frame;
	animhook.m_strAction = m_AnimActions[actionindex].m_strAction;
	animhook.m_strParam = param.Static();
	animhook.m_pAction = NULL;
	animhook.m_pParam = NULL;
	GetAnimationSystem()->SetActionParam(&m_AnimActions[actionindex], animhook);
	return true;
}

//-----------------------------------------------------------------------------
const uint AnimsetDef::GetActionIndex(CStr action) const
{
	return m_AnimActions.GetIndexOf(AnimAction(action));
}

//-----------------------------------------------------------------------------
AnimsetDef::AnimsetDef(CStr defname, AnimsetDef* parent)
{
	m_strName = defname.Static();

	if(parent)
	{
		//TODO: Set should know to copy himself. Check it!
		for(uint a = 0; a < parent->m_AnimActions.GetCardinality(); a++)
			m_AnimActions.Insert(parent->m_AnimActions[a]);

		for(uint a = 0; a < parent->m_AnimAttributes.GetCardinality(); a++)
			m_AnimAttributes.Insert(parent->m_AnimAttributes[a]);

		for(uint a = 0; a < parent->m_Defs.GetCardinality(); a++)
			m_Defs.Insert(parent->m_Defs[a]);
	}
}

//-----------------------------------------------------------------------------
VType GetVarType(CStr type)
{
	VType vtype = VARTYPE_NONE;

	if(type == "bool")
		vtype = VARTYPE_BOOL;
	else if(type == "string")
		vtype = VARTYPE_STRING;
	else if(type == "int")
		vtype = VARTYPE_INT;
	else if(type == "float")
		vtype = VARTYPE_FLOAT;
	else if(type == "vector")
		vtype = VARTYPE_VECTOR;

	return vtype;
}

//-----------------------------------------------------------------------------
bool AnimsetDef::Load(TiXmlNode* node)
{
	SArray<TiXmlElement*, 256> elems;

	for(TiXmlElement* el = node->FirstChildElement(); el; el = el->NextSiblingElement())
	{
		if(CStr("attributes") == el->Value())
		{
			for(TiXmlElement* attr = el->FirstChildElement("attribute"); attr; attr = attr->NextSiblingElement("attribute"))
			{
				CStr name = attr->Attribute("name");

				if(name.IsEmpty())
					continue;

				name = name.Static();

				VType vtype = GetVarType(attr->Attribute("type"));

				m_AnimAttributes.Insert(AnimAttr(name, vtype));
			}
			continue;
		}

		if(CStr("actions") == el->Value())
		{
			for(TiXmlElement* action = el->FirstChildElement("action"); action; action = action->NextSiblingElement("action"))
			{
				CStr name = action->Attribute("name");

				if(name.IsEmpty())
					continue;

				name = name.Static();

				CStr handler = action->Attribute("handler").Static();

				VType vt = GetVarType(action->Attribute("param"));

				m_AnimActions.Insert(AnimAction(name, handler, vt));
			}
			continue;
		}

		if(!elems.IsFull() && CStr("anm") == el->Value() && !el->Attribute("id").IsEmpty())
			elems.Insert(el);
	}

	m_Defs.Reserve(elems.GetCardinality());

	for(uint n = 0; n < elems.GetCardinality(); n++)
	{
		CStr id = elems[n]->Attribute("id");
		const char* desc = NULL;

		for(TiXmlNode* comment = elems[n]->FirstChild(); comment; comment->NextSibling())
		{
			if(comment->Type() == TiXmlNode::COMMENT)
			{
				desc = g_pStatic->AddString(comment->Value());
				break;
			}
		}
		m_Defs.Insert(Def(id.Static(), desc));
	}
	return !elems.IsEmpty();
}

//-----------------------------------------------------------------------------
Animset::Animset(AnimsetDef* def, Animset* parent)
{
	enf_assert(def);
   m_pDef = def;
	def->AddRef();
	m_pParent = parent;
	m_Anims.SetSize(def->m_Defs.GetCardinality());
}

//-----------------------------------------------------------------------------
uint Animset::GetIDIndex(const char* id) const
{
	return m_pDef->GetIndex(id);
}

//-----------------------------------------------------------------------------
uint Animset::GetNumAnims(const char* id) const
{
	uint index = m_pDef->GetIndex(id);

	if(index != INDEX_NOT_FOUND)
	{
		if(index >= m_Anims.GetCardinality())
			return 0;

		return m_Anims[index].GetCardinality();
	}

	//you shouldn't ask with invalid idindex - it's all your fault!
	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------------
uint Animset::GetNumAnims(uint idindex) const
{
	if(idindex >= m_Anims.GetCardinality())
		return 0;

	return m_Anims[idindex].GetCardinality();
}

//-----------------------------------------------------------------------------
void Animset::AddAnimation(uint idindex, AnimRef* ref)
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());
	m_Anims[idindex].Insert(ref);
}

//-----------------------------------------------------------------------------
void Animset::SetAnimation(uint idindex, uint index, CStr animname, Animation* anm)
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());
	AnimRef* ref = m_Anims[idindex].GetElement(index);

	if(ref)
	{
#ifdef ENF_DEVELOPER
		ref->m_strAnimName = animname.cstr();
#endif
		ref->m_pAnim = anm;
	}
}

//-----------------------------------------------------------------------------
void Animset::RemoveAnimation(uint idindex, uint index)
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());
	AnimRef* ref = m_Anims[idindex].GetElement(index);

	if(ref)
	{
		m_Anims[idindex].Remove(ref);
		delete ref;
	}
}

//-----------------------------------------------------------------------------
void Animset::Init()
{
	m_Anims.Clear();
	m_pParent = NULL;
	m_pDef = NULL;
}

//-----------------------------------------------------------------------------
void Animset::Unload()
{
	for(uint n = 0; n < m_Anims.GetCardinality(); n++)
	{
		for(ListIterator<AnimRef> iter(m_Anims[n]); iter.HasMoreElements(); )
		{
			AnimRef* ref = iter.GetNext();
			m_Anims[n].Remove(ref);
			//TODO: this should force release of anims!
			delete ref;
		}
	}

	m_Anims.Clear();
	SAFE_RELEASE(m_pParent);
	SAFE_RELEASE(m_pDef);
}

//-----------------------------------------------------------------------------
const AnimRef* Animset::GetAnimation(const char* id, uint index) const
{
	uint idindex = this->m_pDef->GetIndex(id);

	if(idindex == INDEX_NOT_FOUND)
		return NULL;

	if(m_Anims[idindex].IsEmpty())
		return NULL;

	return m_Anims[idindex].GetElement(index);
}

//-----------------------------------------------------------------------------
const AnimRef* Animset::GetAnimation(uint idindex, uint index) const
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());

	if(m_Anims[idindex].IsEmpty())
		return NULL;

	return m_Anims[idindex].GetElement(index);
}

//-----------------------------------------------------------------------------
AnimRef* Animset::GetAnimation(const char* id, uint index)
{
	uint idindex = this->m_pDef->GetIndex(id);

	if(idindex == INDEX_NOT_FOUND)
		return NULL;

	if(m_Anims[idindex].IsEmpty())
		return NULL;

	return m_Anims[idindex].GetElement(index);
}

//-----------------------------------------------------------------------------
AnimRef* Animset::GetAnimation(uint idindex, uint index)
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());

	if(m_Anims[idindex].IsEmpty())
		return NULL;

	return m_Anims[idindex].GetElement(index);
}

//-----------------------------------------------------------------------------
const TAnimRefList* Animset::GetAnimations(const char* id) const
{
	uint idindex = this->m_pDef->GetIndex(id);

	if(idindex == INDEX_NOT_FOUND)
		return NULL;

	return &m_Anims[idindex];
}

//-----------------------------------------------------------------------------
const TAnimRefList* Animset::GetAnimations(uint idindex) const
{
	enf_assert(idindex < m_pDef->m_Defs.GetCardinality());
	return &m_Anims[idindex];
}

//-----------------------------------------------------------------------------
bool Animset::Save()
{
#ifdef ENF_DEVELOPER
	TiXmlDocument doc(GetName().cstr());

	TiXmlElement* root = doc.InsertEndChild(TiXmlElement("animset"))->ToElement();
	root->SetAttribute("version", "1.0");
	root->SetAttribute("name", "SecurityBot");
	root->SetAttribute("def", m_pDef->GetName().cstr());
	if(m_pParent)
		root->SetAttribute("parent", m_pParent->GetName().cstr());
	
	static const TiXmlElement AnimsElm("anims");
	static const TiXmlElement AnimationElm("anim");
	static const TiXmlElement AtributeElm("attr");
	static const TiXmlElement HookElm("hook");

	const AnimationSystem* as = GetEngine()->GetAnimationSystem();

	const Set<AnimsetDef::Def>& defs = m_pDef->GetDefs();

	for(uint n = 0; n < m_Anims.GetCardinality(); n++)
	{
		if(m_Anims[n].IsEmpty())
			continue;

		TiXmlElement* TempElm = root->InsertEndChild(AnimsElm)->ToElement();
		TempElm->SetAttribute("id", defs[n].m_strID.cstr());

		for(ListIterator<AnimRef> iter(m_Anims[n]); iter.HasMoreElements(); )
		{
			AnimRef* ref = iter.GetNext();

			TiXmlElement* AnimElm = TempElm->InsertEndChild(AnimationElm)->ToElement();
			AnimElm->SetAttribute("name", ref->GetAnimationName().cstr());
			if(ref->GetFPS() != -1)
				AnimElm->SetDoubleAttribute("fps", ref->GetFPS());
			if(ref->GetBlendIn() != -1)
				AnimElm->SetDoubleAttribute("blendin", ref->GetBlendIn());
			if(ref->GetBlendOut() != -1)
				AnimElm->SetDoubleAttribute("blendout", ref->GetBlendOut());

			const Set<AnimAttrValue>& attr = ref->GetAttributes();
			for(uint a = 0; a < attr.GetCardinality(); a++)
			{
				TiXmlElement* AttElm = AnimElm->InsertEndChild(AtributeElm)->ToElement();
				AttElm->SetAttribute("name", m_pDef->GetAttributeName(attr[a].m_iAttrID).cstr());
				AttElm->SetAttribute("value", attr[a].m_strValue.c_str());
			}

			for(uint n = 0; n < ref->GetAnimHooks().GetCardinality(); n++)
			{
				AnimHook& hook = ref->GetAnimHooks()[n];

				TiXmlElement* el = AnimElm->InsertEndChild(HookElm)->ToElement();
				el->SetAttribute("frame", (int)hook.m_fFrame);
				el->SetAttribute("action", hook.m_strAction.cstr());
				el->SetAttribute("param", hook.m_strParam.cstr());
			}
		}
	}

	return doc.SaveFile(this->GetName().cstr());
#else
	return true;
#endif
}

//-----------------------------------------------------------------------------
bool Animset::Load(CInputStream* instream)
{
	return Load(instream, false);
}

//-----------------------------------------------------------------------------
size_t Animset::GetSize() const
{
//FIXME:this is not correct, as it doesn't solve duplicities!
	size_t s = 0;
	for(uint n = 0; n < m_Anims.GetCardinality(); n++)
	{
		for(ListIterator<AnimRef> iter(m_Anims[n]); iter.HasMoreElements(); )
		{
			const AnimRef* ref = iter.GetNext();
			if(ref->m_pAnim)
				s += ref->m_pAnim->GetSize();
		}
	}
	return s;
}

//-----------------------------------------------------------------------------
bool Animset::Load(CInputStream* instream, bool preload)
{
	TiXmlDocument doc;

	enf_assert2(GetAnimationSystem(), "AnimationSystem must be inserted into Engine");

	if(!doc.LoadFile(*instream))
		return false;

	CStr defname = doc.RootElement()->Attribute("def");
	if(defname.IsEmpty())
		return false;

	m_pDef = GetAnimationSystem()->GetAnimsetDef(defname);

	if(m_pDef == NULL)
		return false;

	bool res = true;
	m_pDef.AddRef();

	CStr parentname = doc.RootElement()->Attribute("parent");
	if(!parentname.IsEmpty())
	{
		Animset* aset = Animset::Cast(GetResources()->GetVObject(parentname));
		if(aset == NULL)
		{
			enf::LogF(LL_ERROR, "Animset '%s' cannot found parent animset '%s'",
				GetName().cstr(),
				parentname.cstr());
		}
		else
		{
			enf_assert(aset->HasType(AnimationTypeID));
		}
		m_pParent = aset;
	}

	m_Anims.SetSize(m_pDef->m_Defs.GetCardinality());

	for(TiXmlElement* elem = doc.RootElement()->FirstChildElement("anims"); elem; elem = elem->NextSiblingElement("anims"))
	{
		CStr idname = elem->Attribute("id");
		if(idname.IsEmpty())
			continue;

		uint index = m_pDef->GetIndex(idname);

		if(index == INDEX_NOT_FOUND)
			continue;

		for(TiXmlElement* elem2 = elem->FirstChildElement("anim"); elem2; elem2 = elem2->NextSiblingElement("anim"))
		{
			CStr anmname = elem2->Attribute("name");

			if(anmname.IsEmpty())
				continue;
			Animation* anm = Animation::Cast(GetResources()->GetVObject(anmname, preload ? VLF_PRELOAD : 0));

			CStr fps = elem2->Attribute("fps");

			AnimRef* ref = ENF_NEW AnimRef(anmname, anm);
			m_Anims[index].Insert(ref);

			ref->SetFPS(fps.ToFloat(-1));

			CStr blendin = elem2->Attribute("blendin");
			ref->SetBlendIn(blendin.ToFloat(-1));

			CStr blendout = elem2->Attribute("blendout");
			ref->SetBlendOut(blendout.ToFloat(-1));

			for(TiXmlElement* attr = elem2->FirstChildElement("attr"); attr; attr = attr->NextSiblingElement("attr"))
			{
				CStr name = attr->Attribute("name");
				enf_assert(!name.IsEmpty());
				uint attrid = m_pDef->GetAttributeIndex(name);
				if(attrid == INDEX_NOT_FOUND)
				{
					enf::LogF(LL_ERROR, "Animset '%s' has undefined attribute '%s' for anim-ID '%s'",
						GetName().cstr(),
						name.cstr(),
						idname.cstr());
					continue;
				}
				ref->SetAttributeValue(attrid, attr->Attribute("value"));
			}


			for(TiXmlElement* hook = elem2->FirstChildElement("hook"); hook; hook = hook->NextSiblingElement("hook"))
			{
				CStr frame = hook->Attribute("frame");
				CStr action = hook->Attribute("action");
				CStr parm = hook->Attribute("param");

				enf_assert(!frame.IsNull());
				enf_assert(!action.IsNull());

				uint actionindex = m_pDef->GetActionIndex(action);

				if(actionindex == INDEX_NOT_FOUND)
				{
					enf::LogF(LL_ERROR, "Animset '%s' has undefined action '%s' for anim-ID '%s'",
						GetName().cstr(),
						action.cstr(),
						idname.cstr());
					continue;
				}

				AnimHook ah;

				m_pDef->SetupAnimHook(ah, frame.ToFloat(), actionindex, parm);
				ref->m_Hooks.Insert(ah);
			}
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
class PUBLIC_API LoadAnimsetResource: public LoadResource
{
protected:
	Animset m_Animset;

public:
	ENF_DECLARE_ITEM(LoadAnimsetResource, LoadResource);
	ENF_DECLARE_SMALLOBJECT(LoadAnimsetResource);

	size_t GetDataSize() const
	{
		return m_Animset.GetSize();
	}

	ENF_INLINE Animset* GetAnimset() const
	{
		return static_cast<Animset*>(LoadResource::m_pResource);
	}

	LoadAnimsetResource(Animset* obj): LoadResource(obj)
	{
		obj->AddRef();
	}
	~LoadAnimsetResource()
	{
		m_pResource->FinishLoading(this);
		GetAnimset()->Release();
		m_Animset.Unload();
	}
	bool DoLoad()
	{
		CFileInputStream instream(GetAnimset()->GetFileHandle());
		if(!instream.IsOpen())
			return false;

		return m_Animset.Load(&instream, true);
	}
	void Apply()
	{
		Animset* obj = GetAnimset();

		obj->Animset::Swap(m_Animset);
	}

	CStr GetName() const
	{
		return GetAnimset()->GetName();
	}
};

//-----------------------------------------------------------------------------
void Animset::Swap(Animset& other)
{
	m_Anims.Swap(other.m_Anims);
	enf_swap(m_pParent, other.m_pParent);
	enf_swap(m_pDef, other.m_pDef);
}

//-----------------------------------------------------------------------------
LoadResource* Animset::CreateResourceLoader()
{
	return ENF_NEW LoadAnimsetResource(this);
}


}