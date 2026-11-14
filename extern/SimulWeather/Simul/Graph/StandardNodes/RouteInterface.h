#ifndef ROUTEINTERFACE_H
#define ROUTEINTERFACE_H


namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			class RouteInterface
			{
			public:
				virtual unsigned GetNumVertices() const=0;
				virtual const float *GetVertex(unsigned) const=0;

				virtual void InsertVertex(unsigned)=0;
				virtual void SetVertex(unsigned i,const float *)=0;
			};
		}
	}
}
#endif // ifndef WORLD_H

