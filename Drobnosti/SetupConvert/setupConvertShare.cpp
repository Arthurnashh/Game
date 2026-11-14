#include "setupConvertShare.hpp"

void operator << (SerializeBinStream &f, BankInfo &bi)
{
  bi.SerializeBin(f);
}
void operator << (SerializeBinStream &f, ResFileItem &fi)
{
  fi.SerializeBin(f);
}
