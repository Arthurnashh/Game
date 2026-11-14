#ifndef SIMUL_META_SHOWPROGRESSINTERFACE_H
#define SIMUL_META_SHOWPROGRESSINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace meta
		{
			class Node;
		}
		namespace standardnodes
		{
//! ShowProgressInterface
			class ShowProgressInterface
			{
			public:
				virtual void SetText(const simul::graph::meta::Node *,const char *)=0;	//< Text on progress indicator
				virtual void SetProgress(const simul::graph::meta::Node *,float)=0;		//< Progress from zero to 1.0
				virtual void SetProgressComplete(const simul::graph::meta::Node *)=0;	//< Hide the progress indicator.
				virtual bool GetCancel(const simul::graph::meta::Node *caller)=0;		//< Has the user pressed "cancel"?
			};
		}
	}
}
#endif
