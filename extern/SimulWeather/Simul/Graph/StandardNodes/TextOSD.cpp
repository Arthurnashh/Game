#include "TextOSD.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
using namespace simul::graph::standardnodes;

static char osdtext[1024];	

int TextOSD::AddText(float x,float y)
{
	return AddText(x,y,24,0,0xFFFFFFFF,"");
}
int TextOSD::AddText(float x,float y,int size,char align,int colour,const char *text)
{
	if(textItems.size())
		current_ident++;
	TextItem &T=textItems[current_ident];
	T.x=x;
	T.y=y;
	T.colour=colour;
	if(text)
		T.text=text;
	T.align=align;
	T.size=size;
	return current_ident;
}

void TextOSD::SetTextInternal(int ident,const char *text)
{
	if(textItems.find(ident)==textItems.end())
	{
		std::cerr<<"TextOSD::SetText - Not found ident "<<ident<<std::endl;
		return;
	}
	TextItem &T=textItems[ident];
	T.text=text;
}

void TextOSD::SetText(int ident,const char *fmt,...)
{
	if(!fmt)
		return;
	static va_list ap;
	va_start(ap,fmt);			
		vsprintf_s(osdtext,1024,fmt,ap);
	va_end(ap);
	SetTextInternal(ident,osdtext);
}

void TextOSD::SetTextPosition(int ident,float x,float y)
{
	TextItem &T=textItems[ident];
	T.x=x;
	T.y=y;
}

TextOSD::TextOSD()
{
	current_ident=0;
}

TextOSD::~TextOSD()
{
}

// from TextOSDInterface
int TextOSD::GetNumItems()
{
	return (int)textItems.size();
}
int TextOSD::GetItemIdent(int index)
{
	std::map<int,TextItem>::iterator it=textItems.begin();
	for(int i=0;i<index;i++)
		it++;
	return it->first;
}
const char *TextOSD::GetItemText(int ident)
{
	assert(textItems.find(ident)!=textItems.end());
	if(textItems.find(ident)==textItems.end())
		return 0;
	return textItems[ident].text.c_str();
}
int TextOSD::GetItemPointSize(int ident)
{
	assert(textItems.find(ident)!=textItems.end());
	if(textItems.find(ident)==textItems.end())
		return 0;
	return textItems[ident].size;
}
char TextOSD::GetItemAlign(int ident)
{
	assert(textItems.find(ident)!=textItems.end());
	if(textItems.find(ident)==textItems.end())
		return 0;
	return textItems[ident].align;
}
const float *TextOSD::GetItemPos(int ident)
{
	assert(textItems.find(ident)!=textItems.end());
	if(textItems.find(ident)==textItems.end())
		return 0;
	return &(textItems[ident].x);
}
int TextOSD::GetItemColour(int ident)
{
	assert(textItems.find(ident)!=textItems.end());
	if(textItems.find(ident)==textItems.end())
		return 0;
	ident;
	return (textItems[ident].colour);
}

namespace simul
{
	namespace graph
	{	
		namespace standardnodes
		{	
			const char *FormatText(const char *fmt,...)
			{
				if(!fmt)
					return "";
				static va_list ap;			
				if(fmt==NULL)
					return "";		
				va_start(ap,fmt);			
					vsprintf_s(osdtext,1024,fmt,ap);
				va_end(ap);
				return osdtext;
			}
		}
	}
}