#include "credits.hpp"

#if _MSC_VER && !defined INIT_SEG_COMPILER 
// we know no memory allocation is done here
// we want to be constructed as early as possible. ListCredits object must be created before all other object because otherwise we cannot be sure that initialization will be called before any call of macro REGISTER_CREDITS
// otherwise allocation scopes are not working
#pragma warning(disable:4074)
#pragma init_seg(compiler)
#define INIT_SEG_COMPILER
#endif

static AutoArray<CreditsInfo> ListCredits INIT_PRIORITY_URGENT;

void RegisterCredits(const char *name, const char *copyright, const char *disclaimer)
{
  ListCredits.Add(CreditsInfo(name,copyright,disclaimer));
}

int CountCredits()
{
  return ListCredits.Size();
}
const CreditsInfo &GetCredits(int i)
{
  return ListCredits[i];
}


//REGISTER_CREDITS("Real Virtuality", "2012 Bohemia Interactive","All rights reserved")
