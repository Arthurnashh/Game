#include "precomp.h"

#include "TabbedLogger.h"
#include <iostream>

int TabbedLogger::tab=-1;
TabbedLogger::TabbedLogger()
{
	tab++;
	this_tab=tab;
}

TabbedLogger::~TabbedLogger()
{
	tab--;
}

void TabbedLogger::Log(const char *txt)
{
	for(int i=0;i<this_tab;i++)
		std::cout<<"\t";
	std::cout<<txt<<std::endl;
}
void TabbedLogger::Log(const wchar_t *txt)
{
	for(int i=0;i<this_tab;i++)
		std::cout<<L"\t";
	std::cout<<txt<<std::endl;
}