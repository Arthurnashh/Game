//#include "precomp.h"
#include "engine/enf_module.h"

namespace enf
{
//-----------------------------------------------------------------------
void ClassFactoryCollection::Register(ItemFactory& factory)
{
	//avoid duplicities
	uint index = m_ClassFactorySet.GetIndexOf(MakePair(&factory, (uint)0));
	enf_assert2(index == INDEX_NOT_FOUND, "Class factory already exists");

	if(index != INDEX_NOT_FOUND)
	{
		return;
	}
	index = m_ClassFactories.Insert(&factory);
	m_ClassFactorySet.Insert(MakePair(&factory, index));
}

//-----------------------------------------------------------------------
void ClassFactoryCollection::Unregister(ItemFactory& factory)
{
	uint index = m_ClassFactorySet.GetIndexOf(MakePair(&factory, (uint)0));
	if(index != INDEX_NOT_FOUND)
	{
		m_ClassFactories.RemoveElement(m_ClassFactorySet[index].Data);
		m_ClassFactorySet.RemoveElement(index);
	}
}

//-----------------------------------------------------------------------
BaseItem* ClassFactoryCollection::Create(CStr typeName)
{
	for(uint n = 0; n < m_ClassFactories.GetCardinality(); n++)
	{
		if(m_ClassFactories[n]->GetTypeName() == typeName)
			return m_ClassFactories[n]->CreateItem();
	}
	return NULL;
}

//-----------------------------------------------------------------------
BaseItem* ClassFactoryCollection::Create(uint typeID)
{
	for(uint n = 0; n < m_ClassFactories.GetCardinality(); n++)
	{
		if(m_ClassFactories[n]->GetTypeID() == typeID)
			return m_ClassFactories[n]->CreateItem();
	}
	return NULL;
}

//-----------------------------------------------------------------------
BaseItem* Module::CreateInstance(uint typeId)
{
	for(int n = m_ClassFactories.GetCardinality() - 1; n >= 0; n--)
	{
		BaseItem* item = m_ClassFactories[n]->Create(typeId);
		if(item)
			return item;
	}

	return ClassFactoryCollection::Create(typeId);
}

//-----------------------------------------------------------------------
BaseItem* Module::CreateInstance(const char* typeName)
{
	for(int n = m_ClassFactories.GetCardinality() - 1; n >= 0; n--)
	{
		BaseItem* item = m_ClassFactories[n]->Create(typeName);
		if(item)
			return item;
	}

	return ClassFactoryCollection::Create(typeName);
}

}