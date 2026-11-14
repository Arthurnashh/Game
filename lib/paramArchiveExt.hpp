#ifdef _MSC_VER
#pragma once
#endif

#ifndef _PARAM_ARCHIVE_EXT_HPP
#define _PARAM_ARCHIVE_EXT_HPP

#include <El/ParamArchive/paramArchive.hpp>
#include <El/Color/colors.hpp>
class GameValue;
struct NetworkId;
struct NetworkIdAll;

/// Global template serialize for all types that are able to be serialized by ParamArchive
template <class Type>
LSError Serialize(ParamArchive &ar, const RStringB &name, Type &value, int minVersion)
{
  return ar.Serialize(name, value, minVersion);
}

LSError Serialize(ParamArchive &ar, const RStringB &name, Time &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, TimeSec &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, Vector3 &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, Matrix3 &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, Matrix4 &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, Color &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, GameValue &value, int minVersion);
LSError SerializeArray(ParamArchive &ar, const RStringB &name, AutoArray<Vector3> &value, int minVersion);
LSError SerializeArray(ParamArchive &ar, const RStringB &name, AutoArray<Color> &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, NetworkId &value, int minVersion);
LSError Serialize(ParamArchive &ar, const RStringB &name, NetworkIdAll &value, int minVersion);
struct CreatorId;
LSError Serialize(ParamArchive &ar, const RStringB &name, CreatorId &value, int minVersion);

// default arguments are now implemented using a global template
//LSError Serialize(ParamArchive &ar, const RStringB &name, Time &value, int minVersion, Time defValue);
//LSError Serialize(ParamArchive &ar, const RStringB &name, TimeSec &value, int minVersion, TimeSec defValue);
//LSError Serialize(ParamArchive &ar, const RStringB &name, Vector3 &value, int minVersion, Vector3Par defValue);
//LSError Serialize(ParamArchive &ar, const RStringB &name, Matrix4 &value, int minVersion, Matrix4Par defValue);
//LSError Serialize(ParamArchive &ar, const RStringB &name, Color &value, int minVersion, Color defValue);
//LSError Serialize(ParamArchive &ar, const RStringB &name, GameValue &value, int minVersion, GameValue defValue);

#define SERIAL_EXT(name) \
	CHECK(::Serialize(ar, #name, _##name, 1))
#define SERIAL_EXT_DEF(name, value) \
	CHECK(::Serialize(ar, #name, _##name, 1, value))

#endif
