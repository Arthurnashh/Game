// QStreamTest.cpp : Defines the entry point for the console application.
//

#include <El/elementpch.hpp>
#include <El/QStream/qStream.hpp>
#include <direct.h>
#include <mmsystem.h>


void ShortFileTest()
{
	QOFStream out;
	QOStrStream outStr;
	out.open("Test\\testo.txt");
	out << "12345\r\n";
	outStr << "12345\r\n";

	char buffer[256];

  {
    QIStrStream tst;
    tst.init(outStr.str(),outStr.pcount());

    for(;;)
    {
	    tst.readLine(buffer, 256);
      if (tst.fail() || tst.eof()) break;
      printf("%s\n",buffer);
    }
  }
	out.close();


	QIFStream in;
	in.open("test.txt");
	in.readLine(buffer, 256);

	QIFStream ind;
  //ind.SetBufferSize(100);
	ind.open("test.txt");
  for(;;)
  {
	  ind.readLine(buffer, 256);
    if (ind.fail() || ind.eof()) break;
    printf("%s\n",buffer);
  }

  ind.seekg(156,QIOS::beg);

	ind.readLine(buffer, 256);
  if (!ind.fail() && !ind.eof()) printf("%s\n",buffer);

  ind.seekg(8200,QIOS::beg);

	ind.readLine(buffer, 256);
  if (!ind.fail() && !ind.eof()) printf("%s\n",buffer);

  ind.seekg(599,QIOS::beg);

	ind.readLine(buffer, 256);
  if (!ind.fail() && !ind.eof()) printf("%s\n",buffer);
}

struct TimeScope
{
  DWORD start;
  const char *name;
  
  TimeScope(const char *n){start=::timeGetTime(),name=n;}
  ~TimeScope()
  {
    DWORD time = ::timeGetTime()-start;
    LogF("%s took %d ms",name,time);
  }
};

void CreateLongFile(const char *name, int size)
{
  TimeScope namedScope("CreateLongFile");
  QOFStream out(name);
  while (--size>=0)
  {
    out.put(rand()&0xff);
  }
  out.close();
}
void ReadLongFile(const char *name)
{
  TimeScope namedScope("ReadLongFile");
  QIFStream in;
  in.open(name);
  struct ProcessBuffer
  {
    int &totalSize;
    ProcessBuffer(int &res):totalSize(res){}
    void operator () (const char *buf, int size) const
    {
      totalSize += size;
    }
  };
  int totalSize;
  in.Process(ProcessBuffer(totalSize));
}



int main(int argc, char* argv[])
{
  timeBeginPeriod(1);
	_mkdir("Test");

  CreateLongFile("Test\\test.bin",16*1024*1024);
  ReadLongFile("Test\\test.bin");
  
  ShortFileTest();	

  timeEndPeriod(1);
	return 0;
}
