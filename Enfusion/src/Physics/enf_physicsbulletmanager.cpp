//-----------------------------------------------------------------------------
// File: enf_physicsmqonmanager.cpp
//
// Desc: Interface between world geometry and Bullet physics
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
//#include "enf_engine.h"
//#include "physics/enf_physicsbullet.h"
//#include "physics/enf_physicsbulletinternal.h"
#include "Physics/enf_physicsbullet.h"
#include "Physics/enf_physicsbulletinternal.h"
//#include "common/enf_parser.h"
//#include "vobject/enf_vobject.h"
//
//#include "enf_scriptapi.h"
//#include "enf_resourcemanager.h"
//
//#include "enf_iff.h"
//#include "vobject/meshobject/enf_xobfile.h"
#include "baseWorld/enf_shapes.h"

#include "BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btStridingMeshInterface.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"

#include "lib\Shape\shape.hpp"


#include "El\Evaluator\expressImpl.hpp"
#include "El\ParamFile\paramFile.hpp"
#include "lib\paramFileExt.hpp"

namespace enf
{
BulletManager* g_pBulletManager = 0;

/////////////////////////////////////////
//debugging purposes
struct SPhysMaterials
{
  char* name;
  float dynamicfriction;
	float restitution;
	float lateralstiffness;
	float longitudalstiffness;
	bool aniso;
	bool replaceOp;
	bool singleSided;
  SPhysMaterials(float rest, float frict, char* n, bool ss = false, bool rop = false, bool an = false, float latstiff = 1.0f, float longstiff = 1.0f):
    restitution(rest), 
    dynamicfriction(frict), 
    name(n), 
    singleSided(ss),
    replaceOp(rop),
    aniso(an),
    lateralstiffness(latstiff),
    longitudalstiffness(longstiff)
    {}
};

const int physMatCnt = 8;
const SPhysMaterials physmats[] = {
  SPhysMaterials(0.0f, 0.8f, "material/default"),
  SPhysMaterials(0.9f, 0.1f, "material/ragdollflesh"),
  SPhysMaterials(0.1f, 0.75f, "material/grass"),
  SPhysMaterials(0.1f, 0.7f, "material/wood"),
  SPhysMaterials(0.15f, 0.7f, "material/iron"),
  SPhysMaterials(0.1f, 0.9f, "material/beton"),
  SPhysMaterials(0.15f, 0.7f, "material/metal"),
  SPhysMaterials(0.1f, 0.7f, "material/plastic"),
};

//-----------------------------------------------------------------------
Shape* BulletManager::GetCached(btCollisionShape* shape)
{
	uint cacheIndex = m_ShapeCache.GetIndexOf(MakePair(shape, (Shape*)NULL));
	if(cacheIndex != INDEX_NOT_FOUND)
		return m_ShapeCache[cacheIndex].Data;
	return NULL;
}
//-----------------------------------------------------------------------
void BulletManager::InsertCached(btCollisionShape* shape, Shape* sh)
{
	m_ShapeCache.Insert(MakePair(shape, sh));
}

size_t BulletManager::m_iAllocatedBlocks = 0;
uint BulletManager::m_iAllocated = 0;

//-----------------------------------------------------------------------
uint BulletManager::GetAllocatedBlocks() const
{
	return m_iAllocatedBlocks;
}

//-----------------------------------------------------------------------
size_t BulletManager::GetAllocation() const
{
	return m_iAllocated;
}

//#define ENF_MEMTRACK
//-----------------------------------------------------------------------
void* BulletAllocFunc(size_t size, int line, const char* file)
{
	void* ptr;
	if(file == NULL)
		ptr = GMemFunctions->New(size, __FILE__, __LINE__);
	else
		ptr = GMemFunctions->New(size, file, line);
#ifdef ENF_MEMTRACK
	BulletManager::m_iAllocatedBlocks++;
  BulletManager::m_iAllocated += GMemFunctions->GetSize(ptr);
#endif
	return ptr;
}

void BulletFreeFunc(void *memblock)
{
#ifdef ENF_MEMTRACK
	BulletManager::m_iAllocatedBlocks--;
	BulletManager::m_iAllocated -= GMemFunctions->GetSize(memblock);
#endif
	GMemFunctions->Delete(memblock);
}

//-----------------------------------------------------------------------
BulletPhysicsMaterial* BulletManager::FindMaterial(const char* name) const
{
	return m_MaterialsHash.Find(name);
}

//-----------------------------------------------------------------------
BulletPhysicsMaterial* BulletManager::CloneMaterialData(BulletPhysicsMaterial* cloned, const BulletPhysicsMaterial* mat)
{
	Assert(mat != NULL);
	Assert(cloned != NULL);

	cloned->m_iMaterialIndex = mat->m_iMaterialIndex;
	cloned->m_fRestitution = mat->m_fRestitution;
	cloned->m_fDynamicFriction = mat->m_fDynamicFriction;
	cloned->m_fLateralStiffness = mat->m_fLateralStiffness;
	cloned->m_fLongitudalStiffness = mat->m_fLongitudalStiffness;
	btMaterial* btMat = mat->m_pBtMaterial;
	if(btMat && cloned->m_pBtMaterial)
	{
		cloned->m_pBtMaterial->setFriction(btMat->getFriction());
		cloned->m_pBtMaterial->setRestitution(btMat->getRestitution());
		if(mat->m_pBtMaterial->hasAnisotropicFriction())
			cloned->m_pBtMaterial->setAnisotropicFriction(btMat->getAnisotropicFriction());
	}

	return cloned;
}

//-----------------------------------------------------------------------
BulletPhysicsMaterial* BulletManager::CloneMaterial(CStr dest, uint matindex, const BulletPhysicsMaterial* mat)
{
	Assert(mat != NULL);

	BulletPhysicsMaterial* cloned = new BulletPhysicsMaterial(dest);
	CloneMaterialData(cloned, mat);
	cloned->m_iMaterialIndex = matindex << MATERIAL_SHIFT;
	InsertMaterial(cloned);
	return cloned;
}

//-----------------------------------------------------------------------
BulletPhysicsMaterial::BulletPhysicsMaterial(CStr name):
	PhysicsMaterial(name.cstr()),
	m_pBtMaterial(new btMaterial()),
	m_fRestitution(0.0f),
	m_fDynamicFriction(0.0f),
	m_fLateralStiffness(1.0f),
	m_fLongitudalStiffness(1.0f),
	m_bSingleSided(false)
{
	m_pBtMaterial->setUserPointer(this);
}

//-----------------------------------------------------------------------
BulletPhysicsMaterial::BulletPhysicsMaterial():
	PhysicsMaterial(),
	m_pBtMaterial(new btMaterial()),
	m_fRestitution(0.0f),
	m_fDynamicFriction(0.0f),
	m_fLateralStiffness(1.0f),
	m_fLongitudalStiffness(1.0f),
	m_bSingleSided(false)
{
	m_pBtMaterial->setUserPointer(this);
}

//-----------------------------------------------------------------------
BulletPhysicsMaterial::~BulletPhysicsMaterial()
{
	SAFE_DELETE(m_pBtMaterial);
}

//-----------------------------------------------------------------------
BulletPhysicsMaterialCombiner::~BulletPhysicsMaterialCombiner()
{
	SAFE_DELETE(m_pBtMaterial);
}

//-----------------------------------------------------------------------
void BulletPhysicsMaterial::Load(ParamEntryPar entry)
{
	///read all params
	//RStringB surfaceType = entry.GetName();

  //m_roughness = entry>>"rough";
	m_fDynamicFriction = entry>>"friction";
  m_fRestitution = entry>>"restitution";

  
	///init
	m_pBtMaterial->setFriction(m_fDynamicFriction);
	m_pBtMaterial->setRestitution(m_fRestitution);

	m_pBtMaterial->setSingleSided(m_bSingleSided);
}
//-----------------------------------------------------------------------
/*
bool ParseAttr(CParser& tp, const char* tagname, SString<>& data)
{
	if(CStr(tp.CToken->Text) != tagname)
		return false;

	if(!tp.NextToken(true))
		return false;

	if(!tp.AssertToken(CParser::TAssign))
		return false;

	data = tp.CToken->Text;

	tp.NextToken(false); //skip data token
	return true;
}
*/
//-----------------------------------------------------------------------
BulletObjectGeometry::~BulletObjectGeometry()
{
	{
		PoCriticalSection lock(g_pBulletManager->m_Mutex);

    for(int n = 0; n < m_Elements.GetCardinality(); n++)
		{
			btCollisionShape* shape = m_Elements[n].m_pShape;
			if(shape)
			{
				g_pBulletManager->m_ShapeCache.Remove(MakePair(shape, (Shape*)NULL));
				delete shape;
			}
		}
    
		g_pBulletManager->m_ObjectGeometries.Remove(this);
		g_pBulletManager->m_ObjectGeometriesHash.Remove(GetName().cstr());
	}
	g_pBulletManager->Release();
}

//-----------------------------------------------------------------------
uint BulletObjectGeometry::GetNumElements() const
{
	return m_Elements.GetCardinality();
}

////-----------------------------------------------------------------------
//btConvexHullShape* LoadConvexCollider(const xob::DXConvexCollider& convexCollider, IFFInput* iff)
//{
//  uint numVerts = convexCollider.NumVerts;
//	uint numFaces = convexCollider.NumFaces;
//	uint numEdges = convexCollider.NumEdges;
//	uint numFaceEdges = convexCollider.NumFaceEdges;
//
//	xob::Vec3* verts = ALLOCA(xob::Vec3, numVerts);
//	iff->Read(verts, sizeof(xob::Vec3) * numVerts);
//	for(uint v = 0; v < numVerts; v++)
//	{
//		verts[v][0] = verts[v][0] * MEQ_WORLDISCALE;
//		verts[v][1] = verts[v][1] * MEQ_WORLDISCALE;
//		verts[v][2] = verts[v][2] * MEQ_WORLDISCALE;
//	}
//	btConvexHullShape* shape = new btConvexHullShape((const btScalar*) verts, numVerts, sizeof(xob::Vec3));
//
//	xob::DXConvexEdge* edges = ALLOCA(xob::DXConvexEdge, numEdges);
//	iff->Read(edges, sizeof(xob::DXConvexEdge) * numEdges);
//
//	ushort* faceEdges = ALLOCA(ushort, numFaceEdges);
//	iff->Read(faceEdges, sizeof(ushort) * numFaceEdges);
//
//	xob::DXConvexFace* faces = ALLOCA(xob::DXConvexFace, numFaces);
//	iff->Read(faces, sizeof(xob::DXConvexFace) * numFaces);
//
//	return shape;
//}
//
////-----------------------------------------------------------------------
//class BulletTriangleMeshShape: public btStridingMeshInterface, public btBvhTriangleMeshShape
//{
//protected:
//	mutable btVector3 m_aabbMin;
//	mutable btVector3 m_aabbMax;
//
//	xob::Vec3* m_verts;
//	uint	m_numVerts;
//	ushort* m_indices;
//	uint	m_numIndices;
//
//public:
//
//	BT_DECLARE_ALIGNED_ALLOCATOR();
//
//	BulletTriangleMeshShape(uint numverts, uint numindices):
//		btBvhTriangleMeshShape()
//	{
//		m_verts = new xob::Vec3[numverts];
//		m_numVerts = numverts;
//		m_indices = new ushort[numindices];
//		m_numIndices = numindices;
//		m_aabbMin = btVector3(999999.f,999999.f,999999.f);
//		m_aabbMax = btVector3(-999999.f,-999999.f,-999999.f);
//	}
//
//	virtual ~BulletTriangleMeshShape()
//	{
//		SAFE_DELETE_ARRAY(m_verts);
//		SAFE_DELETE_ARRAY(m_indices);
//	}
//
//	virtual void	getLockedVertexIndexBase(unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& vertexStride,unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart=0)
//	{
//		*vertexbase = (unsigned char*)m_verts;
//		numverts = m_numVerts;
//		type = PHY_FLOAT;
//		vertexStride = sizeof(Vector3);
//		*indexbase = (unsigned char *)m_indices;
//		indexstride = sizeof(ushort) * 3;
//		numfaces = m_numIndices / 3;
//		indicestype = PHY_SHORT;
//	}
//
//	virtual void	getLockedReadOnlyVertexIndexBase(const unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& vertexStride,const unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart=0) const
//	{
//		*vertexbase = (unsigned char*)m_verts;
//		numverts = m_numVerts;
//		type = PHY_FLOAT;
//		vertexStride = sizeof(Vector3);
//		*indexbase = (unsigned char *)m_indices;
//		indexstride = sizeof(ushort) * 3;
//		numfaces = m_numIndices / 3;
//		indicestype = PHY_SHORT;
//	}
//
//	/// unLockVertexBase finishes the access to a subpart of the triangle mesh
//	/// make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
//	virtual void	unLockVertexBase(int subpart)
//	{
//		(void)subpart;
//	}
//
//	virtual void	unLockReadOnlyVertexBase(int subpart) const {(void)subpart;}
//
//	/// getNumSubParts returns the number of seperate subparts
//	/// each subpart has a continuous array of vertices and indices
//	virtual int		getNumSubParts() const { 
//		return 1;
//	}
//
//	virtual void	preallocateVertices(int numverts){(void) numverts;}
//	virtual void	preallocateIndices(int numindices){(void) numindices;}
//
//	virtual bool	hasPremadeAabb() const { return true; }
//	virtual void	setPremadeAabb(const btVector3& aabbMin, const btVector3& aabbMax ) const
//	{
//		m_aabbMin = aabbMin;
//		m_aabbMax = aabbMax;
//	}
//
//	virtual void	getPremadeAabb(btVector3* aabbMin, btVector3* aabbMax ) const
//	{
//		*aabbMin = m_aabbMin;
//		*aabbMax = m_aabbMax;
//	}
//
//};
////-----------------------------------------------------------------------
//btBvhTriangleMeshShape* LoadTriMeshCollider(const xob::DXTriMeshCollider& trimeshCollider, IFFInput* iff)
//{
//	uint numVerts = trimeshCollider.NumVerts;
//	uint numFaces = trimeshCollider.NumFaces;
//
//	BulletTriangleMeshShape* geom = new BulletTriangleMeshShape(numVerts, numFaces * 3);
//
//	{
//#pragma message("jz todo: vectorization adept")
//		xob::Vec3* verts;
//		ushort* indices;
//		int numVerts;
//		PHY_ScalarType type;
//		int vertexStride;
//		int indexStride;
//		int numFaces;
//		PHY_ScalarType indicesType;
//		geom->getLockedVertexIndexBase((unsigned char**)&verts, numVerts, type, vertexStride,(unsigned char **)&indices, indexStride, numFaces, indicesType, 0);
//		Assert(vertexStride == sizeof(xob::Vec3));
//		iff->Read(verts, sizeof(xob::Vec3) * numVerts);
//		float minx = FLT_MAX, maxx = -FLT_MAX;
//		float miny = FLT_MAX, maxy = -FLT_MAX;
//		float minz = FLT_MAX, maxz = -FLT_MAX;
//
//		for(int v = 0; v < numVerts; v++)
//		{
//			float x = verts[v][0] * MEQ_WORLDISCALE;
//			minx = enf_min(minx, x);
//			maxx = enf_max(maxx, x);
//			verts[v][0] = x;
//			float y = verts[v][1] * MEQ_WORLDISCALE;
//			miny = enf_min(miny, y);
//			maxy = enf_max(maxy, y);
//			verts[v][1] = y;
//			float z = verts[v][2] * MEQ_WORLDISCALE;
//			minz = enf_min(minz, z);
//			maxz = enf_max(maxz, z);
//			verts[v][2] = z;
//		}
//		geom->setPremadeAabb(btVector3(minx, miny, minz), btVector3(maxx, maxy, maxz));
//
//		iff->Read(indices, sizeof(ushort) * numFaces * 3);	
//		for(int i = 0; i < (numFaces * 3); i += 3)
//		{
//			ushort i0 = indices[i + 2];
//			ushort i1 = indices[i + 1];
//			ushort i2 = indices[i + 0];
//			indices[i + 0] = i2;
//			indices[i + 1] = i1;
//			indices[i + 2] = i0;
//		}
//		geom->unLockVertexBase(0);
//
//		geom->init(geom, true, true);
//	}
//	return geom;
//}
//
//struct MatRange
//{
//	const PhysicsMaterial*	Material;
//	uint					LastFace;
//};
////-----------------------------------------------------------------------
//btBvhTriangleMeshShape* LoadMultiTriMeshCollider(const xob::DXMultiTriMeshCollider& trimeshCollider, IFFInput* iff, const MatRange* mats)
//{
//	uint numVerts = trimeshCollider.NumVerts;
//	uint numFaces = trimeshCollider.NumFaces;
//
//	BulletTriangleMeshShape* geom = new BulletTriangleMeshShape(numVerts, numFaces * 3);
//
//	{
//
//		xob::Vec3* verts;
//		ushort* indices;
//		int numVerts;
//		PHY_ScalarType type;
//		int vertexStride;
//		int indexStride;
//		int numFaces;
//		PHY_ScalarType indicesType;
//		geom->getLockedVertexIndexBase((unsigned char**)&verts, numVerts, type, vertexStride,(unsigned char **)&indices, indexStride, numFaces, indicesType, 0);
//		Assert(vertexStride == sizeof(xob::Vec3));
//		iff->Read(verts, sizeof(xob::Vec3) * numVerts);
//		float minx = FLT_MAX, maxx = -FLT_MAX;
//		float miny = FLT_MAX, maxy = -FLT_MAX;
//		float minz = FLT_MAX, maxz = -FLT_MAX;
//
//		for(int v = 0; v < numVerts; v++)
//		{
//			float x = verts[v][0] * MEQ_WORLDISCALE;
//			minx = enf_min(minx, x);
//			maxx = enf_max(maxx, x);
//			verts[v][0] = x;
//			float y = verts[v][1] * MEQ_WORLDISCALE;
//			miny = enf_min(miny, y);
//			maxy = enf_max(maxy, y);
//			verts[v][1] = y;
//			float z = verts[v][2] * MEQ_WORLDISCALE;
//			minz = enf_min(minz, z);
//			maxz = enf_max(maxz, z);
//			verts[v][2] = z;
//		}
//		geom->setPremadeAabb(btVector3(minx, miny, minz), btVector3(maxx, maxy, maxz));
//
//		iff->Read(indices, sizeof(ushort) * numFaces * 3);	
//		for(int i = 0; i < (numFaces * 3); i += 3)
//		{
//			ushort i0 = indices[i + 2];
//			ushort i1 = indices[i + 1];
//			ushort i2 = indices[i + 0];
//			indices[i + 0] = i2;
//			indices[i + 1] = i1;
//			indices[i + 2] = i0;
//		}
//		geom->unLockVertexBase(0);
//
//		geom->init(geom, true, true);
//
//		btMaterialRange* ranges = geom->initMaterialRanges(trimeshCollider.NumMaterials);
//
//		for(uint n = 0; n < trimeshCollider.NumMaterials; n++)
//		{
//			ranges[n].m_material = static_cast<const BulletPhysicsMaterial*>(mats[n].Material)->m_pBtMaterial;
//			ranges[n].m_lastFace = mats[n].LastFace;
//		}
//	}
//	return geom;
//}
//
////-----------------------------------------------------------------------
//ObjectGeometry* BulletManager::LoadGeometry(CStr name, IFFInput* iff, const char** stringPool, const Vector3& centerOfMass)
//{
//	//TimeMeasureScope scope("BulletManager::LoadGeometry");
//
//	BulletObjectGeometry* objgeom = new BulletObjectGeometry(name, centerOfMass);
//
//	while(!iff->EndOfHunk())
//	{
//		union
//		{
//			xob::DXCollider collider;
//			xob::DXConvexCollider convexCollider;
//			xob::DXBoxCollider boxCollider;
//			xob::DXSphereCollider sphereCollider;
//			xob::DXCapsuleCollider capsuleCollider;
//			xob::DXCylinderCollider cylinderCollider;
//			xob::DXTriMeshCollider trimeshCollider;
//			xob::DXMultiTriMeshCollider multiTrimeshCollider;
//		};
//		iff->Read(&collider, sizeof(collider));
//
//		GeometryType gtype = GT_NONE;
//		Matrix43 frame;
//		for(uint r = 0; r < 4; r++)
//		{
//			Vector3 row;
//			row.x = collider.Frame[r][0];
//			row.y = collider.Frame[r][1];
//			row.z = collider.Frame[r][2];
//			frame.SetRow(r, row);
//		}
//		float weight = collider.Weight;
//		uint parentNode = collider.Parent;
//		if(parentNode == 0xff)
//			parentNode = 0;
//
//		uint interactionType = collider.InteractionType;
//		btCollisionShape* cshape = NULL;
//		if(collider.Type == xob::DXC_BOX)
//		{
//			iff->Read(&collider + 1, sizeof(boxCollider) - sizeof(collider));
//			Vector3 corner;
//			corner.x = boxCollider.Corner[0];
//			corner.y = boxCollider.Corner[1];
//			corner.z = boxCollider.Corner[2];
//			cshape = new btBoxShape(BulletWorld::Point2btVector(corner));
//			gtype = GT_BOX;
//		}
//		else if(collider.Type == xob::DXC_SPHERE)
//		{
//			iff->Read(&collider + 1, sizeof(sphereCollider) - sizeof(collider));
//			float radius = sphereCollider.Radius;
//			cshape = new btSphereShape(MEQ_WORLDISCALE * radius);
//			gtype = GT_SPHERE;
//		}
//		else if(collider.Type == xob::DXC_CAPSULE)
//		{
//			iff->Read(&collider + 1, sizeof(capsuleCollider) - sizeof(collider));
//			float radius = capsuleCollider.Radius;
//			float length = capsuleCollider.Length;
//			cshape = new btCapsuleShapeZ(MEQ_WORLDISCALE * radius, MEQ_WORLDISCALE * length * 2.0f);
//			gtype = GT_CAPSULE;
//		}
//		else if(collider.Type == xob::DXC_CYLINDER)
//		{
//			iff->Read(&collider + 1, sizeof(cylinderCollider) - sizeof(collider));
//			float radius = cylinderCollider.Radius;
//			float length = cylinderCollider.Length;
//			cshape = new btCylinderShapeZ(btVector3(radius * MEQ_WORLDISCALE, radius * MEQ_WORLDISCALE, length * MEQ_WORLDISCALE));
//			gtype = GT_CYLINDER;
//		}
//		else if(collider.Type == xob::DXC_CONVEX)
//		{
//			iff->Read(&collider + 1, sizeof(convexCollider) - sizeof(collider));
//			cshape = LoadConvexCollider(convexCollider, iff);
//			gtype = GT_CONVEX;
//		}
//		else if(collider.Type == xob::DXC_TRIMESH)
//		{
//			iff->Read(&collider + 1, sizeof(trimeshCollider) - sizeof(collider));
//			cshape = LoadTriMeshCollider(trimeshCollider, iff);
//			gtype = GT_MESH;
//		}
//		else if(collider.Type == xob::DXC_MULTIMATTRIMESH)
//		{
//			iff->Read(&collider + 1, sizeof(multiTrimeshCollider) - sizeof(collider));
//
//			xob::DXMaterialRange* matRanges = ALLOCA(xob::DXMaterialRange, multiTrimeshCollider.NumMaterials);
//			iff->Read(matRanges, sizeof(xob::DXMaterialRange) * multiTrimeshCollider.NumMaterials);
//
//			MatRange* mats = ALLOCA(MatRange, multiTrimeshCollider.NumMaterials);
//
//			for(uint n = 0; n < multiTrimeshCollider.NumMaterials; n++)
//			{
//				mats[n].Material = FindMaterial(stringPool[matRanges[n].NameIndex]);
//				if(mats[n].Material == NULL)
//				{
//					enf::LogF(LL_ERROR, "PhysicsGeometry '%s' has undefined material '%s' on geometry '%s'",
//						name.cstr(), stringPool[matRanges[n].NameIndex], stringPool[collider.NameIndex]);
//				}
//				mats[n].LastFace = matRanges[n].LastFace;
//			}
//
//			cshape = LoadMultiTriMeshCollider(multiTrimeshCollider, iff, mats);
//			gtype = GT_MESH;
//		}
//
//		if(cshape)
//		{
//			PhysicsMaterial* mat = FindMaterial(stringPool[collider.MaterialIndex]);
//			if(mat == NULL)
//			{
//				enf::LogF(LL_ERROR, "PhysicsGeometry '%s' has undefined material '%s' on geometry '%s'",
//					name.cstr(), stringPool[collider.MaterialIndex], stringPool[collider.NameIndex]);
//			}
//			BulletRBElement& elem = objgeom->m_Elements.Append();
//			elem.m_Type = gtype;
//			elem.m_Name = CStr(stringPool[collider.NameIndex]).Static();
//			elem.m_pMaterial = mat;
//			elem.m_Frame = frame;
//			elem.m_pShape = cshape;
//			elem.m_iParentNode = parentNode;
//			cshape->setUserPointer((void*)elem.m_Name.cstr());
//		}
//	}
//
//	Assert(objgeom->m_Elements.Size() != 0);
//
//	{
//		CMutexLock lock(m_Mutex);
//		m_ObjectGeometries.Insert(objgeom);
//		m_ObjectGeometriesHash.Insert(objgeom->GetName().cstr(), objgeom);
//		AddRef();
//	}
//	return objgeom;
//}
//
////-----------------------------------------------------------------------
//ObjectGeometry* BulletManager::GetGeometry(CStr name, LODShape* obj)
//{
//	BulletObjectGeometry* objgeom = BulletObjectGeometry::Cast(obj->GetGeometry());
//	
//	if(objgeom == NULL)
//		objgeom = m_ObjectGeometriesHash.Find(name.cstr());
//
//	if(objgeom)
//	{
//		objgeom->AddRef();
//		return objgeom;
//	}
//
//	return NULL;
//}

//-----------------------------------------------------------------------
void BulletManager::UnloadMaterials()
{
	for(uint n = 0; n < m_Materials.GetCardinality(); n++)
	{
		delete m_Materials[n];
	}

	m_Materials.Clear();
	m_MaterialsHash.Clear();
}

//-----------------------------------------------------------------------
bool BulletManager::LoadRagdollMaterial()
{
  for(int i = 0; i < physMatCnt; ++i)
  {
    BulletPhysicsMaterial* mat = new BulletPhysicsMaterial(physmats[i].name);
    mat->m_fDynamicFriction = physmats[i].dynamicfriction;
	  mat->m_fRestitution = physmats[i].restitution;
	  mat->m_fLateralStiffness = physmats[i].lateralstiffness;
	  mat->m_fLongitudalStiffness = physmats[i].longitudalstiffness;
	  mat->m_bSingleSided = physmats[i].singleSided;
	  mat->m_pBtMaterial->setFriction(physmats[i].dynamicfriction);
	  mat->m_pBtMaterial->setRestitution(physmats[i].restitution);
    if(physmats[i].aniso)
	    mat->m_pBtMaterial->setAnisotropicFriction(btVector3(physmats[i].longitudalstiffness, physmats[i].lateralstiffness, 1.0f));
    if(physmats[i].replaceOp)
      mat->m_pBtMaterial->setOperation(MO_REPLACE);
	  mat->m_pBtMaterial->setSingleSided(physmats[i].singleSided);
	  mat->m_iMaterialIndex = i;//0;
	
	  InsertMaterial(mat);
    m_BaseMaterials[i] = mat;
  }

  return true;
}

//-----------------------------------------------------------------------
bool BulletManager::LoadMaterials(const char* name)
{
  bool ret = LoadRagdollMaterial();

	GameDataNamespace globals(NULL, RString(), false); // TODO: parsing namespace if access to globals needed

	ParamFile matFile;
  matFile.ParseBinOrTxt(name, NULL, NULL, &globals);
  
	BulletPhysicsMaterial* mat = new BulletPhysicsMaterial(name);
	
	if (matFile.GetEntryCount()==0)
	{
		RptF("Cannot load material file %s",cc_cast(name));
		return 0;
	}
	else
	{
		ParamEntryVal par = matFile>>"Materials";
    for(int i = 0; i < par.GetEntryCount(); i++)
    {
      ParamEntryVal entry = par.GetEntry(i);
      if (entry.IsClass())
      {
				mat = new BulletPhysicsMaterial(entry.GetName().Data());
        mat->Load(entry);
				InsertMaterial(mat);
      }
    }
	}

	return ret;
}

//-----------------------------------------------------------------------
BulletManager::~BulletManager()
{
	UnloadRagdolls();
	UnloadMaterials();
  
  for(uint n = 0; n < m_ShapeCache.GetCardinality(); n++)
	{
		m_ShapeCache[n].Data->Destroy();
	}
  
  SAFE_DELETE(m_pConfig);
	if(g_pBulletManager == this)
		g_pBulletManager = NULL;
}

//-----------------------------------------------------------------------
BulletManager::BulletManager():
	m_Ragdolls(NULL),
	m_pConfig(NULL)
{
	btAlignedAllocSetCustom(&BulletAllocFunc, &BulletFreeFunc);

	btDefaultCollisionConstructionInfo info;
	//info.m_useEpaPenetrationAlgorithm = false;
	info.m_defaultMaxPersistentManifoldPoolSize = 2048;
	info.m_defaultMaxCollisionAlgorithmPoolSize = 2048;

	m_pConfig = new btDefaultCollisionConfiguration(info);

	ZeroMem(m_BaseMaterials, sizeof(m_BaseMaterials));

  //prozatim
  SetCurrent();
}

//-----------------------------------------------------------------------------
PhysicsWorld* BulletManager::CreatePhysicsWorld(CStr type)
{
	if(g_pBulletManager)
		return new BulletWorld();

	return NULL;
}

}