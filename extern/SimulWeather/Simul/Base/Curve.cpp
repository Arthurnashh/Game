#include "precomp.h"

#include "Simul/Base/Curve.h"
#include "Simul/Base/StringFunctions.h"
#include <math.h>
#include <string.h>   
#include <stdio.h>
#include <cstdlib>
using std::string;
using namespace std;
using namespace simul::base;
static Curve::iterator GetNearestIterator(Curve &C,float r)
{
	if(C.empty())
		return C.end();
	Curve::iterator i1=C.begin();
	if(i1==C.end())
		return i1;
	Curve::iterator i2=i1;
	i2++;
	while(i2->X<r)
	{
		i1++;
		i2++;
		if(i2==C.end())
			return i1;
	}
	float d=(r-i1->X)/(i2->X-i1->X);
	if(d<=0.5f)
		return i1;
	return i2;
}
static Curve::const_iterator GetNearestConstIterator(const Curve &C,float r)
{
	if(C.empty())
		return C.end();
	Curve::const_iterator i1=C.begin();
	if(i1==C.end()||i1->X>r)
		return i1;
	Curve::const_iterator i2=i1;
	i2++;
	while(i2->X<r)
	{
		i1++;
		i2++;
		if(i2==C.end())
			return i1;
	}
	float d=(r-i1->X)/(i2->X-i1->X);
	if(d<=0.5f)
		return i1;
	return i2;
}
static Curve::iterator GetIteratorAtIndex(Curve &C,int index)
{
	if(C.empty())
		return C.end();
	Curve::iterator i=C.begin();
	int n=0;
	for(i=C.begin();i!=C.end();i++,n++)
	{
		if(n==index)
			return i;
	}
	return C.end();
}
Curve::iterator Curve::GetIteratorOfDatum(CurveDatum *D)
{
	if(empty())
		return end();
	Curve::iterator i=begin();
	for(i=begin();i!=end();i++)
		if(&(*i)==D)
			return i;
	return end();
}
Curve::Curve()
{
	XUnitType=PhysicalUnits::None;
	YUnitType=PhysicalUnits::None;
}
Curve::~Curve()
{
	XUnits="";
	YUnits="";
}
void Curve::Clear()
{
	clear();
	XUnits="";
	YUnits="";
}
CurveDatum* Curve::operator[](int n)
{
	iterator i=GetIteratorAtIndex(*this,n);
	return &(*i);
}

float Curve::operator()(float x) const
{
	return ValueAt(x);
}
CurveDatum* Curve::Add()
{
	push_back(CurveDatum());
	CurveDatum &D=back();
	return &D;
}
void Curve::Cut(CurveDatum *D)
{
	iterator i=GetIteratorOfDatum(D);
	if(i!=end())
		erase(i);
}
//------------------------------------------------------------------------------
void Curve::SetCurvePoint(float r,float t,float snap_tolerance_revs)
{
	if(!empty())
	{
		Curve::iterator i=begin();
		for(;i!=end();i++)
		{
			CurveDatum &TD=*i;
			if(fabs(TD.X-r)<snap_tolerance_revs/2.f)
			{
				TD.Y=t;
				return;
			}
		}
		if(front().X>r)
		{
			this->insert(this->begin(),CurveDatum());
			front().X=r;
			front().Y=t;
			return;
		}
		Curve::iterator j=begin();j++;
		for(i=begin();j!=end();i++,j++)
		{
			CurveDatum &TD=*i;
			CurveDatum &TD2=*j;
			if(TD.X<=r&&TD2.X>r)
			{
				Curve::iterator k=insert(j,CurveDatum());
				if(snap_tolerance_revs)
				{
					r/=snap_tolerance_revs;
					r+=.5;
					int ri=(int)r;
					r=(float)ri;
					r*=snap_tolerance_revs;
				}
				k->X=r;
				k->Y=t;
				return;
			}
		}
		push_back(CurveDatum());
		if(snap_tolerance_revs)
		{
			r/=snap_tolerance_revs;
			r+=.5;
			int ri=(int)r;
			r=(float)ri;
			r*=snap_tolerance_revs;
		}
		back().X=r;
		back().Y=t;
		return;
	}
	insert(begin(),CurveDatum());
	if(snap_tolerance_revs)
	{
		r/=snap_tolerance_revs;
		r+=.5;
		int ri=(int)r;
		r=(float)ri;
		r*=snap_tolerance_revs;
	}
	front().X=r;
	front().Y=t;
}

void Curve::operator=(const Curve &C)
{
	vector<CurveDatum>::operator=(C);
	XUnits=C.XUnits;
	YUnits=C.YUnits;
	XUnitType=C.XUnitType;
	YUnitType=C.YUnitType;
}
CurveDatum *Curve::GetPointAtIndex(int n)
{
	iterator i=GetIteratorAtIndex(*this,n);
	if(i!=end())
		return &(*i);
	return NULL;
}
//------------------------------------------------------------------------------
CurveDatum *Curve::GetPoint(float r)
{
	iterator i=GetNearestIterator(*this,r);
	if(i!=end())
		return &(*i);
	return NULL;
}
const CurveDatum *Curve::GetPoint(float r) const
{
	const_iterator i=GetNearestConstIterator(*this,r);
	if(i!=end())
		return &(*i);
	return NULL;
}
//------------------------------------------------------------------------------
void Curve::DeletePoint(float r)
{
	iterator i=GetNearestIterator(*this,r);
	if(i!=end())
		erase(i);
}
//------------------------------------------------------------------------------
float Curve::ValueAt(float r) const
{
	if(empty())
		return 0;
	const_iterator i1=GetNearestConstIterator(*this,r);
	const_iterator i2=i1;
	if(i1->X>r)
	{
		if(i1==begin())
			return i1->Y;
		i1--;
	}
	else
	{
		i2++;
	}
	if(i2==begin())
		return i2->Y;
	if(i2==end())
		return i1->Y;
	float d=(r-i1->X)/(i2->X-i1->X);
	return i2->Y*d+i1->Y*(1-d);
}

void Curve::Save(string &out,const string &Name,const string &Units) const
{
//	size_t pos2=Units.find(",");
	//XUnits=Units.substr(0,pos2);
	//YUnits=Units.substr(pos2+1,Units.length()-pos2-1);
	float X_UnitConv=1.f/PhysicalUnits::RatioToStandard(XUnits);
	float Y_UnitConv=1.f/PhysicalUnits::RatioToStandard(YUnits);
	WriteXMLOpen(out,Name);
	WriteXMLTokenWithSetting(out,"Units",Units);
	char name[100],txt[100];
	int n=0;
	for(const_iterator i=begin();i!=end();i++,n++)
	{
		sprintf(name,"Datum_%d_",n);
		sprintf(txt,"%g,%g",i->X*X_UnitConv,i->Y*Y_UnitConv);
		WriteXMLTokenWithSetting(out,name,txt);
	}
	WriteXMLClose(out,Name);
}

void Curve::Save(string &out,const string &Name) const
{
	char txt[100];
	//const char *Xunit_text=PhysicalUnits::CommonUnits(XUnitType);
	//const char *Yunit_text=PhysicalUnits::CommonUnits(YUnitType);
	sprintf(txt,"%s,%s",XUnits.c_str(),YUnits.c_str());
	string Units=txt;
	Save(out,Name,Units);
}

void Curve::SetProperty(const string &name,const string &prop)
{
	size_t pos2=prop.find(",");
	std::string x=prop.substr(0,pos2);
	std::string y=prop.substr(pos2+1,prop.length()-pos2-1);
	if(name=="Units"&&pos2<prop.length())
	{
		if(XUnits!=x)
			XUnits=x;
		if(YUnits!=y&&pos2<prop.length()-1)
			YUnits=y;
	}
	iterator D=end();
	if(name.substr(0,6)=="Datum_")
	{
		size_t close_bracket=name.find_last_of("_");
		string index=name.substr(6,close_bracket-1);
		if(index.length())
		{
			int idx=atoi(index.c_str());
			D=GetIteratorAtIndex(*this,idx);
			if(D==end())
			{
				push_back(CurveDatum());
				D=end();
				D--;
			}
		}
	}
	else if(name=="Datum")
	{
		push_back(CurveDatum());
		D=end();
		D--;
	}
	if(D==end())
		return;

	float X_UnitConv=PhysicalUnits::RatioToStandard(XUnits);
	float Y_UnitConv=PhysicalUnits::RatioToStandard(YUnits);
	D->X=NextFloat(x)*X_UnitConv;
	D->Y=NextFloat(y)*Y_UnitConv;
}

void Curve::Load(const string &in)
{
	size_t pos=0;
	string name;
	string setting;
	string contents;
	clear();
	while(FindXMLToken(in,pos,name,setting,contents))
	{
		if(setting=="")
			setting=contents;
		SetProperty(name,setting);
	}
}

float Curve::GetMaxX() const
{
	float maxx=0;
	for(const_iterator i=begin();i!=end();i++)
		if(i->X>maxx)
			maxx=i->X;
	return maxx;
}

float Curve::GetMaxY() const
{
	float maxy=0;
	for(const_iterator i=begin();i!=end();i++)
		if(i->Y>maxy)
			maxy=i->Y;
	return maxy;
}

float Curve::FindFirstOf(float y)
{
	const_iterator i1=begin();
	const_iterator i2=begin();
	if(i2==end())
		return 0;
	i2++;
	for(;i2!=end();i1++,i2++)
	{
		// y=y1+(y2-y1)(x-x1)/(x2-x1);
		// y-y1=(y2-y1)(x-x1)/(x2-x1);
		// (x2-x1)(y-y1)/(y2-y1)=x-x1;
		float y1=i1->Y;
		float y2=i2->Y;
		if(y<y1&&y<y2)
			continue;
		if(y>y1&&y>y2)
			continue;
		float x1=i1->X;
		float x2=i2->X;
		if(y2<=y1)
			continue;
		float x=x1+(x2-x1)*(y-y1)/(y2-y1);
		if(x<x1||x>x2)
			continue;
		return x;
	}
	return 0;
}
