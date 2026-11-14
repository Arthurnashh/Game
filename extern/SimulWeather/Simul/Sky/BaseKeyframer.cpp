#include "precomp.h"

#include "Simul/Sky/BaseKeyframer.h"
#ifndef _MSC_VER
#include <stdio.h>
#include <strings.h>
#define _stricmp strcasecmp
#endif
#include <math.h>
using namespace simul;
using namespace sky;

static const unsigned MAGIC=0xA1903874;
std::ostream &BaseKeyframer::Save(std::ostream &os) const
{
	os.write((const char*)&MAGIC,sizeof(MAGIC));
	int kf=(int)keyframes.size();
	os.write((const char*)&kf,sizeof(kf));
	for(size_t i=0;i<keyframes.size();i++)
	{
		keyframes[i]->Save(os);
	}
	return os;
}

std::istream &BaseKeyframer::Load(std::istream &is)
{
	int num_kf;
	if(!is.good())
		return is;
	for(int i=0;i<3;i++)
		texture_complete[i]=false;
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=MAGIC)
	{
#ifndef XENON
		std::cerr<<"BaseKeyframer::Load - not found magic number in file - possibly out of date file."<<std::endl;
#endif
		return is;
	}

	is.read((char*)&num_kf,sizeof(num_kf));
	for(size_t i=0;i<keyframes.size();i++)
	{
		delete keyframes[i];
	}
	keyframes.clear();
	int kf=current_keyframe;
	current_keyframe=-100000;
	for(size_t i=0;i<(size_t)num_kf;i++)
	{
		InsertKeyframe((float)i/(float)num_kf)->Load(is);
	}
	current_keyframe=kf;
	SortKeyframes();
	this->KeyframesChanged();
	return is;
}
static const unsigned BASE_KEYFRAME_MAGIC=0xA9B133F1;

std::ostream &BaseKeyframe::Save(std::ostream &os) const
{
	os.write((const char*)&BASE_KEYFRAME_MAGIC,sizeof(BASE_KEYFRAME_MAGIC));
	os.write((const char*)&time,sizeof(time));
	return os;
}

std::istream &BaseKeyframe::Load(std::istream &is)
{
	unsigned magic;
	is.read((char*)&magic,sizeof(magic));
	if(magic!=BASE_KEYFRAME_MAGIC)
	{
#ifndef XENON
		std::cerr<<"ColourSkyKeyframer::ColourKeyframe::Load - not found magic number in file - possibly out of date file."<<std::endl;
#endif
		return is;
	}
	is.read((char*)&time,sizeof(time));
	return is;
}

bool BaseKeyframe::HasValue(const char *name) const
{
	return(_stricmp(name,"time")==0);
}

float BaseKeyframe::GetValue(const char *name) const
{
	if(_stricmp(name,"time")==0)
		return time;
	return 0.f;
}

void BaseKeyframe::SetValue(const char *name,float val)
{
	if(_stricmp(name,"time")==0)
		time=val;
}

BaseKeyframer::BaseKeyframer():		keyframe1(0xFFFFFFFF)
									,keyframe2(0xFFFFFFFF)
									,current_keyframe(0)
									,DayLoop(true)
									,LoopRange(1.f)
{

}

BaseKeyframe *BaseKeyframer::GetKeyframe(int i)
{
	if(!keyframes.size())
		return NULL;
	if(i>=(int)keyframes.size())
		i=i%(int)keyframes.size();
	while(i<0)
		i+=(int)keyframes.size();
	return keyframes[i];
}

const BaseKeyframe *BaseKeyframer::GetKeyframe(int i) const
{
	if(!keyframes.size())
		return NULL;
	if(i>=(int)keyframes.size())
		i=i%(int)keyframes.size();
	while(i<0)
		i+=(int)keyframes.size();
	return keyframes[i];
}

void BaseKeyframer::DeleteKeyframe(int i)
{
	if(i<(int)keyframes.size()&&i>=0)
	{
		delete keyframes[i];
		keyframes.erase(keyframes.begin()+i);
	}
}

void BaseKeyframer::ClearKeyframes()
{
	for(size_t i=0;i<keyframes.size();i++)
	{
		delete keyframes[i];
	}
	keyframes.clear();
}

int BaseKeyframer::GetNumKeyframes() const
{
	return (int)keyframes.size();
}

float BaseKeyframer::GetKeyframeTime(int i) const
{
	if(!keyframes.size())
		return 0.f;
	int d=0;
	if(i>=(int)keyframes.size())
	{
		if(DayLoop)
		{
			d=i/(int)keyframes.size();
			i=i%(int)keyframes.size();
		}
		else
			i=(int)keyframes.size()-1;
	}
	if(i<0)
	{
		d=0;
		while(i<0)
		{
			if(DayLoop)
			{
				i+=(int)keyframes.size();
				d--;
			}
			else
			{
				i=0;
			}
		}
	}
	return (float)d*LoopRange+keyframes[i]->time;
}

float BaseKeyframer::GetInterp(int k,float t) const
{
	float last_t=GetKeyframeTime(k);
	float next_t=GetKeyframeTime(k+1);
	if(!keyframes.size())
		return 0;
	if(keyframes.size()==1)
	{
		return 0;
	}
	float time_step=next_t-last_t;
	if(time_step<=0)
	{
		time_step+=LoopRange;
	}
	float interp=(t-last_t)/time_step;
	if(DayLoop)
	{
		interp=(t-last_t)/time_step;
	}
	return interp;
}

int BaseKeyframer::GetKeyframeAtTime(float t) const
{
	int d=0;
	if(DayLoop)
	{
		d=(int)(t/LoopRange);
		if(t<0)
			d--;
		d*=keyframes.size();
		t=FractionalPart(t,LoopRange);
	}
	for(int i=0;i<(int)keyframes.size();i++)
	{
		if(t<keyframes[i]->time)
			return d+(i-1);
	}
	return d+(int)keyframes.size()-1;
}

void BaseKeyframer::SortKeyframes()
{
	bool out_of_place=true;
	while(out_of_place)
	{
		out_of_place=false;
		for(int i=0;i<(int)keyframes.size()-1;i++)
		{
			BaseKeyframe *&K1=keyframes[i];
			BaseKeyframe *&K2=keyframes[i+1];
			if(K1->time>K2->time)
			{
				std::swap(K1,K2);
				out_of_place=true;
			}
		}
	}
	sorted=true;
}

bool BaseKeyframer::ChecksumsFailed(float )
{
	int k1=current_keyframe;
	int k2=k1+1;
	int k3=k1+2;
	BaseKeyframe *K1=GetKeyframe(k1);
	BaseKeyframe *K2=GetKeyframe(k2);
	BaseKeyframe *K3=GetKeyframe(k3);
	unsigned new1=0,new2=0,new3=0;
	unsigned glob=GetGlobalChecksum();
	if(K1)
		new1=glob+K1->GetPropertiesChecksum();
	if(K2)
		new2=glob+K2->GetPropertiesChecksum();
	if(K3)
		new3=glob+K3->GetPropertiesChecksum();
	bool result=false;
	if(keyframe1==k1)
	{
		if(K1&&K1->checksum!=new1)
		{
			texture_complete[0]=false;
			result=true;
		}
		if(K2&&K2->checksum!=new2)
		{
			texture_complete[1]=false;
			result=true;
		}
		if(K3&&K3->checksum!=new3)
		{
			texture_complete[2]=false;
			result=true;
			RestartNextKeyframe();
		}
	}
	keyframe1=k1;
	keyframe2=k2;
	keyframe3=k3;
	if(K1)
		K1->checksum=new1;
	if(K2)
		K2->checksum=new2;
	if(K3)
		K3->checksum=new3;
	return result;
}
