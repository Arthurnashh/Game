#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OBJECT_ID_HPP
#define _OBJECT_ID_HPP

#include "wpch.hpp"
#include <Es/Strings/rString.hpp>
#include <Es/Containers/listBidir.hpp>
#include <El/ParamArchive/serializeClass.hpp>
#include <Es/Common/global.hpp>

#if _HELISIM_LARGE_OBJECTID
typedef int64 OIDInt;
#define OID_FORMAT "lld"
#elif defined _HELISIM_LARGE_OBJECTID
#define OID_FORMAT "d"
typedef int OIDInt;
#else
#error _HELISIM_LARGE_OBJECTID must be defined, use #define _HELISIM_LARGE_OBJECTID 0 to use 32b id
#endif // _HELISIM_LARGE_OBJECTID

class ParamArchive;

//! unique object id (given by visitor)
class VisitorObjId
{
	int _id;

	public:

	//! constructor
	explicit VisitorObjId(int id):_id(id) {}
	//! default constructor - uninitialized Id
	VisitorObjId():_id(-1) {}

	//! get numerical value
	operator int() const {return _id;}

	//! serialize into ParamArchive
	LSError Serialize(ParamArchive &archive, const RStringB &name, int minVersion, const VisitorObjId &defValue);
	//! serialize into ParamArchive, default value nill
	LSError Serialize(ParamArchive &archive, const RStringB &name, int minVersion)
	{
		return Serialize(archive,name,minVersion,VisitorObjId());
	}

	//! encode for saving/transfer etc.
	int Encode() const {return _id;}
	//! decode
	void Decode(int val) {_id=val;}
	//! prepare a new ID
	void Increment() {_id++;}

	//! equality operator
	bool operator ==(const VisitorObjId &with) const {return _id == with._id;}
};

//! no id assigned (positive)
#define VISITOR_NO_ID VisitorObjId(-1)
//! no id assigned (uninitialized, need to be assigned before it can be used)
#define VISITOR_UNINIT_ID VisitorObjId()

enum _vehicleId {VehicleId};

#if _HELISIM_LARGE_OBJECTID
//! unique object id, including information necessary for object loading
class ObjectId
{
  OIDInt _encoded;

  // define bit layout:
  // B63 - isObjectFlag
  // if isObjectFlag:
  //    B62-B42: z
  //    B41-B21: x
  //    B20- B0: id
  // else
  //    B62- B0: id

  static const int ObjZBits = 21U;
  static const int ObjXBits = 21U;
  static const int ObjIdBits = 21U;

  static const int VehicleIdBits = ObjIdBits + ObjXBits + ObjZBits;
  static const int64 IsObjectMask = 0x8000000000000000LL;
  static const int64 VehicleIdMask = 0x7FFFFFFFFFFFFFFFLL;

public:
  static const int MaxObjectId = (1U<</*ObjIdBits*/11)-1U;
  static const int64 MaxVehicleId = 0x7FFFFFFFFFFFFFFFLL;

  static const int MaxXLandGrid = (1U<<ObjXBits)-1U;
  static const int MaxZLandGrid = (1U<<ObjZBits)-1U;

private:
  static inline OIDInt EncodeVehicle(OIDInt id)
  {
    Assert(id>=-1);
    Assert(id<MaxVehicleId);
    return id+1;
  }
  static inline OIDInt EncodeObject(int x, int z, int id)
  {
    Assert(id>=0);
    Assert(id<=MaxObjectId);
    return IsObjectMask|((OIDInt)z<<(ObjIdBits+ObjXBits))|((OIDInt)x<<ObjIdBits)|(OIDInt)id;
  }

public:
  //! static object ID
  ObjectId(int x, int z, int id)
  {
    // explicit coding required?
    _encoded = EncodeObject(x,z,id);
  }
  //! vehicle ID
  ObjectId(_vehicleId,int id)
  {
    _encoded = EncodeVehicle(id);
  }
  //! no ID
  ObjectId()
  {
    _encoded = 0;
  }
  bool IsNull() const
  {
    return _encoded == 0;
  }
  bool IsObject() const
  {
    return (_encoded&IsObjectMask)!=0;
  }
  int GetObjId() const
  {
    Assert(IsObject());
    return _encoded&((1U<</*ObjIdBits*/11)-1);
  }
  int GetObjX() const
  {
    Assert(IsObject());
    return (_encoded>>ObjIdBits)&((1U<<ObjXBits)-1);
  }
  int GetObjZ() const
  {
    Assert(IsObject());
    return (_encoded>>(ObjIdBits+ObjXBits))&((1U<<ObjZBits)-1);
  }

  OIDInt GetVehId() const
  {
    Assert(!IsObject());
    return (_encoded&VehicleIdMask)-1;
  }

  static OIDInt GetObjIdCandidate(OIDInt i)
  {
    return i&MaxObjectId;
  }

  RString GetDebugName() const;

  //! serialize into ParamArchive
  LSError Serialize(ParamArchive &archive, const RStringB &name, int minVersion, const ObjectId &defValue);
  //! serialize into ParamArchive, default value nill
  LSError Serialize(ParamArchive &archive, const RStringB &name, int minVersion)
  {
    return Serialize(archive,name,minVersion,ObjectId());
  }

  //! encode for saving/transfer etc.
  OIDInt Encode() const
  {
    return _encoded;
  }
  //! decode
  void Decode(OIDInt val)
  {
    _encoded = val;
  }

  //! encode 32bit int
  int Encode32() const
  {
    return ((_encoded&IsObjectMask)>>32)|
      ((_encoded&0xFFC0000000000LL)>>21)|
      ((_encoded&0x7FE00000)>>10)|
      (_encoded&0x7FF);
  }

  //! decode 32bit int
  void Decode32(int val)
  {
    _encoded = EncodeObject((val&0x1FF800)>>11, (val&0x7FE00000)>>21, val&0x7FF);
  }

  //! equality operator
  bool operator ==(const ObjectId &with) const {return _encoded == with._encoded;}

  //! equality operator
  bool operator !=(const ObjectId &with) const {return _encoded != with._encoded;}
};

#else
//! unique object id, including information necessary for object loading
class ObjectId
{
	int _encoded;

	enum
	{
		// define bit layout:
		// B31 - isObjectFlag
		// if isObjectFlag:
		//    B30-B21: z
		//    B20-B11: x
		//    B10- B0: id
		// else
		//    B30- B0: id

		ObjZBits=10U,
		ObjXBits=10U,
		ObjIdBits=11U,

		VehicleIdBits=ObjIdBits+ObjXBits+ObjZBits,
		IsObjectMask=0x80000000,
		VehicleIdMask=0x7fffffff,
	};

  public:
	enum
	{
	  // some numbers need to be accessed publicly
		MaxObjectId=(1U<<ObjIdBits)-1U,
		MaxVehicleId=0x7fffffff,
    MaxXLandGrid=(1U<<ObjXBits)-1U,
    MaxZLandGrid=(1U<<ObjZBits)-1U
  };

  private:
	static inline int EncodeVehicle(int id)
	{
		Assert(id>=-1);
		Assert(id<MaxVehicleId);
		return id+1;
	}
	static inline int EncodeObject(int x, int z, int id)
	{
		Assert(id>=0);
		Assert(id<=MaxObjectId);
		return IsObjectMask|(z<<(ObjIdBits+ObjXBits))|(x<<ObjIdBits)|id;
	}

	public:
	//! static object ID
	ObjectId(int x, int z, int id)
	{
		// explicit coding required?
		_encoded = EncodeObject(x,z,id);
	}
	//! vehicle ID
	ObjectId(_vehicleId,int id)
	{
		_encoded = EncodeVehicle(id);
	}
	//! no ID
	ObjectId()
	{
		_encoded = 0;
	}
	bool IsNull() const
	{
		return _encoded == 0;
	}
	bool IsObject() const
	{
		return (_encoded&IsObjectMask)!=0;
	}
	int GetObjId() const
	{
		Assert(IsObject());
		return _encoded&((1U<<ObjIdBits)-1);
	}
	int GetObjX() const
	{
		Assert(IsObject());
		return (_encoded>>ObjIdBits)&((1U<<ObjXBits)-1);
	}
	int GetObjZ() const
	{
		Assert(IsObject());
		return (_encoded>>(ObjIdBits+ObjXBits))&((1U<<ObjZBits)-1);
	}
	int GetVehId() const
	{
		Assert(!IsObject());
		return (_encoded&VehicleIdMask)-1;
	}
	static int GetObjIdCandidate(int i)
	{
		return i&MaxObjectId;
	}

	RString GetDebugName() const;

	//! serialize into ParamArchive
	LSError Serialize(ParamArchive &archive, const RStringB &name, int minVersion, const ObjectId &defValue);
	//! serialize into ParamArchive, default value nill
	LSError Serialize(ParamArchive &archive, const RStringB &name, int minVersion)
	{
		return Serialize(archive,name,minVersion,ObjectId());
	}

	//! encode for saving/transfer etc.
	int Encode() const {return _encoded;}
	//! decode
	void Decode(int val) {_encoded = val;}

	//! equality operator
	bool operator ==(const ObjectId &with) const {return _encoded == with._encoded;}

	//! equality operator
	bool operator !=(const ObjectId &with) const {return _encoded != with._encoded;}
};
#endif // _HELISIM_LARGE_OBJECTID

//! id that will be destroyed, once landscape it points to is destroyed

class ObjectIdAutoDestroy: public ObjectId, public TLinkBidirD
{
  public:
  ObjectIdAutoDestroy()
  {
  }
  ObjectIdAutoDestroy(ObjectId id, TListBidir<ObjectIdAutoDestroy> &list)
  :ObjectId(id)
  {
    if (!IsNull())
    {
      list.Insert(this);
    }
  }
  ~ObjectIdAutoDestroy()
  {
    if (IsInList())
    {
      TLinkBidirD::Delete();
    }
  }

  // make new copy a member of the same list as source link is
  ObjectIdAutoDestroy(const ObjectIdAutoDestroy &src);

  void operator = (const ObjectIdAutoDestroy &src);

};

#endif

