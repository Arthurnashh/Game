#include "wpch.hpp"
#include "paramArchiveExt.hpp"
#include <El/ParamFile/paramFile.hpp>

#include <El/Time/time.hpp>
#include <El/Evaluator/expressImpl.hpp>
#include "global.hpp"
#include "Network/networkObject.hpp"

#define ON_ERROR_EXT(err, ctx) {ar.OnError(err, ctx); return err;}
#define CANCEL_ERROR_EXT {ar.CancelError(); return LSOK;}

#define CHECK_INTERNAL(command) \
	{LSError err = command; if (err != LSOK) return err;}

LSError Serialize(ParamArchive &ar, const RStringB &name, Time &value, int minVersion)
{
	if (ar.GetArVersion() < minVersion) return LSOK;
	if (ar.IsSaving())
	{
		__int64 nDiff = (__int64)value.toInt() - (__int64)Glob.time.toInt();
		float fDiff = 1e-3 * nDiff;
		CHECK_INTERNAL(ar.Serialize(name, fDiff, minVersion))
	}
	else
	{
		if (ar.GetPass() != ParamArchive::PassFirst) return LSOK;
		float fDiff = 0;
		CHECK_INTERNAL(ar.Serialize(name, fDiff, minVersion))
		__int64 nVal = (__int64)(1e3 * fDiff) + (__int64)Glob.time.toInt();
		if (nVal < -INT_MAX) nVal = -INT_MAX;
		else if (nVal > INT_MAX) nVal = INT_MAX;
		value = Time((int)nVal);
	}
	return LSOK;
}

LSError Serialize(ParamArchive &ar, const RStringB &name, TimeSec &value, int minVersion)
{
	if (ar.GetArVersion() < minVersion) return LSOK;
	if (ar.IsSaving())
	{
		__int64 nDiff = (__int64)Time(value).toInt() - (__int64)Glob.time.toInt();
		float fDiff = 1e-3 * nDiff;
		CHECK_INTERNAL(ar.Serialize(name, fDiff, minVersion))
	}
	else
	{
		if (ar.GetPass() != ParamArchive::PassFirst) return LSOK;
		float fDiff = 0;
		CHECK_INTERNAL(ar.Serialize(name, fDiff, minVersion))
		__int64 nVal = (__int64)(1e3 * fDiff) + (__int64)Glob.time.toInt();
		if (nVal < -INT_MAX) nVal = -INT_MAX;
		else if (nVal > INT_MAX) nVal = INT_MAX;
		value = TimeSec(Time((int)nVal));
	}
	return LSOK;
}

LSError Serialize(ParamArchive &ar, const RStringB &name, CreatorId &value, int minVersion)
{
  return value.Serialize(ar, name, minVersion);
}

/*!
\patch 1.76 Date 6/13/2002 by Ondra
- Optimized: Memory usage during save-game further lowered.
*/

LSError Serialize(ParamArchive &ar, const RStringB &name, Vector3 &value, int minVersion)
{
	if (ar.GetArVersion() < minVersion) return LSOK;
	if (ar.IsSaving())
	{
		AUTO_STATIC_ARRAY(float, array, 3);
		array.Resize(3);
		for (int i=0; i<3; i++) array[i] = value[i];
		CHECK_INTERNAL(ar.SerializeArray(name, array, minVersion))
	}
	else
	{
		if (ar.GetPass() != ParamArchive::PassFirst) return LSOK;
		AUTO_STATIC_ARRAY(float, array, 3);
		CHECK_INTERNAL(ar.SerializeArray(name, array, minVersion))
		if (array.Size() == 0)
      return LSNoEntry; // default value
		Assert(array.Size() == 3);
		for (int i=0; i<3; i++) value[i] = array[i];
	}
	return LSOK;
}

LSError Serialize(ParamArchive &ar, const RStringB &name, Matrix3 &value, int minVersion)
{
  if (ar.GetArVersion() < minVersion) return LSOK;
  ParamArchive arSubcls;
  if (!ar.OpenSubclass(name, arSubcls))
    ON_ERROR_EXT(LSNoEntry, name);
  if (ar.IsSaving())
  {
    Vector3 &dir = const_cast<Vector3 &>(value.Direction());
    Vector3 &up = const_cast<Vector3 &>(value.DirectionUp());
    Vector3 &aside = const_cast<Vector3 &>(value.DirectionAside());
    CHECK(Serialize(arSubcls, "dir", dir, minVersion))
    CHECK(Serialize(arSubcls, "up", up, minVersion))
    CHECK(Serialize(arSubcls, "aside", aside, minVersion))
  }
  else
  {
    if (ar.GetPass() != ParamArchive::PassFirst) return LSOK;
    Vector3 dir;
    Vector3 up;
    Vector3 aside;
    CHECK(Serialize(arSubcls, "dir", dir, minVersion))
    CHECK(Serialize(arSubcls, "up", up, minVersion))
    CHECK(Serialize(arSubcls, "aside", aside, minVersion))
    value.SetDirection(dir);
    value.SetDirectionUp(up);
    value.SetDirectionAside(aside);
  }
  ar.CloseSubclass(arSubcls);
  return LSOK;
}

LSError Serialize(ParamArchive &ar, const RStringB &name, Matrix4 &value, int minVersion)
{
	if (ar.GetArVersion() < minVersion) return LSOK;
	ParamArchive arSubcls;
	if (!ar.OpenSubclass(name, arSubcls))
    ON_ERROR_EXT(LSNoEntry, name);
	if (ar.IsSaving())
	{
		Vector3 &pos = const_cast<Vector3 &>(value.Position());
		Vector3 &dir = const_cast<Vector3 &>(value.Direction());
		Vector3 &up = const_cast<Vector3 &>(value.DirectionUp());
		Vector3 &aside = const_cast<Vector3 &>(value.DirectionAside());
		CHECK(Serialize(arSubcls, "pos", pos, minVersion))
		CHECK(Serialize(arSubcls, "dir", dir, minVersion))
		CHECK(Serialize(arSubcls, "up", up, minVersion))
		CHECK(Serialize(arSubcls, "aside", aside, minVersion))
	}
	else
	{
		if (ar.GetPass() != ParamArchive::PassFirst) return LSOK;
		Vector3 pos;
		Vector3 dir;
		Vector3 up;
		Vector3 aside;
		CHECK(Serialize(arSubcls, "pos", pos, minVersion))
		CHECK(Serialize(arSubcls, "dir", dir, minVersion))
		CHECK(Serialize(arSubcls, "up", up, minVersion))
		CHECK(Serialize(arSubcls, "aside", aside, minVersion))
		value.SetPosition(pos);
		value.SetDirection(dir);
		value.SetDirectionUp(up);
		value.SetDirectionAside(aside);
	}
	ar.CloseSubclass(arSubcls);
	return LSOK;
}

LSError Serialize(ParamArchive &ar, const RStringB &name, Color &value, int minVersion)
{
	if (ar.GetArVersion() < minVersion) return LSOK;
	if (ar.IsSaving())
	{
		AUTO_STATIC_ARRAY(float, array, 4);
		array.Resize(4);
		array[0] = value.R();
		array[1] = value.G();
		array[2] = value.B();
		array[3] = value.A();
		CHECK_INTERNAL(ar.SerializeArray(name, array, minVersion))
	}
	else
	{
		if (ar.GetPass() != ParamArchive::PassFirst) return LSOK;
		AUTO_STATIC_ARRAY(float, array, 4);
		CHECK_INTERNAL(ar.SerializeArray(name, array, minVersion))
		if (array.Size() == 0)
      return LSNoEntry; // default value
		Assert(array.Size() == 4);
		value = Color(array[0], array[1], array[2], array[3]);
	}
	return LSOK;
}

LSError Serialize(ParamArchive &ar, const RStringB &name, GameValue &value, int minVersion)
{
  if (ar.GetArVersion() < minVersion) return LSOK;

  ParamArchive arSubcls;
  if (!ar.OpenSubclass(name, arSubcls))
  {
    ar.OnError(LSNoEntry, name);
    return LSNoEntry;
  }

  LSError err = value.Serialize(arSubcls);
  ar.CloseSubclass(arSubcls);

  return err;
}

LSError SerializeArray(ParamArchive &ar, const RStringB &name, AutoArray<Vector3> &value, int minVersion)
{
	if (ar.GetArVersion() < minVersion) return LSOK;
	if (!ar.IsSaving() && ar.GetPass() != ParamArchive::PassFirst) return LSOK;
	ParamArchive arCls;
	if (!ar.OpenSubclass(name, arCls))
    ON_ERROR_EXT(LSNoEntry, name);
	int n;
	if (ar.IsSaving())
	{
		n = value.Size();
		CHECK(arCls.Serialize("items", n, minVersion))
	}
	else
	{
		CHECK(arCls.Serialize("items", n, minVersion))
		value.Realloc(n);
		value.Resize(n);
	}
	for (int i=0; i<n; i++)
	{
		char buffer[256];
		sprintf(buffer, "vector%d", i);
		CHECK(Serialize(arCls, buffer, value[i], minVersion))
	}
	ar.CloseSubclass(arCls);
	return LSOK;
}

LSError SerializeArray(ParamArchive &ar, const RStringB &name, AutoArray<Color> &value, int minVersion)
{
  if (ar.GetArVersion() < minVersion) return LSOK;
  if (!ar.IsSaving() && ar.GetPass() != ParamArchive::PassFirst) return LSOK;
  ParamArchive arCls;
  if (!ar.OpenSubclass(name, arCls))
    ON_ERROR_EXT(LSNoEntry, name);
  int n;
  if (ar.IsSaving())
  {
    n = value.Size();
    CHECK(arCls.Serialize("items", n, minVersion))
  }
  else
  {
    CHECK(arCls.Serialize("items", n, minVersion))
      value.Realloc(n);
    value.Resize(n);
  }
  for (int i=0; i<n; i++)
  {
    char buffer[256];
    sprintf(buffer, "color%d", i);
    CHECK(Serialize(arCls, buffer, value[i], minVersion))
  }
  ar.CloseSubclass(arCls);
  return LSOK;
}

#if 0
#define PARS_4_TO_3_EXT_FUNC(func)												\
{																											\
	if (ar.GetArVersion() < minVersion)													\
	{																										\
		if (!ar.IsSaving()) value = defValue;										\
		return LSOK;																			\
	}																										\
	if (ar.IsSaving() && value == defValue) return LSOK;			\
	LSError err = func(ar, name, value, minVersion);	    	\
	if (err == LSNoEntry)																\
	{value = defValue; CANCEL_ERROR_EXT;}										\
	return err;																					\
}

#define PARS_4_TO_3_EXT PARS_4_TO_3_EXT_FUNC(Serialize)

#define PARS_4_TO_3_EXT_NODEFAULTS												\
{																											\
	if (ar.GetArVersion() < minVersion)													\
	{																										\
		if (!ar.IsSaving()) value = defValue;										\
		return LSOK;																			\
	}																										\
	LSError err = Serialize(ar, name, value, minVersion); \
	if (err==LSNoEntry)	                                  \
	{value = defValue; CANCEL_ERROR_EXT;}										\
	return err; \
}

LSError Serialize(ParamArchive &ar, const RStringB &name, Time &value, int minVersion, Time defValue)
PARS_4_TO_3_EXT
LSError Serialize(ParamArchive &ar, const RStringB &name, TimeSec &value, int minVersion, TimeSec defValue)
PARS_4_TO_3_EXT
LSError Serialize(ParamArchive &ar, const RStringB &name, Vector3 &value, int minVersion, Vector3Par defValue)
PARS_4_TO_3_EXT
LSError Serialize(ParamArchive &ar, const RStringB &name, Matrix4 &value, int minVersion, Matrix4Par defValue)
PARS_4_TO_3_EXT_NODEFAULTS
LSError Serialize(ParamArchive &ar, const RStringB &name, Color &value, int minVersion, Color defValue)
PARS_4_TO_3_EXT_NODEFAULTS
LSError Serialize(ParamArchive &ar, const RStringB &name, GameValue &value, int minVersion, GameValue defValue)
PARS_4_TO_3_EXT_NODEFAULTS
#endif

LSError Serialize(ParamArchive &ar, const RStringB &name, NetworkId &value, int minVersion)
{
  return value.Serialize(ar, name, minVersion);
}

LSError Serialize(ParamArchive &ar, const RStringB &name, NetworkIdAll &value, int minVersion)
{
  return value.Serialize(ar, name, minVersion);
}
