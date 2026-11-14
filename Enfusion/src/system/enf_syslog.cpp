//-----------------------------------------------------------------------------
// File: enf_syslog.h
//
// Desc: output log class
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "system/enf_time.h"
#include "system/enf_vfilesystem.h"
#include <stdio.h>

namespace enf
{
//-----------------------------------------------------------------------
	TLog::TLog(): LogFile(NULL), m_iFlags(0)
{
   Name[0] = 0;
}

//-----------------------------------------------------------------------
TLog::~TLog()
{
	if(LogFile)
		fclose((FILE*)LogFile);
}

//-----------------------------------------------------------------------
void TLog::TimeStamp()
{
SDateTime tim;
CMutexLock lock(m_Mutex);

	if(Open())
	{
		tim.SetCurrent();
		char tmpBuffer[256];
		sprintf(tmpBuffer,"%02d.%02d. %02d:%02d:%02d",
				(int)tim.Day,
				(int)tim.Month,
				(int)tim.Hour,
				(int)tim.Minute,
				(int)tim.Second);

		fwrite(tmpBuffer, 1, strlen(tmpBuffer), (FILE*)LogFile);
		fflush((FILE*)LogFile);
	}
}

//-----------------------------------------------------------------------
void TLog::Open(const char* name, uint flags)
{
   strcpy(Name, name);
	m_iFlags |= flags;
}

//-----------------------------------------------------------------------
bool TLog::IsOpen() const
{
	if(Name[0] != 0)
		return true;
	return false;
}
//-----------------------------------------------------------------------
TLog::TLog(const char* name, uint flags): LogFile(NULL), m_iFlags(flags & ~LOGF_INITIALIZED)
{
	Open(name, flags);
	strcpy(Name, name);
}

//-----------------------------------------------------------------------
int TLog::Open()
{
	if((m_iFlags & LOGF_INITIALIZED) == 0)
	{
		m_iFlags |= LOGF_INITIALIZED;

		#ifdef ENF_PS3
			#ifdef ENF_DEVELOPER
				SString<> temp;
				temp = "/dev_hdd0/game_debug/";
				temp += Name;
				enf::strcpy(Name, temp.cstr());
			#else
				return false;
			#endif
		#else
			if(IsFileSystem())
			{
				SString<> temp;

				#ifdef ENF_XENON
					#ifdef ENF_DEVELOPER
						temp = "devkit:\\";
						temp += Name;
						enf::strcpy(Name, temp.cstr());
					#else
						return false;
					#endif
				#else
					temp = "$profile:";
					temp += Name;

					GetFileSystem()->GetAbsoluteName(temp.cstr(), Name, false);
				#endif
			}
			else
			{
				//do not log until FS exists
				return false;
			}
		#endif

		if(m_iFlags & LOGF_CREATE)
		{
			LogFile = fopen(Name, "w");
		}
		else
		{
			LogFile = fopen(Name, "a+");
		}


		Printf("---------------------------------------------\n");
		Printf("Log %s started at ", Name);
		TimeStamp();
		Printf("\n\n");
	}

	if(LogFile == NULL)
	{
		if(Name[0] == 0)
			return false;

		LogFile = fopen(Name, "a");
	}
	
   return (LogFile != NULL);
}

//-----------------------------------------------------------------------
void TLog::Close()
{
	if(LogFile)
	{
		fclose((FILE*)LogFile);
		LogFile = NULL;
	}
}

//-----------------------------------------------------------------------
void TLog::Printf(const char *text,...)
{
va_list argptr;
CMutexLock lock(m_Mutex);

	if(Open())
	{
		char tmpBuffer[1024];

		va_start (argptr,text);
		vsprintf (tmpBuffer,text,argptr);
		va_end (argptr);
		fputs(tmpBuffer, (FILE*)LogFile);
		fflush((FILE*)LogFile);		
	}
}

//-----------------------------------------------------------------------
void TLog::TSPrintf(const char *text,...)
{
va_list argptr;
CMutexLock lock(m_Mutex);

	if(Open())
	{
		char tmpBuffer[1024];

		SDateTime tim;
		tim.SetCurrent();
		sprintf(tmpBuffer,"%02d:%02d:%02d:%04d",(int)tim.Hour,
											(int)tim.Minute,
												(int)tim.Second,
												(int)tim.Milliseconds);

		fwrite(tmpBuffer, 1, strlen(tmpBuffer), (FILE*)LogFile);

		va_start (argptr,text);
		vsprintf (tmpBuffer,text,argptr);
		va_end (argptr);

		fputs(tmpBuffer, (FILE*)LogFile);
		fflush((FILE*)LogFile);		
	}
}

//-----------------------------------------------------------------------
void TLog::operator<<(const char *text)
{
	CMutexLock lock(m_Mutex);
	if(Open())
	{
		fputs(text, (FILE*)LogFile);
		fflush((FILE*)LogFile);		
	}
}

//-----------------------------------------------------------------------
void TLog::operator<<(int inum)
{
	CMutexLock lock(m_Mutex);
	if(Open())
	{
		char tmpBuffer[16];
		sprintf(tmpBuffer, "%d", inum);
		fputs(tmpBuffer, (FILE*)LogFile);
		fflush((FILE*)LogFile);		
	}
}

//-----------------------------------------------------------------------
void TLog::operator<<(float fnum)
{
	CMutexLock lock(m_Mutex);
	if(Open())
	{
		char tmpBuffer[16];
		sprintf(tmpBuffer, "%f", fnum);
		fputs(tmpBuffer, (FILE*)LogFile);
		fflush((FILE*)LogFile);		
	}
}

//-----------------------------------------------------------------------
void TLog::operator<<(char chr)
{
	CMutexLock lock(m_Mutex);
	if(Open())
	{
		fputc(chr, (FILE*)LogFile);
		fflush((FILE*)LogFile);		
	}
}

//-----------------------------------------------------------------------
void TLog::Dump(const void* data, int num, int size, int row, uint flags)
{
	CMutexLock lock(m_Mutex);
	/*
	if(Open())
   {
   	char *d;
   	int n, r;

      d = (char *)data;
      for(r = n = 0; n < num; n++)
      {
         if(r == row)
         {
            fputc('\n',LogFile);
            r = 0;
         }
			switch(size)
         {
         	case 1:
            	if(flags & LOG_FORM_INTEGER)
               {
                  if(flags & LOG_FORM_HEXA)
                  {
                     fprintf(LogFile,"%02x", (int)*d);
                     break;
                  }
                  else
                  if(flags & LOG_FORM_DEC)
                  {
                     fprintf(LogFile,"%03d", (int)*d);
                     break;
                  }
               }
               fprintf(LogFile,"??");
            break;
         	case 4:
            	if(flags & LOG_FORM_INTEGER)
               {
                  if(flags & LOG_FORM_HEXA)
                  {
                     fprintf(LogFile,"%08x", *(int*)d);
                     break;
                  }
                  else
                  if(flags & LOG_FORM_DEC)
                  {
                     fprintf(LogFile,"%d", *(int *)d);
                     break;
                  }
               }
               else
            	if(flags & LOG_FORM_FLOAT)
               {
                  if(flags & LOG_FORM_DEC)
                  {
                     fprintf(LogFile,"%f", (double)(*(float *)d));
                     break;
                  }
               }
               fprintf(LogFile,"????");
            break;

         }
         fputc(' ',LogFile);	//separator
         d += size;
         r++;
      }
      fputc('\n',LogFile);
   	Close();
   }*/
}

}