#ifndef SIMUL_META_RIGIDBODYHIERARCHYINTERFACE_H
#define SIMUL_META_RIGIDBODYHIERARCHYINTERFACE_H
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
// Graph
// 
			class RigidBodyHierarchyInterface
			{
			public:
				virtual int GetNumMatrices() const=0;
				virtual const float *GetMatrix(int index) const=0;
				virtual int GetParent(int index) const=0;
				virtual const char *GetElementName(int index) const=0;
				virtual const float *GetVelocity() const =0;
				virtual const float *GetAngularVelocity() const =0;
			};
// 
		}
	}
}
#endif