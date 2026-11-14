//-----------------------------------------------------------------------------
// File: enf_pipeline.cpp
//
// Desc: Surface/Model stack methods
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
//#include "baseworld/enf_baseworld.h"
#include "baseworld/enf_baseworldlight.h"
#include "baseworld/enf_entity.h"
#include "materials/enf_materialsystem.h"
#include "pipeline/enf_pipeline.h"
#include "resource/enf_particleeffect.h"
#include "baseWorld/enf_shapes.h"
#include "render/enf_rendmain.h"
#include "script/enf_scriptapivars.h"
#include "common/enf_diag.h"
#include "common/enf_quicksort.h"
#include "materials/enf_materialclass.h"
#include "system/enf_profiler.h"
#include "common/enf_simdmathlib.h"
#include "enf_main.h"

namespace enf
{

//-----------------------------------------------------------------------
void enf_swap(TBaseItem& a, TBaseItem& b)
{
	enf_swap(a.Mesh, b.Mesh);
	enf_swap(a.Model, b.Model);
}

void enf_swap(TItem& a, TItem& b)
{
	enf_swap(a.Mesh, b.Mesh);
	enf_swap(a.Model, b.Model);
	enf_swap(a.Sort, b.Sort);
}

//-----------------------------------------------------------------------
// Object stack
//-----------------------------------------------------------------------
ModelContext::ModelContext(): m_iAmbientLight(0), m_iEnvMap(0)
{
}

//-----------------------------------------------------------------------
#define VERTEX(n) *(Vector3*)((ubyte*)vec + stride * (n))

//-----------------------------------------------------------------------
AAB Mirror::GetBBoxFromMesh(const float* vec, uint stride, uint num)
{
	AAB bbox;

	bbox.Init();

	for(uint n = 0; n < num; n++)
	{
		bbox.Add(VERTEX(n));
	}
	return bbox;
}

//-----------------------------------------------------------------------
bool Mirror::GetPlaneFromMesh(MPlane& plane, const float* vec, uint stride, uint num, const ushort* ind, uint numfaces)
{
	//get plane of mesh, if any
	for(uint n = 0; n < (numfaces * 3); n += 3)
	{
		Vector3 edge0 = (VERTEX(ind[n + 0]) - VERTEX(ind[n + 1])).Normalized();
		Vector3 edge1 = (VERTEX(ind[n + 1]) - VERTEX(ind[n + 2])).Normalized();

		if(edge0.Dot(edge1) < 0.95f)
		{
			plane.FromPoints(VERTEX(ind[n + 0]), VERTEX(ind[n + 1]), VERTEX(ind[n + 2]));
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------
bool Mirror::Build(const MPlane& plane, const AAB& bbox, const Vector3& from)
{
	m_Plane = plane;

	//make reflected matrix by mirror plane
	m_mMirror.Reflect(m_Plane);

	Vector3 from2 = m_mMirror.TransformCoord(from);

	//polygon might be complex. Get boundbox and make simplified frustum from it
	Vector3 size = bbox.GetSize();

	Vector3 quad[4];
	bool winding;

	if(size.x <= size.y && size.x <= size.z)
	{
		//make winding from Y/Z box
		quad[0] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMins().y, bbox.GetMins().z));
		quad[1] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMaxs().y, bbox.GetMins().z));
		quad[2] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMaxs().y, bbox.GetMaxs().z));
		quad[3] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMins().y, bbox.GetMaxs().z));

		winding = m_Plane.x < 0;
	}
	else if(size.y <= size.x && size.y <= size.z)
	{
		//make winding from X/Z box
		quad[0] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMins().y, bbox.GetMins().z));
		quad[1] = m_Plane.Project(Vector3(bbox.GetMaxs().x, bbox.GetMins().y, bbox.GetMins().z));
		quad[2] = m_Plane.Project(Vector3(bbox.GetMaxs().x, bbox.GetMins().y, bbox.GetMaxs().z));
		quad[3] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMins().y, bbox.GetMaxs().z));

		winding = m_Plane.y >= 0;
	}
	else if(size.z <= size.x && size.z <= size.y)
	{
		//make winding from X/Y box
		quad[0] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMins().y, bbox.GetMins().z));
		quad[1] = m_Plane.Project(Vector3(bbox.GetMaxs().x, bbox.GetMins().y, bbox.GetMins().z));
		quad[2] = m_Plane.Project(Vector3(bbox.GetMaxs().x, bbox.GetMaxs().y, bbox.GetMins().z));
		quad[3] = m_Plane.Project(Vector3(bbox.GetMins().x, bbox.GetMaxs().y, bbox.GetMins().z));

		winding = m_Plane.z < 0;
	}

	//build new frustum based on mirrored camera position and mirror edges.
	m_Frustum.Clear();
	MPlane cpl;

	if(winding)
	{
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[0], quad[1]));
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[1], quad[2]));
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[2], quad[3]));
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[3], quad[0]));
	}
	else
	{
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[1], quad[0]));
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[2], quad[1]));
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[3], quad[2]));
		m_Frustum.AddPlane(cpl.FromPoints(from2, quad[0], quad[3]));
	}
	m_Plane.w += 1.0f; //avoid adding exactly on plane geometry

	//add mirror plane
	m_Frustum.AddPlane(m_Plane);

	//get position on mirror as position for PVS
	m_vPVSPosition = m_Plane.Project(from) + (Vector3(m_Plane.x, m_Plane.y, m_Plane.z) * 2.0f);
	return true;
}

//-----------------------------------------------------------------------
Mirror* Pipeline::AddMirror()
{
	if(m_Mirrors.IsFull())
		return NULL;

	Mirror& mirror = m_Mirrors.Append();
	return &mirror;
}

//-----------------------------------------------------------------------
uint Pipeline::InsertEnvMap(MEnvMap* envmap)
{
#ifdef ENF_DONE
	if(envmap->m_iCount != m_pWorld->GetCubeCount())
	{
//		enf_assert(m_pActiveEnvMaps.GetCardinality() < MAX_ENVMAPS);

		envmap->m_iCount = m_pWorld->GetCubeCount();
		uint envmapnum = m_pActiveEnvMaps.Insert(envmap);
		for(uint s = 0; s < SORT_COUNT; s++)
		{
			m_EnvironmentSet[s][envmapnum].Clear();
		}
		envmap->m_iNum = envmapnum;
		return envmapnum;
	}
#endif
	return envmap->m_iNum;
}

//-----------------------------------------------------------------------
uint Pipeline::InsertLight(const ILight* light)
{
#ifdef ENF_DONE
	if(light->m_iCount == m_pWorld->GetLightCount())
		return light->m_iNum;

	light->m_iCount = m_pWorld->GetLightCount();

	if(light->m_iLightingLevel > m_Description.m_iLightingLevel)
	{
		light->m_iNum = INDEX_NOT_FOUND;
		return INDEX_NOT_FOUND;
	}

	uint lightnum = m_ActiveLights.GetCardinality();
	if(lightnum >= MAX_LIGHTS)
	{
		light->m_iNum = INDEX_NOT_FOUND;
		return INDEX_NOT_FOUND;
	}

	//do a culling
	if(light->m_Type != LT_DIRECTIONAL)
	{
		if(light->m_fRange == 0.0f || (light->m_Type == LT_SPOT && light->m_fConePhi == 1.0f))
		{
			light->m_iNum = INDEX_NOT_FOUND;
			return INDEX_NOT_FOUND;
		}

		float ldistance = (m_mCamera.GetTranslation() - light->m_Position).QLength();
		float dist = ldistance + 5.0f;
		float radius = light->m_fRange * 2 * enf_inve(dist);
		float pixels = radius * (float)g_pRender->GetTargetHeight();

		const float MIN_LIGHT = 30.0f;
		if(pixels < MIN_LIGHT)
		{
			light->m_iNum = INDEX_NOT_FOUND;
			return INDEX_NOT_FOUND;
		}
		if(!GetViewFrustum().TestSphere(light->m_Position, light->m_fRange))
		{
			light->m_iNum = INDEX_NOT_FOUND;
			return INDEX_NOT_FOUND;
		}
	}

	//TODO: make copy of light, instead of ref!
	LightRef& lref = m_ActiveLights.Append();
	lref.m_pLight = light;
	lref.m_ShadowCastersBounds.Init();

	for(uint s = 0; s < SORT_COUNT; s++)
	{
		m_LightSet[s][lightnum].Clear();
	}
	m_ShadowSet[lightnum].Clear();

	light->m_iNum = lightnum;
	return lightnum;
#endif

	return -1;
}

//-----------------------------------------------------------------------
void Pipeline::InsertToEnvironmentSet(uint sort, MeshItem* ss, MEnvMap* envmap)
{
	const Material* mat = ss->GetMaterial();
	SortType st = mat->GetSortType();

	if(envmap == NULL)
	{
		m_EnvironmentSet2[st].Insert(sort, ss, m_pLastModelContext);
	}
	else
	{
		//ensure that we are in pipeline
#ifdef ENF_DONE
		enf_assert(envmap->m_iCount == m_pWorld->GetCubeCount());
#endif
		m_EnvironmentSet[st][envmap->m_iNum].Insert(sort, ss, m_pLastModelContext);
	}
}

//-----------------------------------------------------------------------
void Pipeline::InsertToLightSet(uint sort, MeshItem* ss, const ILight* light)
{
	enf_assert(light != NULL);
	//ensure that we are in pipeline
#ifdef ENF_DONE
	enf_assert(light->m_iCount == m_pWorld->GetLightCount());
#endif
	uint lightnum = light->m_iNum;
	const Material* mat = ss->GetMaterial();
	SortType st = mat->GetSortType();
	m_LightSet[st][lightnum].Insert(sort, ss, m_pLastModelContext);
}

//-----------------------------------------------------------------------
void Pipeline::InsertToShadowSet(MeshItem* mesh, const ILight* light)
{
	//ensure that we are in pipeline
#ifdef ENF_DONE
	enf_assert(light->m_iCount == m_pWorld->GetLightCount());
#endif
	uint l = light->m_iNum;
	m_ShadowSet[l].Insert(mesh->GetSort(), mesh, m_pLastModelContext);
}

//-----------------------------------------------------------------------
void Pipeline::InsertToShadowSet(MeshItem* mesh, const ILight* light, const AAB& wbbox)
{
	//ensure that we are in pipeline
#ifdef ENF_DONE
	enf_assert(light->m_iCount == m_pWorld->GetLightCount());
#endif
	uint l = light->m_iNum;
	m_ActiveLights[l].m_ShadowCastersBounds.Add(wbbox);
	m_ShadowSet[l].Insert(mesh->GetSort(), mesh, m_pLastModelContext);
}

//-----------------------------------------------------------------------
void Pipeline::EndObject()
{
	enf_assert(m_pLastModelContext != NULL);
	ENF_PREFETCH(m_pLastModelContext + 1);
	m_pLastModelContext = NULL;
}

//-----------------------------------------------------------------------
uint Pipeline::AllocObject()
{
	if(m_iObjectStackPos >= m_Description.m_iMaxModels)
		return INDEX_NOT_FOUND;
	m_pObjectStack[m_iObjectStackPos].m_pEntity = NULL;
	m_pObjectStack[m_iObjectStackPos].m_pInstance = NULL;
	m_pObjectStack[m_iObjectStackPos].m_iFlags = 0;
	return m_iObjectStackPos++;
}

//-----------------------------------------------------------------------
ModelContext* Pipeline::BeginObject(Entity *ent, int distance, TModelLights* lights, uint flags, uint nummatrices)
{
	if(m_iObjectStackPos >= m_Description.m_iMaxModels)
		return NULL;

	return BeginObjectInternal(m_iObjectStackPos++, ent, distance, lights, flags, nummatrices);

}

//-----------------------------------------------------------------------
ModelContext* Pipeline::BeginObject(uint allocIndex, Entity *ent, int distance, TModelLights* lights, uint flags, uint nummatrices)
{
	enf_assert(allocIndex < m_iObjectStackPos);
	return BeginObjectInternal(allocIndex, ent, distance, lights, flags, nummatrices);
}

//-----------------------------------------------------------------------
ModelContext* Pipeline::BeginObjectInternal(uint allocIndex, Entity* ent, int distance, TModelLights* lights, uint flags, uint nummatrices)
{
	enf_assert2(m_pLastModelContext == NULL, "Illegal nested call to BeginObject");
	//AssertMainThread();

#ifdef ENF_DONE
	VObjectInstance* minst = ent->GetInstance();
#else
	VObjectInstance* minst = NULL;
#endif

	//alloc
	ModelContext* ENF_RESTRICT mc = &m_pObjectStack[allocIndex];

	m_pLastModelContext = mc;

	//initialize
	if(flags & MCF_IDENTWORLD)
	{
		mc->m_mMatrix.Identity();
	}
	else
	{
		#ifdef ENF_DONE
			SimdMatrix34::LoadFloat4x3((const float*)&ent->IGetMatrix()).StoreFloat3x4A16((float*)&mc->m_mMatrix);
		#else
			mc->m_mMatrix = ent->IGetMatrix();
		#endif
	}
	mc->m_vInstanceInfo.Zero();
	mc->m_iFlags = flags;

	uint firstmatrix = 0;
	if(nummatrices)
	{
		enf_assert(nummatrices < m_Description.m_iMaxMatrices);
		//better to have trashed geometry than trashed memory pools ;-)
		if(m_iLastMatrix + nummatrices > m_Description.m_iMaxMatrices)
			m_iLastMatrix = 0;

		firstmatrix = m_iLastMatrix;
	}
	mc->m_iFirstMatrix = firstmatrix;	

#ifdef ENF_DONE
	if(minst->NeedsAmbientLight() || minst->AcceptsLight())
		m_pWorld->SetupLights(this, mc, minst, ent, lights);
#endif

	mc->m_pEntity = ent;
	mc->m_pInstance = minst;
	mc->m_fLODTime = 0;
	//mc->m_pEnvMap = NULL;
	m_iLastMatrix += nummatrices;

#ifdef ENF_DONE
	if(minst->NeedEnvMap())
	{
		MEnvMap* penvmap = minst->GetEnvMap();
		if(penvmap == NULL)
		{
			penvmap = m_pWorld->FindEnvMap(ent, ent->IGetTranslation());
			minst->SetEnvMap(penvmap);
		}

		//		LogF(LL_NORMAL, "envmap %x, num %d", penvmap, m_pWorld->GetNumEnvMaps());
		if(penvmap)
		{
			mc->m_iEnvMap = InsertEnvMap(penvmap);
			mc->SetFlags(MCF_HAS_ENVMAP);
		}
	}
#endif
	return mc;
}

//-----------------------------------------------------------------------
ModelContext* Pipeline::BeginObject(ModelContext* clone, uint flags, uint nummatrices)
{
	if(m_iObjectStackPos >= m_Description.m_iMaxModels)
		return NULL;

	ModelContext* mc = &m_pObjectStack[m_iObjectStackPos++];

	m_pLastModelContext = mc;

	//should be faster, especially on XBOX
	memcpy(mc, clone, sizeof(ModelContext));

	mc->m_iFlags = (clone->m_iFlags & MCF_IDENTWORLD) | flags;
	if(flags & MCF_LOD_1)
		mc->m_fLODTime = 1.0f - clone->m_fLODTime;

	if(nummatrices)
	{
		enf_assert(nummatrices < m_Description.m_iMaxMatrices);
		//better to have trashed geometry than trashed memory pools ;-)
		if(m_iLastMatrix + nummatrices > m_Description.m_iMaxMatrices)
			m_iLastMatrix = 0;

		mc->m_iFirstMatrix = m_iLastMatrix;	
		m_iLastMatrix += nummatrices;
	}

	return mc;
}

//---------------------------------------------------------------------
MeshItem* Pipeline::BeginMesh(PrimitiveType pt, int sortval, Material* mat, uint offsetMatrices, uint numMatrices, Matrix34** pmatrices)
{
	enf_assert(mat);
	enf_assert(numMatrices == 0 || pmatrices != NULL);

	if(!mat->IsRenderable()) //dummy material
		return NULL;

	mat->Touch(0);

	MeshItem* mesh = AllocMeshItem();

	if(mesh == NULL)
		return NULL;

	mesh->m_Type = pt;
	mesh->m_iStrides = 0;
	mesh->m_pIB = NULL;
	mesh->m_pVB[0] = NULL;
	mesh->m_pVB[1] = NULL;
	mesh->m_iOffset = 0;

	mesh->m_iNumMatrices = numMatrices;
	mesh->m_iMatrixOffset = offsetMatrices;
	if(numMatrices)
		*pmatrices = &m_pMatrices[offsetMatrices + m_pLastModelContext->m_iFirstMatrix];
	mesh->m_iFlags = 0;

	mesh->m_pMaterial = mat;
	SortType sortType = mat->GetSortType();
	int sortBias = mat->GetSortBias();

	if(sortType >= SORT_TRANSLUCENT)
	{
		enf_assert(mat->GetSortID() < (1 << TRANSP_CUSTOM_SORT_SHIFT));
		mesh->m_iSort = (sortBias << TRANSP_BIAS_SHIFT) + (mat->GetSortID() << TRANSP_SORTID_SHIFT) + sortval;
	}
	else
		mesh->m_iSort = (sortBias << OPAQUE_BIAS_SHIFT) + (mat->GetSortID() << OPAQUE_SORTID_SHIFT) + sortval;

	//when material needs view from other camera, we have to
	//notify it so we can redraw it later
	if(mat->NeedsRenderView())
	{
		m_iUpdateCameraMask |= (1 << m_pLastModelContext->GetInstance()->GetCameraIndex());
	}


	enf_assert(sortType < MAX_SORT_NUM);
	m_IsSetUsed |= 1 << sortType;

	EngineStats::m_iMeshItemCount++;

	return mesh;
}

//-----------------------------------------------------------------------
#ifdef ENF_ENABLE_DIAG
typedef Pair<VObjectResource*, uint> ObjectStat;
Set<ObjectStat> objectsSet;
FlushableObjectAgent< Set<ObjectStat> > objectsSetAgent(&objectsSet);

class SortObj
{
public:
	int operator()(const ObjectStat& a, const ObjectStat& b) const
	{
		if(a.Data < b.Data) return 1;
		if(a.Data > b.Data) return -1;
		if(a.Key->GetName() == NULL)
			return -1;
		if(b.Key->GetName() == NULL)
			return 1;
		return a.Key->GetName().cmp(b.Key->GetName());
	}
};
#endif

//-----------------------------------------------------------------------
void Pipeline::PrintStats()
{
#ifdef ENF_ENABLE_DIAG
	Array<ObjectStat> arr(objectsSet.GetCardinality());
	for(uint n = 0; n < objectsSet.GetCardinality(); n++)
	{
		arr.Insert(objectsSet[n]);
	}

	QuickSortCmp<ObjectStat, SortObj> (arr.GetCardinality(), arr.GetArray());

	uint totalObjects = 0;
	for(uint n = 0; n < arr.GetCardinality(); n++)
	{
		char temp[256];
		memset(temp, ' ', 255);
		VObjectResource* vobj = arr[n].Key;

		memcpy(temp, vobj->GetName().cstr(), vobj->GetName().Length());
		sprintf(temp + 60, "%d", arr[n].Data);
		temp[255] = 0;
		LogF(LL_NORMAL, temp);
		totalObjects += arr[n].Data;
	}

	LogF(LL_NORMAL, "Total %d objects", totalObjects);
	objectsSet.Clear();
#endif
}

//-----------------------------------------------------------------------
void Pipeline::EndFrame()
{
	EngineStats::m_iModelContextCount = m_iObjectStackPos;
	EngineStats::m_iLightCount += m_ActiveLights.GetCardinality();

#ifdef ENF_ENABLE_DIAG
	if(ENF_DIAG_BOOL(DM_DUMP_OBJECTS))
	{
		for(uint n = 0; n < m_iObjectStackPos; n++)
		{
			if(m_pObjectStack[n].IsFlagSet(MCF_LOD_1))
				continue;
			if(m_pObjectStack[n].GetInstance() == NULL)
				continue;

			VObjectResource* vobj = m_pObjectStack[n].GetInstance()->GetVObject();
			uint index = objectsSet.GetIndexOf(ObjectStat(vobj, 0));
			if(index != INDEX_NOT_FOUND)
			{
				objectsSet[index].Data++;
			}
			else
			{
				objectsSet.Insert(MakePair(vobj, uint(1)));
			}
		}
	}
#endif
	m_iObjectStackPos = 0;
	GetMaterialSystem()->EndFrame();
}

//-----------------------------------------------------------------------
void Pipeline::BeginFrame(BaseWorld* world, RenderType type, uint camera, const ViewFrustum& frustum)
{
	enf_assert(world);
	m_iFrameCount++;
	m_pWorld = world;
	m_Type = type;
	m_iCamera = camera;

#ifdef ENF_DONE
	m_mCamera = world->GetCamera(camera).m_mMatrix;
#else
	m_mCamera.SetIdentity();
#endif
	
	//quantize
	const float invq = 1.0f / TRANS_SORT_GRANULARITY;
	Vector3 cpos = m_mCamera.GetTranslation() * invq;
	m_vQCamera.x = enf_trunc(cpos.x);
	m_vQCamera.y = enf_trunc(cpos.y);
	m_vQCamera.z = enf_trunc(cpos.z);

	m_Mirrors.Clear();

	m_ShadowSet[0].Clear();

	for(uint s = 0; s < SORT_COUNT; s++)
	{
		m_MeshItemSet[s].Clear();
		m_AmbientDepthSet[s].Clear();
		m_EnvironmentSet2[s].Clear();
	}

	m_IsSetUsed = 0;
	m_iStack = 0;
	m_iLastMatrix = 0;
	m_iObjectStackPos = 0;
	m_iInstancedMeshOffset = 0;

	m_iUpdateCameraMask = 0;

	m_pLastMaterial = NULL;
	m_pLastEntity = NULL;
	m_pLastModelContext = NULL;

	m_pActiveDynamicLights.Clear();
	m_ActiveLights.Clear();
	m_pActiveEnvMaps.Clear();

	m_AmbientLights.Reserve(50);
	m_AmbientLights.Clear();

	{
		AmbientLight& l = m_AmbientLights.Append();
		l.Clear();
	}

	AmbientLight& l = m_AmbientLights.Append();

#ifdef ENF_DONE
	const WorldLight* wl = world->GetWorldLight();
#else
	const WorldLight* wl = NULL;
#endif

	if(wl)
	{
		#ifdef ENF_DONE
			world->UpdateWorldLightHDR(camera);
		#endif

		l.m_iFlags = wl->m_iFlags;
		l.m_Direction = wl->m_SpotVec;

		l.m_Diffuse = wl->GetDiffuseHDR();
		l.m_Specular = wl->GetSpecularHDR();
		l.m_Ambient = wl->GetAmbientHDR();
		l.m_fHDRBrightness = wl->GetGlobalHDRBrightness();
		InsertLight(wl);
	}
	else
	{
		l.Clear();
	}

	GetMaterialSystem()->BeginFrame(m_mCamera, type);
	m_ViewFrustum = frustum;
	m_ViewFrustumWithShadow = frustum;

	if(GetDescription().m_iShadowMapSize != 0)
	{
		float shadowMaxDistance = GetMaterialSystem()->GetScalar(MaterialSystem::MV_SHADOW_MAX_DISTANCE);
		m_iShadowExtrudedFrustumPlanes = m_ViewFrustumWithShadow.EnlargeByDirectionalLight(wl->m_SpotVec, shadowMaxDistance);
	//	float enlargementFactor = enf_max(0.0f, Vector3(-wl->m_SpotVec.x,-wl->m_SpotVec.y,0).Normalized().Dot(Vector3(cmatrix._11, cmatrix._12, 0).Normalized()));

#ifdef ENF_DONE
		float enlargementFactor = enf_max(0.0f, wl->m_SpotVec.Dot(Vector3(-m_mCamera._11, -m_mCamera._12, 0).Normalized()));
#else
		float enlargementFactor = 1;
#endif

		m_fMaxShadowCasterDistance = shadowMaxDistance + shadowMaxDistance * enlargementFactor;
	}
	else
	{
		m_iShadowExtrudedFrustumPlanes = 0;
		m_fMaxShadowCasterDistance = 0.0f;
	}

	m_DynamicCache.Advance();

	GetMaterialSystem()->SetScalar(MaterialSystem::MV_DEPTH_RANGE, (float)m_Description.m_iDepthRange);

	//make copy. Because of MT conflicts and int->float conversions on xenon
	UpdateCurrentTargetExtents();
}

//-----------------------------------------------------------------------
void Pipeline::UpdateCurrentTargetExtents()
{
	m_fTargetWidth = g_pRender->GetTargetWidth();
	m_fTargetHeight = g_pRender->GetTargetHeight();
}

//-----------------------------------------------------------------------
class TItemCompare
{
public:
	int operator()(const TItem& i1, const TItem& i2) const
	{
		if(i1.Sort < i2.Sort) return -1;
		if(i1.Sort > i2.Sort) return 1;
		if(i1.Mesh < i2.Mesh) return -1;
		if(i1.Mesh > i2.Mesh) return 1;
		if(i1.Model->GetFlags() < i2.Model->GetFlags()) return -1;
		if(i1.Model->GetFlags() > i2.Model->GetFlags()) return 1;
		//avoid flickering of objects at the same distance
		if(i1.Model->m_pEntity < i2.Model->m_pEntity) return -1;
		if(i1.Model->m_pEntity > i2.Model->m_pEntity) return 1;
		return 0;
	}
};

//-----------------------------------------------------------------------
void MeshItemSet::Sort()
{
	QuickSortCmp<TItem, TItemCompare>(GetCardinality(), GetArray());
}

//-----------------------------------------------------------------------
void Pipeline::Render()
{
}

//-----------------------------------------------------------------------
void Pipeline::GrowInstancedMeshes()
{
	uint newSize = m_iMaxInstancedMeshes * 2;
	InstancedMesh* newMeshes = ENF_NEW InstancedMesh[newSize];
	CopyMem(newMeshes, m_pInstancedMeshes, m_iInstancedMeshOffset * sizeof(InstancedMesh));
	delete[] m_pInstancedMeshes;
	m_pInstancedMeshes = newMeshes;
	m_iMaxInstancedMeshes = newSize;
}

//-----------------------------------------------------------------------
Pipeline::Pipeline(Renderer* rend, const PipelineDescription& desc):
	m_pWorld(NULL),
	m_pRender(rend),
	m_Description(desc),
	m_pLastModelContext(NULL),
	m_pObjectStack(NULL),
	m_iObjectStackPos(0),
	m_iCount(0),
	m_iFrameCount(0),
	m_fTargetWidth(0),
	m_fTargetHeight(0),
	m_pInstancedMeshes(NULL),
	m_iInstancedMeshOffset(0),
	m_iMaxInstancedMeshes(0)
{
	enf_assert(rend);

	//m_iMaxMatrices * 3x4 matrices.
	m_pMatrices = (Matrix34*)ALLOC16(desc.m_iMaxMatrices * sizeof(Matrix34));

	m_pStack = ENF_NEW MeshItem[desc.m_iMaxSurfaces];
	ZeroMem(m_pStack, sizeof(MeshItem) * desc.m_iMaxSurfaces);

	enf_assert((sizeof(ModelContext) & 0x0f) == 0);

	m_pObjectStack = (ModelContext*)ALLOC16(desc.m_iMaxModels * sizeof(ModelContext)),
	ZeroMem(m_pObjectStack, sizeof(ModelContext) * desc.m_iMaxModels);

	m_iMaxInstancedMeshes = desc.m_iMaxInstancedMeshes;
	m_pInstancedMeshes = ENF_NEW InstancedMesh[desc.m_iMaxInstancedMeshes];

	m_StaticCache.Init(desc.m_iStaticCacheVSize, desc.m_iStaticCacheISize, desc.m_iStaticCacheBuffers);
	m_DynamicCache.Init(desc.m_iDynamicCacheVSize, desc.m_iDynamicCacheISize, desc.m_iDynamicCacheBuffers);

	//these sets will be huge. Avoid many tiny reallocations
	m_MeshItemSet[SORT_OPAQUE].Reserve(128);
	m_MeshItemSet[SORT_TRANSLUCENT].Reserve(128);
	m_AmbientDepthSet[SORT_OPAQUE].Reserve(128);
	m_AmbientDepthSet[SORT_TRANSLUCENT].Reserve(128);
	
	GetMaterialSystem()->SetScalar(MaterialSystem::MV_DEPTH_RANGE, (float)desc.m_iDepthRange);
}

//-----------------------------------------------------------------------
Pipeline::~Pipeline()
{
	m_StaticCache.Free();
	m_DynamicCache.Free();

	if(m_pMatrices)
	{
		FREE16(m_pMatrices);
		m_pMatrices = NULL;
	}

	SAFE_DELETE_ARRAY(m_pStack);
	if(m_pObjectStack)
	{
		FREE16(m_pObjectStack);
		m_pObjectStack = NULL;
	}

	SAFE_DELETE_ARRAY(m_pInstancedMeshes);
}
}