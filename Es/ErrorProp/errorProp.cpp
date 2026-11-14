#include <Es/essencepch.hpp>
#include "errorProp.hpp"
#include <Es/Framework/debugLog.hpp>

/**
@param propagateUp when false, a new propagation chain is started
*/
ErrorTry::ErrorTry(bool propagateUp)
{
  _parent = GErrorTry;
  _propagateUp = propagateUp && _parent;
  GErrorTry = this;
}

void ErrorTry::ErrorHandled(ErrorInfo *error)
{
  _error.Delete(error);
}

ErrorTry::~ErrorTry()
{
  // propagation should be destroyed in the opposite order it was created
  Assert(GErrorTry==this);
  GErrorTry = _parent;
  if (!_propagateUp)
  {
    // check if there is any unhandled error
    Assert(_error.First()==NULL);
    while (_error.First()!=NULL)
    {
      delete _error.First();
    }
  }
  else
  {
    if (_error.First())
    {
      _parent->_error.Move(_error);
    }
  }
}

void ErrorTry::Throw(ErrorInfo *error)
{
  _error.Insert(error);
}

ErrorTry *GErrorTry;
