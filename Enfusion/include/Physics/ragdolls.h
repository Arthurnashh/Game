
//ragdoll "jmeno ragdollu", DEFAULT/MIDDLE/LOW

//"bone bonename, simulated (true/false) [, mass]"
// vlozi kost do characteru.
// simulated urcuje, jestli se ma fyzikalne simulovat, nebo je jen virtualni
// mass je hmotnost a MUSI byt uvedena, pokud je simulated = true

//"addbone bonename, offset, simulated [, mass]"
//prida interni kost do characteru. Je to moznost, jak pridat geometrii na koncove kosti

//balljoint axis, angle, twist, damper
// kost bude s parentem spojena jointem typu ball-socket.
// axis: je osa, ktera definuje orientaci jointu (1, 2, 3 odpovida X, Y, Z a -1,-2,-3 odpovida -X, -Y, -Z)
// angle: max.uhel, ktery smi svirat s parentem.
// twist: maximalni kroutici uhel, jaky smi svirat s parentem
// damper: tlumeni

//hingejoint axis, minangle, maxangle, damper
// kost bude s parentem spojena jointem typu hinge.
// axis: je osa, ktera definuje orientaci jointu (1, 2, 3 odpovida X, Y, Z a -1,-2,-3 odpovida -X, -Y, -Z)
// minangle: min.uhel, ktery smi svirat s parentem.
// maxangle: max.uhel, ktery smi svirat s parentem.
// damper: tlumeni

//axis se da definovat i vektorem "x y z", pokud nevyhovuji zakladni osy kosti.
//Je mozne dosahnout napriklad uhlu 45' pomoci "1 1 0", protoze vektor se normalizuje

//geometry bone1, bone2, radius[, cgroup]
//prida capsule geometrii mezi kosti bone1 a bone2 o prumeru radius. cgroup je kolizni maska. Default cgroup = 1

#include "El/Math/math3d.hpp"

#define JOINT_DAMPER 0.2
#define HEAD_JOINT_DAMPER 0.7
#define HUMANOID_MATERIAL "material/ragdollflesh"

struct geometrydef
{
char* name1;
char* name2;
float fval;
char* mat;
geometrydef(char* n1, char* n2, float f, char* m):name1(n1), name2(n2), fval(f), mat(m){};
};

const geometrydef hrgeom[] = {
/*
geometrydef("pelvis", "spine3", 5.0, HUMANOID_MATERIAL),
geometrydef("spine3", "head", 6.5, HUMANOID_MATERIAL),
//geometrydef("head", "HeadTerm", 3.5, HUMANOID_MATERIAL),
geometrydef("leftarm", "leftforearm", 2.0, HUMANOID_MATERIAL),
geometrydef("leftforearm", "lefthand", 2.0, HUMANOID_MATERIAL),
geometrydef("rightarm", "rightforearm", 2.0, HUMANOID_MATERIAL),
geometrydef("rightforearm", "righthand", 2.0, HUMANOID_MATERIAL),
geometrydef("leftupleg", "leftleg", 3.0, HUMANOID_MATERIAL),
geometrydef("rightupleg", "rightleg", 2.5, HUMANOID_MATERIAL),
geometrydef("leftleg", "leftfoot", 3.0, HUMANOID_MATERIAL),
geometrydef("rightleg", "rightfoot", 2.5, HUMANOID_MATERIAL),
geometrydef("rightfoot", "righttoebase", 2.5, HUMANOID_MATERIAL),
geometrydef("leftfoot", "lefttoebase", 2.5, HUMANOID_MATERIAL)
*/
geometrydef("pelvis", "spine3", 0.05, HUMANOID_MATERIAL),
geometrydef("spine3", "head", 0.065, HUMANOID_MATERIAL),
//geometrydef("head", "HeadTerm", 3.5, HUMANOID_MATERIAL),
geometrydef("leftarm", "leftforearm", 0.020, HUMANOID_MATERIAL),
geometrydef("leftforearm", "lefthand", 0.020, HUMANOID_MATERIAL),
geometrydef("rightarm", "rightforearm", 0.020, HUMANOID_MATERIAL),
geometrydef("rightforearm", "righthand", 0.020, HUMANOID_MATERIAL),
geometrydef("leftupleg", "leftleg", 0.030, HUMANOID_MATERIAL),
geometrydef("rightupleg", "rightleg", 0.025, HUMANOID_MATERIAL),
geometrydef("leftleg", "leftfoot", 0.030, HUMANOID_MATERIAL),
geometrydef("rightleg", "rightfoot", 0.025, HUMANOID_MATERIAL),
geometrydef("rightfoot", "righttoebase", 0.025, HUMANOID_MATERIAL),
geometrydef("leftfoot", "lefttoebase", 0.025, HUMANOID_MATERIAL)

};

struct bonedef
{
char* name;
bool yesno;
float fval;
bonedef(char* n, bool y, float f):name(n), yesno(y), fval(f){};
};

const bonedef hrbones[] = {
	bonedef("pelvis", true, 0.4),
	bonedef("spine3", true, 0.4),
	bonedef("head", true, 0.025),
	bonedef("leftarm", true, 0.07),
	bonedef("leftforearm", true, 0.1),
	bonedef("lefthand", false, 0.f), //tam neni zadnej float?
	bonedef("rightarm", true, 0.07),
	bonedef("rightforearm", true, 0.1),
	bonedef("righthand", false, 0.f), //taky zadnej float
	bonedef("leftupleg", true, 0.2),
	bonedef("leftleg", true, 0.15),
	bonedef("leftfoot", true, 0.05),
	bonedef("lefttoebase", false, 0.f),	//opet bez floatu
	bonedef("rightupleg", true, 0.2),
	bonedef("rightleg", true, 0.15),
	bonedef("rightfoot", true, 0.05),
	bonedef("righttoebase", false, 0.f)	//posledni bez floatu
};

struct hingejointdef
{
Vector3 vec;
int a;
int b;
float f;
hingejointdef(Vector3 v, int ia, int ib, float ff):vec(v), a(ia), b(ib), f(ff){}
};

const hingejointdef hrhingejoint [] = {
hingejointdef(Vector3(1.f, -1.f, 0.f), 5, 110, JOINT_DAMPER),
hingejointdef(Vector3(1.f, 1.f, 0.f), 5, 110, JOINT_DAMPER),
hingejointdef(Vector3(1.f, 0.f, 0.f), -120, 0, JOINT_DAMPER),
hingejointdef(Vector3(1.f, 0.f, 0.f), -120, 0, JOINT_DAMPER)
};

struct balljointdef
{
Vector3 vec;
int a;
int b;
float f;
balljointdef(Vector3 v, int ia, int ib, float ff):vec(v), a(ia), b(ib), f(ff){}
};

const balljointdef hrballjoint [] = {
balljointdef(Vector3(0.f, 1.f, 0.f), 50, 50, JOINT_DAMPER),
//balljointdef(Vector3(0.f, 1.f, 0.f), 30, 40, HEAD_JOINT_DAMPER),
balljointdef(Vector3(1.f, -0.3f, 1.f), 110, 30, JOINT_DAMPER),
balljointdef(Vector3(-1.f, -0.3f, 1.f), 110, 30, JOINT_DAMPER),
balljointdef(Vector3(0.f, -1.f, 1.f), 55, 10, JOINT_DAMPER),
balljointdef(Vector3(0.f, -1.f, 1.f), 55, 10, JOINT_DAMPER)

};

struct addbonedef
{
char* name;
bool yesno;
Vector3 vec;
float fval;
addbonedef(char* n, Vector3 v, bool y, float f = 0.f):name(n), vec(v), fval(f), yesno(y){};
};

const addbonedef hraddbones[] = {
	addbonedef("HeadTerm", Vector3(0.f, 0.f, 6.f), false)
	};

const unsigned int humanRagdolLnCnt = 67;
char* humanRagdol[humanRagdolLnCnt] = {
"ragdoll",
"{", 
"bone", 
"{",
"bone", 
"{",
"balljoint",
"bone", 
//"{",
//"balljoint",
//"addbone",
//"}",
"bone", 
"{",
"balljoint",
"bone",
"{",
"hingejoint", 
"bone", 
"}",
"}",
"bone", 
"{",
"balljoint", 
"bone", 
"{",
"hingejoint", 
"bone", 
"}",
"}",
"}",
"bone", 
"{",
"balljoint", 
"bone", 
"{",
"hingejoint", 
"bone", 
"{",
"fixedjoint",
"bone", 
"}",
"}",
"}",
"bone", 
"{",
"balljoint", 
"bone", 
"{",
"hingejoint", 
"bone", 
"{",
"fixedjoint",
"bone", 
"}",
"}",
"}",
"}",
"geometry",  
"geometry",  
//"geometry",  
"geometry",  
"geometry",  
"geometry",  
"geometry",  
"geometry",  
"geometry",  
"geometry",  
"geometry",  
"geometry",  
"geometry",  
"}"
};
