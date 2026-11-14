#ifdef _MSC_VER
#pragma once
#endif

#ifndef _HIER_WALKER_HPP
#define _HIER_WALKER_HPP

#include <El/Enum/dynEnum.hpp>

class ParamEntry;
class ParamArchive;

// iDrawText.hpp

// Attributes and hierarchy nodes

// forward declaration
struct AttributeTypes;

//! Interface for attribute
class IAttribute : public RefCount
{
public:
	virtual void Load(RString value) = 0;
	virtual LSError Serialize(ParamArchive &ar) = 0;
	virtual bool IsEqualTo(const IAttribute *with) const = 0;
};

typedef IAttribute *CreateAttrFunction();
TypeIsSimple(CreateAttrFunction *)

struct AttributeTypes
{
	DynEnumCS attributes;
	AutoArray<CreateAttrFunction *> createFunctions;

	IAttribute *CreateAttribute(RString type) const;
};

struct AttributeList : public RefArray<IAttribute>
{
	void Update(const AttributeList &src);
	void SetAttribute(RString name, RString value, const AttributeTypes &types);
	IAttribute *AccessAttribute(RString name, const AttributeTypes &types);
	IAttribute *AccessAttribute(int index, const AttributeTypes &types);
	LSError Serialize(ParamArchive &ar, const AttributeTypes &types);
};

void SetAttributes(AttributeList &attributes, const ParamEntry &cls, const AttributeTypes &types);

class AttributeManager
{
protected:
	AutoArray<AttributeList> _stack;

public:
	AttributeManager(AttributeList &def);

	void Push();
	void Pop();

	void SetAttributes(const AttributeList &src);
	const IAttribute *GetAttribute(int index) const;

	const AttributeList &GetAttributeList() const;
};

// common attribute types
IAttribute *CreateAttrBool();
bool GetAttrBool(const IAttribute *attr);
void SetAttrBool(IAttribute *attr, bool value);

IAttribute *CreateAttrInt();
int GetAttrInt(const IAttribute *attr);
void SetAttrInt(IAttribute *attr, int value);

IAttribute *CreateAttrFloat();
float GetAttrFloat(const IAttribute *attr);
void SetAttrFloat(IAttribute *attr, float value);

IAttribute *CreateAttrString();
RString GetAttrString(const IAttribute *attr);
void SetAttrString(IAttribute *attr, RString value);

// forward declaration
struct NodeTypes;

//! Base (abstract) class for common hierarchy node
class INode : public RefCount
{
private:
	//! Attributes
	AttributeList _attributes;

public:
	virtual RString GetType() const = 0;
	virtual LSError Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes);
	virtual bool IsEqualTo(const INode *with) const;

	virtual bool Add(INode *text) {return false;}

	void SetAttribute(RString name, RString value, const AttributeTypes &types)
	{
		_attributes.SetAttribute(name, value, types);
	}
	IAttribute *AccessAttribute(RString name, const AttributeTypes &types)
	{
		return _attributes.AccessAttribute(name, types);
	}

	//! Return true if processing finished
	virtual bool Process(AttributeManager &manager, void *context) = 0;

	const AttributeList &GetAttributes() const {return _attributes;}
	void SetAttributes(const AttributeList &attributes) {_attributes = attributes;}

	bool PerformProcess(AttributeManager &manager, void *context);
};

typedef INode *CreateNodeFunction();
TypeIsSimple(CreateNodeFunction *)

struct NodeTypes
{
	DynEnumCS nodes;
	AutoArray<CreateNodeFunction *> createFunctions;

	INode *CreateNode(RString type) const;
};

class NodeStructured : public INode
{
	typedef INode base;

protected:
	RefArray<INode> _nodes;

public:
	RString GetType() const {return "NodeStructured";}
	LSError Serialize(ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes);
	bool IsEqualTo(const INode *with) const;

	bool Add(INode *node) {_nodes.Add(node); return true;}
	bool Process(AttributeManager &manager, void *context);
};

LSError SerializeNode(RString name, INode *&value, ParamArchive &ar, const NodeTypes &types, const AttributeTypes &attrTypes);

#endif

