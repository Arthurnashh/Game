// build packed files from folders
#include <El/elementpch.hpp>
#include <Es/Framework/consoleBase.h>

#include <string.h>
#include <malloc.h>

#include <sys/stat.h>
#ifdef __GNUC__
#define __cdecl
#include <unistd.h> 
#include <dirent.h> 
#else
#include <io.h>
#endif

#ifndef __GNUC__
#include <windows.h>
#endif

#if _VBS1_USER
#include <winCrypt.h>
#endif

#include <El/QStream/packFiles.hpp>
#include <El/QStream/qbstream.hpp>
#include <Es/Files/filenames.hpp>


#include <stdarg.h>

#if _ENABLE_PBO_PROTECTION
#include <EL/Encryption/PBOHeaderProtection.h>
#include <EL/Encryption/AESEncryption.h>
#endif

#if _VBS1_USER

HCRYPTPROV OpenProvider()
{
  const char *name = "VBS1Container";
  HCRYPTPROV handle = NULL;

  //--------------------------------------------------------------------
  // Attempt to acquire a context and a key
  // container. The context will use the default CSP
  // for the RSA_FULL provider type. DwFlags is set to 0
  // to attempt to open an existing key container.

  if (CryptAcquireContext(&handle, name, NULL, PROV_RSA_FULL, 0))
    return handle;

  //--------------------------------------------------------------------
  // An error occurred in acquiring the context. This could mean
  // that the key container requested does not exist. In this case,
  // the function can be called again to attempt to create a new key 
  // container. 
  if (GetLastError() == 0x80090016)
  {
    if (CryptAcquireContext(&handle, name, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
      return handle;
    printf("Could not create a new key container.\n");
    return NULL;
  }
  else
  {
    printf("A cryptographic service handle could not be acquired.\n");
    return NULL;
  }
}

void CloseProvider(HCRYPTPROV handle)
{
  CryptReleaseContext(handle, 0); 
}

static BYTE publicKey[] =
{
  0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x31, 0x00, 0x08, 0x00, 0x00, 
  0x01, 0x00, 0x01, 0x00, 0x37, 0xc4, 0xe8, 0xa6, 0x30, 0x90, 0x7c, 0x55, 0x68, 0x1c, 0x6b, 0x7b, 
  0xcd, 0x0f, 0x1e, 0x40, 0xc1, 0x0d, 0x06, 0x06, 0x0e, 0x02, 0x42, 0x6b, 0xb5, 0x7c, 0xa8, 0x68, 
  0xbf, 0x62, 0xae, 0x73, 0xe8, 0x6e, 0x49, 0x5d, 0xc8, 0x68, 0xec, 0x50, 0x69, 0xd7, 0x9c, 0xd8, 
  0x63, 0xe9, 0x71, 0x9d, 0x74, 0x21, 0xc1, 0xa4, 0x17, 0x40, 0x62, 0x46, 0x30, 0x50, 0x71, 0xd0, 
  0x93, 0xec, 0x97, 0xed, 0x17, 0xe6, 0xea, 0x77, 0x3f, 0xd1, 0x6c, 0x73, 0x72, 0x6b, 0xf0, 0x30, 
  0xf4, 0xbb, 0x4c, 0xf4, 0xdb, 0xc9, 0x6e, 0x2a, 0x01, 0x9c, 0x5a, 0x8d, 0xbf, 0xae, 0x0a, 0xb9, 
  0xc7, 0x06, 0x77, 0x83, 0xd5, 0x79, 0x01, 0x00, 0xa9, 0xc3, 0x80, 0x51, 0x42, 0x28, 0xed, 0x6f, 
  0x4d, 0x41, 0x82, 0xb6, 0x55, 0xae, 0xc9, 0x64, 0xd9, 0x38, 0xfb, 0xbf, 0x2e, 0x29, 0x0f, 0x8c, 
  0x4a, 0x6f, 0x61, 0x0c, 0xe0, 0x3c, 0x7a, 0x7b, 0xcc, 0x7d, 0x45, 0x07, 0x55, 0xb8, 0xd1, 0x43, 
  0xa1, 0x30, 0x81, 0x5b, 0x6e, 0x0d, 0xf7, 0x72, 0x5e, 0x4f, 0xd3, 0x93, 0xc9, 0x1f, 0x61, 0xfd, 
  0xb9, 0x93, 0xa9, 0x07, 0x79, 0x30, 0x89, 0xfa, 0x72, 0x2c, 0x94, 0x96, 0x68, 0xce, 0x39, 0x28, 
  0xfd, 0x69, 0xc0, 0x42, 0xfb, 0xe3, 0x46, 0x82, 0x06, 0xce, 0xc1, 0xc1, 0x50, 0x94, 0xda, 0x23, 
  0x40, 0xde, 0x63, 0x7a, 0x8a, 0xc0, 0xe9, 0x57, 0x49, 0x00, 0xa0, 0x6d, 0x29, 0x0f, 0x28, 0x00, 
  0xc2, 0xf7, 0x4a, 0xf7, 0xc3, 0x04, 0xeb, 0x15, 0x84, 0xa2, 0x43, 0x17, 0x06, 0x82, 0x68, 0x21, 
  0x37, 0x4a, 0x9b, 0xa7, 0x82, 0xb1, 0x11, 0x0c, 0xf0, 0x3b, 0x91, 0x39, 0xad, 0x6e, 0x24, 0xf4, 
  0x9b, 0xd6, 0x3a, 0x45, 0x2c, 0x08, 0xda, 0xa0, 0x43, 0xba, 0x33, 0x3e, 0x8b, 0xf3, 0x8a, 0x1d, 
  0x87, 0x38, 0x58, 0xc0
};

IFilebankEncryption *CreateVBS1Encryption()
{
  // Read licence from file
  DWORD value;
  static const DWORD length = 256;
  BYTE signature[length];
  FILE *in = fopen("licence.txt", "rt");
  if (!in)
  {
    printf("Licence file not found.\n");
    return NULL;
  }
  fscanf(in, "%d\n", &value);
  for (DWORD i=0; i<length; i++)
  {
    int c1 = fgetc(in);
    int c2 = fgetc(in);
    if (c1 == EOF || c2 == EOF)
    {
      printf("Cannot read licence file.\n");
      return NULL;
    }
    int x1 = c1 < 'a' ? c1 - '0' : c1 - 'a' + 10;
    int x2 = c2 < 'a' ? c2 - '0' : c2 - 'a' + 10;
    BYTE x = (x1 << 4) + x2;
    signature[i] = x;
  }
  fclose(in);

  HCRYPTPROV provider = OpenProvider();
  if (!provider) return NULL;

  // Import the pubic key
  HCRYPTKEY key = NULL; 
  if (!CryptImportKey(provider, publicKey, sizeof(publicKey) / sizeof(*publicKey), 0, 0, &key))
  {
    printf("Cannot verify licence file.\n");
    CloseProvider(provider);
    return NULL;
  }

  // Create the hash object
  HCRYPTHASH hash = NULL;
  if (!CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
  {
    printf("Cannot verify licence file.\n");
    CryptDestroyKey(key);
    CloseProvider(provider);
    return NULL;
  }

  // Compute the cryptographic hash of the buffer.
  if (!CryptHashData(hash, (BYTE *)&value, sizeof(value), 0))
  {
    printf("Cannot verify licence file.\n");
    CryptDestroyHash(hash);
    CryptDestroyKey(key);
    CloseProvider(provider);
    return NULL;
  }

  // Validate the digital signature
  if (!CryptVerifySignature(hash, signature, length, key, NULL, 0))
  {
    printf("Licence file is not valid.\n");
    CryptDestroyHash(hash);
    CryptDestroyKey(key);
    CloseProvider(provider);
    return NULL;
  }

  CryptDestroyHash(hash);
  CryptDestroyKey(key);
  CloseProvider(provider);

  char buffer[16];
  itoa(value, buffer, 10);
  return CreateFilebankEncryption("VBS1", buffer);
}

#endif

static void CreatePath(const char *path)
{
  // string will be changed temporary
  char *end = unconst_cast(path);
  if (end[0] != 0 && end[1] == ':' && end[2] == '\\') end += 3;
  while (end = strchr(end, '\\'))
  {
    *end = 0;
#ifdef _WIN32
    ::CreateDirectoryA(path, NULL);
#else
    ::CreateDirectory(path, NULL);
#endif
    *end = '\\';
    end++;
  }
}

RString GetDestination(const char * destination,const char *source,RString fileExt = ".pbo")
{
  char buf[1024];
  strncpy(buf,source,1024); 
  RString target;
  if (destination)
  {
    
    char * pboName = strrchr(buf,'\\');
    if (!pboName) 
    {
      pboName = buf;
    }
    else
    {
      ++pboName; 
    }

    int dstLen = strlen(destination); 
    if (destination[dstLen-1]=='\\')
    {
      target = RString(destination);
    }
    else
    {
      target = RString(destination)+RString("\\");
    }
    CreatePath(target);
    target= target+RString(pboName);
  }
  else
  {
	  target = RString(buf);
  }
  target = target +  fileExt;
  return target;
};

static void AddSourcePrefix(AutoArray<QFProperty> &properties, const char *source)
{
  // check if there is any Prefix property
  for (int i=0; i<properties.Size(); i++)
  {
    if (!strcmpi(properties[i].name,"prefix")) return;
  }
  // no prefix - add it
  if (source[0] && source[1]==':')
  {
    // remove drive letter
    source += 2;
  }
  if (source[0]=='\\' || source[0]=='/')
  {
    source++;
  }
  if (!*source) return;
  QFProperty &prop = properties.Append();
  prop.name = "prefix";
  prop.value = source;
}

#if _ENABLE_PBO_PROTECTION
static int EncryptHeaders(IFilebankEncryption* headerProtection, RString hpEncryptSrc)
{
  Assert(headerProtection);
  if (!headerProtection)
  {
    return 1;
  }

  PBOHeaderProtection::TransformResult result = PBOHeaderProtection::TransformPBO(hpEncryptSrc, 0, headerProtection);
  if (result != PBOHeaderProtection::TROk)
  {
    RString error = RString("Encryption of headers of \"") + hpEncryptSrc + "\" failed - \"" + PBOHeaderProtection::TransformResultToString(result) + "\"";
    ErrF(cc_cast(error));
    printf(cc_cast(error));
    return 1;
  }
  
  return 0;
}

#endif //#if _ENABLE_PBO_PROTECTION

#if __GNUC__
	#define stricmp strcasecmp
#endif

int consoleMain( int argc, const char *argv[] )
{
#if _VBS2 && (_VBS2_LITE || _VBS2_ENC)
	  // register vbs2 encryption AES 128 bit
    IFilebankEncryption *CreateEncryptVBS2AESAddon(const void *context);
    RegisterFilebankEncryption("VBS2",CreateEncryptVBS2AESAddon);
#else
	#if _VBS1_USER || _VBS1_ADMIN
	// register known encryption methods
	IFilebankEncryption *CreateEncryptXOR1024(const void *context);
	RegisterFilebankEncryption("XOR1024",CreateEncryptXOR1024);

	IFilebankEncryption *CreateEncryptVBS1(const void *context);
	RegisterFilebankEncryption("VBS1",CreateEncryptVBS1);
	#endif
#endif

	argv++;
	argc--;
	if (argc<=0)
	{
		Usage:
#if _VBS1_USER
    printf("Usage\n");
    printf("   VBS1MakeAddon {options} source [source]\n");
    printf("   source may be directory name or log file name (.log)\n");
    printf("\n");
    printf("   Options:\n");
#else
    printf("Usage\n");
		printf("   FileBank {options} source [source]\n");
		printf("   source may be directory name or log file name (.log)\n");
		printf("\n");
#if _ENABLE_PBO_PROTECTION
    printf("   FileBank -hpencrypt source\n");
    printf("   encrypts header of given source PBO. PBO must be prepared for header encryption\n");
    printf("\n");
#endif
		printf("   Options:\n");
	#if _VBS2 && (_VBS2_LITE || _VBS2_ENC)
		printf("   -encrypt encryption_name password_file\n");
	#endif
#endif
		printf("   -property name=value   Store a named property\n");
    printf("   -exclude filename      List of patterns which should not be included in the pbo file\n");
    printf("   -dst path              path to folder to store PBO\n");
#if _ENABLE_PBO_PROTECTION
    printf("   -hpprepare             creates PBO, so that headers are prepared for encryption (but are not encrypted yet\n");
    printf("   -hpencrypt src         encrypts headers of existing PBO. PBO headers must be prepared for encryption\n");
#endif
		return 1;		
	}
	Ref<IFilebankEncryption> encrypt = NULL;
	AutoArray<QFProperty> properties;
	FindArray<RString> excludeFiles;
  const char * destination = NULL;
#if _ENABLE_PBO_PROTECTION
  SRef<IFilebankEncryption> headerProtection = 0;
  bool hpPrepare = false;
  bool hpEncrypt = false;
  RString hpEncryptSrc;
#endif
	while( argc>0 )
	{
		const char *arg=*argv;
		if( !arg ) {argc--,argv++;continue;}
		#if __GNUC__
		if( *arg=='-' )
		#else
		if( *arg=='-' || *arg=='/' )
		#endif
		{
			// some option
			argv++; // skip argument
			argc--;
			arg++; // skip dash
			if( !stricmp(arg,"compress") )
			{
			  fprintf(stderr,"-compress no longer supported");
			}
			else if( !stricmp(arg,"property") && argc>0 )
			{
				// read property name and value
				const char *prop = *argv;
				argv++,argc--; // skip argument
				const char *value = strchr(prop,'=');
				if (!value) goto Usage;
				QFProperty property;
				property.name = RString(prop,value-prop);
				property.value = value+1;
				properties.Add(property);
			}
			else if (!stricmp(arg,"exclude") && argc>0)
			{
			  // read exclude file list
			  const char *excludeFile = *argv;
			  argv++,argc--;
			  // read exclude patterns

        FILE *f = fopen(excludeFile,"r");
        if (f)
        {
          // read single filename
          char line[1024];
          while (fgets(line,sizeof(line),f))
          {
            // remove end of line
            int l = strlen(line);
            if (l>0 && line[l-1]=='\n') line[l-1] = 0;
            if (*line==0 || *line==';' || *line=='#') continue;
            strlwr(line);
            excludeFiles.Add(line);
          }
          fclose(f);
        }      
			}
			else if( !stricmp(arg,"dst") && argc>0 )
			{
			  destination = *argv;
			  argv++,argc--;
			}
#if _ENABLE_PBO_PROTECTION
      else if( !stricmp(arg,"hpencrypt") && argc>0 )
      {
        headerProtection = PBOHeaderProtection::CreateSetupHeaderProtection();
        hpEncrypt = true;
        hpEncryptSrc = *argv;
        argv++,argc--;
      }
      else if( !stricmp(arg,"hpprepare"))
      {
        headerProtection = PBOHeaderProtection::CreateSetupHeaderProtection();
        hpPrepare = true;
      }
#endif
#if _VBS2 && (_VBS2_LITE || _VBS2_ENC)
			else if( !stricmp(arg,"encrypt"))
			{
				if (argc<=2)
				{
					goto Usage;
				}
				// read property name and value
				const char *name = *argv;
				argv++; // skip argument
				argc--;
				const char *password = *argv;
				argv++; // skip argument
				argc--;
				// create encryption manager
				encrypt = CreateFilebankEncryption(name,password);
				if (!encrypt)
				{
					printf("Unknown encryption %s\n",name);
					goto Usage;
				}
				QFProperty property;
				property.name = "encryption";
				property.value = name;
				properties.Add(property);
			}
#endif
		}
		else break;
	}
#if _ENABLE_PBO_PROTECTION
  if (hpEncrypt)
  {
    return EncryptHeaders(headerProtection.GetRef(), hpEncryptSrc);
  }
#endif    

	if (argc<=0) goto Usage;
#if _VBS1_USER
  encrypt = CreateVBS1Encryption();
  if (!encrypt) return 1;
  QFProperty property;
  property.name = "encryption";
  property.value = "VBS1";
  properties.Add(property);
#endif
	while( --argc>=0 )
	{
		RString source=*argv++;
		if (*source=='-') goto Usage;
		// check if extension is given
		// if .log is given, use log
		const char *ext = GetFileExt(GetFilenameExt(source));
		FileBankManager man;
		if (!strcmpi(ext,".log"))
		{
#if _VBS2 && _VBS2_LITE
      LogF("Log not supported in lite version");
#else
			char buf[1024];
			strcpy(buf,source);
			*GetFileExt(GetFilenameExt(buf))=0;
			AddSourcePrefix(properties,buf);
      RString target = GetDestination(destination,buf);
			RString folder = buf;
			man.Create(
				target,folder,false,true,source,
				DefFileBankNoCompress,
				properties.Data(),properties.Size(),
				excludeFiles.Data(),excludeFiles.Size()
#if _ENABLE_PBO_PROTECTION
     , hpPrepare ? headerProtection.GetRef() : 0
#endif
			);
#endif
		}
		else if (encrypt)
		{
			AddSourcePrefix(properties,source);
#if _VBS2 && (_VBS2_LITE || _VBS2_ENC)
      RString target = GetDestination(destination,source,".xbo");
#else
      RString target = GetDestination(destination,source);  
#endif 
			
			QOFStream out;
			out.open(target);
			man.Create(
			  out,source,encrypt,
			  properties.Data(),properties.Size(),
				excludeFiles.Data(),excludeFiles.Size()
			);
			out.close();
		}
		else
		{
#if _VBS2 && _VBS2_LITE
      LogF("Pack not supported in lite version");
#else
			AddSourcePrefix(properties,source);
      RString target = GetDestination(destination,source);
			man.Create(
				target,source,false,true,NULL,
				DefFileBankNoCompress,
				properties.Data(),properties.Size(),
				excludeFiles.Data(),excludeFiles.Size()
#if _ENABLE_PBO_PROTECTION
               ,hpPrepare ? headerProtection.GetRef() : 0
#endif
			);
#endif
		}
	}
	return 0;
}

#include <Es/Memory/normalNew.hpp>

#if _MSC_VER
/*
void * CCALL operator new ( size_t size, const char *file, int line )
{
	return malloc(size);
}
*/
#endif

