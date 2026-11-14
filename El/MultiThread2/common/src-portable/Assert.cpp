#include "../Assert.h"
#include "stdio.h"
#include "stdlib.h"
#include <ctype.h>

#ifdef _DEBUG
#define ASSERT_MEMORY_SIZE 256
#else
#define ASSERT_MEMORY_SIZE 16
#endif



struct AssertMemoryInfo
{
  const char *file;
  int line;

  AssertMemoryInfo(const char *file=0, int line=0):file(file),line(line) {}  
};

static AssertMemoryInfo GAssertMemory[ASSERT_MEMORY_SIZE];
static int GAssertMemorySz=0;

static void SaveToMemory(const char *file, int line)
{
  if (GAssertMemorySz>=ASSERT_MEMORY_SIZE) return;
  GAssertMemory[GAssertMemorySz++]=AssertMemoryInfo(file,line);
}

static bool SearchInMemory(const char *file, int line)
{
  for (int i=0;i<GAssertMemorySz;i++)
    if (GAssertMemory[i].file==file && GAssertMemory[i].line==line) return true;
  return false;
}

void ImplementBreak(bool test,char const *expression,char const *file,int line,char const *desc)
{
  if (test) return;
  if (desc==0) desc="Assertation failed in following expression:";
  puts("+----------------------------------------------------------------------------------------------+");
  puts("ASSERT FAILED: ");
  printf("%s:%d: %s\n\tExpression: %s\n",file,line,desc,expression);
  puts("");
  if (SearchInMemory(file, line)==false)
  {
    bool rep;
    do
    {
        printf("(A)bort, (B)reak, (C)ontinue, (I)gnore Always, or (Enter) to continue\n"); 
        puts("");
        puts("+----------------------------------------------------------------------------------------------+");
        int chr=getchar();
    
        rep=false;
        switch (toupper(chr))
        {
            case 'A': exit(-1); break;
            case 'B': abort(); break; ///debugger (eclipse) shows this location. Otherwise core dump is generated
            case '\n':
            case 'C':;break;
            case 'I':SaveToMemory(file,line);break;
            default: rep=true;
        }
        while (chr!='\n') chr=getchar();
        
    }
    while (rep);
  }
}

void ImplementDebugPrint(const char *pattern, va_list args)
{
  //vfprintf(stderr,pattern,args);  //it gives segmentation fault??!
  vfprintf(stdout,pattern,args);
}

