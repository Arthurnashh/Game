#include "common_Win.h"

#include ".\tls.h"

namespace MultiThread
{


TlsBase::TlsBase(void)
{
  _index=TlsAlloc();
  if (_index==TLS_OUT_OF_INDEXES)
    throw TlsOutOfIndexes;

}

TlsBase::~TlsBase(void)
{
  TlsFree(_index);
}

void TlsBase::SetValue(void *ptr)
{
  TlsSetValue(_index,ptr);
}
void *TlsBase::GetValue()
{
  return TlsGetValue(_index);
}

};