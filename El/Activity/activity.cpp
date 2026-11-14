#include <El/elementpch.hpp>
#include <El/Activity/activity.hpp>


TypeIsSimple(const ActivityClassTypeInfo *)


/// global activity type manager
ClassTypeManager<ActivityClassTypeInfo> GActivityTypeManager;

void Activity::RegisterType(const ActivityClassTypeInfo *info)
{
  GActivityTypeManager.Register(info);
}


Activity *Activity::CreateObject(ParamArchive &ar)
{
  RStringB typeName;
  if( ar.Serialize("activityType",typeName,0)!=LSOK )
  {
    return NULL;
  }
  const ActivityClassTypeInfo *type =  GActivityTypeManager.Find(typeName);
  if (!type)
  {
    return NULL;
  }
  return type->CreateObject(ar);
}

LSError Activity::LoadLLinkRef(ParamArchive &ar, LLink<Activity> &ref)
{
  RStringB typeName;
  CHECK( ar.Serialize("activityType",typeName,0) )
  if (typeName.GetLength()<=0)
  {
    ref = NULL;
    return LSOK;
  }
  const ActivityClassTypeInfo *type =  GActivityTypeManager.Find(typeName);
  if (!type)
  {
    ref = NULL;
  }
  else
  {
    ref = type->LoadRef(ar);
  }
  return LSOK;
}

LSError Activity::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving())
  {
    RStringB typeName = GetTypeInfo().GetName();
    CHECK( ar.Serialize("activityType",typeName,0) )
  }
  return LSOK;
}

LSError Activity::SaveLLinkRef(ParamArchive &ar, LLink<Activity> &ref)
{
  if (ref)
  {
    RStringB typeName = ref->GetTypeInfo().GetName();
    CHECK( ar.Serialize("activityType",typeName,0) )
  }
  else
  {
    RStringB typeName;
    CHECK( ar.Serialize("activityType",typeName,0) )
  }
  return LSOK;
}

LSError ActivityEvent::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("activity",_activity,0));
  return LSOK;
}
