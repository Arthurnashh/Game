#ifndef SETUP_CONVERT_SHARE
#define SETUP_CONVERT_SHARE

// The ResFileItem and BankInfo are used also in Setup project. Do not change it. //TODO - share it
#include <El/QStream/serializeBin.hpp>
struct ResFileItem
{
  RString name;
  int len;
  char whereSaved;

  ResFileItem() {};
  ResFileItem(RString nm, int ln, char kam) {name=nm; len=ln; whereSaved=kam;}

  void SerializeBin(SerializeBinStream &stream)
  {
    stream << name;
    stream << len;
    stream << whereSaved;
  }

  ClassIsMovable(ResFileItem);
};

struct BankInfo
{
  RString testCode;
  AutoArray<int> bankLen;

  BankInfo() : testCode("BANK") {};
  BankInfo(int num) : bankLen(num),testCode("BANK") {};
  void Add(unsigned int len) { bankLen.Add(len); }

  void SerializeBin(SerializeBinStream &stream)
  {
    if (stream.IsLoading())
    {
      testCode.CreateBuffer(5);
    }
    char *buf = testCode.MutableData();
    for (int i=0; i<5; i++)
    {
      stream << buf[i]; //this should be there for testing we are interpretting right data when trying to read from the end of file
    }
    buf[4] = 0;
    if (stricmp(buf, "BANK")==0)
      stream << bankLen;
  }

  ClassIsMovable(BankInfo);
};

void operator << (SerializeBinStream &f, BankInfo &bi);
void operator << (SerializeBinStream &f, ResFileItem &fi);

#endif
