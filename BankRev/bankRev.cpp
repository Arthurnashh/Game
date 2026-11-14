// build packed files from folders
//#include <fstream>
//#include <string>
#include <El/elementpch.hpp>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#include <math.h>
#include <Es/Framework/debugLog.hpp>
#include <Es/Common/fltOpts.hpp>
#include <Es/Memory/fastAlloc.hpp>
#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>

#include <El/QStream/fileinfo.h>
#include <El/QStream/qstream.hpp>
#include <El/QStream/qbstream.hpp>
#include <El/Statistics/statistics.hpp>

TypeIsSimple(FileInfo);

bool CheckSameThread(int &id)
{
  return true;
};

unsigned long GlobalTickCount ()
{
	return 0;
}

void __cdecl LogF(char const *,...)
{
}

void __cdecl ErrF(char const *,...)
{
}

void __cdecl LstF(char const *,...)
{
}

void __cdecl WarningMessage(char const *,...)
{
}

void __cdecl ErrorMessage(char const *,...)
{
}
#if COUNT_CLASS_INSTANCES // only in debug
  AllCountersLink *AllCountInstances[];
  int AllCountInstancesCount;
#endif

struct SaveContext
{
	QFBank *bank;
	RString folder;
	bool setTime;
};

//! create path to given directory
static void CreatePath(char *path)
{
	// string will be changed temporary
	char *end = path;
	while (end = strchr(end, '\\'))
	{
		*end = 0;
		mkdir(path);
		*end = '\\';
		end++;
	}
}

FILETIME TimetToFileTime( time_t t )
{
  FILETIME pft;
  LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
  pft.dwLowDateTime = (DWORD) ll;
  pft.dwHighDateTime = ll >>32;
  return pft;
}

static void SaveFile(const FileInfoO &fi, const FileBankType *files, void *context)
{
	SaveContext *sc = (SaveContext *)context;
	// save given file
	QIFStreamB file;
	file.open(*sc->bank,fi.name);
	// note: file may be compressed - auto handled by bank

	RString outFilename=sc->folder+RString("\\")+RString(fi.name);
	// note file name may contain subfolder
	char folderName[1024];
	strcpy(folderName,outFilename);
	*GetFilenameExt(folderName) = 0;
	if (strlen(folderName)>0)
	{
		if (folderName[strlen(folderName)-1]='\\') folderName[strlen(folderName)-1]=0;
		// FIX: create whole path (not only topmost directory)
		CreatePath(folderName);
		mkdir(folderName);
	}
	// 
//	FILE *tgt = fopen(outFilename,"wb");
//	if (!tgt) return;

//	setvbuf(tgt,NULL,_IOFBF,256*1024);
  QOFStream out(outFilename);
  file.copy(out);
  out.close();
  
  if (sc->setTime && fi.time> 0)
  {
    HANDLE hFile = CreateFile(outFilename, GENERIC_READ|FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
      DWORD err = GetLastError();
      printf("Cannot set file time, failed with error %lu.\n", err);
    }
    else
    {
      FILETIME filetime = TimetToFileTime(fi.time);
      SetFileTime(hFile, &filetime, &filetime, &filetime);
      CloseHandle(hFile);
    }
  }
//  fwrite(file.act(),file.rest(),1,tgt);

//	fclose(tgt);
}


struct LogContext
{
  FILE *file;
};

static void LogPBO(const FileInfoO &fi, const FileBankType *files, void *context)
  {
  printf("%s\n",cc_cast(fi.name));
  }

struct FindContext
{
  QFBank *bank1;
  QFBank *bank2;
  RString file;
  RString prefix;
};

static void FindNewFile(const FileInfoO &fi, const FileBankType *files, void *context)
	{
	FindContext *sc = (FindContext *)context;
  if (!sc->bank2->FileExists(fi.name))
    {
    printf("A\t%s\n",cc_cast(sc->prefix+fi.name));
    }
}
static void FindDeletedFile(const FileInfoO &fi, const FileBankType *files, void *context)
{
  FindContext *sc = (FindContext *)context;
  if (!sc->bank2->FileExists(fi.name))
  {
    printf("D\t%s\n",cc_cast(sc->prefix+fi.name));
  }
}

// print all files that have different size
static void SizeFile(const FileInfoO &fi, const FileBankType *files, void *context)
{
	FindContext *sc = (FindContext *)context;
  if (sc->bank2->FileExists(fi.name))
  {
    const FileInfoO fi2 = sc->bank2->FindFileInfo(fi.name);
    unsigned long fi_size = ( fi.compressedMagic==CompMagic )?fi.uncompressedSize:fi.length;
    unsigned long fi2_size = ( fi2.compressedMagic==CompMagic )?fi2.uncompressedSize:fi2.length;

    if (fi_size!=fi2_size)
    {
      printf("U (size) %s (%dB!=%dB)\n",cc_cast(sc->prefix+fi.name),fi_size,fi2_size);
    }
  }
}

// check file date for files that are same length
static void DateFile(const FileInfoO &fi, const FileBankType *files, void *context)
{
	FindContext *sc = (FindContext *)context;
  if (sc->bank2->FileExists(fi.name))
  {
    const FileInfoO fi2 = sc->bank2->FindFileInfo(fi.name);
    if (fi.length==fi2.length && fi.time!=fi2.time)
    {
      time_t fi_time = (time_t)fi.time;
      time_t fi2_time = (time_t)fi2.time;

      char fi_buf[80],fi2_buf[80];
      strftime (fi_buf,80,"%H:%M %d.%m.%y",localtime(&fi_time));
      strftime (fi2_buf,80,"%H:%M %d.%m.%y",localtime(&fi2_time));

      printf("U (time) %s (%s)-(%s)\n",cc_cast(sc->prefix+fi.name),fi_buf,fi2_buf);    
    }
  }
}

// compare files that have same size and date.
// if those files are different it determine that PBO can be corupted.
static void CmpFile(const FileInfoO &fi, const FileBankType *files, void *context)
{
	FindContext *sc = (FindContext *)context;
  if (sc->bank2->FileExists(fi.name))
  {
    const FileInfoO fi2 = sc->bank2->FindFileInfo(fi.name);
    unsigned long fi_size = ( fi.compressedMagic==CompMagic )?fi.uncompressedSize:fi.length;
    unsigned long fi2_size = ( fi2.compressedMagic==CompMagic )?fi2.uncompressedSize:fi2.length;

    if (fi_size==fi2_size && fi.time==fi2.time)
    {
	    QIFStreamB f1,f2;
	    f1.open(*sc->bank1,fi.name);
	    f2.open(*sc->bank2,fi.name);

      bool filesSame = true;
      while (filesSame && !f1.eof() && !f2.eof()) 
      {
        int d1 = f1.get();
        int d2 = f2.get();
        filesSame = (d1==d2); 
      }
      if (filesSame) filesSame = (f1.eof() && f2.eof());
      if (!filesSame)
      {
        printf("U (cmp)  %s\n",cc_cast(sc->prefix+fi.name),fi.length,fi2.length);
        printf("ERROR: different file content in %s with same date in PBO can determine that one of PBOs is corrupted\n",cc_cast(sc->prefix+fi.name));
      }
      f1.close();
      f2.close();
    }
  }
}

static void ShowHelp()
{
  printf("Bankrev [-f|-folder destiantion] [-t|-time] PBO_file [PBO_file_2 ...]"
       "\n      or -d|-diff PBO_file1 PBO_file2 (compare two PBOs)"
       "\n      or -l|-log PBO_file (write to stdout list of files in archive)"
       "\n      or -p|-properties PBO_file (write out PBO properties)"
       "\n      or -hash PBO_file (write out hash of PBO)"
       "\n      or -statistics PBO_file (write the statistics of files lengths)"
       "\n      or -t|-time (keep file time from archive) \n");
}

static void ShowLog(const char * fileName)
{
  char file[1024];
  strcpy(file, fileName);
  *GetFileExt(file) = 0;

  QFBank bank;
  if (!bank.open(file)) 
  {
    fprintf(stderr,"unable open %s\n",file);
  }
  else
    bank.ForEach(LogPBO,NULL);
}

static void ShowHash(const char * fileName)
{
  char file[1024];
  strcpy(file, fileName);
  *GetFileExt(file) = 0;

  QFBank bank;
  if (!bank.open(file)) 
  {
    fprintf(stderr,"unable open %s\n",file);
    return; 
  }

  Temp<char> hash;
  if (!bank.Load())
  {
    fprintf(stderr,"unable load %s\n",file);
    return; 
  }

  if (!bank.GetHash(hash))
  {
    fprintf(stderr,"unable to get hash from %s\n",file);
    return; 
  }

  if (hash.Size()!=20)
  {
    fprintf(stderr,"wrong length of hash in %s\n",file);
    return; 
  }

  for (int i=0;i<hash.Size();i+=2)
    printf("%02X%02X",(unsigned char)hash[i],(unsigned char)hash[i+1]);

  printf("\n");
}

static void ShowProperties(const char * fileName)
{
  char file[1024];
  strcpy(file, fileName);
  *GetFileExt(file) = 0;

  QFBank bank;
  if (!bank.open(file)) 
  {
    fprintf(stderr,"unable open %s\n",file);
  }
  else
  {
    if (!bank.GetPrefix().IsEmpty())
      printf("prefix = %s\n",cc_cast(bank.GetPrefix()));

    for (int i = 0; i<bank.NProperties();++i)
    {
      printf("%s = %s\n",cc_cast(bank.GetProperty(i).name),cc_cast(bank.GetProperty(i).value));
    }
  }
}

struct ExtHistogram
{
  StatisticsByName _extSize;
  StatisticsByName _extCount;
  int _total;

  ExtHistogram() : _total(0) {}
  static void ProcessItemCallback(const FileInfoO &fi, const FileBankType *files, void *context)
  {
    reinterpret_cast<ExtHistogram *>(context)->ProcessItem(fi,files);
  }
  void ProcessItem(const FileInfoO &fi, const FileBankType *files)
  {
    _total += fi.length;
    _extSize.Count(GetFileExt(fi.name),fi.length);
    _extCount.Count(GetFileExt(fi.name),1);
  }
  void ProcessBank(const QFBank &bank)
  {
    bank.ForEach(ProcessItemCallback,this);
  }
};

static void printfStat(const char *name, int count, void *context)
{
  int *total = static_cast<int *>(context);
  if (total)
    printf(" %6s: %10d ... %2.1f%%\n",name,count,100*float(count)/(*total));
  else
    printf(" %6s: %d\n",name,count);
}

static void ShowStatistics(const char * fileName)
{
  QFBank bank;
  bank.open(fileName);

  // enumerate files in bank
  ExtHistogram statistics;
  statistics.ProcessBank(bank);
  printf("Size by extension:\n");
  statistics._extSize.Report(printfStat,0,false, (void *)&statistics._total);
  printf("\nCount by extension:\n");
  statistics._extCount.Report(printfStat,0,false, NULL);
}

static void ShowDifferences(const char * fileName1,const char * fileName2)
{
  char file1[1024];
  strcpy(file1, fileName1);
  *GetFileExt(file1) = 0;

  char file2[1024];
  strcpy(file2, fileName2);
  *GetFileExt(file2) = 0;

  QFBank bank1,bank2;
  if (!bank1.open(file1)) 
  {
    fprintf(stderr,"unable open %s\n",file1);
  }
  else if (!bank2.open(file2)) 
  {
    fprintf(stderr,"unable open %s\n",file2);
  }
  else 
  {
    if (bank1.GetPrefix()!=bank2.GetPrefix())
      printf("Different prefix:\n%s(%s)\n%s(%s)\n",file1,cc_cast(bank1.GetPrefix()),file2,cc_cast(bank2.GetPrefix()));

    FindContext findContext;
    findContext.prefix= bank1.GetPrefix(); 

    findContext.bank2 = &bank2;
    findContext.file = file2;
    bank1.ForEach(FindNewFile,&findContext);

    findContext.bank2 = &bank1;
    findContext.file = file1;
    bank2.ForEach(FindDeletedFile,&findContext);

    findContext.bank2 = &bank2;
    findContext.file = file1;
    bank1.ForEach(SizeFile,&findContext);

    findContext.bank2 = &bank2;
    findContext.bank1 = &bank1;
    findContext.file = file1;
    bank1.ForEach(DateFile,&findContext);

    findContext.bank2 = &bank2;
    findContext.bank1 = &bank1;
    findContext.file = file1;
    bank1.ForEach(CmpFile,&findContext);
  }
}
static void UnpackPBO(const char * fileName,const RString destFolder, const bool setTime = false)
{
		char folderName[1024];
  strcpy(folderName, fileName);
		*GetFileExt(folderName) = 0;

		RString folder;
    if (!destFolder.IsEmpty())
    {
      char *subFolder= strrchr(folderName,'\\');
      if (subFolder) ++subFolder;
      else subFolder=folderName;

      if (destFolder[destFolder.GetLength()-1]!='\\')
      {
        folder=destFolder+"\\"+subFolder;
      }
      else 
      {
        folder = destFolder+subFolder;
      }
    }
    else
    {
      folder = folderName;
    }

		RString source=folderName;

		QFBank bank;
		// enumerate files in bank

		bank.open(source);

		// create a destination folder
    char folderMkdir[1024];
    strcpy(folderMkdir,folder);
    CreatePath(folderMkdir);
		printf("Created %s\n",folderMkdir);


		SaveContext saveContext;
		saveContext.bank = &bank;
    saveContext.folder = folder;
  saveContext.setTime = setTime;  
		bank.ForEach(SaveFile,&saveContext);
    
    // Save properties to pboname.txt file
    FILE *f;
    if (fopen_s(&f,folder+".txt","w")==0)
    {
      RString prexif = bank.GetPrefix();
      if (!prexif.IsEmpty())
      {
        RString text = "prefix="+prexif+"\n";
        fwrite(text,1,text.GetLength(),f);
      }
      for (int i=0,end = bank.NProperties();i<end;++i)
      {
        QFProperty _property = bank.GetProperty(i);
        //RString name = bank.GetPropertyName(i);
        RString propLine = _property.name+ "=" + _property.value+"\n";
        fwrite(propLine,1,propLine.GetLength(),f);
      }
      fclose(f);
    }
}

static bool TestArgsCond(const bool test)
{
  if (test) 
    fprintf(stderr,"Not enough parameters\n");

  return test;  
};

int main( int argc, const char *argv[] )
{
  RString destFolder = RString();
  bool setTime = false;
  if (argc==1) 
    ShowHelp();

  for (int i=1; i<argc; ++i)
  {
    if (*argv[i]=='-' || *argv[i]=='/')
    {
      if ((stricmp(argv[i]+1,"log")==0)||(stricmp(argv[i]+1,"l")==0)) // write to console all files stored in PBO.
      {
        if (TestArgsCond(argc < i+2)) return -1;
        ShowLog(argv[++i]);
    	}
      else if ((stricmp(argv[i]+1,"help")==0)||(stricmp(argv[i]+1,"h")==0)) // show help.
        ShowHelp();
      else if (stricmp(argv[i]+1,"hash")==0) // show HASH of PBO.
      {
        if (TestArgsCond(argc < i+2)) return -1;
        ShowHash(argv[++i]);
      }
      else if ((stricmp(argv[i]+1,"folder")==0)||(stricmp(argv[i]+1,"f")==0)) // change destination folder for following PBO files.
        destFolder = argv[++i];  
      else if ((stricmp(argv[i]+1,"properties")==0)||(stricmp(argv[i]+1,"p")==0)) // write to console properties of following file.
      {
        if (TestArgsCond(argc < i+2)) return -1;
        ShowProperties(argv[++i]);
      }
      else if ((stricmp(argv[i]+1,"statistics")==0)||(stricmp(argv[i]+1,"p")==0)) // write to console the statisticks about containing files
      {
        if (TestArgsCond(argc < i+2)) return -1;
        ShowStatistics(argv[++i]);
      }
      else if ((stricmp(argv[i]+1,"diff")==0)||(stricmp(argv[i]+1,"d")==0)) // compare and show differences between two PBO files.
      {
        if (TestArgsCond(argc < i+3)) return -1;
        ShowDifferences(argv[i+1],argv[i+2]);
        i+=2;
      }
      else if ((stricmp(argv[i]+1,"time")==0)||(stricmp(argv[i]+1,"t")==0)) // will keep date and time from archive.
        setTime = true;  
      else
        fprintf(stderr,"unknown command %s\n",argv[i]);
    }
    else // if no command then unpack it.
      UnpackPBO(argv[i],destFolder,setTime);
  }
	return 0;
}

void *CCALL operator new(unsigned int size ,char const *file,int line )
{
	return malloc(size);
}

