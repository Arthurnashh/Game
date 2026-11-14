#include ".\progressbar.h"
#include <El/MultiThread/Tls.h>

static TLS(ProgressBarFunctions) GProgressBarFunctions=0;

static ProgressBarFunctions DefaultPB;
ProgressBarFunctions *ProgressBarFunctions::GetGlobalProgressBarInstance()
{
  if (GProgressBarFunctions==0) return &DefaultPB;
  else return GProgressBarFunctions;
}

ProgressBarFunctions *ProgressBarFunctions::SelectGlobalProgressBarHandler(ProgressBarFunctions *newHandler)
{
  ProgressBarFunctions *old=GProgressBarFunctions;
  GProgressBarFunctions=newHandler;
  return old;
}
