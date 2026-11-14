#include "ps.h"


//#define IDENTIFY_SHADERS

/*!
helper debugging function used for shaders identify
*/

//XXD, XXDS
void IdentifyShader(uniform int ID, inout half4 outColor)
{
#ifdef IDENTIFY_SHADERS	
	outColor.rgb = 0;

	if (ID == 7)	outColor.rgb = half3(0.7, 0.0, 0.0); //SpecularAlpha)          
	if (ID == 8)	outColor.rgb = half3(0.7, 0.2, 0.0); //SpecularNormalMapGrass) 	
	if (ID == 25)	outColor.rgb = half3(0.7, 0.5, 0.0); //SpecularNormalMapDiffuse)            	
	if (ID == 26)	outColor.rgb = half3(0.7, 0.8, 0.0); //SpecularNormalMapDiffuseMacroAS)     	
                                                     
	if (ID == 1)	outColor.rgb = half3(0.3, 0.1, 0.0); //Detail)               
	if (ID == 2)	outColor.rgb = half3(0.3, 0.4, 0.0); //DetailSpecularAlpha)	
	if (ID == 13)	outColor.rgb = half3(0.3, 0.7, 0.0); //DetailMacroAS)                       
	if (ID == 14)	outColor.rgb = half3(0.3, 1.0, 0.0); //DetailSpecularAlphaMacroAS)          	
                                                     
	if (ID == 15)	outColor.rgb = half3(0.0, 0.0, 1.0); //NormalMap)                           	
	if (ID == 3)	outColor.rgb = half3(0.0, 0.5, 1.0); //Normal
	if (ID == 6)	outColor.rgb = half3(0.0, 1.0, 1.0); //NormalDXTA
                                                     
	if (ID == 16)	outColor.rgb = half3(0.3, 0.0, 0.7); //NormalMapDetailMacroASSpecularMap)   	
	if (ID == 17)	outColor.rgb = half3(0.3, 0.3, 0.7); //NormalMapDetailMacroASSpecularDIMap) 	
	if (ID == 18)	outColor.rgb = half3(0.3, 0.5, 0.7); //NormalMapDetailSpecularMap)          	
	if (ID == 19)	outColor.rgb = half3(0.3, 0.8, 0.7); //NormalMapDetailSpecularDIMap)        	
	if (ID == 20)	outColor.rgb = half3(0.7, 0.0, 0.7); //NormalMapMacroAS)                    	
	if (ID == 21)	outColor.rgb = half3(0.7, 0.5, 0.7); //NormalMapMacroASSpecularMap)         	
	if (ID == 22)	outColor.rgb = half3(0.7, 1.0, 0.7); //NormalMapMacroASSpecularDIMap)       	
	if (ID == 23)	outColor.rgb = half3(1.0, 0.0, 0.7); //NormalMapSpecularMap)                	
	if (ID == 24)	outColor.rgb = half3(1.0, 1.0, 0.7); //NormalMapSpecularDIMap)              	
                                                     
	if (ID == 35)	outColor.rgb = half3(0.5, 0.5, 1.0); //Glass

	if (ID == 50 || ID == 51 || ID == 64)	outColor.rgb = half3(0.5, 0.0, 0.5); //Sprite, SpriteSimple, SpriteExtTi

	if (ID == 53)	outColor.rgb = half3(0.7, 0.7, 0.3); 	//Multi

	if (ID == 52 || ID == 66 || ID == 67)	outColor.rgb = half3(0.5, 0.2, 0.5); //Super, SuperExt, SuperAToC

	if (ID >= 36 && ID <= 49)	outColor.rgb = half3(1.0, 1.0, 1.0); 	//Crater1)

	if (ID == 9)  	outColor.rgb = half3(1.0, 0.2, 0.0); //SpecularNormalMapThrough
	if (ID == 10) 	outColor.rgb = half3(1.0, 0.5, 0.0); //SpecularNormalMapThroughSimple
	if (ID == 11) 	outColor.rgb = half3(1.0, 0.8, 0.0); //SpecularNormalMapSpecularThrough
	if (ID == 12) 	outColor.rgb = half3(1.0, 1.0, 0.0); //SpecularNormalMapSpecularThroughSimple

	if (ID == 4 || ID == 62) 	outColor.rgb = half3(0.0, 0.5, 0.0); //Grass, GrassAToC

	if (ID == 57 || ID == 61 || ID == 58 || ID == 59 || ID == 63)	outColor.rgb = half3(0.0, 0.5, 0.0); 	//Tree, TreeAToC, TreePRT, TreeSimple, TreeSN

	if (ID == 65 || ID == 54 || ID == 55) outColor.rgb = half3(0.2, 0.3, 0.2); //TerrainSNX, TerrainX, TerrainSimpleX
	if (ID == 56 || ID == 29) outColor.rgb = half3(0.2, 0.8, 0.2); //	TerrainGrassX, TerrainGrass15

	if (ID ==	30 || ID == 31) outColor.rgb = half3(0.3, 0.0, 0.2);	//Road, Road2Pass

	//trees
	if (ID == 100) 	outColor.rgb = half3(0.7, 1.0, 0.7); //PSSSSMTreeAdv
	if (ID == 101) 	outColor.rgb = half3(0.35, 0.5, 0.35); //PSSSSMTreeAdvSimple

	if (ID == 102) 	outColor.rgb = half3(0.39, 0.22, 0.13); //PSSSSMTreeAdvTrunk
	if (ID == 103) 	outColor.rgb = half3(0.8,  0.45, 0.26); //PSSSSMTreeAdvTrunkSimple
	
	outColor.rgb *= 0.5;
	/*
	not necessary 
	
	27,		Terrain15,       
	28,		TerrainSimple15, 
	32,		Shore,       
	33,		ShoreFoam,   	
	34,		ShoreWet,    	
	*/	
	
#endif
}

//! Macro to create the specified PS
#define CREATEPSSN(id, name,structureName) \
OPT_INPUTS void PSSSSM##name(structureName input, out half4 oColor: COLOR) \
{ \
	FOut fout = PF##name(input,true,false); \
	oColor = GetFinalColorSSSM(fout, input.tShadowBuffer, input.vPos); \
	IdentifyShader(id, oColor); \
}
//! Macro to create the specified PS
#define CREATEPSSN_DIRECT(id, name,structureName) \
OPT_INPUTS void PSSSSM##name(structureName input, out half4 oColor: COLOR) \
{ \
	PF##name(input,true,input.tShadowBuffer,input.vPos,FinalColorSSSM,oColor); \
	IdentifyShader(id, oColor);\
}

#define CREATEPSSN_NS(id, name,structureName) \
OPT_INPUTS void PS##name(structureName input, out half4 oColor: COLOR) \
{ \
	FOut fout = PF##name(input,false,input.vPos); \
	oColor = GetWaterFinalColor(fout); \
	IdentifyShader(id, oColor); \
}

// skin shader
OPT_INPUTS void PSSSSMSkin(SSkin i, out half4 oColor: COLOR)
{
	half4 as = PFSkin0( i, true, false );
	half shadowCoef = GetShadowBasicSSSM( i.tShadowBuffer, i.vPos, i.landShadow, as.b );
	SPFSkinOut o = PFSkin1( i, as, shadowCoef, true, false );
	oColor = PFSkinFinColor( o, shadowCoef );
}

// advanced tree PS
OPT_INPUTS void PSSSSMTreeAdv(STreeAdv i, out half4 oColor: COLOR)
{
	SPFTreeAdv s;
	half diffShadowCoef = 1;
	s.shadow = GetShadowBasicSSSM( i.tShadowBuffer, i.vPos, i.lightLocal_landShadow.w, diffShadowCoef );
	PFTreeAdv1( s, i, true, false, false, false );
	oColor = s.colorFinal;
	IdentifyShader(100, oColor);
}  
OPT_INPUTS void PSSSSMTreeAdvSimple(STreeAdv i, out half4 oColor: COLOR)
{
	SPFTreeAdv s;
	half diffShadowCoef = 1;
	s.shadow = GetShadowBasicSSSM( i.tShadowBuffer, i.vPos, i.lightLocal_landShadow.w, diffShadowCoef );
	PFTreeAdv1( s, i, true, true, false, false );
	oColor = s.colorFinal;
	IdentifyShader(101, oColor);
}
OPT_INPUTS void PSSSSMTreeAdvAToC(STreeAdv i, out half4 oColor: COLOR)
{
	SPFTreeAdv s;
	half diffShadowCoef = 1;
	s.shadow = GetShadowBasicSSSM( i.tShadowBuffer, i.vPos, i.lightLocal_landShadow.w, diffShadowCoef );
	PFTreeAdv1( s, i, true, false, true, false );
	oColor = s.colorFinal;
	IdentifyShader(100, oColor);
}  
OPT_INPUTS void PSSSSMTreeAdvSimpleAToC(STreeAdv i, out half4 oColor: COLOR)
{
	SPFTreeAdv s;
	half diffShadowCoef = 1;
	s.shadow = GetShadowBasicSSSM( i.tShadowBuffer, i.vPos, i.lightLocal_landShadow.w, diffShadowCoef );
	PFTreeAdv1( s, i, true, true, true, false );
	oColor = s.colorFinal;
	IdentifyShader(101, oColor);
}
OPT_INPUTS void PSSSSMTreeAdvTrunk(STreeAdv i, out half4 oColor: COLOR)
{
	SPFTreeAdv s;
	half diffShadowCoef = 1;
	s.shadow = GetShadowBasicSSSM( i.tShadowBuffer, i.vPos, i.lightLocal_landShadow.w, diffShadowCoef );
	PFTreeAdv1( s, i, false, false, false, false );
	oColor = s.colorFinal;
	IdentifyShader(102, oColor);
}
OPT_INPUTS void PSSSSMTreeAdvTrunkSimple(STreeAdv i, out half4 oColor: COLOR)
{
	SPFTreeAdv s;
	half diffShadowCoef = 1;
	s.shadow = GetShadowBasicSSSM( i.tShadowBuffer, i.vPos, i.lightLocal_landShadow.w, diffShadowCoef );
	PFTreeAdv1( s, i, false, true, false, false );
	oColor = s.colorFinal;
	IdentifyShader(103, oColor);
}

	
#define CREATESKINPS(id, name)


//! Define the shaders from list stored in header
#define PS_DEFC(id, name) CREATEPSSN(id, name,S##name)
#define PS_DEFS(id, name,structureName) CREATEPSSN(id, name,structureName)
#define PS_DEFN(id, name,structureName) CREATEPSSN_NS(id, name,structureName)
#define PS_DEFD(id, name) CREATEPSSN_DIRECT(id, name,S##name)
#define PS_DEFDS(id, name,structureName) CREATEPSSN_DIRECT(id, name,structureName)
PS_LIST(PS_DEFC,PS_DEFS,PS_DEFN,PS_DEFD,PS_DEFDS, CREATESKINPS)
