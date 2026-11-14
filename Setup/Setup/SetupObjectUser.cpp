/////////////////////////////////////////////////////////////////////////////
// Modifikace hlavního okna


#include "stdafx.h"
#include "SetupObject.h"
#include "Setup.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// jednotlivé kroky instalace

int	CSetupObject::DoUserSetupStep(int nState)
{
	return SETUP_STATE_FINAL;
};

