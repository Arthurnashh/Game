#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MAPFILE_HPP
#define _MAPFILE_HPP

#include <Es/Strings/bString.hpp>
#include <Es/Containers/array.hpp>
#include <stdint.h>

#ifdef MAPINFO_EXTENDED
#define MapInfo MapIntoExt
#define MapFile MapFileExt
#endif

struct MapInfo
{
  BString<512> name;
	int section;
	uintptr_t physAddress; // symbol value / absolute virtual address
	uintptr_t logAddress; // logical address

#ifdef MAPINFO_EXTENDED
  BString<128> module;
#endif
	
	/// undecorated (human readable) name
	const char *ReadableName() const;
};

typedef uintptr_t MapInfo::*MapAddressId;

TypeIsSimpleZeroed(MapInfo);

class AllocatorWin
{
	public:
	//AllocatorWin(){}
	public:
	#if ALLOC_DEBUGGER
	static void *Alloc( size_t &size, const char *file, int line, const char *postfix );
	#else
	static void *Alloc( size_t &size );
	#endif
	static void Free( void *mem, size_t size );
	static void *Realloc( void *mem, size_t oldSize, size_t size );
	static void Unlink( void *mem ) {}
  static inline int MinGrow() {return 32;}
};

class MapFile
{
	BString<512> _name;
	AutoArray<MapInfo,AllocatorWin> _map;
	bool _parseDone;
  enum {MaxSections=32};
  //! index of first item not in given section
  int _sectionEnd[MaxSections];
  //! index of first item not in section 1
  //int _firstSection2;

	public:
	MapFile();
	~MapFile();

	void ParseMapFile(const char *name=NULL);
	// add given offset to all physical addresses
	void OffsetPhysicalAddress(intptr_t offset);
	void Clear();

	const char *GetName() const {return _name;}
	const char *MapName( uintptr_t address, MapAddressId id, uintptr_t *lower=NULL ) const;
  const char *MapRawName( uintptr_t address, int section, MapAddressId id, uintptr_t *lower=NULL ) const;
	uintptr_t FunctionStart( uintptr_t address, MapAddressId id) const;
	const char *MapNameFromLogical( uintptr_t lAddress, uintptr_t *lower=NULL ) const
	{
		return MapName(lAddress,&MapInfo::logAddress,lower);
	}
	const char *MapNameFromLogical( uintptr_t lAddress, int *lower ) const
	{
		uintptr_t lowerWide = 0;
		const char *name = MapName(lAddress,&MapInfo::logAddress,lower ? &lowerWide : NULL);
		if (lower) *lower = (int)lowerWide;
		return name;
	}
	uintptr_t FunctionStartFromLogical(uintptr_t lAddress) const
	{
		return FunctionStart(lAddress,&MapInfo::logAddress);
	}

	const MapInfo *FindByPhysical( uintptr_t fAddress ) const;

	const char *MapNameFromPhysical( uintptr_t fAddress, uintptr_t *lower=NULL ) const;
	const char *MapNameFromPhysical( uintptr_t fAddress, int *lower ) const
	{
		uintptr_t lowerWide = 0;
		const char *name = MapNameFromPhysical(fAddress,lower ? &lowerWide : NULL);
		if (lower) *lower = (int)lowerWide;
		return name;
	}
	void *FunctionStartFromPhysical(void *pAddress) const;

	uintptr_t Address( const char *name, MapAddressId id ) const ;
	uintptr_t AddressBySubstring( const char *name, MapAddressId id ) const;
	uintptr_t MinAddress( MapAddressId id ) const ;
	uintptr_t MaxAddress( MapAddressId id, int section=1 ) const ;

	uintptr_t PhysicalAddress( const char *name ) const {return Address(name,&MapInfo::physAddress);}
	uintptr_t LogicalAddress( const char *name ) const {return Address(name,&MapInfo::logAddress);}

	uintptr_t PhysicalAddressBySubstring( const char *name ) const {return AddressBySubstring(name,&MapInfo::physAddress);}
	uintptr_t LogicalAddressBySubstring( const char *name ) const {return AddressBySubstring(name,&MapInfo::logAddress);}

	uintptr_t MinPhysicalAddress() const {return MinAddress(&MapInfo::physAddress);}
	uintptr_t MaxPhysicalAddress() const {return MaxAddress(&MapInfo::physAddress);}
	uintptr_t MinLogicalAddress() const {return MinAddress(&MapInfo::logAddress);}
	uintptr_t MaxLogicalAddress() const {return MaxAddress(&MapInfo::logAddress);}

	bool Empty() const {return _map.Size()<=0;}
	bool Parsed() const {return _parseDone;}
	
  protected:
  //@{ allow inherited classes to use access to symbols
	int GetSymbolCount() const {return _map.Size();}
	MapInfo &GetSymbol(int i) {return _map[i];}
	const MapInfo &GetSymbol(int i) const {return _map[i];}
	//@}
};

#endif
