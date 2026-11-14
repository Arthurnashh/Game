#include ".\progressbar.h"

static ProgressBarFunctions DefaultPB;
static ProgressBarFunctions *GProgressBarFunctions=0;

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