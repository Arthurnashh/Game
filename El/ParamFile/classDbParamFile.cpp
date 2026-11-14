#include <El/elementpch.hpp>
#include "classDbParamFile.hpp"

#include <Es/Memory/normalNew.hpp>

ParamClassDb::ParamClassDb()
{
}
ParamClassDb::~ParamClassDb()
{
}

bool ParamClassDb::Load(const char *name, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
	if (binary)
  {
    if (signature)
      return _file.ParseSigned(name, NULL, parentVariables, globalVariables);
    else
		  return _file.ParseBin(name, NULL, parentVariables, globalVariables);
  }
	else
  {
    Assert(!signature);
		return _file.Parse(name, NULL, parentVariables, globalVariables) == LSOK;
  }
}

bool ParamClassDb::Load(QIStream &in, bool binary, bool signature, IEvaluatorVariables *parentVariables, IEvaluatorNamespace *globalVariables)
{
  if (binary)
  {
    return _file.ParseBin(in, NULL, parentVariables, globalVariables);
  }
  else
  {
    Assert(!signature);
    _file.Parse(in, NULL, parentVariables, globalVariables);
    return in.fail() ? false : true;
  }
}

bool ParamClassDb::Save(const char *name, bool binary, bool signature)
{
	if (binary)
  {
    if (signature)
      return _file.SaveSigned(name);
    else
      return _file.SaveBin(name);
  }
	else
  {
    Assert(!signature);
    return _file.Save(name) == LSOK;
  }
}

bool ParamClassDb::Save(QOStream &out, bool binary, bool signature)
{
  if (binary)
  {
    return _file.SaveBin(out);
  }
  else
  {
    Assert(!signature);
    _file.Save(out, 0);
    return true;
  }
}

DEFINE_FAST_ALLOCATOR(ParamClassArrayItem)
DEFINE_FAST_ALLOCATOR(ParamClassEntry)

#include <Es/Memory/debugNew.hpp>
