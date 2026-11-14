#include "precomp.h"

#include "Simul/Base/StringFunctions.h"
#include "Simul/Base/Units.h"
#include <stdio.h>
#include <stdlib.h>

using namespace simul::base;
namespace simul
{
	namespace base
	{
			#define LB_TO_NEWTONS (4.448f)
			#define FEET_TO_METRES (12.f*2.54f/100.f)
			#define BHP_TO_WATTS (745.7f)
			#define KMH_TO_MS (0.277777778f)
			#define MPH_TO_MS (0.44704f)
			using std::string;
			size_t ttab;
			//------------------------------------------------------------------------------
			std::string ExtractDelimited(std::string &str,char delim)
			{
				string first;
				GetDelimited(str,first,str,delim);
				return first;
			}
			std::string ExtractDelimited(std::string &str,const std::string &delim)
			{
				string first;
				GetDelimited(str,first,str,delim);
				return first;
			}
			std::string ExtractDelimited(std::string &str,char delim,bool match_brackets)
			{
				return ExtractDelimited(str,string(&delim),match_brackets);
			}
			std::string ExtractDelimited(std::string &str,const std::string &delim,bool match_brackets)
			{
				string First;
				if(!str.length())
				{
					First="";
					str="";
					return First;
				}
			//	assert(delim!="<"&&delim!=">");
				size_t pos=0;
				int num_brackets=0;
				while(pos<str.length())
				{
					size_t pos_delim=str.find(delim,pos);
					size_t pos_open=str.find('<',pos);
					size_t pos_close=str.find('>',pos);
					if(num_brackets==0||!match_brackets)
						if(pos_delim<pos_open)
						{
							First=str.substr(0,pos_delim);
							str=str.substr(pos_delim+delim.length(),str.length()-pos_delim-delim.length());
							return First;
						}
					if(pos_delim>=str.length()||(pos_open>=str.length()&&pos_close>=str.length()))
					{
						First=str;
						str="";
						return First;
					}
					if(pos_open<str.length()&&pos_open<pos_close)
					{
						num_brackets++;
						pos=pos_open;
						pos++;
					}
					if(pos_close<str.length()&&pos_close<pos_open)
					{
						num_brackets--;
						pos=pos_close;
						pos++;
					}
				}
				First=str;
				str="";
				return First;
			}
			bool GetDelimited(const string &str,string &First,string &Remainder,const std::string &delim)
			{
				size_t pos=str.find(delim);
				if(!str.length())
				{
					First="";
					Remainder="";
					return false;
				}
				if(pos>=str.length())
				{
					First=Remainder;
					Remainder="";
					return true;
				}
				First=str.substr(0,pos);
				Remainder=str.substr(pos+delim.length(),str.length()-pos-delim.length());
				return true;
			}
			bool GetDelimited(const string &str,string &First,string &Remainder,char delim)
			{
				size_t pos=str.find(delim);
				if(!str.length())
				{
					First="";
					Remainder="";
					return false;
				}
				if(pos>=str.length())
				{
					First=Remainder;
					Remainder="";
					return true;
				}
				First=str.substr(0,pos);
				Remainder=str.substr(pos+1,str.length()-pos-1);
				return true;
			}
			string NextNum(const string &Line,size_t &idx)
			{
				string str="";
				if(Line.length()<1)
					return str;
				if(idx>=Line.length())
					return str;
				char x=Line[idx];
				while((x<'0'||x>'9')&&(x!='.')&&(x!='-'))
				{
					if(idx>=Line.length())
						return str;
					idx++;
					x=Line[idx];
				}
				while((x>='0'&&x<='9')||(x=='.')||(x=='-')||(x=='+')||(x=='e'))
				{
					idx++;
					str+=x;
					if(idx>=(size_t)Line.length())
						x=0;
					else
						x=Line[idx];
				}
				return str;
			}
			void ClipWhitespace(std::string &Line)
			{
				while(Line.length()?Line[0]==' ':false)
					Line=Line.substr(1,Line.length()-1);
				while(Line.length()?Line[Line.length()-1]==' ':false)
					Line=Line.substr(0,Line.length()-1);
			}
			string ExtractNextName(std::string &Line)
			{
				size_t pos=0;
				string name=NextName(Line,pos);
				Line=Line.substr(pos,Line.length()-pos);
				ClipWhitespace(Line);
				return name;
			}
			string NextName(const string &Line)
			{
				size_t idx=0;
				return NextName(Line,idx);
			}
			//------------------------------------------------------------------------------
			string NextName(const string &Line,size_t &idx)
			{
				string str;
				str="";
				if(Line.length()<1)
					return str;
				if(idx>=Line.length())
					return str;
				char x=Line[idx];
				while((x<'a'||x>'z')&&(x<'A'||x>'Z')&&(x<'0'||x>'9')&&x!='_'&&x!='-'&&x!='.')
				{
					idx++;
					if(idx>=(size_t)Line.length())
						return "";
					else
						x=Line[idx];
				}
				while((x>='a'&&x<='z')||(x>='A'&&x<='Z')||(x>='0'&&x<='9')||x=='_'||x=='-'||x=='.')
				{
					str+=x;
					idx++;
					if(idx>=(size_t)Line.length())
						x=0;
					else
						x=Line[idx];
				}
				return str;
			}
			//------------------------------------------------------------------------------
			string NextNum(const string &Line)
			{
				size_t idx=0;
				string str="";
				if(Line.length()<1)
					return str;
				char x=Line[idx];
				while(x&&((x<'0'||x>'9')&&(x!='.')&&(x!='-')&&(x!='e')))
				{
					idx++;
					if(idx>=(size_t)Line.length())
						x=0;
					else
						x=Line[idx];
				}
				while((x>='0'&&x<='9')||(x=='.')||(x=='-')||(x=='e'))
				{
					idx++;
					str+=x;
					if(idx>=(size_t)Line.length())
						x=0;
					else
						x=Line[idx];
				}
				return str;
			}
			//------------------------------------------------------------------------------
			int NextInt(const string &Line)
			{
				string num=NextNum(Line);
				char *stopstring;
				int val=strtol(num.c_str(),&stopstring,10);
				return val;
			}
			//------------------------------------------------------------------------------
			bool NextBool(const string &Line)
			{
				if(Line.find("true")<Line.length())
					return true;
				if(Line.find("false")<Line.length())
					return false;
				string num=NextNum(Line);
				char *stopstring;
				size_t val=strtol(num.c_str(),&stopstring,10);
				return val!=0;
			}
			//------------------------------------------------------------------------------
			int NextInt(const string &Line,size_t &idx)
			{
				string num=NextNum(Line,idx);
				char *stopstring;
				int val=strtol(num.c_str(),&stopstring,10);
				return val;
			}
			//------------------------------------------------------------------------------
			const float *NextVector2(const string &Line)
			{
				static float v[3];
				size_t pos=0;
				string num1=NextNum(Line,pos);
				string num2=NextNum(Line,pos);
				char *stopstring;
				v[0]=(float)strtod(num1.c_str(),&stopstring);
				v[1]=(float)strtod(num2.c_str(),&stopstring);
				v[2]=0;
				return v;
			}
			//------------------------------------------------------------------------------
			const float *NextVector3(const string &Line)
			{
				static float v[4];
				size_t pos=0;
				string num1=NextNum(Line,pos);
				string num2=NextNum(Line,pos);
				string num3=NextNum(Line,pos);
				char *stopstring;
				v[0]=(float)strtod(num1.c_str(),&stopstring);
				v[1]=(float)strtod(num2.c_str(),&stopstring);
				v[2]=(float)strtod(num3.c_str(),&stopstring);
				v[3]=0;
				return v;
			}
			//------------------------------------------------------------------------------
			float NextFloat(const string &Line)
			{
				string num=NextNum(Line);
				char *stopstring;
				float val=(float)strtod(num.c_str(),&stopstring);
				return val;
			}
			//------------------------------------------------------------------------------
			float NextFloat(const string &Line,size_t &idx)
			{
				string num=NextNum(Line,idx);
				char *stopstring;
				float val=(float)strtod(num.c_str(),&stopstring);
				return val;
			}
			//------------------------------------------------------------------------------
			float NextTorque(const string &Line,size_t &idx)
			{
				float val=NextFloat(Line,idx);
				size_t pos=Line.find("lbft",idx);
				if(pos<Line.length())
					val*=LB_TO_NEWTONS*FEET_TO_METRES;
				return val;
			}
			//------------------------------------------------------------------------------
			float AsPhysicalValue(const string &Line)
			{
				float val=NextFloat(Line);
				float UnitConv=PhysicalUnits::RatioToStandard(Line);
				return val*UnitConv;
			}
			//------------------------------------------------------------------------------
			float NextPower(const string &Line,size_t &idx)
			{
				float val=NextFloat(Line,idx);
				size_t pos=Line.find("bhp",idx);
				if(pos<=Line.length())
					val*=BHP_TO_WATTS;
				return val;
			}
			//------------------------------------------------------------------------------
			float NextSpeed(const string &Line,size_t &idx)
			{
				float val=NextFloat(Line,idx);
				size_t pos=Line.find("km/h",idx);
				if(pos<=Line.length())
					val*=KMH_TO_MS;
				pos=Line.find("mph",idx);
				if(pos<=Line.length())
					val*=MPH_TO_MS;
				return val;
			}
			//------------------------------------------------------------------------------
			float NextTorque(const string &Line)
			{
				size_t idx=0;
				return NextTorque(Line,idx);
			}
			//------------------------------------------------------------------------------
			float NextPower(const string &Line)
			{
				size_t idx=0;
				return NextPower(Line,idx);
			}
			//------------------------------------------------------------------------------
			bool NextSettingFromTag(string &tag,string &Name,string &Value)
			{
				size_t equalpos=tag.find("=");
				if(equalpos>=tag.length())
					return false;
				size_t quotepos=tag.find("\"");
				size_t quotepos2=tag.find("\"",quotepos+1);
				if(quotepos>=tag.length())
					return false;
				if(quotepos2>=tag.length())
					return false;
				Name=tag.substr(0,equalpos);
				Value=tag.substr(quotepos+1,quotepos2-quotepos-1);
				tag=tag.substr(quotepos2+1,tag.length()-quotepos2-1);
				return true;
			}
			//------------------------------------------------------------------------------
			void GetXMLSetting(string &setting,string &Value,const string &Name)
			{
				size_t pos=setting.find(Name);
				if(pos>=setting.length())
				{
					Value="";
					return;
				}
				size_t equalpos=setting.find("=",pos);
				if(equalpos!=pos+Name.length())
				{
					Value="";
					return;
				}
				size_t openquote=setting.find("\"",equalpos);
				size_t closequote=setting.find("\"",openquote+1);
				Value=setting.substr(openquote+1,closequote-openquote-1);
			}
			//------------------------------------------------------------------------------
			void StartXMLWrite(string &)
			{
				ttab=0;
			}
			//------------------------------------------------------------------------------
			bool FindXMLToken(const string &in,size_t &pos0)
			{
				size_t openpos=in.find("<",pos0);
				if(openpos>=in.length())
					return false;
				size_t pos2=in.find(">",openpos+1);
				pos0=pos2+1;
				return true;
			}
			//------------------------------------------------------------------------------
			bool FindXMLToken(const string &in,size_t &pos0,string &name,string &setting,string &contents)
			{
				size_t openpos=in.find("<",pos0);
				if(openpos>=in.length())
					return false;
				size_t pos2=in.find(">",openpos+1);
				string tag=in.substr(openpos+1,pos2-openpos-1);
				size_t pos=0;
				name=NextName(tag,pos);
				//size_t equalpos=tag.find("=");
				if(pos<(size_t)tag.length()-1)
					setting=tag.substr(pos+1,tag.length()-pos-1);
				else
					setting="";
				if(name.length()==0)
					return false;
				char tt=tag[0];
				if(tt=='\\'||tt=='?'||tt=='!'||(tag[tag.length()-1]=='/'))
				{
					contents="";
					pos0=pos2+1;
					return true;
				}
				string closing=string("</")+name+string(">");
				size_t closepos=in.find(closing,pos0);
				if(closepos>=in.length())
					return false;
				contents=in.substr(pos2+1,closepos-pos2-1);
				pos0=closepos+closing.length();
				return true;
			}
			//------------------------------------------------------------------------------
			void WriteXMLOpen(string &out,const string &name,const string &setting)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				out+="<"+name;
				if(setting.length())
					out+=" "+setting;
				out+=">\r\n";
				ttab++;
			}
			//------------------------------------------------------------------------------
			void WriteXMLOpen(string &out,const string &name)
			{
				WriteXMLOpen(out,name,"");
			}
			//------------------------------------------------------------------------------
			void WriteXMLClose(string &out,const string &name)
			{
				ttab--;
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				out+="</"+name;
				out+=">\r\n";
			}
			//------------------------------------------------------------------------------
			void WriteXMLTokenWithSetting(string &out,const string &name,const string &setting)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				out+="<"+name+">";
				out+=setting;
				out+="</"+name+">";
				out+="\r\n";
			}
			//------------------------------------------------------------------------------
			void WriteXMLToken(string &out,const string &name)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				out+="<"+name;
				out+="/>\r\n";
			}
			//------------------------------------------------------------------------------
			void WriteFloat(string &out,float setting)
			{
				char txt[100];
				sprintf(txt,"%0.6g",setting);
				out=txt;
			}
			//------------------------------------------------------------------------------
			void WriteInt(string &out,size_t setting)
			{
				char txt[100];
				sprintf(txt,"%d",setting);
				out=txt;
			}
			//------------------------------------------------------------------------------
			void WriteXMLFloat(string &out,const string &name,float setting)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				string str;
				WriteFloat(str,setting);
				WriteXMLTokenWithSetting(out,name,str);
			}
			//------------------------------------------------------------------------------
			void WriteXMLInt(string &out,const string &name,int setting)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				string str=stringof(setting);
				WriteXMLTokenWithSetting(out,name,str);
			}
			//------------------------------------------------------------------------------
			void WriteXMLBoolean(string &out,const string &name,bool setting)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				string str="false";
				if(setting)
					str="true";
				WriteXMLTokenWithSetting(out,name,str);
			}

			void WriteXMLVector(std::string &out,const std::string &name,const float *v)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				out+="<"+name+">";
				string s1,s2,s3;
				WriteFloat(s1,v[0]);
				WriteFloat(s2,v[1]);
				WriteFloat(s3,v[2]);
				out+=s1+","+s2+","+s3;
				out+="</"+name+">";
				out+="\r\n";
			}

			//------------------------------------------------------------------------------
			void WriteXMLTokenIfNonZero(string &out,const string &name,float setting)
			{
				if(!setting)
					return;
				WriteXMLFloat(out,name,setting);
			}
			void Tab(string &out)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
			}
			//------------------------------------------------------------------------------
			void WriteXMLFloatInUnits(string &out,const string &name,float setting,const string &Units)
			{
				float UnitConv=PhysicalUnits::RatioToStandard(Units);
				Tab(out);
				char txt[100];
				setting/=UnitConv;
				sprintf(txt,"%0.6g ",setting);
				string str(txt);
				str+=Units;
				WriteXMLTokenWithSetting(out,name,str);
			}
			//------------------------------------------------------------------------------
			void WriteXMLTorque(string &out,const string &name,float setting)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				char txt[100];
				setting/=(LB_TO_NEWTONS*FEET_TO_METRES);
				sprintf(txt,"%0.6g lbft",setting);
				string str(txt);
				WriteXMLTokenWithSetting(out,name,str);
			}
			//------------------------------------------------------------------------------
			void WriteXMLPower(string &out,const string &name,float setting)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				char txt[100];
				setting/=BHP_TO_WATTS;
				sprintf(txt,"%0.6g bhp",setting);
				string str(txt);
				WriteXMLTokenWithSetting(out,name,str);
			}
			//------------------------------------------------------------------------------
			void WriteXMLVector2(string &out,const string &name,float s1,float s2)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				char txt[100];
				sprintf(txt,"%0.6g,%0.6g",s1,s2);
				string str(txt);
				WriteXMLTokenWithSetting(out,name,str);
			}
			//------------------------------------------------------------------------------
			void WriteXMLVector3(string &out,const string &name,float s1,float s2,float s3)
			{
				for(size_t i=0;i<ttab;i++)
					out+="\t";
				char txt[100];
				sprintf(txt,"%0.6g,%0.6g,%0.6g",s1,s2,s3);
				string str(txt);
				WriteXMLTokenWithSetting(out,name,str);
			}
			size_t GoToLine(const string &Str,size_t line)
			{
				size_t pos=0;
				char txt[100];
				sprintf(txt,"\r\n");
				string CR;
				for(size_t i=0;i<line;i++)
				{
					pos=Str.find(13,pos);
					pos++;
				}
				//pos++;
				return pos;
			}
			// for streaming
			using std::string;
			string MakeXMLOpen(const string &name)
			{
				string str="";
				for(size_t i=0;i<ttab;i++)
					str+="\t";
				str+="<"+name;
				str+=">\r";
				ttab++;
				return str;
			}
			string MakeXMLClose(const string &name)
			{
				string str="";
				ttab--;
				for(size_t i=0;i<ttab;i++)
					str+="\t";
				str+="</"+name;
				str+=">\r";
				return str;
			}
			string MakeXMLToken(const string &name,const string &value)
			{
				string str="";
				for(size_t i=0;i<ttab;i++)
					str+="\t";
				str+="<"+name+">";
				str+=value;
				str+="</"+name+">";
				str+="\r";
				return str;
			}
			string MakeXMLToken(const string &name,int value)
			{
				string str="";
				WriteInt(str,value);
				return MakeXMLToken(name,str);
			}
			string MakeXMLToken(const string &name,float value)
			{
				string str="";
				WriteFloat(str,value);
				return MakeXMLToken(name,str);
			}
			string stringof(int i)
			{
				char txt[100];
				sprintf(txt,"%d",i);
				return string(txt);
			}
			string stringof(float f)
			{
				char txt[100];
				sprintf(txt,"%g",f);
				return string(txt);
			}
			string StringOf(int i)
			{
				char txt[100];
				sprintf(txt,"%d",i);
				return string(txt);
			}
			string StringOf(float i)
			{
				char txt[100];
				sprintf(txt,"%g",i);
				return string(txt);
			}
void StripDirectoryFromFilename(string &str)
{
	int pos=(int)str.find("\\");
	do
	{
		str=str.substr(pos+1,str.length()-pos);
		pos=(int)str.find("\\");
	} while(pos>=0);
}
	}
}
