
#if 1
enabled = true;
#else

#ifdef MSC_VER
unsigned short serNum = 0;
unsigned short maxCompLen = 0;
unsigned short sz=256;
char label[256];
char drive[256];
sprintf(drive,"C:\\");
unsigned short flags=0;
unsigned short sz2=256;
char fileSystem[256];
long Ret = GetVolumeInformation(drive,label,sz,&serNum,&maxCompLen,&flags,fileSystem,sz2);
#endif
unsigned sdk_expire_date=license_code[0]^(~license_code[1]);
time_t system_time;
time(&system_time);
struct tm today;
#ifdef _MSC_VER
int err = _localtime64_s( &today, &system_time );
#else
today = *(localtime( &system_time ));
int err=0;
#endif
int expire_year=sdk_expire_date/512;
enabled=true;
if(expire_year!=10000)
{
  int expire_month=(sdk_expire_date-512*expire_year)/32;			// January==0
  int expire_day=sdk_expire_date-512*expire_year-32*expire_month;
  bool fail=today.tm_year+1900>(expire_year);
  fail|=(today.tm_year+1900==(expire_year)&&today.tm_mon+1>expire_month);
  fail|=(today.tm_year+1900==expire_year&&today.tm_mon+1==expire_month&&today.tm_mday>expire_day);
  std::cout<<"Simul SDK: evaluation expires day "<<expire_day<<", month "<<(expire_month)<<
    ", year " <<(expire_year)<<std::endl;
  if (err||fail||expire_year<2009||expire_month>12||expire_year>2011)
  {
    std::cerr<<"Simul SDK: the evaluation version has expired. Please contact support@simul.co.uk for help.";
    enabled=false;
  }
}
#if defined(_MSC_VER) && defined(VFORGE_OK)
char path[120];
unsigned nSize=90;
GetModuleFileName(NULL,path,nSize);
std::string fullpath=path;
size_t pos=fullpath.find_last_of('\\');
std::string filename=fullpath.substr(pos+1,fullpath.length()-pos-1);
if(_stricmp(filename.c_str(),"vforge.exe")==0)
enabled=true;
#endif

#endif
