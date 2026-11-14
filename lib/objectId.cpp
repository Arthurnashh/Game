#include "wpch.hpp"
#include "objectId.hpp"

#include <El/ParamArchive/paramArchive.hpp>
#include <El/QStream/qStream.hpp>
#include <Es/Strings/rString.hpp>
#include "landscape.hpp"

#if 0
/// some ids of objects which have lights attached to them
/** used to track Link to XXXX not released problems */
inline bool InterestedInId(int i)
{
  return (
    i==0xb76970b4 ||
    i==0x93a8f4d8 ||
    i==0x98e6a437 ||
    i==0x93a8f4d8 ||
    i==0x9d27fc0a ||
    i==0x9ea780d6 ||
    i==0xb12aeba3 ||
    i==0xb12ae954 ||
    i==0xaee9a01a ||
    i==0xb76970b4 ||
    i==0xb4c8a1c2 ||
    i==0xa7487e97 ||
    i==0x98e6a437 ||
    i==0xa52b18b6 ||
    i==0x9c4a6a6d ||
    i==0x9769df8d ||
    i==0x9f279daf ||
    i==0x9a4707c0
  );
}
#endif

LSError VisitorObjId::Serialize(ParamArchive &archive, const RStringB &name, int minVersion, const VisitorObjId &defValue)
{
	return archive.Serialize(name,_id,minVersion,defValue._id);
}

RString ObjectId::GetDebugName() const
{
	if (IsObject())
	{
		return Format("Obj-%d,%d:%d",GetObjX(),GetObjZ(),GetObjId());
	}
	else
	{
		return Format("Veh-%d",GetVehId());
	}
}

LSError ObjectId::Serialize(ParamArchive &archive, const RStringB &name, int minVersion, const ObjectId &defValue)
{
	return archive.Serialize(name,_encoded,minVersion,defValue._encoded);
}

ObjectIdAutoDestroy::ObjectIdAutoDestroy(const ObjectIdAutoDestroy &src)
:ObjectId(src),TLinkBidirD()
{
//  if (InterestedInId(src.Encode()))
//  {
//    __asm nop;
//  }
	if (src.IsInList())
	{
		GLandscape->GetIdList().InsertAfter(const_cast<ObjectIdAutoDestroy *>(&src),this);
	}
}

void ObjectIdAutoDestroy::operator = (const ObjectIdAutoDestroy &src)
{
//  if (InterestedInId(src.Encode()))
//  {
//    __asm nop;
//  }
	// SLink should be removed from the old list
	if (IsInList()) TLinkBidirD::Delete();
	ObjectId::operator =(src);
	if (!IsNull())
	{
		GLandscape->GetIdList().InsertAfter(const_cast<ObjectIdAutoDestroy *>(&src),this);
	}
}

ObjectIdAutoDestroy SoftLinkIdTraits<ObjectId,ObjectIdAutoDestroy>::Store(const ObjectId &id)
{
//  if (InterestedInId(id.Encode()))
//  {
//    __asm nop;
//  }
  return ObjectIdAutoDestroy(id,GLandscape->GetIdList());
};
