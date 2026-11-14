#ifndef TEXTOSD_H
#define TEXTOSD_H

#include "Simul/Graph/StandardNodes/TextOSDInterface.h"
#include "Export.h"
#include <map>
#include <string>
// C4251 "class '...' needs to have dll-interface to be used by clients of ..."
// This warning can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(push)
#pragma warning(disable:4251)

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! A class that stores text items, along with onscreen positions for them. This implements the
			//! TextOSDInterface interface. Derive from TextOSD to make a class that can easily draw text.
			SIMUL_STD_EXPORT_CLASS TextOSD:public simul::graph::standardnodes::TextOSDInterface
			{
				int current_ident;
				struct TextItem
				{
					std::string text;
					int size;
					int colour;
					char align;
					float x,y;
				};
				std::map<int,TextItem> textItems;
				void SetTextInternal(int ident,const char *text);
			public:
				TextOSD();
				virtual ~TextOSD();
			// from TextOSDInterface
				virtual int GetNumItems();
				virtual int GetItemIdent(int index);
				virtual const char *GetItemText(int ident);
				virtual int GetItemPointSize(int ident);
				virtual char GetItemAlign(int ident);
				virtual const float *GetItemPos(int ident);
				virtual int GetItemColour(int ident);

				int AddText(float x,float y);
				int AddText(float x,float y,int size,char align,int colour,const char *text=NULL);
				void SetText(int ident,const char *text,...);
				void SetTextPosition(int ident,float x,float y);
			};
			extern SIMUL_STD_EXPORT const char * FormatText(const char *fmt,...); //< Returns a formatted text string.
		}
	}
}

#pragma warning(pop)
#endif

