// DayZ special content:
//
//		- solve world parsing
//		- collect spawn information
//		- serialize it
//		- spawns objects


#include "../wpch.hpp"

#include "../serializeBinExt.hpp"

#include "../object.hpp"

#include <El/Evaluator/express.hpp>


#include "../gameStateExt.hpp"
#include "../Shape/shape.hpp"

#include "WorldLoot.h"

//#define OUTPUT_PROXYEXPORT_CONSOLE	// output proxy export to console (list all buildings, containers and proxies)
//#define OUTPUT_AREATEST // if radius should be checked on spawn


static const int s_iCONTENT_FILE_VERSION = 0x00000011;

static const char* s_sBINPROXY_FILEHEADER = "BINARIZED_PROXY_EXPORT";

// parsed config fils
static const char* s_sCONFIGSOURCES[] = {"cfgVehicles","cfgWeapons","cfgMagazines"};
static const char s_sCONFIGLOOT[] = {"cfgLootSpawns"};
static const char s_sCONFIGVEHICLES[] = {"cfgVehicles"};
static const char s_sCONFIGGLOBAL[] = {"cfgLootGlobal"};

static const float MAX_PROXY_RANGE = 0.5f;
static const float MIN_PROXY_HEIGHT = 0.2f; // 20cm


static const float PROXY_TEST_BELOW = 3.0f; // test placement below
static const float PROXY_TEST_ABOVE = 2.0f; // test placement above

//-----------------------------------------------------------------------------
WRClassHierarchy::~WRClassHierarchy()
{
	for( int c = 0; c < m_aItem.Size(); c++ )
		delete m_aItem[c];

	if( m_pName )
		delete m_pName;

	m_aIndexed.Clear();
	m_aEndClass.Clear();

	for( int c = 0; c < m_aAttachments.Size(); c++ )
		delete m_aAttachments[c];

	for( int c = 0; c < m_aCargo.Size(); c++ )
		delete m_aCargo[c];

}
//-----------------------------------------------------------------------------
void WRClassHierarchy::RegisterLoot( WRLoot* pWRLoot, int iLevels )
{
	BuildContentHiearchy(pWRLoot,iLevels,iLevels);
}
//-----------------------------------------------------------------------------
void WRClassHierarchy::BuildEndClass( WRClassHierarchy* pParent )
{

	for( int c = 0; c < m_aItem.Size(); c++ )
		m_aItem[c]->BuildEndClass(this);

	if( pParent )
	{
		for( int c = 0; c < m_aEndClass.Size(); c++ )
			pParent->AddToEndClass(m_aEndClass[c]);

		if( IsEndClass() )
			pParent->AddToEndClass(this);
	};

	// do not forget parent
	m_pParent = pParent;

}
//-----------------------------------------------------------------------------
void WRClassHierarchy::Add( WRClassHierarchy* pItem )
{
	m_aItem.Add(pItem);
}
//-----------------------------------------------------------------------------
void WRClassHierarchy::AddToIndex( WRClassHierarchy* pItem )
{
	pItem->SetIndex(m_aIndexed.Size());
	m_aIndexed.Add(pItem);
}
//-----------------------------------------------------------------------------
void WRClassHierarchy::AddToEndClass( WRClassHierarchy* pItem )
{
	m_aEndClass.Add(pItem);
}
//-----------------------------------------------------------------------------
void WRClassHierarchy::AddAttachment( const char* sName )
{
	RString* str = new RString(sName);
	m_aAttachments.Add(str);
}
//-----------------------------------------------------------------------------
void WRClassHierarchy::AddCargo( const char* sName )
{
	RString* str = new RString(sName);
	m_aCargo.Add(str);
}
//-----------------------------------------------------------------------------
void WRClassHierarchy::BuildContentHiearchy( WRLoot* pWRLoot, int iLevels, int iRootLevel )
{
//	SetLevel(iLevels);

	RString rootClass1 = "InventoryBase";
	RString rootClass2 = "DefaultWeapon";
	RString rootClass3 = "MagazineBase";

	//this really should be refactored to a rotation vector from config
	RString rDropped = "rotateDropped";
	RString rDroppedY = "rotateY";
	RString rDroppedZ = "rotateZ";
	RString rAlways = "rotateAlways";
	RString rNever = "rotateNever";
	RString rInvert = "rotateInvert";

	RString rBaseAttachments = "baseAttachments";
	RString rItemsCargo = "itemsCargo";

	RString rShapeName = "model";

	for( int confCount = 0; confCount < sizeof(s_sCONFIGSOURCES)/sizeof(s_sCONFIGSOURCES[0]); confCount++ )
	{


		ParamEntryPtr par = Pars.FindEntry(s_sCONFIGSOURCES[confCount]);
		for( int parCount = 0; parCount < par->GetEntryCount(); parCount++ )
		{
			ParamEntryVal v = par->GetEntry(parCount);
			if( !v.IsClass() )
				continue;

			// we do not care about children at root level
			bool isProperChildrenOrBase = false;

			if( iLevels == iRootLevel )
			{
				// on root level we limit to root class
				if( rootClass1 == v.GetName() )
					isProperChildrenOrBase = true;
				else if( rootClass2 == v.GetName() )
					isProperChildrenOrBase = true;
				else if( rootClass3 == v.GetName() )
					isProperChildrenOrBase = true;
			}
			else if( m_pName )
			{
				// non-root level
				const char* baseName = v.GetClassInterface() ? v.GetClassInterface()->GetBaseName() : NULL;
				if( baseName )
					isProperChildrenOrBase = *m_pName==baseName;
			};

	
			// add children
			if( isProperChildrenOrBase )
			{
				RString shapeName;

				WRClassHierarchy* item = new WRClassHierarchy();
				item->SetName(v.GetName().Data());
				item->SetLevel(iLevels);
				item->SetParent(this);

				// normally add to branch
				Add(item);
				// add to root to indexing array
				pWRLoot->GetHierarchy()->AddToIndex(item);

				// use parent's data as primary
				item->SetFlags(GetFlags());

				// read rest of arguments
				for( int vCount = 0; vCount < v.GetEntryCount(); vCount++ )
				{
					ParamEntryVal vvv = v.GetEntry(vCount);
					if( vvv.GetName() == rDropped )
					{
						bool res = vvv;
						if( res )
							item->SetFlags(ECH_ROTATEDROPPED);
						else
							item->ClearFlags(ECH_ROTATEDROPPED);
					}
					if( vvv.GetName() == rInvert )
					{
						bool res = vvv;
						if( res )
							item->SetFlags(ECH_ROTATEINVERT);
						else
							item->ClearFlags(ECH_ROTATEINVERT);
					}
					if( vvv.GetName() == rDroppedZ )
					{
						bool res = vvv;
						if( res )
							item->SetFlags(ECH_ROTATEDROPPEDZ);
						else
							item->ClearFlags(ECH_ROTATEDROPPEDZ);
					}
					else if( vvv.GetName() == rAlways )
					{
						bool res = vvv;
						if( res )
							item->SetFlags(ECH_ROTATEALWAYS);
						else
							item->ClearFlags(ECH_ROTATEALWAYS);
					}
					else if( vvv.GetName() == rDroppedY )
					{
						bool res = vvv;
						if( res )
							item->SetFlags(ECH_ROTATEDROPPEDY);
						else
							item->ClearFlags(ECH_ROTATEDROPPEDY);
					}
					else if( vvv.GetName() == rNever )
					{
						bool res = vvv;
						if( res )
							item->SetFlags(ECH_ROTATENEVER);
						else
							item->ClearFlags(ECH_ROTATENEVER);
					}
					else if( vvv.GetName() == rShapeName )
					{
						shapeName = vvv;
					}
					else if( vvv.IsArray() )
					{

						if( vvv.GetName() == rBaseAttachments )
						{
							for( int size = 0; size < vvv.GetSize(); size++ )
							{
								const IParamArrayValue& x = vvv[size];
								if( x.IsTextValue() )
								{
									RString str = x.GetValueRaw();
									str.Lower();

									item->AddAttachment(str.Data());
								};
							};
						}
						else if( vvv.GetName() == rItemsCargo )
						{
							for( int size = 0; size < vvv.GetSize(); size++ )
							{
								const IParamArrayValue& x = vvv[size];
								if( x.IsTextValue() )
								{
									RString str = x.GetValueRaw();
									str.Lower();

									item->AddCargo(str.Data());
								};
							};
						};


					};

				};


				if( !shapeName.IsEmpty() )
				{
					const char* mdl = shapeName.Data();

//				    Ref<LODShape> lShape = Shapes.New("data3d\\ural.p3d",ShapeReversed|ShapeShadow);
					Ref<LODShape> lShape = Shapes.New(&mdl[1],ShapeNormal);
          const Shape* memory = lShape->MemoryLevel();

					Vector3 _boundingBox[2];
          InventoryItemType::ComputeBoundingBox(lShape, memory, _boundingBox);

					// bounding box
					item->SetBBox(_boundingBox[0],_boundingBox[1]);

					float sphere = lShape->BoundingSphere();

					// bounding sphere
					item->SetBSphere(sphere);
				
				};

			};

		};
	};


	// lower level
	iLevels--;

	// recursive
	if( iLevels > 0 )
	{
		for( int c = 0; c < m_aItem.Size(); c++ )
			m_aItem[c]->BuildContentHiearchy(pWRLoot,iLevels,iRootLevel);
	};

}
//-----------------------------------------------------------------------------
void WRClassHierarchy::ListToConsoleIndexed( void )
{
	if( m_pName )
		RptF("%03d %s", m_iIndex, m_pName->Data() );
	else
		RptF("%03d", m_iIndex );

	if( m_aIndexed.Size() > 0 )
	{
		RptF("Index:" );
		for( int c = 0; c < m_aIndexed.Size(); c++ )
			m_aIndexed[c]->ListToConsoleIndexed();
	};

}
//-----------------------------------------------------------------------------
void WRClassHierarchy::ListToConsole( int iIndent )
{
	int indentLen = (IsEndClass()?iIndent:0);

	static char indent[CONSOLEOUTPUT_INDENT_MAX+CONSOLEOUTPUT_INDENT_STEP];
	for( int c = 0; c < indentLen; c++ )
		indent[c] = ' ';

	indent[indentLen] = 0;

	if( m_pName )
	{
		if( m_aItem.Size() > 0 )
			RptF("%03d %s %s [%d] children", m_iIndex, indent, m_pName->Data(), m_aItem.Size() );
		else
			RptF("%03d %s %s", m_iIndex, indent, m_pName->Data() );
	}
	else
	{
		if( m_aItem.Size() > 0 )
			RptF("%03d %s [%d] children", m_iIndex, indent, m_aItem.Size() );
		else
			RptF("%03d %s", m_iIndex, indent );
	};

	iIndent += CONSOLEOUTPUT_INDENT_STEP;
	if( iIndent > (CONSOLEOUTPUT_INDENT_MAX-CONSOLEOUTPUT_INDENT_STEP) )
		iIndent = (CONSOLEOUTPUT_INDENT_MAX-CONSOLEOUTPUT_INDENT_STEP);

	for( int c = 0; c < m_aItem.Size(); c++ )
		m_aItem[c]->ListToConsole(iIndent);

}
//-----------------------------------------------------------------------------
WRClassHierarchy* WRClassHierarchy::GetClassByIndex( int iIndex )
{
	if( m_aIndexed.Size() < iIndex )
		return NULL;

	return m_aIndexed[iIndex];
}
//-----------------------------------------------------------------------------
WRClassHierarchy* WRClassHierarchy::FindClassByIndex( int iIndex )
{
	if( m_pName )
	{
		if( m_iIndex == iIndex )
			return this;
	};

	for( int c = 0; c < m_aItem.Size(); c++ )
	{
		WRClassHierarchy* cl = m_aItem[c]->FindClassByIndex(iIndex);
		if( cl )
			return cl;
	};

	// not found
	return NULL;
}
//-----------------------------------------------------------------------------
WRClassHierarchy* WRClassHierarchy::FindClass( const char* sName )
{
	if( m_pName != NULL )
	{
		if( *m_pName == sName )
			return this;
	};

	for( int c = 0; c < m_aItem.Size(); c++ )
	{
		WRClassHierarchy* cl = m_aItem[c]->FindClass(sName);
		if( cl )
			return cl;
	};

	// not found
	return NULL;
}
//-----------------------------------------------------------------------------
int WRClassHierarchy::CountClass( const char* sName )
{
	int iCount = 0;

	if( m_pName )
	{
		if( *m_pName == sName )
			iCount++;
	};

	for( int c = 0; c < m_aItem.Size(); c++ )
		iCount += m_aItem[c]->CountClass(sName);

	// not found
	return iCount;
}
//-----------------------------------------------------------------------------
bool WRClassHierarchy::IsChildOf( const char* sName )
{
	WRClassHierarchy* p = this;
	while( p = p->GetParent() )
	{
		if( *p->m_pName == sName )
			return true;
	};

	return false;
}
//-----------------------------------------------------------------------------
int WRClassHierarchy::CheckForDuplicities( WRClassHierarchy* pHierarchy )
{
	int count = 0;

	if( m_pName )
	{
		int duplicities = pHierarchy->CountClass(m_pName->Data());
		if( duplicities > 1 )
		{
			RptF("!!! %s has %d duplicates.", m_pName->Data(), duplicities );
			count++;
		};
	};

	if( pHierarchy != NULL )
	{
		for( int c = 0; c < m_aItem.Size(); c++ )
			count += m_aItem[c]->CheckForDuplicities(pHierarchy);
	};

	return count;
}
//-----------------------------------------------------------------------------
int WRClassHierarchy::CheckForDuplicities( void )
{
	return CheckForDuplicities(this);
}
//-----------------------------------------------------------------------------
bool WRClassHierarchy::Save( SerializeBinStream* pStream )
{
	pStream->SaveInt(m_iLevel);
	pStream->SaveInt(m_iIndex);
	pStream->SaveInt(m_iFlags);

	pStream->SaveVector3(m_vBBox[0].X(),m_vBBox[0].Y(),m_vBBox[0].Z());
	pStream->SaveVector3(m_vBBox[1].X(),m_vBBox[1].Y(),m_vBBox[1].Z());
	pStream->SaveFloat(m_fBSphere);

	if( m_pName )
	{
		pStream->SaveChar(1);
		pStream->SaveString(m_pName->Data());
	}
	else
	{
		pStream->SaveChar(0);
	};

	pStream->SaveInt(m_aItem.Size());
	for( int c = 0; c < m_aItem.Size(); c++ )
		m_aItem[c]->Save(pStream);

	pStream->SaveInt(m_aAttachments.Size());
	for( int c = 0; c < m_aAttachments.Size(); c++ )
		pStream->SaveString(m_aAttachments[c]->Data());

	pStream->SaveInt(m_aCargo.Size());
	for( int c = 0; c < m_aCargo.Size(); c++ )
		pStream->SaveString(m_aCargo[c]->Data());

	return true;
}
//-----------------------------------------------------------------------------
bool WRClassHierarchy::Load( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	m_iLevel = pStream->LoadInt();
	m_iIndex = pStream->LoadInt();
	m_iFlags = pStream->LoadInt();

	float x, y, z;

	pStream->LoadVector3(&x,&y,&z);
	m_vBBox[0] = Vector3(x,y,z);

	pStream->LoadVector3(&x,&y,&z);
	m_vBBox[1] = Vector3(x,y,z);

	m_fBSphere = pStream->LoadFloat();


	if( m_pName )
		delete m_pName;

	if( pStream->LoadChar() > 0 )
		m_pName = new RString(pStream->LoadString());

	int count = pStream->LoadInt();
	if( count > 0 )
	{
		m_aItem.Realloc(count);
		for( int c = 0; c < count; c++ )
		{
			WRClassHierarchy* item = new WRClassHierarchy();

			// normally add to branch
			Add(item);

			item->Load(pWRLoot,pStream);
		};
	};

	int countAttachments = pStream->LoadInt();
	if( countAttachments > 0 )
	{
		m_aAttachments.Realloc(countAttachments);
		for( int c = 0; c < countAttachments; c++ )
		{
			RString at = pStream->LoadString();
			AddAttachment(at.Data());
		};
	};

	int countCargo = pStream->LoadInt();
	if( countCargo > 0 )
	{
		m_aCargo.Realloc(countCargo);
		for( int c = 0; c < countCargo; c++ )
		{
			RString ct = pStream->LoadString();
			AddCargo(ct.Data());
		};
	};

	// post load for root
	if( !m_pName )
		PostLoad();

	return true;
}
//-----------------------------------------------------------------------------
void WRClassHierarchy::PostLoad( void )
{
	// build indexed array
	for( int c = 0; c < 8192; c++ )
	{
		WRClassHierarchy* cl = FindClassByIndex(c);
		if( cl == NULL ) 
			break;

		AddToIndex(cl);
	};

}
//-----------------------------------------------------------------------------
int WRClassHierarchy::GetEndClassCount( void )
{
	// just this class
	if( m_aItem.Size() < 1 )
		return m_pParent ? 1 : 0;

	// count children
	int count = 0;
	for( int c = 0; c < m_aItem.Size(); c++ )
		count += m_aItem[c]->GetEndClassCount();

	return count;
}
//-----------------------------------------------------------------------------
WRClassHierarchy* WRClassHierarchy::GetFirstEndClass( void )
{
	if( IsEndClass() )
		return this;

	for( int c = 0; c < m_aItem.Size(); c++ )
	{
		WRClassHierarchy* ret = m_aItem[c]->GetFirstEndClass();
		if( ret )
			return ret;
	};

	// no end class found
	return NULL;
}
//-----------------------------------------------------------------------------
WRClassHierarchy* WRClassHierarchy::GetRandomEndClass( void )
{
	// endclass itself
	if( IsEndClass() )
		return this;

	// no endclass
	if( m_aEndClass.Size() < 1 )
		return NULL;

	if( m_aEndClass.Size() == 1 )
		return m_aEndClass[0];

	int idx = GRandGen.RandomInt(0,m_aEndClass.Size());
	return m_aEndClass[idx];
}
//-----------------------------------------------------------------------------
WRClassHierarchy* WRClassHierarchy::GetEndClass( int iIndex )
{
	return m_aEndClass[iIndex];
}
//-----------------------------------------------------------------------------
AutoArray<RString*>& WRClassHierarchy::GetAttachments( void )
{
	if( m_aAttachments.Size() > 0 )
		return m_aAttachments;

	WRClassHierarchy* p = this;
	while( p = p->GetParent() )
	{
		if( p->HasAttachments() )
			return p->GetAttachments();
	};

	return m_aAttachments;
}
//-----------------------------------------------------------------------------
AutoArray<RString*>& WRClassHierarchy::GetCargo( void )
{
	if( m_aCargo.Size() > 0 )
		return m_aCargo;

	WRClassHierarchy* p = this;
	while( p = p->GetParent() )
	{
		if( p->HasCargo() )
			return p->GetCargo();
	};

	return m_aCargo;
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
RString* WRStringPool::AddString( const char* sString )
{
	// check if not added yet
	for( int c = 0; c < m_aPool.Size(); c++ )
	{
		if( *m_aPool[c] == sString )
			return m_aPool[c];
	};

	// insert new
	RString* str = new RString(sString);
	m_aPool.Add(str);

	return m_aPool.Last();
}
//-----------------------------------------------------------------------------
RString* WRStringPool::AddStringWithoutSlash( const char* sString )
{
	const char* subString = strrchr(sString,'\\');
	return AddString(subString?(subString+1):sString);
}
//-----------------------------------------------------------------------------
bool WRStringPool::Save( SerializeBinStream* pStream )
{
	pStream->SaveInt(m_aPool.Size());

	for( int c = 0; c < m_aPool.Size(); c++ )
		pStream->SaveString(m_aPool[c]->Data());

	return true;
}
//-----------------------------------------------------------------------------
bool WRStringPool::Load( SerializeBinStream* pStream )
{
	int count = pStream->LoadInt();
	if( count > 0 )
	{
		m_aPool.Realloc(count);
		for( int c = 0; c < count; c++ )
			m_aPool.Add( new RString(pStream->LoadString()) );
	};

	return true;
}
//-----------------------------------------------------------------------------
int WRStringPool::GetIndex( const char* sString )
{
	for( int c = 0; c < m_aPool.Size(); c++  )
	{
		if( *m_aPool[c] == sString )
			return c;
	};

	return -1;
}
//-----------------------------------------------------------------------------
int WRStringPool::GetIndex( RString* pString )
{
	for( int c = 0; c < m_aPool.Size(); c++  )
	{
		if( m_aPool[c] == pString )
			return c;
	};

	return -1;
}
//-----------------------------------------------------------------------------
int WRStringPool::GetIndex( const RString& pString )
{
	for( int c = 0; c < m_aPool.Size(); c++  )
	{
		if( *m_aPool[c] == pString )
			return c;
	};

	return -1;
}
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
bool WRLoot_Point::Save( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	pStream->SaveInt( m_iFlags );

	if( (m_iFlags&EPROXY_POINT) == 0 )
	{
		pStream->SaveInt( pWRLoot->GetIndexForProxyName(m_pName) );
	};

	pStream->SaveVector3( m_vOrigin.X(), m_vOrigin.Y(), m_vOrigin.Z() );

	pStream->SaveFloat(m_fRange);
	pStream->SaveFloat(m_fHeight);

	return true;
}
//-----------------------------------------------------------------------------
bool WRLoot_Point::Load( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
//	m_iFlags = pStream->LoadInt();

	if( (m_iFlags&EPROXY_POINT) == 0 )
	{
		int stringID = pStream->LoadInt();
		m_pName = pWRLoot->GetProxyName(stringID);
	};

	float x, y, z;

	pStream->LoadVector3(&x,&y,&z);
	m_vOrigin[0] = x;
	m_vOrigin[1] = y;
	m_vOrigin[2] = z;

	m_fRange = pStream->LoadFloat();
	m_fHeight = pStream->LoadFloat();

	return true;
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
bool WRLoot_Proxy::Save( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	WRLoot_Point::Save(pWRLoot,pStream);

	pStream->SaveVector3( m_vDirection.X(), m_vDirection.Y(), m_vDirection.Z() );
	pStream->SaveVector3( m_vDirUp.X(), m_vDirUp.Y(), m_vDirUp.Z() );

	return true;
}
//-----------------------------------------------------------------------------
bool WRLoot_Proxy::Load( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	WRLoot_Point::Load(pWRLoot,pStream);

	float x, y, z;

	pStream->LoadVector3(&x,&y,&z);
	m_vDirection[0] = x;
	m_vDirection[1] = y;
	m_vDirection[2] = z;

	pStream->LoadVector3(&x,&y,&z);
	m_vDirUp[0] = x;
	m_vDirUp[1] = y;
	m_vDirUp[2] = z;

	return true;
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
bool WRLoot_Container::Save( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	pStream->SaveInt( pWRLoot->GetIndexForContainerName(m_pName) );

	pStream->SaveInt(m_aProxy.Size());
	for( int c = 0; c < m_aProxy.Size(); c++ )
		m_aProxy[c]->Save(pWRLoot,pStream);

	return true;
}
//-----------------------------------------------------------------------------
bool WRLoot_Container::Load( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	int stringID = pStream->LoadInt();
	m_pName = pWRLoot->GetContainerName(stringID);

	int count = pStream->LoadInt();
	if( count > 0 )
	{
		m_aProxy.Realloc(count);
		for( int c = 0; c < count; c++ )
		{
			int flags = pStream->LoadInt();

			WRLoot_Point* pp = ((flags&WRLoot_Proxy::EPROXY_POINT) == NULL)
									? new WRLoot_Proxy(this,flags)
									: new WRLoot_Point(this,flags);

			pp->Load(pWRLoot,pStream);

			m_aProxy.Add(pp);
		};
	};

	return true;
}
//-----------------------------------------------------------------------------
int WRLoot_Container::HowManyProxiesUseFlag( int iFlag )
{
	int count = 0;
	for( int c = 0; c < m_aProxy.Size(); c++ )
	{
		if( m_aProxy[c]->CheckFlags(iFlag) )
			count++;
	};

	return count;
}
//-----------------------------------------------------------------------------
WRLoot_Proxy* WRLoot_Container::InsertProxy( void )
{
	WRLoot_Proxy* px = new WRLoot_Proxy(this);
	m_aProxy.Add(px);
	return px;
}
//-----------------------------------------------------------------------------
WRLoot_Point* WRLoot_Container::InsertPoint( void )
{
	WRLoot_Point* px = new WRLoot_Point(this);
	m_aProxy.Add(px);
	return px;
}
//-----------------------------------------------------------------------------
WRLoot_Point* WRLoot_Container::GetRandomProxy( int iFlags, float fMinimalHeight )
{
	// no endclass
	if( m_aProxy.Size() < 1 )
		return NULL;

	if( m_aProxy.Size() == 1 )
	{
		if( m_aProxy[0]->HasFlags(iFlags) && m_aProxy[0]->GetHeight()>=fMinimalHeight )
			return m_aProxy[0];
		
		return NULL;
	};

	int idx = GRandGen.RandomInt(0,m_aProxy.Size());
	for( int c = 0; c < m_aProxy.Size(); c++ )
	{

		if( m_aProxy[idx]->HasFlags(iFlags) && m_aProxy[0]->GetHeight()>=fMinimalHeight )
			return m_aProxy[idx];
		
		if( (++idx) >= m_aProxy.Size() )
			idx = 0;
	};

	return NULL;
}
//-----------------------------------------------------------------------------
void WRLoot_Container::GetBoundaries( float& fX1, float& fZ1, float& fX2, float& fZ2 )
{
	for( int c = 0; c < GetTotalCount(); c++ )
	{
		float x = m_aProxy[c]->GetOrigin().X();
		float z = m_aProxy[c]->GetOrigin().Z();

		if( x < fX1 )
			fX1 = x;
		if ( x > fX2 )
			fX2 = x;
		if( z < fZ1 )
			fZ1 = z;
		if ( z > fZ2 )
			fZ2 = z;

	};
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
WRLoot_Group::~WRLoot_Group()
{
	for( int c = 0; c < m_aSelection.Size(); c++ )
		delete m_aSelection[c];
}
//-----------------------------------------------------------------------------
bool WRLoot_Group::Save( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	pStream->SaveInt( pWRLoot->GetIndexForGroupName(m_pName) );

	pStream->SaveInt(m_aSelection.Size());
	for( int c = 0; c < m_aSelection.Size(); c++ )
		m_aSelection[c]->Save(pWRLoot,pStream);

	pStream->SaveVector3( m_vOrigin.X(), m_vOrigin.Y(), m_vOrigin.Z() );

	return true;
}
//-----------------------------------------------------------------------------
bool WRLoot_Group::Load( WRLoot* pWRLoot, SerializeBinStream* pStream )
{
	int stringID = pStream->LoadInt();
	m_pName = pWRLoot->GetGroupName(stringID);

	int count = pStream->LoadInt();
	if( count > 0 )
	{
		m_aSelection.Realloc(count);
		for( int c = 0; c < count; c++ )
		{
			WRLoot_Container* selection = new WRLoot_Container(this);
			selection->Load(pWRLoot,pStream);

			m_aSelection.Add(selection);
		};
	};

	float x, y, z;
	pStream->LoadVector3(&x,&y,&z);
	m_vOrigin[0] = x;
	m_vOrigin[1] = y;
	m_vOrigin[2] = z;

	return true;
}
//-----------------------------------------------------------------------------
WRLoot_Container* WRLoot_Group::InsertContainer( void )
{
	WRLoot_Container* sel = new WRLoot_Container(this);
	m_aSelection.Add(sel);
	return sel;
}
//-----------------------------------------------------------------------------
int WRLoot_Group::GetProxyCount( void )
{
	int count = 0;
	for( int c = 0; c < m_aSelection.Size(); c++ )
		count += m_aSelection[c]->GetProxyCount();

	return count;
}
//-----------------------------------------------------------------------------
int WRLoot_Group::GetPointCount( void )
{
	int count = 0;
	for( int c = 0; c < m_aSelection.Size(); c++ )
		count += m_aSelection[c]->GetPointCount();

	return count;
}
//-----------------------------------------------------------------------------
int WRLoot_Group::GetTotalCount( void )
{
	int count = 0;
	for( int c = 0; c < m_aSelection.Size(); c++ )
		count += m_aSelection[c]->GetTotalCount();

	return count;
}
//-----------------------------------------------------------------------------
WRLoot_Container* WRLoot_Group::FindSelection( const char* sName )
{
	for( int c = 0; c < m_aSelection.Size(); c++ )
	{
		if( m_aSelection[c]->CheckName(sName) )
			return m_aSelection[c];
	};

	return NULL;
}
//-----------------------------------------------------------------------------
int WRLoot_Group::HowManyProxiesUseFlag( int iFlag )
{
	int count = 0;
	for( int c = 0; c < m_aSelection.Size(); c++ )
		count += m_aSelection[c]->HowManyProxiesUseFlag(WRLoot_Proxy::EPROXY_INCONVENIENT);

	return count;
}
//-----------------------------------------------------------------------------
void WRLoot_Group::GetBoundaries( float& fX1, float& fZ1, float& fX2, float& fZ2 )
{
	for( int c = 0; c < GetCount(); c++ )
		m_aSelection[c]->GetBoundaries(fX1,fZ1,fX2,fZ2);
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
void WRSpawnItem::ListToConsole( void )
{
	RptF("        %s, %0.4f, %0.4f", name.Data(), m_fGroupProbability, m_fItemProbability );
}
//-----------------------------------------------------------------------------
bool WRSpawnItem::ShouldSpawn( void )
{
	int pp = GRandGen.RandomInt(0,100);
	if( (int)(m_fItemProbability*100.0f) < pp )
		return true;

	return false;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void WRSpawnLocation::ListToConsole( void )
{
	RptF("        %s, %d", name.Data(), m_iCount );
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
WRContainer::~WRContainer()
{
	for( int c = 0; c < m_aLocations.Size(); c++ )
		delete m_aLocations[c];
	for( int c = 0; c < m_aItems.Size(); c++ )
		delete m_aItems[c];
}
//-----------------------------------------------------------------------------
void WRContainer::Normalize( void )
{
	float max = 0.0f;
	for( int c = 0; c < m_aItems.Size(); c++ )
	{
		float cur = m_aItems[c]->GetGroupProbability();
		if( cur > max )
			max = cur;
	};

	// already max
	if( max == 1.0f || max == 0.0f )
		return;

	float div = 1.0f / max;

	// normalize
	for( int c = 0; c < m_aItems.Size(); c++ )
		m_aItems[c]->SetGroupProbability(m_aItems[c]->GetGroupProbability()*div);
}
//-----------------------------------------------------------------------------
WRSpawnLocation* WRContainer::AddLocation( void )
{
	WRSpawnLocation* loc = new WRSpawnLocation(this);

	m_aLocations.Add(loc);
	return loc;
}
//-----------------------------------------------------------------------------
WRSpawnItem* WRContainer::AddItem( void )
{
	WRSpawnItem* item = new WRSpawnItem();

	m_aItems.Add(item);
	return item;
}
//-----------------------------------------------------------------------------
void WRContainer::ListToConsole( void )
{
	RptF("    %s (%d) locations (%d) items", name.Data(), m_aLocations.Size(), m_aItems.Size() );

	RptF("      _locations:" );
	for( int c = 0; c < m_aLocations.Size(); c++ )
		m_aLocations[c]->ListToConsole();

	RptF("      _items:" );
	for( int c = 0; c < m_aItems.Size(); c++ )
		m_aItems[c]->ListToConsole();
}
//-----------------------------------------------------------------------------
void WRContainer::RegisterLocations( ParamEntryVal* pVal )
{


}
//-----------------------------------------------------------------------------
void WRContainer::RegisterItems( ParamEntryVal* pVal )
{

}
//-----------------------------------------------------------------------------
WRSpawnLocation* WRContainer::GetRandomLocation( void )
{
	// no endclass
	if( m_aLocations.Size() < 1 )
		return NULL;

	if( m_aLocations.Size() == 1 )
	{
		if( m_aLocations[0]->CanSpawn() )
			return m_aLocations[0];
	};

	int idx = GRandGen.RandomInt(0,m_aLocations.Size());
	for( int c = 0; c < m_aLocations.Size(); c++ )
	{
		if( m_aLocations[idx]->CanSpawn() )
			return m_aLocations[idx];
		
		if( (++idx) >= m_aLocations.Size() )
			idx = 0;
	};

	return NULL;
}
//-----------------------------------------------------------------------------
WRSpawnItem* WRContainer::GetRandomItem( void )
{
	// no endclass
	if( m_aItems.Size() < 1 )
		return NULL;

	if( m_aItems.Size() == 1 )
		return m_aItems[0];

	int idx = GRandGen.RandomInt(0,m_aItems.Size());
	return m_aItems[idx];
}
//-----------------------------------------------------------------------------
WRSpawnItem* WRContainer::GetItemByProbability( float fAdjustedProbability )
{
	// no endclass
	if( m_aItems.Size() < 1 )
		return NULL;

	// let decide probability we want to have
	float fRandGroup = 0.001f * fAdjustedProbability * (float)GRandGen.RandomInt(0,1000);
//	RptF("GetItemByProbability(%0.4f)",fRandGroup );

	// ignore... nothing to return at all
	if( fRandGroup < m_fGroupLow )
		return NULL;

//	for( int c = 0; c < m_aItems.Size(); c++ )
//	{
//		RptF(" %s %0.4f", m_aItems[c]->GetName(), m_aItems[c]->GetComplementaryProbability() );
//	};

	// find nearest one
	for( int c = 0; c < m_aItems.Size(); c++ )
	{
		if( m_aItems[c]->GetGroupProbability() >= (fRandGroup*fRandGroup) )
		{
			int size = m_aItems.Size() - c;
			if( size < 1 )
				return NULL;

			if( size == 1 )
			{
//				RptF(" >>>>> %s", m_aItems.Last()->GetName() );

				return m_aItems.Last();
			};

			int idx = GRandGen.RandomInt(c,m_aItems.Size());
//				RptF(" >>>> %s", m_aItems[idx]->GetName() );

			return m_aItems[idx];

			break;
		};
	};

	return NULL;
}
/*WRSpawnItem* WRContainer::GetItemByProbability( void )
{
	// no endclass
	if( m_aItems.Size() < 1 )
		return NULL;

	// let decide probability we want to have
	float fRand = 0.001f * (float)GRandGen.RandomInt(0,1000);

//	RptF("  fRand [%0.4f]", fRand );

	// selected something suitable
	int size = m_aItems.Size();
	for( int c = 0; c < size; c++ )
	{
		int idx = (m_iLastCheckIndex+c);
		if( idx >= size )
			idx -= size;

//		RptF("      : %s (%0.4f)",  m_aItems[idx]->GetName(), m_aItems[idx]->GetProbability() );

		if( (++m_iLastCheckIndex) >= size )
			m_iLastCheckIndex = 0;

		if( m_aItems[idx]->ShouldSpawn(fRand) )
		{
//		RptF("      OK %0.4f vs. %0.4f", m_aItems[idx]->GetProbability()*m_aItems[idx]->GetProbability(), fRand );

			return m_aItems[idx];
		};
	};

//		RptF("      FAIL" );
	return NULL;
}*/
//-----------------------------------------------------------------------------
WRSpawnItem* WRContainer::GetItem( const char* sItemName )
{
	RString n(sItemName);

	for( int c = 0; c < m_aItems.Size(); c++ )
	{
		if( n == m_aItems[c]->GetName() )
			return m_aItems[c];
	};

	return NULL;
}
//-----------------------------------------------------------------------------
void WRContainer::OnReset( void )
{
	for( int c = 0; c < m_aLocations.Size(); c++ )
		m_aLocations[c]->OnReset();
}
//-----------------------------------------------------------------------------
void WRContainer::Sort( void )
{
	Normalize();

	// sort by group probability
	for( int c = 0; c < m_aItems.Size(); c++ )
	{
		for( int d = (c + 1); d < m_aItems.Size(); d++ )
		{
			WRSpawnItem* a = m_aItems[c];
			WRSpawnItem* b = m_aItems[d];

			if( a->GetComplementaryProbability() > b->GetComplementaryProbability() )
			{
				m_aItems[c] = b;
				m_aItems[d] = a;
			};
		};
	};

}
//-----------------------------------------------------------------------------
void WRContainer::ExtentGroupProbability( float fProbability, bool bForceSet )
{
	if( bForceSet )
	{
		m_fGroupLow =
			m_fGroupHigh = fProbability;
	}
	else
	{
		if( fProbability < m_fGroupLow )
			m_fGroupLow = fProbability;
		else if( fProbability > m_fGroupHigh )
			m_fGroupHigh = fProbability;
	};
}
//-----------------------------------------------------------------------------
int WRContainer::GetItemLimit( void )
{
	int count = 0;
	for( int c = 0; c < m_aLocations.Size(); c++ )
		count += m_aLocations[c]->GetCount();
	return count;
}
//-----------------------------------------------------------------------------
WRSpawnLocation* WRContainer::FindLocation( const char* sName )
{
	for( int c = 0; c < m_aLocations.Size(); c++ )
	{
		if( m_aLocations[c]->CheckName(sName) )
			return m_aLocations[c];
	};

	return NULL;
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
WRContainer* WRSpawnClass::Add( const char* sName )
{
	WRContainer* spawn = new WRContainer();
	spawn->SetName(sName);

	m_aSpawn.Add(spawn);
	return spawn;
}
//-----------------------------------------------------------------------------
int WRSpawnClass::GetTotalLocationCount( void )
{
	int count = 0;
	for( int c = 0; c < m_aSpawn.Size(); c++ )
		count += m_aSpawn[c]->GetLocationCount();
	return count;
}
//-----------------------------------------------------------------------------
int WRSpawnClass::GetTotalItemsCount( void )
{
	int count = 0;
	for( int c = 0; c < m_aSpawn.Size(); c++ )
		count += m_aSpawn[c]->GetItemCount();
	return count;
}
//-----------------------------------------------------------------------------
int WRSpawnClass::GetTotalItemLimit( void )
{
	int count = 0;
	for( int c = 0; c < m_aSpawn.Size(); c++ )
		count += m_aSpawn[c]->GetItemLimit();
	return count;
}
//-----------------------------------------------------------------------------
void WRSpawnClass::ListToConsole( void )
{
	RptF("  %s (%d) Spawn classes (%d) Location setup (%d) Item setups (%d) Item limit", name.Data(), m_aSpawn.Size(), GetTotalLocationCount(), GetTotalItemsCount(), GetTotalItemLimit() );
	for( int c = 0; c < m_aSpawn.Size(); c++ )
		m_aSpawn[c]->ListToConsole();
}
//-----------------------------------------------------------------------------
WRSpawnItem* WRSpawnClass::GetItem( const char* sItemName )
{
	for( int c = 0; c < m_aSpawn.Size(); c++ )
	{
		WRSpawnItem* item = m_aSpawn[c]->GetItem(sItemName);
		if( item )
			return item;
	};

	return NULL;
}
//-----------------------------------------------------------------------------
WRContainer* WRSpawnClass::FindContainer( const char* sName )
{
	for( int c = 0; c < m_aSpawn.Size(); c++ )
	{
		if( m_aSpawn[c]->CheckName(sName) )
			return m_aSpawn[c];
	};

	return NULL;
}
//-----------------------------------------------------------------------------
WRSpawnLocation* WRSpawnClass::FindLocation( const char* sName )
{
	for( int c = 0; c < m_aSpawn.Size(); c++ )
	{
		WRSpawnLocation* loc = m_aSpawn[c]->FindLocation(sName);
		if( loc )
			return loc;
	};

	return NULL;
}
//-----------------------------------------------------------------------------
void WRSpawnClass::OnReset( void )
{
	for( int c = 0; c < m_aSpawn.Size(); c++ )
		m_aSpawn[c]->OnReset();
}
//-----------------------------------------------------------------------------
void WRSpawnClass::Sort( void )
{
	for( int c = 0; c < m_aSpawn.Size(); c++ )
		m_aSpawn[c]->Sort();
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
WRSpawnClass* WRLootSpawns::Add( const char* sName )
{
	WRSpawnClass* spawn = new WRSpawnClass();
	spawn->SetName(sName);

	m_aSpawnClass.Add(spawn);
	return spawn;
}
//-----------------------------------------------------------------------------
WRSpawnClass* WRLootSpawns::FindByName( const char* sName )
{
	for( int c = 0; c < m_aSpawnClass.Size(); c++ )
	{
		if( m_aSpawnClass[c]->CheckName(sName) )
			return m_aSpawnClass[c];
	};

	return NULL;
}
//-----------------------------------------------------------------------------
void WRLootSpawns::ListToConsole( void )
{
	RptF("  (%d) Spawn group", m_aSpawnClass.Size() );
	for( int c = 0; c < m_aSpawnClass.Size(); c++ )
		m_aSpawnClass[c]->ListToConsole();
}
//-----------------------------------------------------------------------------
WRSpawnItem* WRLootSpawns::GetItem( const char* sItemName )
{
	for( int c = 0; c < m_aSpawnClass.Size(); c++ )
	{
		WRSpawnItem* item = m_aSpawnClass[c]->GetItem(sItemName);
		if( item )
			return item;
	};

	return NULL;
}
//-----------------------------------------------------------------------------
void WRLootSpawns::Sort( void )
{
	for( int c = 0; c < m_aSpawnClass.Size(); c++ )
		m_aSpawnClass[c]->Sort();
}
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
WRLoot::WRLoot() :
	m_pGroupName(NULL),
	m_pContainerName(NULL),
	m_pProxyName(NULL),
	m_pHierarchy(NULL),
	m_pSpawns(NULL)
{
	m_pGroupName = new WRStringPool();
	m_pContainerName = new WRStringPool();
	m_pProxyName = new WRStringPool();

	m_pHierarchy = new WRClassHierarchy();
//	m_pHierarchy->SetName("ROOT_CLASS_HIERARCHY");
	m_pSpawns = new WRLootSpawns();

}
//-----------------------------------------------------------------------------
WRLoot::~WRLoot()
{
	delete m_pGroupName;
	delete m_pContainerName;
	delete m_pProxyName;

	delete m_pHierarchy;
	delete m_pSpawns;

}
//-----------------------------------------------------------------------------
bool WRLoot::SaveToFile( const char* sFileName )
{
  QOFStream out;
  out.open(sFileName);
  if (out.fail())
  {
    RptF("!!! Failed to save Proxy data to file \"%s\" - failed to open the file.", cc_cast(sFileName));
    return false;
  }

  SerializeBinStream bs(&out);

  Save(&bs);

  out.close();

  return true;
}
//-----------------------------------------------------------------------------
bool WRLoot::LoadFromFile( const char* sFileName )
{
  QIFStream in;
  in.open(sFileName);
  if (in.fail())
  {
    RptF("!!! Failed to load Proxy data to file \"%s\" - failed to open the file.", cc_cast(sFileName));
    return false;
  }

  SerializeBinStream bs(&in);

  Load(&bs);

  in.close();

  return true;
}
//-----------------------------------------------------------------------------
bool WRLoot::Save( SerializeBinStream* pStream )
{
	pStream->SaveInt(s_iCONTENT_FILE_VERSION);

	pStream->SaveString(s_sBINPROXY_FILEHEADER);

	if( !m_pHierarchy->Save(pStream) )
		return false;

	if( !m_pGroupName->Save(pStream) )
		return false;

	if( !m_pContainerName->Save(pStream) )
		return false;

	if( !m_pProxyName->Save(pStream) )
		return false;

	pStream->SaveInt(m_aGroup.Size());
	for( int c = 0; c < m_aGroup.Size(); c++ )
		m_aGroup[c]->Save(this,pStream);

	return true;
}
//-----------------------------------------------------------------------------
bool WRLoot::Load( SerializeBinStream* pStream )
{
	int ver = pStream->LoadInt();
	if( ver != s_iCONTENT_FILE_VERSION )
	{
		RptF("!!! Failed to load Proxy data file - version %d, expected %d... Please regenerate!",ver,s_iCONTENT_FILE_VERSION);
		return false;
	};

	RString header = pStream->LoadString();
	if( !(header == s_sBINPROXY_FILEHEADER) )
	{
		RptF("!!! Failed to load Proxy data file - Invalid format!");
		return false;
	};

	if( !m_pHierarchy->Load(this,pStream) )
	{
		RptF("!!! Failed to load Proxy data file - Structure fail!");
		return false;
	};

	if( !m_pGroupName->Load(pStream) )
	{
		RptF("!!! Failed to load Proxy data file - Structure fail!");
		return false;
	};

	if( !m_pContainerName->Load(pStream) )
	{
		RptF("!!! Failed to load Proxy data file - Structure fail!");
		return false;
	};

	if( !m_pProxyName->Load(pStream) )
	{
		RptF("!!! Failed to load Proxy data file - Structure fail!");
		return false;
	};


/*------*/		GetHierarchy()->ListToConsole();

	int count = pStream->LoadInt();
	if( count > 0 )
	{
		m_aGroup.Realloc(count);
		for( int c = 0; c < count; c++ )
		{
			WRLoot_Group* group = new WRLoot_Group();
			group->Load(this,pStream);

			m_aGroup.Add(group);
		};
	};

	return true;
}
//-----------------------------------------------------------------------------
void WRLoot::TestRange( WRLoot_Point* pPoint, Vector3Par vCenter, float fMaxRange )
{
	Vector3 e = vCenter;

	float below = 0.0f, above = 0.0f;

	// test distance below
	e[1] -= PROXY_TEST_BELOW;

	float dx, dz;
	float elevation = GLandscape->RoadSurfaceY(vCenter, &dx, &dz);

	CollisionBuffer col1;
	GLandscape->ObjectCollisionLine(0,col1,Landscape::FilterNone(),vCenter,e,0.0f,ObjIntersectFire,ObjIntersectView,CF_NEAREST_CONTACT);
	if( col1.Size() > 0 )
	{
#ifdef USER_raist
		if( elevation > col1[0].pos.Y() )
		{
			below = vCenter.Y() - elevation;
			pPoint->SetOrigin(Vector3(col1[0].pos.X(),elevation,col1[0].pos.Z())); // override position
		}
		else
		{
			below = vCenter.Y() - col1[0].pos.Y();
			pPoint->SetOrigin(col1[0].pos); // override position
		};
#else
		below = vCenter.Y() - col1[0].pos.Y();

		pPoint->SetOrigin(col1[0].pos); // override position
#endif

		if( col1[0].surface )
		{
			if( col1[0].surface->_isWater )
				pPoint->SetFlags(WRLoot_Proxy::EPROXY_WATER);

	/*	}
		else
		{
			if( col1[0].object )
			{
				ObjectType type;

				if( col1[0].object->GetType() )
					pProxy->SetFlags(WRLoot_Proxy::EPROXY_INDOOR);
			};*/
		};

	}
	else if( (vCenter.Y()-elevation) < PROXY_TEST_BELOW )
	{
		Vector3 npos(vCenter.X(),elevation,vCenter.Z());

		pPoint->SetOrigin(npos); // override position

		below = vCenter.Y() - elevation;
	}
	else
	{
		below = PROXY_TEST_BELOW;
	};

	// test distance above
	e = vCenter;
	e[1] += PROXY_TEST_ABOVE;

	CollisionBuffer col2;
	GLandscape->ObjectCollisionLine(0,col2,Landscape::FilterNone(),vCenter,e,0.0f,ObjIntersectFire,ObjIntersectView,CF_NEAREST_CONTACT);
	if( col2.Size() > 0 )
		above = col2[0].pos.Y() - vCenter.Y();
	else
		above = PROXY_TEST_ABOVE;

	float height = above + below;
	pPoint->SetHeight(height);

	// to small space for use no need to test anything else
/*	if( height < MIN_PROXY_HEIGHT )
	{
		pProxy->SetFlags(WRLoot_Proxy::EPROXY_INCONVENIENT);
		return;
	};*/

#ifdef OUTPUT_AREATEST

	// test spiral range
	float spiralMaxRange = 2.0f; // radius (meters)
	float spiralPi = 3.0*M_PI; // how many PIs spiral takes before reached outter range
	float spiralInner = 7.0f; // steps performed before reached max range
	float spiralCircle = 4.0f; // steps performed on circle outside spiral
	
	float circlePi = M_PI; // circle length
	float circleRange = 0.0f; // determined circle range

	float result = 0.0f;

	// process inner spiral
	float count = 0.0f;
	float stepIn = spiralPi / spiralInner;
	for( float sIn = stepIn; sIn < spiralPi; sIn += stepIn )
	{
		float current = below;

		circleRange = spiralMaxRange * (sIn/spiralPi); // we want to create spiral test from center

		Vector3 eee(vCenter);

		eee[0] += circleRange*sinf(sIn);
		eee[1] -= 3.0f;
		eee[2] += circleRange*cosf(sIn);

		CollisionBuffer col;
		GLandscape->ObjectCollisionLine(0,col,Landscape::FilterNone(),vCenter,eee,0.0f,ObjIntersectFire,ObjIntersectView,CF_NEAREST_CONTACT);
		if( col.Size() > 0 )
			current = vCenter.Y() - col[0].pos.Y();

		result = (sIn==0.0f) ? current : (current+result);

		if( result < current || result > current )
		{
			break;
		};

		count += 1.0f;
	};

	// not even close step worked out
	if( circleRange <= 0.0f || count < 0.0f )
	{
		pProxy->SetFlags(WRLoot_Proxy::EPROXY_INCONVENIENT);
		return;
	};

	// process circle test
	float stepOut = circlePi / spiralCircle;
	for( float sOut = 0.0f; sOut < circlePi; sOut += stepIn )
	{
		float current = below;

		Vector3 eee(vCenter);

		eee[0] += circleRange*sinf(sOut);
		eee[1] -= 3.0f;
		eee[2] += circleRange*cosf(sOut);

		CollisionBuffer col;
		GLandscape->ObjectCollisionLine(0,col,Landscape::FilterNone(),vCenter,eee,0.0f,ObjIntersectFire,ObjIntersectView,CF_NEAREST_CONTACT);
		if( col.Size() > 0 )
			current = vCenter.Y() - col[0].pos.Y();

		result = (sOut==0.0f) ? current : (current+result);

		if( result < current || result > current )
		{
			break;
		};

	};

	// result
	result /= count;

	// calculate range
	pProxy->SetRange(result);

	RptF(" xxtest %0.4f %0.4f - %0.4f", below, result, circleRange );

#endif

	// calculate range
	pPoint->SetRange(0.0f);

}
//-----------------------------------------------------------------------------
WRLoot_Group* WRLoot::InsertGroup( const char* sName,  Object* pObject )
{
	if( !pObject )
		return NULL;

	LODShape* lShape = pObject->GetShape();
	if( !lShape )
		return NULL;

//#ifdef USER_raist
//	RString name = "Land_fortified_nest_big";
//	if( name == sName )
//		lShape = lShape;

//#endif

	const Shape* memory = lShape->MemoryLevel();
	if( !memory )
		return NULL;

	WRLoot_Group* group = NULL;

#ifdef OUTPUT_PROXYEXPORT_CONSOLE
	RptF("----- building [%s] ----", pObject->GetDisplayName().Data() );
#endif

	// check all selections
	for( int selIndex = 0; selIndex < memory->NNamedSel(); selIndex++ )
	{
		const NamedSelection& sel = memory->NamedSel(selIndex);

		// check if selection exist
		int cIdx = m_pContainerName->GetIndex(sel.GetName());
		if( cIdx == -1 )
			continue;

#ifdef OUTPUT_PROXYEXPORT_CONSOLE
		RptF(".. selection name %s.", sel.GetName().Data() );
#endif

		// create new group
		if( group == NULL )
		{
			group = new WRLoot_Group();

			int gId = m_pGroupName->GetIndex(sName);
			group->SetName(m_pGroupName->GetString(gId));

			m_aGroup.Add(group);
		};

		// parse through memory LOD structure
		WRLoot_Container* container = group->InsertContainer();
		container->SetName(m_pContainerName->GetString(cIdx));

/*				RString ttx = "variouscoffetable";

				if( sel.GetName() == ttx )
				{
					selIndex = selIndex;

					int is = sel.NSections();
					is = is;
				};*/


		// no faces... use memory points
		if( sel.Faces().Size() == 0 )
		{
			IndexSelection _sel;
			sel.CopyVertexData(_sel);

			for( int fi = 0; fi < sel.Size(); fi++ )
			{
				int idx = sel[fi];

				Vector3Val val = memory->Pos(idx);

				Matrix4 worldTrans = pObject->FutureVisualState().ModelToWorld();
				Vector3 npos = worldTrans.FastTransform(val);

				// add proxy
				WRLoot_Point* pp = container->InsertPoint();

				group->SetOrigin(worldTrans.Position());

				pp->SetOrigin( npos );
//				pp->SetDirection( VForward );
//				pp->SetDirUp( VUp );
				pp->SetFlags(WRLoot_Proxy::EPROXY_POINT);
				pp->SetName(NULL);

				TestRange(pp,npos,MAX_PROXY_RANGE);


			};
/*
			for( int fi = 0; fi < memory->NPoints(); fi++ )
			{

				Vector3Val val = memory->Pos(fi);

				Matrix4 worldTrans = pObject->FutureVisualState().ModelToWorld();
				Vector3 npos = worldTrans.FastTransform(val);

				// add proxy
				WRLoot_Proxy* px = container->InsertProxy();

				group->SetOrigin(worldTrans.Position());

				px->SetName(NULL);
				px->SetOrigin( npos );
				px->SetDirection( VForward );
				px->SetDirUp( VUp );
				px->SetFlags(WRLoot_Proxy::EPROXY_POINT);

			};*/

		}
		else
		{

			// grab the selection and find a proxy in it
			for( int fi = 0; fi < sel.Faces().Size(); fi++ )
			{
				VertexIndex fSel = sel.Faces()[fi];

				// proxies
				for( int pi = 0; pi < memory->NProxies(); pi++ )
				{

					const ProxyObject& proxy = memory->Proxy(pi);
					const NamedSelection::Access& proxySel = memory->NamedSel(proxy.selection,lShape);

					if( proxySel.Faces().Size() == 1 ) // proxy should always be one face
					{
						VertexIndex pSel = proxySel.Faces()[0];
						if( fSel == pSel )
						{
							Matrix4 worldTrans = pObject->FutureVisualState().ModelToWorld()*proxy.GetTransform();

							// add proxy
							WRLoot_Proxy* px = container->InsertProxy();
							RString* str = m_pProxyName->AddStringWithoutSlash(proxy.name.Data());

							group->SetOrigin(worldTrans.Position());
						
							px->SetName(str);
							px->SetOrigin(worldTrans.Position());
							px->SetDirection(worldTrans.Direction());
							px->SetDirUp(worldTrans.DirectionUp());

//							TestRange(px,npos,MAX_PROXY_RANGE);

						};
					};

				};

			};

		};


	};

	// none suitable selections found
	if( group == NULL )
		return NULL;

	int badProxies = group->HowManyProxiesUseFlag(WRLoot_Proxy::EPROXY_INCONVENIENT);
	if( badProxies > 0 )
	{
		RptF("!!! Group named: \"%s\" contain %d poorly placed proxies of %d.", sName, badProxies, group->GetProxyCount() );
	};

#ifdef OUTPUT_PROXYEXPORT_CONSOLE
	RptF(".. containers %d.", group->GetCount() );
	for( int c = 0; c < group->GetCount(); c++ )
		RptF("  [%d] proxies %d.", c, group->GetContainer(c)->GetCount() );
#endif

	// result
	return group;
}
//-----------------------------------------------------------------------------
int WRLoot::GetContainerCount( void ) const
{
	int count = 0;
	for( int c = 0; c < m_aGroup.Size(); c++ )
	{
		count += m_aGroup[c]->GetCount();
	};

	return count;
}
//-----------------------------------------------------------------------------
int WRLoot::GetProxyCount( void ) const
{
	int count = 0;
	for( int c = 0; c < m_aGroup.Size(); c++ )
		count += m_aGroup[c]->GetProxyCount();

	return count;
}
//-----------------------------------------------------------------------------
int WRLoot::GetPointCount( void ) const
{
	int count = 0;
	for( int c = 0; c < m_aGroup.Size(); c++ )
		count += m_aGroup[c]->GetPointCount();

	return count;
}
//-----------------------------------------------------------------------------
int WRLoot::GetTotalCount( void ) const
{
	int count = 0;
	for( int c = 0; c < m_aGroup.Size(); c++ )
		count += m_aGroup[c]->GetTotalCount();

	return count;
}
//-----------------------------------------------------------------------------
int WRLoot::GetBadProxyCount( void ) const
{
	int count = 0;
	for( int c = 0; c < m_aGroup.Size(); c++ )
		count += m_aGroup[c]->HowManyProxiesUseFlag(WRLoot_Proxy::EPROXY_INCONVENIENT);

	return count;
}
//-----------------------------------------------------------------------------
int WRLoot::GetProxyCountWithingRange( Vector3Par vPos, float fDistance )
{
	int count = 0;
	for( int c = 0; c < m_aGroup.Size(); c++ )
	{
		for( int d = 0; d < m_aGroup[c]->GetCount(); d++ )
		{
			if( m_aGroup[c]->Distance(vPos,fDistance) )
				count += m_aGroup[c]->GetContainer(d)->GetTotalCount();
		};
	};

	return count;
}
//-----------------------------------------------------------------------------
WRSpawnItem* WRLoot::GetItem( const char* sItemName )
{
	return m_pSpawns ? m_pSpawns->GetItem(sItemName) : NULL;
}
//-----------------------------------------------------------------------------
void WRLoot::ListToConsole( void )
{
#ifdef USER_raist
	RptF("[&] Hierarchy:" );
	m_pHierarchy->ListToConsole();
#endif
//	RptF("[&] Hierarchy (Indexed):" );
//	m_pHierarchy->ListToConsoleIndexed();*/

#ifdef USER_raist
	RptF("[&] Endclass List:");
	for( int c = 0; c < GetUniqueSpawnableClassCount(); c++ )
	{
		WRClassHierarchy* cls = m_pHierarchy->GetEndClass(c);
		if( cls )
			RptF("  %s", cls->GetName() );
	};
#endif


	RptF("[&] Content Statistics:" );
	RptF("  Total Groups == %d.", GetCount() );
	RptF("   Total Containers == %d.", GetContainerCount() );
	RptF("    Total Points == %d.", GetPointCount() );
	RptF("    Total Proxy == %d.", GetProxyCount() );
	RptF("    Bad Proxy == %d.", GetBadProxyCount() );
	RptF("  Unique Group Names == %d.", GetUniqueGroupNameCount() );
	RptF("  Unique Container Names == %d.", GetUniqueContainerNameCount() );
	RptF("  Unique Proxy Names == %d.", GetUniqueProxyNameCount() );
	RptF("  Unique Spawnable Classes == %d.", GetUniqueSpawnableClassCount() );

}
//-----------------------------------------------------------------------------
void WRLoot::RegisterGroups( void )
{
	RString locName = "locations";
	RString dontUse = "HouseBase";
	RString className = "Spawns";

	ParamEntryPtr par = Pars.FindEntry(s_sCONFIGLOOT);
	if( !par )
	{
		RptF("!!! Failed to load \"%s\" - No loot can be spawned.",s_sCONFIGLOOT);
		return;
	};

	for( int parCount = 0; parCount < par->GetEntryCount(); parCount++ )
	{
		ParamEntryVal v = par->GetEntry(parCount);
		if( v.GetClassInterface() )
		{
			const ParamClass* parent = v.GetClassInterface()->GetParent();
			if( parent && parent->IsClass() )
			{
				ParamEntryPtr sPar = v.GetClassInterface()->FindEntry(className);
				if( sPar )
				{
					for( int sCount = 0; sCount < sPar->GetEntryCount(); sCount++ )
					{
						ParamEntryVal spawns = sPar->GetEntry(sCount);
						for( int innerCount = 0; innerCount < spawns.GetEntryCount(); innerCount++ )
						{
							ParamEntryVal iPar = spawns.GetEntry(innerCount);
							if( iPar.GetName() == locName )
							{
								if( !(v.GetName()==dontUse) )
									m_pGroupName->AddString(v.GetName().Data());
							};					

						};

					};
				};

			};
		};

	};

}
//-----------------------------------------------------------------------------
void WRLoot::RegisterSetup( void )
{

	if( m_pSpawns )
	{
		delete m_pSpawns;
		m_pSpawns = new WRLootSpawns();
	};

	RString locName = "locations";
	RString itemName = "items";
	RString className = "Spawns";

	ParamEntryPtr par = Pars.FindEntry(s_sCONFIGLOOT);
	if( !par )
	{
		RptF("!!! Failed to load \"%s\" - No loot can be spawned.",s_sCONFIGLOOT);
		return;
	};

	for( int parCount = 0; parCount < par->GetEntryCount(); parCount++ )
	{
		ParamEntryVal v = par->GetEntry(parCount);
		if( v.GetClassInterface() )
		{
			const ParamClass* parent = v.GetClassInterface()->GetParent();
			if( parent && parent->IsClass() )
			{
				ParamEntryPtr sPar = v.GetClassInterface()->FindEntry(className);
				if( sPar )
				{
					WRSpawnClass* spClass = m_pSpawns->Add(v.GetName().Data());

					for( int sCount = 0; sCount < sPar->GetEntryCount(); sCount++ )
					{
						ParamEntryVal spawns = sPar->GetEntry(sCount);

						WRContainer* sp = NULL;

						for( int innerCount = 0; innerCount < spawns.GetEntryCount(); innerCount++ )
						{
							ParamEntryVal iPar = spawns.GetEntry(innerCount);
							if( iPar.GetName() == locName )
							{
								if( sp == NULL )
								{
									sp = spClass->Add(spawns.GetName());
								};

								sp->RegisterLocations(NULL);

								// -------------------
								if( iPar.IsArray() )
								{
									for( int size = 0; size < iPar.GetSize(); size++ )
									{
										const IParamArrayValue& x = iPar[size];

										if( x.IsArrayValue() )
										 {
											WRSpawnLocation* loc = NULL;

											 for( int inSize = 0; inSize < x.GetItemCount(); inSize++ )
											 {
												const IParamArrayValue& y = *x.GetItem(inSize);

												 if( y.IsTextValue() )
												 {
													RString str = y.GetValueRaw();
													str.Lower();

													loc = sp->AddLocation();
													loc->SetName(str.Data());

													// register also container name
													m_pContainerName->AddString(str.Data());

												 }
												 else if( y.IsFloatValue() || y.IsIntValue() )
												 {
													float f = y.GetFloat();

													if( loc )
														loc->SetCount((int)f);
												 };


											 };

										 };


									};
    
								};
								// -------------------


							}
							else if( iPar.GetName() == itemName )
							{
								if( sp == NULL )
								{
									sp = spClass->Add(spawns.GetName());
								};

								sp->RegisterItems(NULL);

								// -------------------
								if( iPar.IsArray() )
								{
									for( int size = 0; size < iPar.GetSize(); size++ )
									{
										const IParamArrayValue& x = iPar[size];
										if( x.IsArrayValue() )
										{
											bool bName = false;
											bool bGroupProbability = false;
											bool bItemProbability = false;

											WRSpawnItem* itm = NULL;

											 for( int inSize = 0; inSize < x.GetItemCount(); inSize++ )
											 {
												const IParamArrayValue& y = *x.GetItem(inSize);

												if( y.IsTextValue() )
												{
													if( !bName )
													{
														RString str = y.GetValueRaw();

														WRClassHierarchy* cls = m_pHierarchy->FindClass(str.Data());
														if( cls == NULL )
														{
															RptF("!!! %s :: Unknown class name \"%s\"",s_sCONFIGLOOT,str.Data());
															break;
														};

														itm = sp->AddItem();
														itm->SetClass(cls);
														itm->SetName(str.Data());

														bName = true;
													};
												}
												else if( y.IsFloatValue() || y.IsIntValue() )
												{
													float f = y.GetFloat();

													if( itm )
													{
														if( !bGroupProbability )
														{
															itm->SetGroupProbability(f);

															bGroupProbability = true;
														}
														else if( !bItemProbability )
														{
															itm->SetItemProbability(f);

															bItemProbability = true;
														};
													};
												};

											 };

										 }
										else
										{
											size = size;
										};

									};
    
								};
								// -------------------


							};

						};

					};

				};

			};
		};

	};

	// process sort
	m_pSpawns->Sort();

/*#ifdef USER_raist
	RptF("[&] Spawn Setup:");
	m_pSpawns->ListToConsole();
#endif*/

}
//-----------------------------------------------------------------------------
void WRLoot::RegisterLoot( bool bFull )
{

	// parse config data && register all spawnable classes for further usage
	if( bFull )
	{
		GetHierarchy()->RegisterLoot(this,CLASS_LEVEL_DEEPNESS);
		GetHierarchy()->CheckForDuplicities();
	};

	GetHierarchy()->BuildEndClass();

}
//-----------------------------------------------------------------------------
void WRLoot::GetBoundaries( float& fX1, float& fZ1, float& fX2, float& fZ2 )
{
	for( int c = 0; c < GetCount(); c++ )
		m_aGroup[c]->GetBoundaries(fX1,fZ1,fX2,fZ2);
}
//-----------------------------------------------------------------------------

