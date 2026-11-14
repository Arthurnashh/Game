#ifndef SIMUL_META_TEXTOSDINTERFACE_H
#define SIMUL_META_TEXTOSDINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! An interface for a class that can show text on-screen.
			class TextOSDInterface
			{
			public:
				//! Get how many text items are to be shown.
				virtual int GetNumItems()=0;
				//! return an int that uniquely identifies the item - if items are re-arranged, the idents are also re-arranged,
				//! so the ident always refers to the same text item.
				virtual int GetItemIdent(int index)=0;
				//! Get the text for the item with the stated \a ident.
				virtual const char *GetItemText(int ident)=0;
				//! Get the font size for the item with the stated \a ident.
				virtual int GetItemPointSize(int ident)=0;
				//! Get the alignment for the item with the stated \a ident. Either -1, 0 or 1.
				virtual char GetItemAlign(int ident)=0;
				//! Get the xy position for the item with the stated \a ident.
				virtual const float *GetItemPos(int ident)=0;
				//! Get the colour as a 32-bit hex integer (RRGGBBAA) for the item with the stated \a ident.
				virtual int GetItemColour(int ident)=0;
			};
		}
	}
}
#endif