#include <string.h>
#include "archivestream.h"


void ArchiveStream ::Reserved(int bytes)
{
  char buff[256];
  int i;
  if (IsError()) return;
  if (IsStoring())
    if (bytes>256) memset(buff,0,256); else memset(buff,0,bytes);
  for (i=0;i<bytes-256;i+=256) DataExchange(buff,256);
  if (IsError()) return;
  if (i<bytes) DataExchange(buff,bytes-i);
}
