// CheckBank.cpp : Defines the entry point for the console application.
//

#ifdef _XBOX
#include <xtl.h>
#endif

#include <El/elementpch.hpp>
#include <Es/Strings/rString.hpp>
#include <Es/Strings/bString.hpp>
#include <Es/Files/fileContainer.hpp>
#include <El/QStream/QBStream.hpp>
#include <El/CRC/crc.hpp>

static int LoadInt(QIStream &f)
{
  int i = 0;
  f.read(&i,sizeof(i));
  if (f.fail()) i = 0;
  return i;
}

static RString LoadFromFile(QIStream &in)
{
  BString<1024> buf;
  for(;;)
  {
    char c[2];
    int c0 = in.get();
    if (c0==EOF) break;
    if (c0==0) break;
    c[0] = c0;
    c[1] = 0;
    buf += c;
  }
  return RString(buf);
}

static void LoadFileInfo( FileInfoO &i, QIStream &f)
{
  // read zero terminated name
  char name[512];
  char *n=name;
  int maxLen=sizeof(name)-1;
  for (int l=0; l<maxLen; l++)
  {
    int c = f.get();
    if (c<0)
    {
      // error during file reading
      i.name = "";
      i.startOffset = 0;
      i.length = 0;
      return;
    }
    if (!c) break;
    *n++=c;
  }
  *n=0; // zero terminate in any case
  strlwr(name);
  i.name=name;
  i.compressedMagic = LoadInt(f);
  i.uncompressedSize = LoadInt(f);
  i.startOffset = LoadInt(f);
  i.time = LoadInt(f);
  i.length = LoadInt(f);
}

class CalculateSum
{
protected:
  CRCCalculator *_calculator;
  int _begin;
  int _end;
  int _pos;

public:
  CalculateSum(int begin, int end, CRCCalculator *calculator);
  void operator () (const void *buf, int size);
};

CalculateSum::CalculateSum(int begin, int end, CRCCalculator *calculator)
{
  _begin = begin;
  _end = end;
  _pos = 0;
  _calculator = calculator;
}

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

static bool logAll = false;

void CalculateSum::operator () (const void *buf, int size)
{
  if (!_calculator) return;

  int begin = max(0, _begin - _pos);
  int end = min(size, _end - _pos);

  if (begin < end) _calculator->Add((const char *)buf + begin, end - begin);

  _pos += size;
}

#ifdef _XBOX
void TestBank(RString path, QOStream &out)
#else
void TestBank(RString path)
#endif
{
  QIFStream in;
  in.open(path);

  int size = 0;
  int fileCount = 0;
  FileInfoO info;
  for (;;)
  {
    LoadFileInfo(info, in);
    if (info.name.GetLength() <= 0) break;
    fileCount++;
    size += info.length;
  }

  // if bank is empty, it may be "new bank" with product identification
  // check if there is normal terminator, or special
  if
  (
    fileCount == 0 &&
    info.compressedMagic == VersionMagic &&
    info.length == 0 &&
    info.time == 0
  )
  {
    // skip properties
    for (;;)
    {
      RString name = LoadFromFile(in);
      if (name.GetLength() == 0)
      {
        break;
      }
      RString value = LoadFromFile(in);
    }

    for (;;)
    {
      // from optimized bank different loading
      LoadFileInfo(info, in);
      if (info.name.GetLength() <= 0) break;
      fileCount++;
      size += info.length;
    }
  }

  int dataPos = in.tellg();
  int endPos = dataPos + size;

  int filesize = QIFileFunctions::GetFileSize(path);

  if (filesize == endPos)
  {
#ifndef _XBOX
    if (logAll) printf("%s - not checked - old version\n", (const char *)path);
#endif
  }
  else if (filesize == endPos + sizeof(bool) + sizeof(unsigned long))
  {
    bool reversed;
    unsigned long check;
    in.seekg(endPos);
    in.read(&reversed, sizeof(reversed));
    in.read(&check, sizeof(check));
    CRCCalculator calculator;
    calculator.Reset();
    if (reversed)
    {
      CalculateSum func1(dataPos, endPos, &calculator);
      in.seekg(0);
      in.Process(func1);
      CalculateSum func2(0, dataPos, &calculator);
      in.seekg(0);
      in.Process(func2);
    }
    else
    {
      CalculateSum func(0, endPos, &calculator);
      in.seekg(0);
      in.Process(func);
    }
    unsigned long result = calculator.GetResult();
    if (result == check)
    {
#ifndef _XBOX
      if (logAll)
        printf("%s - OK\n", (const char *)path);
#endif
    }
    else
    {
#ifdef _XBOX
      BString<256> buffer;
      sprintf(buffer, "%s - ERROR wrong CRC 0x%x != 0x%x\n", (const char *)path, result, check);
      out << buffer;
#else
      printf("%s - ERROR wrong CRC 0x%x != 0x%x\n", (const char *)path, result, check);
#endif
    }
  }
  else
  {
#ifdef _XBOX
    BString<256> buffer;
    sprintf(buffer, "%s - ERROR - wrong size %d, end of data %d\n", (const char *)path, filesize, endPos);
    out << buffer;
#else
    printf("%s - ERROR - wrong size %d, end of data %d\n", (const char *)path, filesize, endPos);
#endif
  }

  in.close();
}

#ifdef _XBOX

bool CheckBank(const FileItem &file, QOStream &out)
{
  TestBank(file.path + file.filename, out);
  return false; // continue with enumeration
}

void __cdecl main()
{
  QOFStream out;

  out.open("U:\\checkBanks.log");

  ForMaskedFileR("D:\\", "*.pbo", CheckBank, out);

  out.close();

  LD_LAUNCH_DASHBOARD ld;
  ZeroMemory(&ld, sizeof(ld));
  ld.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
  XLaunchNewImage(NULL, PLAUNCH_DATA(&ld));
}

#else

bool CheckBank(const FileItem &file, int &dummy)
{
  TestBank(file.path + file.filename);
  return false; // continue with enumeration
}

int main(int argc, char* argv[])
{
  argv++;
  argc--;
  if (argc<=0)
  {
Usage:
    printf("Usage\n");
    printf("   CheckBank {options} source [source]\n");
    printf("   source may be directory name or pbo file name (.log)\n");
    printf("\n");
    printf("   Options:\n");
    printf("   -all\n");
    return 1;		
  }
  while (argc > 0)
  {
    const char *arg = *argv;
    argc--;
    argv++;
    if (!arg) continue;
#if __GNUC__
    if (*arg == '-')
#else
    if (*arg == '-' || *arg == '/')
#endif
    {
      // some option
      arg++; // skip dash
      if (stricmp(arg, "all") == 0) logAll = true;
      else goto Usage;
    }
    else
    {
      // unget
      argc++;
      argv--;
      break;
    }
  }
  if (argc<=0) goto Usage;
  while (--argc >= 0)
  {
    RString source = *argv++;
    if (*source == '-') goto Usage;
    const char *ext = GetFileExt(GetFilenameExt(source));
    if (strcmpi(ext, ".pbo") == 0 && QIFileFunctions::FileExists(source))
    {
      TestBank(source);
    }
    else if (QIFileFunctions::DirectoryExists(source))
    {
      int dummy;
      ForMaskedFileR(source + RString("\\"), "*.pbo", CheckBank, dummy);
    }
  }
	return 0;
}

#endif