//-----------------------------------------------------------------------------
// File: enf_physicsragdoll.cpp
//
// Desc: Bullet ragdoll implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
//#include "enf_engine.h"
//#include "baseworld/enf_baseworld.h"
//#include "baseworld/enf_baseworldtrace.h"
//#include "baseworld/enf_entity.h"
//#include "enf_scriptapi.h"
//#include "common/enf_parser.h"
#include "physics/enf_physicsbulletinternal.h"
#include "baseWorld/enf_shapes.h"
//#include "vobject/meshobject/enf_meshobjectinternal.h"
#include "physics/enf_physicsragdoll.h"
//#include "enf_animationsystem.h"
#include "common\enf_strings.h"
#include "lib/vehicle.hpp"
#include "lib/rtAnimation.hpp"

#include <Es/Memory/normalNew.hpp>
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"

#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

#include "Physics/ragdolls.h"


namespace enf
{
char RDMissingBone[] = "RagdollCreate: %s can't found bone '%s'";
char RPMissingBone[] = "LoadRagdolls: %s can't found bone '%s'";
char RPMissingMaterial[] = "LoadRagdolls: %s can't found material '%s'";

//#define ENF_VISUALIZE_RAGDOLL

////////////////
Matrix3P FromDirVector(const Vector3P& dir)
{
  //if(fabs(dir.DotProduct(Vector3P(1,0,0))) < fabs(dir.DotProduct(Vector3P(0,1,0))))
  if(fabs(dir.DotProduct(Vector3P(0,1,0))) < 0.1f)
  {
    Vector3P up = up.CrossProduct(Vector3P(1,0,0));
    Vector3P aside = (up.CrossProduct(dir)).Normalized();
    up = (dir.CrossProduct(aside)).Normalized();
    return Matrix3P(aside, up, dir);
  }
  else
  {
    Vector3P aside = (Vector3P(0,1,0)).CrossProduct(dir);
    Vector3P up = (dir.CrossProduct(aside)).Normalized();
    aside = (up.CrossProduct(dir)).Normalized();
    return Matrix3P(aside, up, dir);
  }
}

Matrix3P FromUpVector(const Vector3P& up)
{
  if(fabs(up.DotProduct(Vector3P(0,0,1))) < 0.9f)//fabs(up.DotProduct(Vector3P(1,0,0))))
  {
    Vector3P aside = up.CrossProduct(Vector3P(0,0,1));
    Vector3P dir = (aside.CrossProduct(up)).Normalized();
    aside = (up.CrossProduct(dir)).Normalized();
    return Matrix3P(aside, up, dir);
  }
  else
  {
    Vector3P dir = (Vector3P(1,0,0)).CrossProduct(up);
    Vector3P aside = (up.CrossProduct(dir)).Normalized();
    dir = (aside.CrossProduct(up)).Normalized();
    return Matrix3P(aside, up, dir);
  }
}


//-----------------------------------------------------------------------
RagdollDef::~RagdollDef()
{
}

//-----------------------------------------------------------------------
const RagdollDef::Bone* RagdollDef::FindBone(const char* name) const
{
	for(uint n = 0; n < Bones.GetCardinality(); n++)
	{
		if(!enf_strcmpi(Bones[n].Name, name))
			return &Bones[n];
	}

	return NULL;
}

//-----------------------------------------------------------------------
RagdollDef::RagdollDef(const char *name): Bones(32), Joints(16), Geoms(20)
{
	Name = g_pStatic->AddString(name);
}

/*
//-----------------------------------------------------------------------
bool TokenToFloat(CParser& tp, float& val)
{
	if(!tp.AssertType(TT_NUMBER))
		return false;

	val = atof(tp.CToken->Text);
	return true;
}

//-----------------------------------------------------------------------
bool TokenToInt(CParser& tp, int& val)
{
	if(!tp.AssertType(TT_NUMBER))
		return false;

	val = atoi(tp.CToken->Text);
	return true;
}

//-----------------------------------------------------------------------
bool TokenToUInt(CParser& tp, uint& val)
{
	if(!tp.AssertType(TT_NUMBER))
		return false;

	val = atoi(tp.CToken->Text);
	return true;
}

//-----------------------------------------------------------------------
bool TokenToBool(CParser& tp, bool& val)
{
	if(!tp.AssertType(TT_STRING))
		return false;

	val = (enf_strcmpi(tp.CToken->Text, "true") == 0);
	return true;
}

//-----------------------------------------------------------------------
bool TokenToString(CParser& tp, const char** val)
{
	if(!tp.AssertType(TT_QUOTE))
		return false;

	*val = g_pStatic->AddString(tp.CToken->Text);
	return true;
}

//-----------------------------------------------------------------------
bool TokenToVector(CParser& tp, Vector3& vec)
{
	if(!tp.AssertType(TT_QUOTE))
		return false;

	sscanf(tp.CToken->Text, "%f %f %f", &vec._e[0], &vec._e[1], &vec._e[2]);

	return true;
}
*/
//-----------------------------------------------------------------------
#define CASSERT(a) if(!(a)) {res = false; continue;}

//-----------------------------------------------------------------------
void BulletManager::UnloadRagdolls()
{
	for(uint r = 0; r < m_Ragdolls.GetCardinality(); r++)
	{
		delete m_Ragdolls[r];
	}

	m_Ragdolls.Clear();
	m_RagdollsHash.Clear();
}

//-----------------------------------------------------------------------
bool BulletManager::LoadCharRagdoll()
{
  /*
  AnimationSystem* asys = GetEngine()->GetAnimationSystem();
  Assert2(asys != NULL, "You need AnimationSystem for Ragdolls");

	if(asys == NULL)
		return false;
  */

  /*
	EParser tp;

	if(tp.Parse(defname) != PE_OK)
   	return false;
  */

RagdollDef *rd = NULL;
//uint bindex;
const char *bnames[96];//MAX_MESH_BONES];
int blevel = 0;

	bool res = true;
	bool first = true;

  uint boneIdx = 0;
  uint balljointIdx = 0;
  uint addboneIdx = 0;
  uint hingejointIdx = 0;
  uint geometryIdx = 0;

	for(uint l = 0; l < humanRagdolLnCnt && res; l++)
   {
		if(first)
			first = false;

		//ISTOKEN(tp, "ragdoll")
    if(humanRagdol[l] == "ragdoll")
		{
			if(rd)
			{
				Assert(0);
        res = false;
				continue;
			}

			rd = new RagdollDef("humanoidtest");

			m_Ragdolls.Insert(rd);
			m_RagdollsHash.Insert(rd->GetName(), rd);

			blevel = 0;
			bnames[blevel] = NULL;

			continue;
		}

    if(!rd)
    {
      Assert(0); 
      res = false;
      continue;
    }

    if(humanRagdol[l] == "{")//TTDefEqual(tp.CToken, CParser::TLCurly))
		{
 			blevel++;
      continue;
		}

    if(humanRagdol[l] == "}")//TTDefEqual(tp.CToken, CParser::TRCurly))
    {
			blevel--;
			if(blevel == 0)
			{
				rd = NULL;
			}
      continue;
		}

		//ISTOKEN(tp, "bone")
    if(humanRagdol[l] == "bone")
		{
#pragma message("jz todo: sparovat indexy")
			/*bindex = asys->GetBoneIndex(tp.CToken->Text);

			if(bindex == INDEX_NOT_FOUND)
			{
				//enf::LogF(LL_ERROR, "LoadRagdolls: Cannot found bone '%s'", tp.CToken->Text);
        LogF("LoadRagdolls: Cannot found bone '%s'", tp.CToken->Text);
				continue;
			}

			
			RagdollDef::Bone bone;
			bnames[blevel] = bone.Name = asys->GetBoneName(bindex);
			*/
      RagdollDef::Bone bone;
      bnames[blevel] = bone.Name = hrbones[boneIdx].name;//tp.CToken->Text;

			bone.Offset.Zero();
			bone.Added = false;
			
			bone.Simulated = hrbones[boneIdx].yesno;
      //for simulated bones 'mass' must be set!
			bone.Mass = hrbones[boneIdx].fval;
      boneIdx++;
			
			bone.Parent = bnames[blevel - 1];
			rd->Bones.Insert(bone);
			continue;
		}

		//ISTOKEN(tp, "geometry")
    if(humanRagdol[l] == "geometry")
		{
			RagdollDef::Geometry geom;

			const RagdollDef::Bone* pbone;

      pbone = rd->FindBone(hrgeom[geometryIdx].name1); //tp.CToken->Text);

			if(pbone == NULL)
			{
				//enf::LogF(LL_ERROR, RPMissingBone, rd->GetName(), tp.CToken->Text);
        LogF(RPMissingBone, rd->GetName(), hrgeom[geometryIdx].name1);
				res = false;
				continue;
			}

			geom.Bone1 = pbone->Name;

			pbone = rd->FindBone(hrgeom[geometryIdx].name2);  //tp.CToken->Text);

			if(pbone == NULL)
			{
				//enf::LogF(LL_ERROR, RPMissingBone, rd->GetName(), geom.Bone2);
        LogF(RPMissingBone, rd->GetName(), geom.Bone2);
				res = false;
				continue;
			}

			geom.Bone2 = pbone->Name;
			
			geom.Radius = hrgeom[geometryIdx].fval;
      geom.Material = FindMaterial(hrgeom[geometryIdx].mat);
      geometryIdx++;

      if(geom.Material == NULL)
			{
				//enf::LogF(LL_ERROR, RPMissingMaterial, rd->GetName(), tp.CToken->Text);
        LogF(RPMissingMaterial, rd->GetName(), hrgeom[geometryIdx].mat);
				res = false;
				continue;
			}
      
      rd->Geoms.Insert(geom);
			continue;
		}

		if(blevel < 2)
		{
         Assert(0);
         res = false;
         continue;
		}

//added bones must be here, as they need parent bone
		//ISTOKEN(tp, "addbone")
    if(humanRagdol[l] == "addbone")
		{
			RagdollDef::Bone bone;
      bnames[blevel] = bone.Name = g_pStatic->AddString(hraddbones[addboneIdx].name);
			bone.Added = true;
			
      bone.Offset = hraddbones[addboneIdx].vec;
      bone.Simulated = hraddbones[addboneIdx].yesno;
      
			//for simulated bones 'mass' must be set!
      bone.Mass = hraddbones[addboneIdx].fval;
      addboneIdx++;

			bone.Parent = bnames[blevel - 1];
			rd->Bones.Insert(bone);
			continue;
		}

		//ISTOKEN(tp, "hingejoint")
    if(humanRagdol[l] == "hingejoint")
		{
			RagdollDef::Joint joint;
			joint.Type = RagdollDef::JOINT_HINGE;
			joint.Name = bnames[blevel - 1];

      joint.VAxis = hrhingejoint[hingejointIdx].vec;
			joint.Angle1 = hrhingejoint[hingejointIdx].a;
			joint.Angle2 = hrhingejoint[hingejointIdx].b;
			joint.Damp = hrhingejoint[hingejointIdx].f;
      hingejointIdx++;
			rd->Joints.Insert(joint);
			continue;
		}

		//ISTOKEN(tp, "fixedjoint")
    if(humanRagdol[l] == "fixedjoint")
		{
			RagdollDef::Joint joint;
			joint.Type = RagdollDef::JOINT_FIXED;
			joint.Name = bnames[blevel - 1];
			rd->Joints.Insert(joint);
			continue;
		}

		//ISTOKEN(tp, "balljoint")
    if(humanRagdol[l] == "balljoint")
		{
			RagdollDef::Joint joint;

			joint.Type = RagdollDef::JOINT_BALL;
			joint.Name = bnames[blevel - 1];

      joint.VAxis = hrballjoint[balljointIdx].vec;
      joint.Angle1 = hrballjoint[balljointIdx].a;
      joint.Angle2 = hrballjoint[balljointIdx].b;
      joint.Damp = hrballjoint[balljointIdx].f;
      balljointIdx++;
			rd->Joints.Insert(joint);
			continue;
		}

		Assert(0);//tp.LError(CParser::ParmParseError);
		res = false;
   }

	return res;
}

//-----------------------------------------------------------------------
bool BulletManager::LoadRagdolls(const char *defname)
{
  return LoadCharRagdoll();
  
//  Assert(defname != NULL);
//
//  AnimationSystem* asys = GetEngine()->GetAnimationSystem();
//  Assert2(asys != NULL, "You need AnimationSystem for Ragdolls");
//
//	if(asys == NULL)
//		return false;
//  
//	Parser tp;
//
//	if(tp.Parse(defname) != PE_OK)
//   	return false;
//  
//RagdollDef *rd = NULL;
//uint bindex;
//const char *bnames[MAX_MESH_BONES];
//int blevel;

//	bool res = true;
//	bool first = true;
//
//	for(uint l = 0; l < tp.GetNumLines() && res; l++)
//   {
//		if(!first)
//		{
//			if(!tp.NextLine(false))
//				break;
//		}
//		else
//			first = false;
//
//		//HACK: solve it in CParser
//		if(tp.CLine->m_iNumTokens == 0)
//			continue;
//
//		ISTOKEN(tp, "ragdoll")
//		{
//			if(rd)
//			{
//				tp.LError(CParser::SequenceError);
//				res = false;
//				continue;
//			}
//
//			CASSERT(tp.NextToken(true));
//
//			if(tp.AssertType(TT_QUOTE))
//			{
//				rd = new RagdollDef(tp.CToken->Text);
//
//				m_Ragdolls.Insert(rd);
//				m_RagdollsHash.Insert(rd->GetName(), rd);
//
//				blevel = 0;
//				bnames[blevel] = NULL;
//
//				continue;
//			}
//			res = false;
//			continue;
//		}
//
//      if(!rd)
//      {
//         tp.LError(CParser::SequenceError);
//         res = false;
//         continue;
//      }
//
//		if(TTDefEqual(tp.CToken, CParser::TLCurly))
//		{
// 			blevel++;
//      	continue;
//		}
//
//		if(TTDefEqual(tp.CToken, CParser::TRCurly))
//      {
//			blevel--;
//			if(blevel == 0)
//			{
//				rd = NULL;
//			}
//         continue;
//		}
//
//		ISTOKEN(tp, "bone")
//		{
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertType(TT_QUOTE));
//
//#pragma message("jz todo: sparovat indexy")
//			/*bindex = asys->GetBoneIndex(tp.CToken->Text);
//
//			if(bindex == INDEX_NOT_FOUND)
//			{
//				//enf::LogF(LL_ERROR, "LoadRagdolls: Cannot found bone '%s'", tp.CToken->Text);
//        LogF("LoadRagdolls: Cannot found bone '%s'", tp.CToken->Text);
//				continue;
//			}
//
//			
//			RagdollDef::Bone bone;
//			bnames[blevel] = bone.Name = asys->GetBoneName(bindex);
//			*/
//      RagdollDef::Bone bone;
//      bnames[blevel] = bone.Name = tp.CToken->Text;
//
//			bone.Offset.Zero();
//			bone.Added = false;
//			bone.Mass = 0.0f;
//
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToBool(tp, bone.Simulated));
//
//			//for simulated bones 'mass' must be set!
//			if(tp.NextToken())
//			{
//				CASSERT(tp.AssertToken(CParser::TComma));
//				CASSERT(TokenToFloat(tp, bone.Mass));
//			}
//
//			bone.Parent = bnames[blevel - 1];
//			rd->Bones.Insert(bone);
//			continue;
//		}
//
//		ISTOKEN(tp, "geometry")
//		{
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertType(TT_QUOTE));
//
//			RagdollDef::Geometry geom;
//
//			const RagdollDef::Bone* pbone;
//
//			pbone = rd->FindBone(tp.CToken->Text);
//
//			if(pbone == NULL)
//			{
//				//enf::LogF(LL_ERROR, RPMissingBone, rd->GetName(), tp.CToken->Text);
//        LogF(RPMissingBone, rd->GetName(), tp.CToken->Text);
//				res = false;
//				continue;
//			}
//
//			geom.Bone1 = pbone->Name;
//
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(tp.AssertType(TT_QUOTE));
//
//			pbone = rd->FindBone(tp.CToken->Text);
//
//			if(pbone == NULL)
//			{
//				//enf::LogF(LL_ERROR, RPMissingBone, rd->GetName(), geom.Bone2);
//        LogF(RPMissingBone, rd->GetName(), geom.Bone2);
//				res = false;
//				continue;
//			}
//
//			geom.Bone2 = pbone->Name;
//			
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToFloat(tp, geom.Radius));
//
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(tp.AssertType(TT_QUOTE));
//			geom.Material = FindMaterial(tp.CToken->Text);
//
//			if(geom.Material == NULL)
//			{
//				//enf::LogF(LL_ERROR, RPMissingMaterial, rd->GetName(), tp.CToken->Text);
//        LogF(RPMissingMaterial, rd->GetName(), tp.CToken->Text);
//				res = false;
//				continue;
//			}
//
//			rd->Geoms.Insert(geom);
//			continue;
//		}
//
//		if(blevel < 2)
//		{
//         tp.LError(CParser::SequenceError);
//         res = false;
//         continue;
//		}
//
////added bones must be here, as they need parent bone
//		ISTOKEN(tp, "addbone")
//		{
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertType(TT_QUOTE));
//
//			RagdollDef::Bone bone;
//			bnames[blevel] = bone.Name = g_pStatic->AddString(tp.CToken->Text);
//			bone.Added = true;
//			bone.Mass = 0.0f;
//
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(tp.AssertType(TT_QUOTE));
//
//			sscanf(tp.CToken->Text, "%f %f %f", &bone.Offset._e[0], &bone.Offset._e[1], &bone.Offset._e[2]);
//
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToBool(tp, bone.Simulated));
//
//			//for simulated bones 'mass' must be set!
//			if(tp.NextToken())
//			{
//				CASSERT(tp.AssertToken(CParser::TComma));
//				CASSERT(TokenToFloat(tp, bone.Mass));
//			}
//
//			bone.Parent = bnames[blevel - 1];
//			rd->Bones.Insert(bone);
//			continue;
//		}
//
//		ISTOKEN(tp, "hingejoint")
//		{
//			CASSERT(tp.NextToken(true));
//
//			RagdollDef::Joint joint;
//			joint.Type = RagdollDef::JOINT_HINGE;
//			joint.Name = bnames[blevel - 1];
//
//			CASSERT(TokenToVector(tp, joint.VAxis));
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToFloat(tp, joint.Angle1));
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToFloat(tp, joint.Angle2));
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToFloat(tp, joint.Damp));
//			rd->Joints.Insert(joint);
//			continue;
//		}
//
//		ISTOKEN(tp, "fixedjoint")
//		{
//			RagdollDef::Joint joint;
//			joint.Type = RagdollDef::JOINT_FIXED;
//			joint.Name = bnames[blevel - 1];
//			rd->Joints.Insert(joint);
//			continue;
//		}
//
//		ISTOKEN(tp, "balljoint")
//		{
//			CASSERT(tp.NextToken(true));
//
//			RagdollDef::Joint joint;
//
//			joint.Type = RagdollDef::JOINT_BALL;
//			joint.Name = bnames[blevel - 1];
//
//			CASSERT(TokenToVector(tp, joint.VAxis));
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToFloat(tp, joint.Angle1));
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToFloat(tp, joint.Angle2));
//			CASSERT(tp.NextToken(true));
//			CASSERT(tp.AssertToken(CParser::TComma));
//			CASSERT(TokenToFloat(tp, joint.Damp));
//			rd->Joints.Insert(joint);
//			continue;
//		}
//
//		tp.LError(CParser::ParmParseError);
//		res = false;
//   }
//
//	return res;
}

//-----------------------------------------------------------------------
bool BulletWorld::dRagdollIntersect(Object* ent, BaseTrace* trace)
{
	return false;
}

//-----------------------------------------------------------------------
struct RagdollSourceBone
{
	CStr		name;
	uint		parent;
	uint		originIndex;
	bool		simulated;
	Matrix43	transform;
	float		weight;
};

struct RagdollGeometry
{
	uint		bone0, bone1;
	float		radius;
	float		length;
	Matrix43	transform;
	PhysicsMaterial* material;
};

struct RagdollJoint
{
	CStr		name;
	int		type;
	uint		bone;
	Vector3	axis;
	float		angle1, angle2, angle3;
	float		damping;
};

//-----------------------------------------------------------------------
void Matrix2Shape(Object* ent, const Matrix43& mat, float scale = 1.0f, float size = 0.05f)
{
#ifdef ENF_VISUALIZE_RAGDOLL
  Vector3 start = mat * Vector3(0, 0, 0);
  Vector3 end = start + mat.Direction() * 0.15f;
  Shape::CreateLine(start, end, 0xffff0000, SS_ONCE);

  end = start + mat.DirectionAside() * 0.15f;
  Shape::CreateLine(start, end, 0xffff00ff, SS_ONCE);

  end = start + mat.DirectionUp() * 0.15f;
  Shape::CreateLine(start, end, 0xff0000ff, SS_ONCE);

#endif
}

//-----------------------------------------------------------------------
struct TempGeom
{
	Matrix43	rb2bone;
	Matrix43	rbTransform;
	uint		originIndex;
	BulletBoneController*	body;
	TempGeom(): originIndex(INDEX_NOT_FOUND), body(NULL) {}
};

class RagdollFactory
{
protected:
	Array<RagdollSourceBone>	m_Bones;
	Array<RagdollGeometry>		m_Geoms;
	Array<RagdollJoint>			m_Joints;
	Matrix43							m_Root;

public:
	void Clear()
	{
		m_Bones.Clear();
	}

	RagdollSourceBone* AddBone(Object* ent, CStr name, CStr parent, uint originIndex, const Matrix43& transform, bool simulated, float weight = 0.0f)
	{
		Assert(!BadMatrix43(transform));
		Assert(!BadFloat(weight));

		uint parentIndex = INDEX_NOT_FOUND;
		if(!parent.IsEmpty())
		{
			parentIndex = FindBone(parent);
			if(parentIndex == INDEX_NOT_FOUND)
				return NULL;
		}

		RagdollSourceBone& bone = m_Bones.Append();
		bone.name = name;
		bone.parent = parentIndex;
		bone.originIndex = originIndex;
		bone.transform = transform;
		bone.simulated = simulated;
		bone.weight = weight;
#ifdef ENF_VISUALIZE_RAGDOLL //add bone -> correct T position
		if(parentIndex != INDEX_NOT_FOUND)
    {
      	Shape::CreateLine(m_Root * transform.Position(), m_Root * m_Bones[parentIndex].transform.Position(), 0xffff00ff, SS_ONCE);
    }
#endif
		return &bone;
	}

	uint FindBone(CStr name)
	{
		for(uint n = 0; n < m_Bones.GetCardinality(); n++)
		{
			if(m_Bones[n].name == name)
				return n;
		}
		return INDEX_NOT_FOUND;
	}

	void AddGeometry(Object* ent, CStr boneName0, CStr boneName1, float radius, PhysicsMaterial* mat)
	{
		Assert(!BadFloat(radius));
		uint b0 = FindBone(boneName0);
		uint b1 = FindBone(boneName1);

		if(b0 != INDEX_NOT_FOUND && b1 != INDEX_NOT_FOUND)
		{
			RagdollSourceBone& bone0 = m_Bones[b0];
			RagdollSourceBone& bone1 = m_Bones[b1];
	
			const Matrix43& mat1 = bone0.transform;
			const Matrix43& mat2 = bone1.transform;
		
      Vector3 boneVec = (mat2.Position() - mat1.Position());
			Vector3 boneAxis = boneVec;
			float boneLength = boneAxis.Normalize();

			Matrix33 boneRotation = FromUpVector(boneAxis);//FromVectorAlongZ(boneAxis);
			
      Matrix43 boneMatrix(boneRotation, mat1.Position() + boneRotation.TransformVector(Vector3(0,boneLength * 0.5f,0)));
			
			Assert(!BadMatrix43(boneMatrix));
			Assert(!BadFloat(boneLength));

			RagdollGeometry& geom = m_Geoms.Append();
			geom.bone0 = b0;
			geom.bone1 = b1;
			geom.transform = boneMatrix;
			geom.radius = radius;
			geom.length = boneLength;
			geom.material = mat;
#ifdef ENF_VISUALIZE_RAGDOLL  //t pose geom ok, matrices ok
		  Vector3 start = m_Root * (bone0.transform.Position() + Vector3(0.f, 1.5f, 0.f));
      Vector3 end = m_Root * (bone1.transform.Position()  + Vector3(0.f, 1.5f, 0.f));
      Shape::CreateLine(start, end, 0xffff00ff, SS_ONCE);
      
      Vector3 center = start + ((end - start)*0.5f);
      Matrix43 test = m_Root * boneMatrix;
      test.SetPosition(test.Position() +  Vector3(0.f, 1.5f, 0.f));
      Matrix2Shape(ent, test);
#endif
		}
	}

	uint AddHingeJoint(CStr name, const Vector3& axis, float lowerLimit, float upperLimit, float damping)
	{
		Assert(!BadVec(axis));
		uint index = FindBone(name);

		if(index == INDEX_NOT_FOUND)
			return INDEX_NOT_FOUND;

		RagdollJoint& joint = m_Joints.Append();
		joint.name = name;
		joint.type = RagdollDef::JOINT_HINGE;
		joint.bone = index;
		joint.axis = axis;
		joint.angle1 = lowerLimit;
		joint.angle2 = upperLimit;
		joint.angle3 = 0;
		joint.damping = damping;
		return m_Joints.GetCardinality() - 1;
	}
	
	uint AddFixedJoint(CStr name)
	{
		uint index = FindBone(name);

		if(index == INDEX_NOT_FOUND)
			return INDEX_NOT_FOUND;

		RagdollJoint& joint = m_Joints.Append();
		joint.name = name;
		joint.type = RagdollDef::JOINT_FIXED;
		joint.bone = index;
		joint.axis.Zero();
		joint.angle1 = 0;
		joint.angle2 = 0;
		joint.angle3 = 0;
		joint.damping = 0;
		return m_Joints.GetCardinality() - 1;
	}
	
	uint AddBallSocketJoint(CStr name, const Vector3& axis, float coneLimit1, float coneLimit2, float twistLimit, float damping)
	{
		Assert(!BadVec(axis));
		uint index = FindBone(name);

		if(index == INDEX_NOT_FOUND)
			return INDEX_NOT_FOUND;

		RagdollJoint& joint = m_Joints.Append();
		joint.name = name;
		joint.type = RagdollDef::JOINT_BALL;
		joint.bone = index;
		joint.axis = axis;
		joint.angle1 = coneLimit1;
		joint.angle2 = coneLimit2;
		joint.angle3 = twistLimit;
		joint.damping = damping;
		return m_Joints.GetCardinality() - 1;
	}

	uint FindParent(uint bone, uint parent)
	{
		while(bone != INDEX_NOT_FOUND)
		{
			if(m_Bones[bone].simulated && bone == parent)
				return bone;
			bone = m_Bones[bone].parent;
		}
		return INDEX_NOT_FOUND;
	}

	BulletRagdollController* Build(BulletWorld* world, Entity* ent, const Matrix43* bindPose, const Matrix43* currentPose, float totalWeight, uint interactionLayer)
	{
		//normalize weight;
		float sumWeight = 0;
		for(uint n = 0; n < m_Bones.GetCardinality(); n++)
		{
			if(m_Bones[n].simulated)
				sumWeight += m_Bones[n].weight;
		}
		float scaleWeight = 1.0f / sumWeight;
		for(uint n = 0; n < m_Bones.GetCardinality(); n++)
		{
			if(m_Bones[n].simulated)
				m_Bones[n].weight *= scaleWeight;
		}

		BulletRagdollController* ragdoll = new BulletRagdollController();

		ragdoll->m_pEntity = ent;
		ragdoll->m_iInteractionLayer = interactionLayer;
		TempGeom* geoms = ALLOCAINIT(TempGeom, m_Geoms.GetCardinality());

		Ptr<BulletBoneController>* bones = ALLOCAINIT(Ptr<BulletBoneController>, m_Bones.GetCardinality());

    ent->SetPhysicsController(ragdoll);

		//1.find the first simulated bone geometries parents and attach them together (compute matrix for rb->bone space)
		for(uint n = 0; n < m_Geoms.GetCardinality(); n++)
		{
			RagdollGeometry& geom = m_Geoms[n];

			uint simulatedParent0 = enf_min(geom.bone0, geom.bone1);
			//uint simulatedParent1 = enf_max(geom.bone0, geom.bone1);
			Assert(FindParent(enf_max(geom.bone0, geom.bone1), simulatedParent0) != INDEX_NOT_FOUND);

			uint originIndex = m_Bones[simulatedParent0].originIndex;
			const Matrix43& currentBoneMat = currentPose[originIndex];
      //Matrix43 bind2current = bindPose[originIndex].GetInverse() * geom.transform;
      Matrix43 bind2current = bindPose[originIndex].InverseGeneral() * geom.transform;
#ifdef ENF_VISUALIZE_RAGDOLL  //matice current pozice
      Matrix2Shape(ent, m_Root * currentBoneMat);
#endif
			
			geoms[n].rbTransform = currentBoneMat * bind2current;
			geoms[n].originIndex = originIndex;
			geoms[n].rb2bone = geoms[n].rbTransform.InvTransform(currentBoneMat);
      
#ifdef ENF_VISUALIZE_RAGDOLL  //geometry arrows + matrices
    //	Shape::CreateCapsule(Vector3(0,0,0), geom.radius, geom.length + geom.radius, 0x4f1f5f3f, SS_NOZWRITE|SS_TRANSP)->SetMatrix(m_Root * geoms[n].rbTransform);
			Matrix43 lineMat = m_Root * geoms[n].rbTransform;
      Matrix2Shape(ent, lineMat);

      Vector3P row2 = lineMat.DirectionUp();//Vector3(lineMat.DirectionAside().Z(), lineMat.DirectionUp().Z(), lineMat.Direction().Z());
      Vector3 start = lineMat.Position() + row2 * (m_Geoms[n].length * 0.5f);
      Vector3 end = lineMat.Position() - row2 * (m_Geoms[n].length * 0.5f);
      Shape::CreateLine(start, end, 0xffffffff, 0);
      //ent->AddDiagCapsule(0.01f, m_Geoms[n].length * 0.5, 0.01f, lineMat, Color(1.0f, 1.0f, 1.0f, 1.0f));
      
      //Shape::CreateLine(lineMat.Position() + lineMat.GetRow(2) * (m_Geoms[n].length * 0.5f), lineMat.Position() - lineMat.GetRow(2) * (m_Geoms[n].length * 0.5f), 0xffffffff, 0);
#endif
			//2.get geometries and turn them to rigid bodies
     BulletBoneController* boneController = world->CreateRagdollBone(
					ent,
					m_Root * geoms[n].rbTransform,
					geoms[n].rb2bone,
					geoms[n].originIndex,
					m_Geoms[n].radius,
					m_Geoms[n].length,
					m_Geoms[n].material,
					m_Bones[simulatedParent0].weight * totalWeight,
					interactionLayer);

			boneController->m_strName = m_Bones[simulatedParent0].name;

			Assert(boneController);
			geoms[n].body = boneController;
			bones[simulatedParent0] = boneController;
		}

		//3.connect them by joints
		for(uint n = 0; n < m_Joints.GetCardinality(); n++)
		{
			RagdollJoint& joint = m_Joints[n];
			RagdollSourceBone& bone = m_Bones[joint.bone];
			if(bone.parent != INDEX_NOT_FOUND)
			{
				//find the bodies
				BulletBoneController* boneController0 = bones[joint.bone];
				BulletBoneController* boneController1 = bones[m_Bones[joint.bone].parent];

				RagdollSourceBone& bone = m_Bones[joint.bone];

				Matrix43 jointMat = currentPose[bone.originIndex] * bindPose[bone.originIndex].InvTransform(bone.transform);
        Vector3 jointAxis = currentPose[bone.originIndex].Orientation().TransformVector(bindPose[bone.originIndex].Orientation().InvTransformVector(joint.axis));
        
				jointMat = m_Root * jointMat;
				jointAxis = m_Root.Orientation().TransformVector(jointAxis);

				BulletPhysicsJoint physJoint;

				if(joint.type == RagdollDef::JOINT_HINGE)
				{
					PhysicsHingeJoint hinge = world->CreateHingeJoint(boneController0, boneController1,
						boneController0->GetWorldTransform().InvTransformCoord(jointMat.Position()),
						boneController0->GetWorldTransform().Orientation().InvTransformVector(jointAxis),
						boneController1->GetWorldTransform().InvTransformCoord(jointMat.Position()),
						boneController1->GetWorldTransform().Orientation().InvTransformVector(jointAxis),
						true); 

					hinge.SetLimit(DEG2RAD(joint.angle1), DEG2RAD(joint.angle2), joint.damping, 0.3f, 1.0f);
#ifdef ENF_VISUALIZE_RAGDOLL //hinge joint yellow axis
          Shape::CreateLine(jointMat.Position(), jointMat.Position() + jointAxis * 0.15f, 0xffff00ff, 0);    
					//Shape::CreateLine(jointMat.Position(), jointMat.Position() + jointAxis * 16.0f, 0xff000000, 0);
#endif
					physJoint = hinge;
				}
				else if(joint.type == RagdollDef::JOINT_BALL)
				{
					//Matrix43 coneMatrix(FromVectorAlongX(jointAxis), jointMat.Position());
          Matrix43 coneMatrix(FromDirVector(jointAxis), jointMat.Position());
          
#ifdef ENF_VISUALIZE_RAGDOLL//1  //ball joint purple sphere + matrix
          //ent->AddDiagArrow(coneMatrix.Position(), coneMatrix.Position() + (jointAxis.Normalized() * 0.4f), Color(0.f, 1.f, 1.f));  //show axis which should be in mx direction
          //ent->AddDiagSphere(coneMatrix.Position(), 0.02f, Color(0.f, 0.f, 1.f, 1.f));
					Matrix2Shape(ent, coneMatrix, 4.0f, 0.05f);
#endif
					PhysicsConeTwistJoint coneTwist = world->CreateConeTwistJoint(boneController0, boneController1,
						boneController0->GetWorldTransform().InvTransform(coneMatrix),
						boneController1->GetWorldTransform().InvTransform(coneMatrix),
            true); 
					coneTwist.SetLimit(DEG2RAD(joint.angle1), DEG2RAD(joint.angle1), DEG2RAD(joint.angle2), joint.damping);
#ifdef ENF_VISUALIZE_RAGDOLL
          Shape::CreateLine(jointMat.Position(), jointMat.Position() + (jointAxis * 0.15f), 0xffff00ff, 0);
          //Shape::CreateLine(jointMat.Position(), jointMat.Position() + jointAxis * 16.0f, 0xff000000, 0);
#endif
					physJoint = coneTwist;
				}
				else if(joint.type == RagdollDef::JOINT_FIXED)
				{
					physJoint = world->CreateFixedJoint(boneController0, boneController1,
						boneController0->GetWorldTransform().InvTransformCoord(jointMat.Position()),
						boneController1->GetWorldTransform().InvTransformCoord(jointMat.Position()),
						true); 
				}
				ragdoll->m_Joints.Insert(physJoint);
#ifdef ENF_VISUALIZE_RAGDOLL
				//Matrix2Shape(jointMat, 2.0f, 0.05f);
#endif
			}
		}

#ifdef ENF_VISUALIZE_RAGDOLL
    for(int idx = 0; idx < ragdoll->GetNumBones(); idx++)
    {
      btTransform t = ragdoll->GetBone(idx)->GetRigidBody()->getWorldTransform();
      static bool dis = false;
      if(dis)
        ragdoll->GetBone(idx)->GetRigidBody()->setActivationState(DISABLE_SIMULATION);
      Shape::CreateSphere(enf::BulletWorld::btVector2Point(t.getOrigin()), 0.02f, 0xffff0000, SS_ONCE);
    }
#endif

		return ragdoll;
	}

	RagdollFactory(const Matrix43& mat): m_Root(mat)
	{
	}

	~RagdollFactory()
	{
	}
};

//-----------------------------------------------------------------------
void BulletBoneController::getWorldTransform(btTransform& centerOfMassWorldTrans ) const
{
}

//-----------------------------------------------------------------------
void BulletBoneController::setWorldTransform(const btTransform& centerOfMassWorldTrans)
{
  Entity* ent = (Entity*)m_pRagdoll->GetOwner();
	const Matrix43& entTransform =  ent->IGetMatrix();

	Matrix43 bodyMat = BulletWorld::btTransform2Matrix(centerOfMassWorldTrans);
	Matrix43 localBodyMat = entTransform.InvTransform(bodyMat);
	Matrix43 boneMat = localBodyMat * m_Body2Bone;

#ifdef ENF_VISUALIZE_RAGDOLL
	Shape::CreateLine(bodyMat.Position(), (entTransform * boneMat).Position(), 0xffffffff, SS_ONCE);
#endif
	//ent->SetBoneMatrix(boneMat, ent->FutureVisualState(), m_iOriginalIndex, m_strName.cstr());
}

//-----------------------------------------------------------------------
void BulletBoneController::SetWorldTransform(const Matrix43& trans)
{
	btTransform transform = BulletWorld::Matrix2btTransform(trans);
	setWorldTransform(transform);
	GetRigidBody()->setCenterOfMassTransform(transform);
}

//-----------------------------------------------------------------------
bool BulletRagdollController::IsActive() const
{
	for(uint n = 0; n < m_Bones.GetCardinality(); n++)
	{
		if(m_Bones[n]->IsActive())
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------
void BulletRagdollController::AddBone(BulletBoneController* bone)
{
	m_Bones.Insert(bone);
	bone->m_pRagdoll = this;
}

//-----------------------------------------------------------------------
BulletBoneController* BulletWorld::CreateRagdollBone(Object* ent, const Matrix43& mat, const Matrix43& body2bone, uint originalIndex, float radius, float length, PhysicsMaterial* material, float mass, uint interactionLayer)
{
  Assert(ent);
	Assert(material != NULL);
	Assert(!BadFloat(radius) && radius > 0.0f);
	Assert(!BadFloat(length) && length >= 0.0f);
	Assert(!BadFloat(mass) && mass > 0.0f);
	Assert(!BadMatrix43(mat));
	Assert(!BadMatrix43(body2bone));

  bool btype = ent->GetPhysics()->HasType(BulletRagdollControllerTypeID);
	BulletRagdollController* ragdoll	= BulletRagdollController::SafeCast(ent->GetPhysics());
	if(ragdoll && btype)
	{
		btRigidBody::btRigidBodyConstructionInfo constructionInfo(0.0f, NULL, NULL);
		constructionInfo.m_linearDamping = 0.3f;
		constructionInfo.m_angularDamping = 0.6f;
		constructionInfo.m_linearSleepingThreshold = 0.9f;
		constructionInfo.m_angularSleepingThreshold = 1.1f;
		btRigidBody* rb = new btRigidBody(constructionInfo);

		BulletBoneController* boneController = new BulletBoneController(rb, body2bone, originalIndex);

		rb->setUserPointer((void*)ent);
		rb->setMotionState(boneController);

		btCollisionShape* cshape;
		if(length == 0)
			cshape = new btSphereShape(radius * MEQ_WORLDISCALE);
		else
			cshape = new btCapsuleShape(radius * MEQ_WORLDISCALE, length * MEQ_WORLDISCALE);

		cshape->setMaterial(((BulletPhysicsMaterial*)material)->m_pBtMaterial);
		rb->setCollisionShape(cshape);
		rb->setInteractionLayer(interactionLayer);

		if(mass == 0.0f)
			rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
		else
		{
			btVector3 inertia;
			cshape->calculateLocalInertia(mass, inertia);
			rb->setMassProps(mass, inertia);
		}

#ifdef ENF_VISUALIZE_RAGDOLL
    	Shape::CreateSphere(mat.Position(), 0.02f, 0xffff0000, 0);
		Matrix2Shape(ent, mat, 4.0f, 0.05f);
#endif

    btTransform trans = BulletWorld::Matrix2btTransform(mat);
		rb->setCenterOfMassTransform(trans);

		m_pDynamicsWorld->addRigidBody(rb);
		rb->activate(true);
		ragdoll->AddBone(boneController);

		return boneController;
	}
	return NULL;
}

//-----------------------------------------------------------------------
BulletBoneController::BulletBoneController(btRigidBody* body, const Matrix43& body2bone, uint originalIndex): BulletDynamicController(body), m_pRagdoll(NULL), m_Body2Bone(body2bone), m_iOriginalIndex(originalIndex)
{
  //LogF("OriginalIndex: %i\n", m_iOriginalIndex);
}

//-----------------------------------------------------------------------
Matrix4 BulletRagdollController::GetWorldTransform() const 
{
  return m_pEntity->IGetMatrix();
}

//-----------------------------------------------------------------------
bool BulletRagdollController::TraceMove(BaseTrace* trace)
{
  RlsAssert2(0, "not implemented");
  return false;
  /*
	if(trace->Len == 0 || (trace->Flags & TRACE_RAGDOLLS) == 0)
		return false;

	Object* owner = m_pEntity;
	BulletWorld* mw = (BulletWorld*)owner->GetWorld()->GetPhysics();

	if(!mw->GetBulletWorld()->getBroadphase()->getOverlappingPairCache()->hasInteractionEnabled(trace->InteractionLayer, m_iInteractionLayer))
		return false;

	btVector3 btStart = BulletWorld::Point2btVector(trace->Start);
	btVector3 btStop = BulletWorld::Point2btVector(trace->EndPos);
	btTransform trStart;
	trStart.getBasis().setIdentity();
	trStart.setOrigin(btStart);
	btTransform trStop;
	trStop.getBasis().setIdentity();
	trStop.setOrigin(btStop);

	btDynamicsWorld* bulletWorld = mw->GetBulletWorld();

	bulletWorld->getBroadphase()->getOverlappingPairCache()->setCurrent();
	bool hasHit = false;
	for(uint n = 0; n < m_Bones.GetCardinality(); n++)
	{
		BulletBoneController* bc = m_Bones[n];
		btRigidBody* body = bc->GetRigidBody();
		btCollisionShape* shape = body->getCollisionShape();

		btCollisionWorld::ClosestRayResultCallback rrc(btStart, btStop);

		bulletWorld->rayTestSingle(trStart, trStop,
										body,
										shape,
										body->getWorldTransform(),
										rrc,
										trace->InteractionLayer);

		if(rrc.hasHit() && rrc.m_closestHitFraction < trace->Fraction)
		{
			Vector3 endpos = trace->Start.Lerp(trace->EndPos, rrc.m_closestHitFraction);
			
			trace->Plane.x = rrc.m_hitNormalWorld.getX();
			trace->Plane.y = rrc.m_hitNormalWorld.getY();
			trace->Plane.z = rrc.m_hitNormalWorld.getZ();

			trace->Plane.w = trace->Plane.Dot(endpos);
			trace->Contents = C_SOLID;
			uint surfParms = 0;
			if(rrc.m_collisionShape)
			{
				const btMaterial* mat = rrc.m_collisionShape->getMaterial();
				if(mat)
				{
					const BulletPhysicsMaterial* pmat = (const BulletPhysicsMaterial*)mat->getUserPointer();
					if(pmat)
					{
						surfParms = pmat->m_iMaterialIndex;
					}
				}
			}
			trace->SurfParms = surfParms;
			trace->ClipEntity = owner;
			trace->MaterialPtr = NULL;
			trace->Bone = n;
			trace->Fraction = rrc.m_closestHitFraction;
			hasHit = true;
		}
	}
	return hasHit;
  */
}

//-----------------------------------------------------------------------
bool BulletRagdollController::Intersects(const Vector3& start, const Vector3& stop, uint interactionLayer)
{
	Object* owner = m_pEntity;
  BulletWorld* mw = (BulletWorld*)owner->GetWorldPhysics();//GetWorld()->GetPhysics();
	btDynamicsWorld* bulletWorld = mw->GetBulletWorld();

	if(!bulletWorld->getBroadphase()->getOverlappingPairCache()->hasInteractionEnabled(interactionLayer, m_iInteractionLayer))
		return false;

	btVector3 btStart = BulletWorld::Point2btVector(start);
	btVector3 btStop = BulletWorld::Point2btVector(stop);
	btTransform trStart;
	trStart.getBasis().setIdentity();
	trStart.setOrigin(btStart);
	btTransform trStop;
	trStop.getBasis().setIdentity();
	trStop.setOrigin(btStop);

	bulletWorld->getBroadphase()->getOverlappingPairCache()->setCurrent();
	for(uint n = 0; n < m_Bones.GetCardinality(); n++)
	{
		BulletBoneController* bc = m_Bones[n];
		btRigidBody* body = bc->GetRigidBody();
		btCollisionShape* shape = body->getCollisionShape();

		btCollisionWorld::ClosestRayResultCallback rrc(btStart, btStop);

		bulletWorld->rayTestSingle(trStart, trStop,
										body,
										shape,
										body->getWorldTransform(),
										rrc,
										interactionLayer);

		if(rrc.hasHit())
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------
BulletBoneController* BulletRagdollController::FindBoneByNodeIndex(uint index)
{
	for(uint n = 0; n < m_Bones.GetCardinality(); n++)
	{
		if(m_Bones[n]->m_iOriginalIndex == index)
			return m_Bones[n];
	}
	return NULL;
}

//-----------------------------------------------------------------------
BulletBoneController* BulletRagdollController::FindBoneByName(CStr name)
{
	for(uint n = 0; n < m_Bones.GetCardinality(); n++)
	{
		if(m_Bones[n]->m_strName == name)
			return m_Bones[n];
	}
	return NULL;
}

//-----------------------------------------------------------------------
BulletRagdollController::~BulletRagdollController()
{
  BulletWorld* bw = (BulletWorld*)m_pEntity->GetWorldPhysics();//GetWorld()->GetPhysics();
	for(uint j = 0; j < m_Joints.GetCardinality(); j++)
	{
		bw->DestroyJoint(m_Joints[j]);
	}
	for(uint n = 0; n < m_Bones.GetCardinality(); n++)
	{
		delete m_Bones[n];
	}

#pragma message("jz todo: give back that stuff bellow")
  /*
	enf::MeshObjectAnimatedInstance* inst = enf::MeshObjectAnimatedInstance::SafeCast(m_pEntity->GetInstance());
	if(inst)
		inst->SetManaged(BoneMask());
  */
}

#if _ENABLE_PHYSICS
//-----------------------------------------------------------------------
BulletRagdollController* BulletWorld::CreateRagdoll(Entity* ent, CStr ragdollDefName, float mass, uint interactionLayer)
{
  Assert(ent != NULL);
	Assert(ent->GetPhysics() == NULL);
	Assert(!ragdollDefName.IsEmpty());
	Assert(mass > 0);
	
  const RagdollDef* rd = g_pBulletManager->m_RagdollsHash.Find(ragdollDefName.cstr());

	if(rd == NULL)
	{
		LogF("Can't found ragdoll definition %s", ragdollDefName.cstr());
		return NULL;
	}


	Matrix43 emat = ent->RenderVisualState().ModelToWorld();
  //emat.SetOrientation(emat.Orientation().NormalTransform());
  
	RagdollFactory factory(emat);

  Skeleton* skeleton = ent->GetShape()->GetSkeleton();
  Matrix4* cpose = (Matrix4*)GMemFunctions->New(sizeof(Matrix4) * skeleton->NBones());
  Matrix4* tpose = (Matrix4*)GMemFunctions->New(sizeof(Matrix4) * skeleton->NBones());
  ent->GetSkeletonMatrices(cpose, tpose);
  
  //add bones
	for(uint n = 0; n < rd->Bones.GetCardinality(); n++)
	{
		const RagdollDef::Bone& bone = rd->Bones[n];

		if(bone.Added)
		{ 
      int tindex = ent->GetBone(bone.Parent);

			if(tindex == SkeletonIndexNone)
			{
				LogF(RDMissingBone, ragdollDefName.cstr(), bone.Parent);
				return 0;
			}

			const Matrix43& tmat = tpose[tindex];
			factory.AddBone(ent, bone.Name, bone.Parent, INDEX_NOT_FOUND, Matrix43(tmat.Orientation(), tmat.Position() + bone.Offset), bone.Simulated, bone.Mass);
		}
		else
		{
			int index = ent->GetBone(bone.Name);

			if(index == SkeletonIndexNone)
			{
				LogF(RDMissingBone, ragdollDefName.cstr(), bone.Name);
				return 0;
			}

			//if(bone.Simulated)
			//	mask.SetBit(index);
      
			const Matrix43& tmat = tpose[index];
			factory.AddBone(ent, bone.Name, bone.Parent, index, tmat, bone.Simulated, bone.Mass);
		}
	}

	//add geometries
	for(uint n = 0; n < rd->Geoms.GetCardinality(); n++)
	{
		const RagdollDef::Geometry& geom = rd->Geoms[n];
		factory.AddGeometry(ent, geom.Bone1, geom.Bone2, geom.Radius, geom.Material);
	}

	//add joints
	for(uint n = 0; n < rd->Joints.GetCardinality(); n++)
	{
		const RagdollDef::Joint& joint = rd->Joints[n];

    int index = ent->GetBone(joint.Name);

		if(index == -1)
		{
			LogF(RDMissingBone, ragdollDefName.cstr(), joint.Name);
			return NULL;
		}
    
		switch(joint.Type)
		{
		case RagdollDef::JOINT_FIXED:
			factory.AddFixedJoint(joint.Name);
			break;

		case RagdollDef::JOINT_HINGE:
			factory.AddHingeJoint(joint.Name, joint.VAxis, joint.Angle1, joint.Angle2, joint.Damp);
			break;

		case RagdollDef::JOINT_BALL:
			factory.AddBallSocketJoint(joint.Name, joint.VAxis, joint.Angle1, joint.Angle1, joint.Angle2, joint.Damp);
			break;
		}
	}

	//ai->SetManaged(mask);
	return factory.Build(this, ent, tpose, cpose, mass, interactionLayer);
}
#endif

}