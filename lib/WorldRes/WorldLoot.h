
#ifndef WR_LOOT
	#define WR_LOOT

#include <El/Math/math3dP.hpp>
#include <Es/Strings/rString.hpp>

class WRLoot;
class Object;
class GameState;

class ParamEntryVal;

class SerializeBinStream;

class WRLoot_Container;
class WRLoot_Group;

class WRContainer;

//! Hierarchy (sorting) helper
class WRClassHierarchy
{
protected:

	static const int CONSOLEOUTPUT_INDENT_MAX = 256;
	static const int CONSOLEOUTPUT_INDENT_STEP = 1;

	WRClassHierarchy* m_pParent; ///< Parent

	bool m_bObsolete; ///< Mark obsolete classes (will not be used)

	int m_iFlags; ///< Class flags

	int m_iLevel; ///< Level of branch
	int m_iIndex; ///< Index (virtual number, technically it's count of item globally allocated)

	Vector3 m_vBBox[2]; ///< Bounding box
	float m_fBSphere; ///< Bounding sphere

	//! Build hierarchy level
	void BuildContentHiearchy( WRLoot* pWRLoot, int iLevels, int iRootLevel );

	//! Check if any duplicate classes somewhere in hierarchy
	int CheckForDuplicities( WRClassHierarchy* pHierarchy );

	AutoArray<RString*> m_aAttachments; ///< Attachment names

	//! Add base attachment
	void AddAttachment( const char* sName );

	AutoArray<RString*> m_aCargo; ///< Cargo names

	//! Add cargo content
	void AddCargo( const char* sName );

public:

	enum EClassHFlag
	{
		/*
		edit: rocket 21/11/2013, added rotateZ in middle
		*/
		ECH_NONE				= 0,
		ECH_ROTATEDROPPED		= 1,
		ECH_ROTATEDROPPEDY		= 2,
		ECH_ROTATEDROPPEDZ		= 4,
		ECH_ROTATEALWAYS		= 8,
		ECH_ROTATENEVER			= 16,
		ECH_ROTATEINVERT		= 32
	};

	RString* m_pName;

	AutoArray<WRClassHierarchy*> m_aItem; ///< Item definition
	AutoArray<WRClassHierarchy*> m_aIndexed; ///< Indexed item array
	AutoArray<WRClassHierarchy*> m_aEndClass; ///< Contained end classes (no hierarchy)

	WRClassHierarchy() :
		m_pParent(NULL),
		m_bObsolete(false),
		m_iFlags(ECH_NONE),
		m_iLevel(0),
		m_iIndex(0),
		m_fBSphere(0.0f),
		m_pName(NULL)
	{
		m_vBBox[0].Zero();
		m_vBBox[1].Zero();
	};
	~WRClassHierarchy();

	void SetName( const char* sName )
	{
		if( m_pName )
			delete m_pName;

		m_pName = new RString(sName);
	};
	const char* GetName( void ) const
		{ return m_pName ? m_pName->Data() : NULL; };

	int GetLevel( void ) const
		{ return m_iLevel; };
	void SetLevel( int iLevel )
		{ m_iLevel = iLevel; };

	int GetIndex( void ) const
		{ return m_iIndex; };
	void SetIndex( int iIndex )
		{ m_iIndex = iIndex; };

	//! Add item
	void Add( WRClassHierarchy* pItem );
	//! Add indexed item
	void AddToIndex( WRClassHierarchy* pItem );
	//! Add end class item
	void AddToEndClass( WRClassHierarchy* pItem );

	//! Build hierarchy level
	void RegisterLoot( WRLoot* pWRLoot, int iLevels );
	//! Build end class arrays
	void BuildEndClass( WRClassHierarchy* pParent = NULL );

	//! Output indexed statistics
	void ListToConsoleIndexed( void );
	//! Output statistics
	void ListToConsole( int iIndent = 0 );

	//! Find class/root (not directly)
	WRClassHierarchy* FindClassByIndex( int iIndex );
	//! Get class/root (get class directly by index)
	WRClassHierarchy* GetClassByIndex( int iIndex );

	//! Find class by name
	WRClassHierarchy* FindClass( const char* sName );
	//! Count class appearances in hierarchy
	int CountClass( const char* sName );

	//! Check if class is children of defined class
	bool IsChildOf( const char* sName );

	//! Set parent
	void SetParent( WRClassHierarchy* pParent )
		{ m_pParent = pParent; };
	//! Get parent
	WRClassHierarchy* GetParent( void )
		{ return m_pParent; };

	//! Check if any duplicate classes somewhere in hierarchy
	int CheckForDuplicities( void );

	//! Save data
	bool Save( SerializeBinStream* pStream );
	//! Load data
	bool Load( WRLoot* pWRLoot, SerializeBinStream* pStream );

	//! Load indexed data
	void PostLoad( void );

	//! Count end classes only (last level of hierarchy ei. class with no children)
	int GetEndClassCount( void );

	//! Check if class is end class
	bool IsEndClass( void ) const
		{ return m_aItem.Size() < 1; };

	//! Get first end class you can find
	WRClassHierarchy* GetFirstEndClass( void );
	//! Get random end class
	WRClassHierarchy* GetRandomEndClass( void );
	//! Get end class by index
	WRClassHierarchy* GetEndClass( int iIndex );

	void SetFlags( int iFlags )
		{ m_iFlags |= iFlags; };
	void ClearFlags( int iFlags )
		{ m_iFlags &= ~iFlags; };
	int GetFlags( void ) const
		{ return m_iFlags; };
	bool CheckFlags( int iFlags ) const
		{ return (m_iFlags&iFlags)!=0; };

	//! Check if class has attachments
	bool HasAttachments( void ) const
		{ return (m_aAttachments.Size()>0); };
	//! Get attachments (or from any parent)
	AutoArray<RString*>& GetAttachments( void );

	//! Check if class has cargo
	bool HasCargo( void ) const
		{ return (m_aCargo.Size()>0); };
	//! Get cargo (or from any parent)
	AutoArray<RString*>& GetCargo( void );

	void SetBBox( Vector3Par v1, Vector3Par v2 )
	{
		m_vBBox[0] = v1;
		m_vBBox[1] = v2;
	};
	void SetBSphere( float fSphere )
		{ m_fBSphere = fSphere; };

	Vector3Par GetBBox( int iIdx ) const
		{ return m_vBBox[iIdx]; };
	float GetBSphere( void ) const
		{ return m_fBSphere; };

};



//! String pool object (manage to hold unique string instances to avoid mass duplication)
class WRStringPool
{
private:

	AutoArray<RString*> m_aPool;

public:

	WRStringPool() {};
	~WRStringPool() {};

	//! Add string to pool
	RString* AddString( const char* sString );
	//! Add string from last slash in path (for example: "\dz\data\proxies\spawnitemmedium" -> "spawnitemmedium")
	RString* AddStringWithoutSlash( const char* sString );

	//! Get string
	RString* GetString( int iID )
		{ return m_aPool[iID]; };

	//! Get index of string
	int GetIndex( const char* sString );
	//! Get index of string
	int GetIndex( RString* pString );
	//! Get index of string
	int GetIndex( const RString& pString );

	//! Save data
	bool Save( SerializeBinStream* pStream );
	//! Load data
	bool Load( SerializeBinStream* pStream );

	int GetCount( void ) const
		{ return m_aPool.Size(); };

};


//! Memory point
class WRLoot_Point
{
protected:

	RString* m_pName;

	WRLoot_Container* m_pParent; ///< Parent container

	Vector3 m_vOrigin;

	float m_fRange; ///< Radius of range (that should be safe for spawn of object)
	float m_fHeight; ///< Height for spawn

	int m_iFlags;

public:

	enum EWRLoot_ProxyFlags
	{
		EPROXY_FREE				= 0,
		EPROXY_OCCUPIED			= 1,
		EPROXY_POINT			= 2,
		EPROXY_INCONVENIENT		= 4,
		EPROXY_INDOOR			= 8,
		EPROXY_WATER			= 16,
	};

	WRLoot_Point( WRLoot_Container* pParent ) :
		m_pParent(pParent),
		m_pName(NULL),
		m_fRange(0.0f),
		m_fHeight(0.0f),
		m_iFlags(EPROXY_FREE)
	{
	};
	WRLoot_Point( WRLoot_Container* pParent, int iFlags ) :
		m_pParent(pParent),
		m_pName(NULL),
		m_fRange(0.0f),
		m_fHeight(0.0f),
		m_iFlags(iFlags)
	{
	};

	WRLoot_Container* GetParent( void )
		{ return m_pParent; };

	//! Save data
	virtual bool Save( WRLoot* pWRLoot, SerializeBinStream* pStream );
	//! Load data
	virtual bool Load( WRLoot* pWRLoot, SerializeBinStream* pStream );

	int GetFlags( void ) const
		{ return m_iFlags; };
	void SetFlags( int iFlags )
		{ m_iFlags |= iFlags; };
	void ClearFlags( int iFlags )
		{ m_iFlags &= ~iFlags; };

	bool CheckFlags( int iFlags ) const
		{ return ((m_iFlags&iFlags)!=0); };

	bool HasFlags( int iFlags ) const
	{
		if( iFlags == EPROXY_FREE )
			return ((m_iFlags&EPROXY_OCCUPIED)==0);

		return ((m_iFlags&iFlags)!=0); 
	};

	void SetOrigin( Vector3Par vVector )
		{ m_vOrigin = vVector; };
	virtual void SetDirection( Vector3Par vVector )
		{};
	virtual void SetDirUp( Vector3Par vVector )
		{};


	Vector3Par GetOrigin( void ) const
		{ return m_vOrigin; };
	virtual Vector3Par GetDirection( void ) const
		{ return VForward; };
	virtual Vector3Par GetDirUp( void ) const
		{ return VUp; };

	//! Check if within distance
	bool Distance( Vector3Par vPos, float fDistance ) const
		{ return m_vOrigin.DistanceXZ2(vPos)<Square(fDistance); };

	const char* GetName( void ) const
		{ return m_pName ? m_pName->Data() : NULL; };
	void SetName( RString* pName )
		{ m_pName = pName; };

	//! Check if proxy is just a point
	bool IsPoint( void ) const
		{ return (m_iFlags&EPROXY_POINT)!=0; };

	float GetRange( void ) const
		{ return m_fRange; };
	void SetRange( float fRange )
		{ m_fRange = fRange; };
	bool CheckRange( float fRange )
		{ return (m_fRange>=fRange); };

	float GetHeight( void ) const
		{ return m_fHeight; };
	void SetHeight( float fHeight )
		{ m_fHeight = fHeight; };
	bool CheckHeight( float fHeight )
		{ return (m_fHeight>=fHeight); };

};

//! Proxy data
class WRLoot_Proxy : public WRLoot_Point
{
protected:

	Vector3 m_vDirection;
	Vector3 m_vDirUp;

public:

	WRLoot_Proxy( WRLoot_Container* pParent ) :
	  WRLoot_Point(pParent)
	{
	};
	WRLoot_Proxy( WRLoot_Container* pParent, int iFlags ) :
		WRLoot_Point(pParent,iFlags)
	{
	};

	//! Save data
	virtual bool Save( WRLoot* pWRLoot, SerializeBinStream* pStream );
	//! Load data
	virtual bool Load( WRLoot* pWRLoot, SerializeBinStream* pStream );

	virtual void SetDirection( Vector3Par vVector )
		{ m_vDirection = vVector; };
	virtual void SetDirUp( Vector3Par vVector )
		{ m_vDirUp = vVector; };

	virtual Vector3Par GetDirection( void ) const
		{ return m_vDirection; };
	virtual Vector3Par GetDirUp( void ) const
		{ return m_vDirUp; };

};


//! Type of proxy (container)
class WRLoot_Container
{
private:

	RString* m_pName;

	AutoArray<WRLoot_Point*> m_aProxy; ///< Proxy list

	WRLoot_Group* m_pParent; ///< Parent group

public:

	WRLoot_Container( WRLoot_Group* pParent ) :
		m_pParent(pParent)
	{
	};

	WRLoot_Group* GetParent( void )
		{ return m_pParent; };

	const char* GetName( void ) const
		{ return m_pName->Data(); };
	void SetName( RString* pName )
		{ m_pName = pName; };

	bool CheckName( const char* sName )
		{ return (*m_pName==sName); };

	//! Save data
	bool Save( WRLoot* pWRLoot, SerializeBinStream* pStream );
	//! Load data
	bool Load( WRLoot* pWRLoot, SerializeBinStream* pStream );

	int GetProxyCount( void )
		{ return GetTotalCount()-HowManyProxiesUseFlag(WRLoot_Proxy::EPROXY_POINT); };
	int GetPointCount( void )
		{ return HowManyProxiesUseFlag(WRLoot_Proxy::EPROXY_POINT); };
	int GetTotalCount( void ) const
		{ return m_aProxy.Size(); };

	//! Count proxies with flag set
	int HowManyProxiesUseFlag( int iFlag );

	//! Insert proxy
	WRLoot_Proxy* InsertProxy( void );
	//! Insert point
	WRLoot_Point* InsertPoint( void );

	//! Get proxy by index
	WRLoot_Point* GetProxy( int iIndex )
		{ return m_aProxy[iIndex]; };

	//! Get random proxy
	WRLoot_Point* GetRandomProxy( int iFlags, float fMinimalHeight = 0.0f );

	//! Get boundaries
	void GetBoundaries( float& fX1, float& fZ1, float& fX2, float& fZ2 );

};



//! Proxy group (typically building)
class WRLoot_Group
{
protected:

	RString* m_pName;

	Vector3 m_vOrigin;

	AutoArray<WRLoot_Container*> m_aSelection; ///< Proxy selections

public:

	WRLoot_Group() :
	  m_pName(NULL)
	{
	};
	~WRLoot_Group();

	//! Save data
	bool Save( WRLoot* pWRLoot, SerializeBinStream* pStream );
	//! Load data
	bool Load( WRLoot* pWRLoot, SerializeBinStream* pStream );

	const char* GetName( void ) const
		{ return m_pName->Data(); };
	void SetName( RString* pName )
		{ m_pName = pName; };

	bool CheckName( const char* sName )
		{ return (*m_pName==sName); };

	void SetOrigin( Vector3Par vVector )
		{ m_vOrigin = vVector; };

	Vector3Par GetOrigin( void ) const
		{ return m_vOrigin; };

	int GetCount( void ) const
		{ return m_aSelection.Size(); };

	//! Get proxy count
	int GetProxyCount( void );
	//! Get point count
	int GetPointCount( void );
	//! Get total proxy+point count
	int GetTotalCount( void );

	//! Check if within distance
	bool Distance( Vector3Par vPos, float fDistance ) const
		{ return m_vOrigin.DistanceXZ2(vPos)<Square(fDistance); };


	//! Insert container
	WRLoot_Container* InsertContainer( void );

	//! Get selection by index
	WRLoot_Container* GetContainer( int iIndex )
		{ return m_aSelection[iIndex]; };
	//! Get selection by name
	WRLoot_Container* FindSelection( const char* sName );

	//! Count proxies with flag set
	int HowManyProxiesUseFlag( int iFlag );

	//! Get boundaries
	void GetBoundaries( float& fX1, float& fZ1, float& fX2, float& fZ2 );

};


//! Spawnable item setup
class WRSpawnItem
{
protected:
	
	RString name; ///< Name is equal to class

	float m_fGroupProbability; ///< Probability of group spawn
	float m_fItemProbability; ///< Probability of item spawn

	WRClassHierarchy* m_pClass; ///< Class

public:

	WRSpawnItem() :
		m_fGroupProbability(0.0f),
		m_fItemProbability(0.0f),
		m_pClass(NULL)
	{
	};

	const char* GetName( void ) const
		{ return name.Data(); };
	void SetName( const char* sName )
		{ name = sName; };

	float GetGroupProbability( void ) const
		{ return m_fGroupProbability; };
	void SetGroupProbability( float fProbability )
		{ m_fGroupProbability = fProbability; };

	float GetItemProbability( void ) const
		{ return m_fItemProbability; };
	void SetItemProbability( float fProbability )
		{ m_fItemProbability = fProbability; };

	//! Return "helper" probability
	float GetComplementaryProbability( void ) const
		{ return m_fItemProbability + m_fGroupProbability*1000.0f; };

	//! Output statistics
	void ListToConsole( void );

	void SetClass( WRClassHierarchy* pClass )
		{ m_pClass = pClass; };
	WRClassHierarchy* GetClass( void )
		{ return m_pClass; };

	//! Calculate random spawn result base on probability
	bool ShouldSpawn( void );

	//! Calculate spawn result base on given probability
	bool ShouldSpawn( float fProbability )
		{ return (m_fItemProbability>=fProbability); };
//		{ return ((m_fProbability*m_fProbability)>=fProbability); };

};

//! Spawnable location setup
class WRSpawnLocation
{
protected:

	RString name;

	WRContainer* m_pParent;
	
	int m_iCount; ///< Count limit for population
	int m_iCurrent; ///< Current population

public:

	WRSpawnLocation( WRContainer* pParent ) :
		m_pParent(pParent),
		m_iCount(0),
		m_iCurrent(0)
	{
	};

	WRContainer* GetParent( void ) 
		{ return m_pParent; };

	const char* GetName( void ) const
		{ return name.Data(); };
	void SetName( const char* sName )
		{ name = sName; name.Lower(); };
	
	bool CheckName( const char* sName )
		{ return (name==sName); };

	int GetCount( void ) const
		{ return m_iCount; };
	void SetCount( int iCount )
		{ m_iCount = iCount; };

	//! Output statistics
	void ListToConsole( void );

	//! Get current population
	int GetCurrent( void ) const
		{ return m_iCurrent; };

	//! Called on spawn
	void OnSpawn( void )
		{ ++m_iCurrent; };
	//! Called on spawn reset
	void OnReset( void )
		{ m_iCurrent = 0; };

	//! Check if some space remain reserved for spawning
	bool CanSpawn( void ) const
		{ return (m_iCurrent<m_iCount); };

};


//! Spawnable loot container
class WRContainer
{
protected:

	RString name;

	AutoArray<WRSpawnLocation*> m_aLocations; ///< Proxy selections
	AutoArray<WRSpawnItem*> m_aItems; ///< Proxy selections

	int m_iLastCheckIndex; ///< This will help us to avoid spawn same items

	float m_fGroupLow;
	float m_fGroupHigh;

	//! Normalize probabilities
	void Normalize( void );

public:

	WRContainer() :
	  m_iLastCheckIndex(0),
		m_fGroupLow(0.0),
		m_fGroupHigh(0.0)
	{
	};

	~WRContainer();

	const char* GetName( void ) const
		{ return name.Data(); };
	void SetName( const char* sName )
		{ name = sName; };

	bool CheckName( const char* sName )
		{ return (name==sName); };

	//! Add location
	WRSpawnLocation* AddLocation( void );
	//! Add item declaration
	WRSpawnItem* AddItem( void );

	//! Output statistics
	void ListToConsole( void );

	//! 
	void RegisterLocations( ParamEntryVal* pVal );
	//!
	void RegisterItems( ParamEntryVal* pVal );

	int GetLocationCount( void ) const
		{ return m_aLocations.Size(); };
	int GetItemCount( void ) const
		{ return m_aItems.Size(); };

	WRSpawnLocation* GetLocation( int iIndex )
		{ return m_aLocations[iIndex]; };
	WRSpawnItem* GetItem( int iIndex )
		{ return m_aItems[iIndex]; };

	//! Get just random location
	WRSpawnLocation* GetRandomLocation( void );

	//! Get just random item
	WRSpawnItem* GetRandomItem( void );

	//! Get random item by it's probability
	WRSpawnItem* GetItemByProbability( float fAdjustedProbability = 1.0f );

	//! Get item definition for particular item
	WRSpawnItem* GetItem( const char* sItemName );

	//! Find location by name
	WRSpawnLocation* FindLocation( const char* sName );

	//! Called on spawn reset
	void OnReset( void );

	//! Sort item array
	void Sort( void );

	//! Extent group probability
	void ExtentGroupProbability( float fProbability, bool bForceSet = false );

	//! Get total item limit
	int GetItemLimit( void );

};


//! Spawnable loot container array
class WRSpawnClass
{
protected:

	RString name;

	AutoArray<WRContainer*> m_aSpawn; ///< Spawn groups

public:

	~WRSpawnClass()
	{
		for( int c = 0; c < m_aSpawn.Size(); c++ )
			delete m_aSpawn[c];
	};

	const char* GetName( void ) const
		{ return name.Data(); };
	void SetName( const char* sName )
		{ name = sName; };

	bool CheckName( const char* sName )
		{ return (name==sName); };

	//! Output statistics
	void ListToConsole( void );

	//! Add spawn type
	WRContainer* Add( const char* sName );

	int GetCount( void )
		{ return m_aSpawn.Size(); };
	WRContainer* Get( int iIndex )
		{ return m_aSpawn[iIndex]; };

	//! Get next index
	int NextIndex( int iIndex )
		{ return ((++iIndex)<m_aSpawn.Size()) ? iIndex : 0; };

	//! Get item definition for particular item
	WRSpawnItem* GetItem( const char* sItemName );

	//! Find container by name
	WRContainer* FindContainer( const char* sName );
	//! Find location by name
	WRSpawnLocation* FindLocation( const char* sName );

	//! Called on spawn reset
	void OnReset( void );

	//! Sort item array
	void Sort( void );

	//! Get total spawnable location count
	int GetTotalLocationCount( void );
	//! Get total spawnable items
	int GetTotalItemsCount( void );
	//! Get total item limit
	int GetTotalItemLimit( void );

};


//! Spawnable classes
class WRLootSpawns
{
protected:

	AutoArray<WRSpawnClass*> m_aSpawnClass; ///< Groups in map

public:

	//! Add type
	WRSpawnClass* Add( const char* sName );
	//! Find by name
	WRSpawnClass* FindByName( const char* sName );
	
	//! Output statistics
	void ListToConsole( void );

	int GetCount( void )
		{ return m_aSpawnClass.Size(); };
	WRSpawnClass* Get( int iIndex )
		{ return m_aSpawnClass[iIndex]; };

	//! Get item definition for particular item
	WRSpawnItem* GetItem( const char* sItemName );

	//! Sort item array
	void Sort( void );

};



//! Content holder
class WRLoot
{
private:

	static const int CLASS_LEVEL_DEEPNESS = 10; ///< How deep classes can go

	WRClassHierarchy* m_pHierarchy; ///< Spawnable class hierarchy

	WRLootSpawns* m_pSpawns; ///< Spawnable data setup

	WRStringPool* m_pGroupName; ///< Names of groups
	WRStringPool* m_pContainerName; ///< Names of selections
	WRStringPool* m_pProxyName; ///< Names of proxy types

	AutoArray<WRLoot_Group*> m_aGroup; ///< Groups in map

	//! Test area around point and return available "safe" range
	void TestRange( WRLoot_Point* pPoint, Vector3Par vCenter, float fMaxRange );

public:

	WRLoot();
	~WRLoot();

	//! Add group
	WRLoot_Group* InsertGroup( const char* sName, Object* pObject );
	//! Add container to last group
	WRLoot_Container* ParseSelection( void );

	//! Save to file
	bool SaveToFile( const char* sFileName );
	//! Load from file
	bool LoadFromFile( const char* sFileName );

	//! Save data
	bool Save( SerializeBinStream* pStream );
	//! Load data
	bool Load( SerializeBinStream* pStream );
	
	//! Get count of unique strings (group names)
	int GetUniqueGroupNameCount( void ) const
		{ return m_pGroupName ? m_pGroupName->GetCount() : 0; };
	//! Get count of unique strings (container names)
	int GetUniqueContainerNameCount( void ) const
		{ return m_pContainerName ? m_pContainerName->GetCount() : 0; };
	//! Get count of unique strings (proxy names)
	int GetUniqueProxyNameCount( void ) const
		{ return m_pProxyName ? m_pProxyName->GetCount() : 0; };
	//! Get count of unique spawnable classes
	int GetUniqueSpawnableClassCount( void ) const
		{ return m_pHierarchy ? m_pHierarchy->GetEndClassCount() : 0; };

	//! Group count
	int GetCount( void ) const
		{ return m_aGroup.Size(); };

	WRStringPool* GetGroupNames( void )
		{ return m_pGroupName; };
	WRStringPool* GetContainerNames( void )
		{ return m_pContainerName; };
	WRStringPool* GetProxyNames( void )
		{ return m_pProxyName; };

	//! Get total container count
	int GetContainerCount( void ) const;
	//! Get proxy count
	int GetProxyCount( void ) const;
	//! Get point count
	int GetPointCount( void ) const;
	//! Get total proxy+point count
	int GetTotalCount( void ) const;
	//! Get total bad proxy count
	int GetBadProxyCount( void ) const;
	//! Get proxy count withing specified range
	int GetProxyCountWithingRange( Vector3Par vPos, float fDistance );

	//! Get group by index
	WRLoot_Group* GetGroup( int iIndex )
		{ return m_aGroup[iIndex]; };

	//! Get index of string for group name
	int GetIndexForGroupName( RString* pString )
		{ return m_pGroupName->GetIndex(pString); };
	//! Get index of string for container name
	int GetIndexForContainerName( RString* pString )
		{ return m_pContainerName->GetIndex(pString); };
	//! Get index of string for proxy name
	int GetIndexForProxyName( RString* pString )
		{ return m_pProxyName->GetIndex(pString); };

	//! Get group name
	RString* GetGroupName( int iIndex )
		{ return  m_pGroupName->GetString(iIndex); };
	//! Get container name
	RString* GetContainerName( int iIndex )
		{ return  m_pContainerName->GetString(iIndex); };
	//! Get proxy name
	RString* GetProxyName( int iIndex )
		{ return  m_pProxyName->GetString(iIndex); };

	//! Get class hierarchy
	WRClassHierarchy* GetHierarchy( void )
		{ return m_pHierarchy; };

	//! Get spawn setup
	WRLootSpawns* GetSetup( void )
		{ return m_pSpawns; };

	//! Get item definition for particular item
	WRSpawnItem* GetItem( const char* sItemName );

	//! Output statistics
	void ListToConsole( void );

	//! Register groups
	void RegisterGroups( void );
	//! Register distribution setup
	void RegisterSetup( void );
	//! Register spawnable loot
	void RegisterLoot( bool bFull = false );

	//! Get boundaries
	void GetBoundaries( float& fX1, float& fZ1, float& fX2, float& fZ2 );

};


#endif