#pragma once
#include "Simul/Base/Export.h"
SIMUL_BASE_EXPORT_CLASS TabbedLogger
{
	static int tab;
	int this_tab;
public:
	TabbedLogger();
	~TabbedLogger();
	void Log(const char *txt);
	void Log(const wchar_t *wtxt);
};

