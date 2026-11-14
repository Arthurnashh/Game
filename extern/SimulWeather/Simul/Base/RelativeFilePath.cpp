#include "precomp.h"

#include "Simul/Base/RelativeFilePath.h"
#include <queue>
#include <stack>
#include <stdio.h>
#include <string.h>
using namespace std;
typedef queue<char*> QueuePtrChar;
typedef stack<char*> StackPtrChar;
#ifndef MAX_PATH
#define MAX_PATH 500
#endif
#if defined(_MSC_VER)
	#define path_separator "\\"
	#define path_sep_char  '\\'
#elif defined(__GNUC__)
	#define path_separator "/"
	#define path_sep_char  '/'
#elif defined(__BORLANDC__)
	#define path_separator "\\"
	#define path_sep_char  '\\'   
	#define _stricmp stricmp
#else
	#error define your compiler
#endif

std::string simul::base::Abs2Rel(const std::string &pcAbsPath,const std::string &pcCurrDir)
{
	char pcRelPath[MAX_PATH];
	char acTmpCurrDir[MAX_PATH];
	char acTmpAbsPath[MAX_PATH];
	strcpy(acTmpCurrDir,pcCurrDir.c_str());
	strcpy(acTmpAbsPath,pcAbsPath.c_str());
	
	StackPtrChar tmpStackAbsPath;
	StackPtrChar tmpStackCurrPath;
	StackPtrChar tmpStackOutput;
	QueuePtrChar tmpMatchQueue;
	
	char *sTmp = strtok(acTmpAbsPath,path_separator);
	while(sTmp)
	{
		tmpStackAbsPath.push(sTmp);
		sTmp = strtok(0, path_separator);
	}

	sTmp = strtok(acTmpCurrDir,path_separator);
	while(sTmp)
	{
		tmpStackCurrPath.push(sTmp);
		sTmp = strtok(0, path_separator);
	}

	sTmp = pcRelPath;
	while(tmpStackCurrPath.size() > tmpStackAbsPath.size() )
	{
		*sTmp++ = '.';
		*sTmp++ = '.';
		*sTmp++ = path_sep_char;
		tmpStackCurrPath.pop();
	}

	while(tmpStackAbsPath.size() > tmpStackCurrPath.size() )
	{
		char *pcTmp = tmpStackAbsPath.top();
		tmpStackOutput.push(pcTmp);
		tmpStackAbsPath.pop();
	}

	while(tmpStackAbsPath.size() > 0)
	{
#if !defined(SN_TARGET_PS3) && !defined(__GNUC__)
		if(_stricmp(tmpStackAbsPath.top(),tmpStackCurrPath.top())== 0  )
#else
		if(strcmp(tmpStackAbsPath.top(),tmpStackCurrPath.top())== 0  )
#endif
			tmpMatchQueue.push(tmpStackAbsPath.top());
		else
		{
			while(tmpMatchQueue.size() > 0)
			{
				tmpStackOutput.push(tmpMatchQueue.front());
				tmpMatchQueue.pop();
			}
			tmpStackOutput.push(tmpStackAbsPath.top());
			*sTmp++ = '.';
			*sTmp++ = '.';
			*sTmp++ = path_sep_char;
		}
		tmpStackAbsPath.pop();
		tmpStackCurrPath.pop();	
	}
	while(tmpStackOutput.size() > 0)
	{
		char *pcTmp= tmpStackOutput.top();
		while(*pcTmp != '\0')	
			*sTmp++ = *pcTmp++;
		tmpStackOutput.pop();
		*sTmp++ = path_sep_char;
	}
	if(sTmp!=pcRelPath)
		*(--sTmp) = '\0';
	else return "";


	return std::string(pcRelPath);
}
// From one relative path, make another relative to a different directory.
std::string simul::base::Rel2Rel(const std::string &pcFirstRelPath,const std::string &pcRelCurrDir)
{
	// count the depth of each path:
	string fakeCurrDir;
	int pos=0;

	int depth1=0;
	while((pos=(int)pcFirstRelPath.find(path_separator,++pos))>=0)
		depth1++;

	int depth2=0;
	while((pos=(int)pcRelCurrDir.find(path_separator,++pos))>=0)
		depth2++;

	int depth=std::max(depth1,depth2);

	for(int i=0;i<depth;i++)
		fakeCurrDir+="fake\\";

	string pcAbsPath=Rel2Abs(pcFirstRelPath,fakeCurrDir);
	string pcCurrDir=Rel2Abs(pcRelCurrDir,fakeCurrDir);

	return Abs2Rel(pcAbsPath,pcCurrDir);
}

std::string simul::base::Rel2Abs(const std::string &pcRelPath,const std::string &pcCurrDir)
{
	char acTmpCurrDir[MAX_PATH];
	char acTmpRelPath[MAX_PATH];
	strcpy(acTmpCurrDir,pcCurrDir.c_str());
	strcpy(acTmpRelPath,pcRelPath.c_str());
	
	QueuePtrChar tmpQueueRelPath;
	StackPtrChar tmpStackCurrPath;
	StackPtrChar tmpStackOutPath;
	
	char *sTmp = strtok(acTmpRelPath,path_separator);
	while(sTmp)
	{
		tmpQueueRelPath.push(sTmp);
		sTmp = strtok(0, path_separator);
	}

	sTmp = strtok(acTmpCurrDir,path_separator);
	while(sTmp)
	{
		tmpStackCurrPath.push(sTmp);
		sTmp = strtok(0, path_separator);
	}


	while(tmpQueueRelPath.size() > 0)
	{
		char *pcTmp= tmpQueueRelPath.front();
		if(strcmp(pcTmp, "..") == 0)
			tmpStackCurrPath.pop();
		else
			tmpStackCurrPath.push(pcTmp);
		tmpQueueRelPath.pop();
	}
	while(tmpStackCurrPath.size() > 0)
	{
		tmpStackOutPath.push(tmpStackCurrPath.top());
		tmpStackCurrPath.pop();
	}


	char pcAbsPath[MAX_PATH];
	sTmp = pcAbsPath;
#if defined(__GNUC__)
	*sTmp++ = path_sep_char;
#endif
	while(tmpStackOutPath.size() > 0)
	{
		char *pcTmp= tmpStackOutPath.top();
		while(*pcTmp != '\0')	
			*sTmp++ = *pcTmp++;
		tmpStackOutPath.pop();
		*sTmp++ = path_sep_char;
	}
	*(--sTmp) = '\0';

	return pcAbsPath; 	
}
