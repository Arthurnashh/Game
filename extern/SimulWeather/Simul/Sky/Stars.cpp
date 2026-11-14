#include "precomp.h"

#include "Stars.h"
#include "DefaultStars.h"
#include <stdio.h>
#include <fstream>
using namespace simul;
using namespace sky;

Stars::Stars()
{
}

Stars::~Stars()
{
}

static double StringToRadians(const std::string &str,double scale1)
{
	size_t pos1=str.find(' ');
	std::string deg_str=str.substr(0,pos1);
	double deg=atof(deg_str.c_str());

	size_t pos2=str.find(' ',pos1+1);
	std::string minute_str=str.substr(pos1+1,pos2-pos1-1);
	double minute=atof(minute_str.c_str());
	size_t pos3=str.length();
	std::string seconds_str=str.substr(pos2+1,pos3-pos2-1);
	double seconds=atof(seconds_str.c_str());

	double radians=3.1415926536/scale1*deg;
	radians+=3.1415926536*minute/scale1/60.0;
	radians+=3.1415926536*seconds/scale1/60.0/60.0;
	return radians;
}
static double RightAscensionToRadians(const std::string &str)
{
	return StringToRadians(str,12.0);
}
static double DeclinationToRadians(const std::string &str)
{
	return StringToRadians(str,180.0);
}
static double StringToDouble(const std::string &str)
{
	double d=atof(str.c_str());
	return d;
}

void Stars::UseDefaultStars(float max_magnitude)
{
	Star *star=default_stars;
	int num_stars=0;
	while(star->ascension!=0.0)
	{
		star++;
		if(star->magnitude<=max_magnitude)
			num_stars++;
	};

	stars.resize(num_stars);
	star=default_stars;
	int i=0;
	while(star->ascension!=0.0)
	{
		if(star->magnitude<=max_magnitude)
		{
			stars[i].ascension=star->ascension;
			stars[i].declination=star->declination;
			stars[i].magnitude=star->magnitude;
			i++;
		}
		star++;
	};
}

void Stars::LoadAsText(const char *filename)
{
	char str[255];
		int numlines=0;
	{
		std::ifstream ifs(filename);
		if(!ifs.good())
			return;
		while(!ifs.eof())
		{
			ifs.getline(str,255);
			if(str[0]!='|')
				continue;
			numlines++;
		}
	}
	stars.resize(numlines);
	{
		std::ifstream ifs(filename);
		int star=0;
		if(!ifs.good())
			return;
		while(!ifs.eof())
		{
			ifs.getline(str,255);
			if(str[0]!='|')
				continue;
			std::string line(str);
			size_t pos=0;
			pos=line.find('|',pos+1);
			pos=line.find('|',pos+1);
			size_t next_pos=0;
			next_pos=line.find('|',pos+1);
			std::string ra_str=line.substr(pos+1,next_pos-pos-1);
			pos=next_pos;
			next_pos=line.find('|',pos+1);
			std::string dec_str=line.substr(pos+1,next_pos-pos-1);
			pos=next_pos;
			next_pos=line.find('|',pos+1);
			std::string mag_str=line.substr(pos+1,next_pos-pos-1);
			double ra=RightAscensionToRadians(ra_str);
			double dec=DeclinationToRadians(dec_str);
			double mag=StringToDouble(mag_str);
			stars[star].declination=dec;
			stars[star].ascension=ra;
			stars[star].magnitude=mag;
			star++;
		}
	}
}

static std::string StringOf(double value)
{
	char txt[30];
	sprintf(txt,"%15g",value);
	std::string str=txt;
	return str;
}

void Stars::SaveAsText(const char *filename)
{
	std::ofstream ofs(filename);
	for(size_t i=0;i<stars.size();i++)
	{
		std::string line="{";
		line+=StringOf(stars[i].ascension);
		line+=",";
		line+=StringOf(stars[i].declination);
		line+=",";
		line+=StringOf(stars[i].magnitude);
		line+="},";
		ofs<<line.c_str()<<"\n";
	}
}

void Stars::Load(const char *)
{
}

void Stars::Save(const char *)
{
}

int Stars::GetNumStars() const
{
	return (int)stars.size();
}

const Star &Stars::GetStar(int i) const
{
	return stars[i];
}