// WFilePath.cpp: implementation of the WFilePath class.
//
//////////////////////////////////////////////////////////////////////

#include "objshape.h"
//#include "Objektiv2.h"
#include "WFilePath.h"
#include <malloc.h>
#include <direct.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WFilePath::WFilePath(const char *name)
  {
  char curr[_MAX_PATH+1];
  _getcwd(curr,_MAX_PATH );
  Parse(curr);
  folder+=filename+"\\";
  filename="*.*";
  Parse(name);
  InternalFormat();
  }

//--------------------------------------------------

void WFilePath::InternalFormat()
  {
  WString::operator=(drive+folder+filename);
  }

//--------------------------------------------------

void WFilePath::Parse(const char *pathname)
  {
  if (pathname==NULL) return;  
  char *beginpath;
  char *lastslash;
  if (pathname[0] && pathname[1]==':')
    {
    drive=pathname[0];
    drive+=pathname[1];
    beginpath=(char *)pathname+2;
    }
  else 	
    beginpath=(char *)pathname;	
  lastslash=strrchr(pathname,'\\');
  if (lastslash!=NULL)
    {
    int size=lastslash-beginpath+1;
    char *c=(char *)alloca(size+1);
    strncpy(c,beginpath,size);
    c[size]=0;
    folder=c;
    lastslash++;
    }
  else lastslash=beginpath;
  if (*lastslash)
    {
    filename=lastslash;	
    }
  }

//--------------------------------------------------

bool WFilePath::SetExtension(const char *ext)
  {
  char *c=(char *)alloca(filename.GetLength()+strlen(ext)+2);
  strcpy(c,filename);
  char *d=strrchr(c,'.');
  if (d==NULL) strcat(c,ext);else strcpy(d,ext);
  filename=c;  
  InternalFormat();  
  return true;
  }

//--------------------------------------------------

const char *WFilePath::GetNameFromPath(const char *path)
  {
  char *c=strrchr(path,'\\');
  if (c!=NULL) c++;else return path;
  return c;
  }

//--------------------------------------------------

