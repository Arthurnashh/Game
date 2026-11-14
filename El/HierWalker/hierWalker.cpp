#include <El/elementpch.hpp>
#include "hierWalker.hpp"
#include <El/ParamFile/paramFile.hpp>
#include <El/ParamArchive/paramArchive.hpp>

// Attribute types

IAttribute *AttributeTypes::CreateAttribute(RString type) const
{
	int i = attributes.GetValue(type);
	if (i < 0) return NULL;
	return createFunctions[i]();
}

// Attribute list

LSError SerializeAttribute(RString name, IAttribute *&value, ParamArchive &ar, const AttributeTypes &types)
{
	if (ar.IsSaving() || ar.GetPass() == ParamArchive::PassSecond)
	{
		if (value)
		{
			ParamArchive arSubcls;
			ar.OpenSubclass(name, arSubcls);
			LSError err = value->Serialize(arSubcls);
			ar.CloseSubclass(arSubcls);
			return err;
		}
	}
	else
	{
		value = NULL;
		ParamArchive arSubcls;
		if (ar.OpenSubclass(name, arSubcls))
		{
			value = types.CreateAttribute(name);
			LSError err = LSStructure;
			if (value) err = value->Serialize(arSubcls);
			ar.CloseSubclass(arSubcls);
			return err;
		}
	}
	return LSOK;
}

void AttributeList::Update(const AttributeList &src)
{
	int n = src.Size();
	if (Size() < n) Resize(n);
	for (int i=0; i<n; i++)
		if (src[i]) Set(i) = src[i];
}

void AttributeList::SetAttribute(RString name, RString value, const AttributeTypes &types)
{
	int i = types.attributes.GetValue(name);
	if (i >= 0)
	{
		IAttribute *attr = types.createFunctions[i]();
		attr->Load(value);
		Access(i);
		Set(i) = attr;
	}
	else
	{
		RptF("Unknown attribute %s", (const char *)name);
	}
}

IAttribute *AttributeList::AccessAttribute(int index, const AttributeTypes &types)
{
	Access(index);
	IAttribute *attr = Set(index);
	if (!attr)
	{
		attr = types.createFunctions[index]();
		Set(index) = attr;
	}
	return attr;
}

IAttribute *AttributeList::AccessAttribute(RString name, const AttributeTypes &types)
{
	int i = types.attributes.GetValue(name);
	if (i >= 0) return AccessAttribute(i, types);
	RptF("Unknown attribute %s", (const char *)name);
	return NULL;
}

LSError AttributeList::Serialize(ParamArchive &ar, const AttributeTypes &types)
{
	int n;
	if (ar.IsSaving() || ar.GetPass() == ParamArchive::PassSecond)
	{
		n = Size();
		CHECK(ar.Serialize("count", n, 1, 0))
	}
	else
	{
		CHECK(ar.Serialize("count", n, 1, 0))
		Resize(n);
	}
	
	for (int i=0; i<n; i++)
	{
		RString name = types.attributes.GetName(i);
		IAttribute *attribute = Get(i);
		CHECK(SerializeAttribute(name, attribute, ar, types))
		Set(i) = attribute;
	}
	return LSOK;
}

void SetAttributes(AttributeList &attributes, const ParamEntry &cls, const AttributeTypes &types)
{
	for (int i=0; i<types.attributes.FirstInvalidValue(); i++)
	{
		ConstParamEntryPtr entry = cls.FindEntry(types.attributes.GetName(i));
		if (entry) 
		{
			IAttribute *attr = types.createFunctions[i]();
			attr->Load(*entry);
			attributes.Access(i);
			attributes[i] = attr;
		}
	}
}

// Attribute manager

AttributeManager::AttributeManager(AttributeList &def)
{
	_stack.Add(def);
}

void AttributeManager::Push()
{
	int n = _stack.Size();
	if (n > 0) _stack.Add(_stack[n - 1]);
	else _stack.Add();
}

void AttributeManager::Pop()
{
	int n = _stack.Size();
	if (n > 0) _stack.Delete(n - 1);
}

void AttributeManager::SetAttributes(const AttributeList &src)
{
	int n = _stack.Size();
	if (n > 0)
		_stack[n - 1].Update(src);
}

const IAttribute *AttributeManager::GetAttribute(int index) const
{
	int n = _stack.Size();
	if (n > 0)
	{
		const AttributeList &list = _stack[n - 1];
		if (index < list.Size()) return list[index];
	}
	return NULL;
}

const AttributeList &AttributeManager::GetAttributeList() const
{
	int n = _stack.Size();
	DoAssert(n > 0);
	return _stack[n - 1];
}

// common attribute types

// Bool attribute
class AttrBool : public IAttribute
{
protected:
	bool _value;

public:
	void Load(RString value);
	bool GetValue() const {return _value;}
	void SetValue(bool value) {_value = value;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

void AttrBool::Load(RString value)
{
	if (stricmp(value, "true") == 0 || strcmp(value, "1") == 0) _value = true;
	else
	{
		DoAssert(stricmp(value, "false") == 0 || strcmp(value, "0") == 0);
		_value = false;
	}
}

LSError AttrBool::Serialize(ParamArchive &ar)
{
	CHECK(ar.Serialize("value", _value, 1));
	return LSOK;
}

bool AttrBool::IsEqualTo(const IAttribute *with) const
{
	const AttrBool *w = static_cast<const AttrBool *>(with);
	return GetValue() == w->GetValue();
}

IAttribute *CreateAttrBool() {return new AttrBool();}
bool GetAttrBool(const IAttribute *attr) {return static_cast<const AttrBool *>(attr)->GetValue();}
void SetAttrBool(IAttribute *attr, bool value) {static_cast<AttrBool *>(attr)->SetValue(value);}


// Float attribute
class AttrFloat : public IAttribute
{
protected:
	float _value;

public:
	void Load(RString value);
	float GetValue() const {return _value;}
	void SetValue(float value) {_value = value;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

void AttrFloat::Load(RString value)
{
	_value = atof(value);
}

LSError AttrFloat::Serialize(ParamArchive &ar)
{
	CHECK(ar.Serialize("value", _value, 1));
	return LSOK;
}

bool AttrFloat::IsEqualTo(const IAttribute *with) const
{
	const AttrFloat *w = static_cast<const AttrFloat *>(with);
	return GetValue() == w->GetValue();
}

IAttribute *CreateAttrFloat() {return new AttrFloat();}
float GetAttrFloat(const IAttribute *attr) {return static_cast<const AttrFloat *>(attr)->GetValue();}
void SetAttrFloat(IAttribute *attr, float value) {static_cast<AttrFloat *>(attr)->SetValue(value);}

// String attribute
class AttrString : public IAttribute
{
protected:
	RString _value;

public:
	void Load(RString value);
	RString GetValue() const {return _value;}
	void SetValue(RString value) {_value = value;}

	LSError Serialize(ParamArchive &ar);
	bool IsEqualTo(const IAttribute *with) const;
};

void AttrString::Load(RString value)
{
	_value = value;
}

LSError AttrString::Serialize(ParamArchive &ar)
{
	CHECK(ar.Serialize("value", _value, 1));
	return LSOK;
}

bool AttrString::IsEqualTo(const IAttribute *with) const
{
	const AttrString *w = static_cast<const AttrString *>(with);
	return GetValue() == w->GetValue();
}

IAttribute *CreateAttrString() {return new AttrString();}
RString GetAttrString(const IAttribute *attr) {return static_cast<const AttrString *>(attr)->GetValue();}
void SetAttrString(IAttribute *attr, RString value) {static_cast<AttrString *>(attr)->SetValue(value);}

// Node types

INode *NodeTypes::CreateNode(RString type) const
{
	int i = nodes.GetValue(type);
	if (i < 0) return NULL;
	return createFunctions[i]();
}

// Common nodes

LSError SerializeNode(RString name, INode *&value, ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes)
{
	if (ar.IsSaving() || ar.GetPass() == ParamArchive::PassSecond)
	{
		if (value)
		{
			ParamArchive arSubcls;
			ar.OpenSubclass(name, arSubcls);
			RString type = value->GetType();
			LSError err = arSubcls.Serialize("type", type, 1);
			if (err == LSOK) err = value->Serialize(arSubcls, types, attrTypes);
			ar.CloseSubclass(arSubcls);
			return err;
		}
	}
	else
	{
		value = NULL;
		ParamArchive arSubcls;
		if (ar.OpenSubclass(name, arSubcls))
		{
			RString type;
			CHECK(arSubcls.Serialize("type", type, 1))
			value = types.CreateNode(type);
			LSError err = LSStructure;
			if (value) err = value->Serialize(arSubcls, types, attrTypes);
			ar.CloseSubclass(arSubcls);
			return err;
		}
	}
	return LSOK;
}

bool INode::PerformProcess(AttributeManager &manager, void *context)
{
  manager.Push();
  manager.SetAttributes(_attributes);
  bool finish = Process(manager, context);
	manager.Pop();
	return finish;
}

LSError INode::Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes)
{
	ParamArchive arSubcls;
	if (!ar.OpenSubclass("Attributes", arSubcls))
	{
		LSError err = arSubcls.ErrorNoEntry();
		arSubcls.OnError(err, "Attributes");
		return err;
	}
	LSError err = _attributes.Serialize(arSubcls, attrTypes);
	ar.CloseSubclass(arSubcls);
	return err;
}

bool INode::IsEqualTo(const INode *with) const
{
	if (strcmp(GetType(), with->GetType()) != 0) return false;

	int m = GetAttributes().Size();
	int n = with->GetAttributes().Size();

	if (m > n)
	{
		for (int i=n; i<m; i++) if (GetAttributes()[i]) return false;
	}
	else if (m < n)
	{
		for (int i=m; i<n; i++) if (with->GetAttributes()[i]) return false;
		n = m;
	}
	
	for (int i=0; i<n; i++)
	{
		IAttribute *attr1 = GetAttributes()[i];
		IAttribute *attr2 = with->GetAttributes()[i];
		if (attr1)
		{
			if (attr2) return attr1->IsEqualTo(attr2);
			else return false;
		}
		else if (attr2) return false;
	}
	return true;
}

bool NodeStructured::Process(AttributeManager &manager, void *context)
{
	for (int i=0; i<_nodes.Size(); i++)
		if (_nodes[i]->PerformProcess(manager, context)) return true;
	return false;
}

LSError NodeStructured::Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes)
{
	CHECK(base::Serialize(ar, types, attrTypes))

	int n;
	if (ar.IsSaving() || ar.GetPass() == ParamArchive::PassSecond)
	{
		n = _nodes.Size();
		CHECK(ar.Serialize("count", n, 1, 0))
	}
	else
	{
		CHECK(ar.Serialize("count", n, 1, 0))
		_nodes.Resize(n);
	}
	
	for (int i=0; i<n; i++)
	{
		char name[256];
		sprintf(name, "Item%d", i);
		INode *node = _nodes[i];
		CHECK(SerializeNode(name, node, ar, types, attrTypes))
		_nodes[i] = node;
	}
	return LSOK;
}

bool NodeStructured::IsEqualTo(const INode *with) const
{
	if (!base::IsEqualTo(with)) return false;

	const NodeStructured *w = static_cast<const NodeStructured *>(with);
	int n = _nodes.Size();
	if (w->_nodes.Size() != n) return false;
	for (int i=0; i<n; i++)
	{
		INode *node1 = _nodes[i];
		INode *node2 = w->_nodes[i];
		if (node1)
		{
			if (node2)
			{
				if (!node1->IsEqualTo(node2)) return false;
			}
			else return false;
		}
		else if (node2) return false;
	}
	return true;
}
