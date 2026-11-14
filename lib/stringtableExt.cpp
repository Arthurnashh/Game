#include "wpch.hpp"
#include "stringtableExt.hpp"

#define STRING(x) int IDS_##x;
#include "stringids.hpp"
#undef STRING

#include <El/Modules/modules.hpp>

INIT_MODULE(StringtableExt, 1)
{
//#define STRING(x) IDS_##x = RegisterString("STR_"#x);
//#include "stringids.hpp"
//#undef STRING
  struct StringIds
  {
    int *ids;
    const char *name;
  };
  const static StringIds sIds[]=
  {
#define STRING(x) {&IDS_##x, "STR_"#x},
#include "stringids.hpp"
#undef STRING
  };
  for (int i=0; i<sizeof(sIds)/sizeof(*sIds); i++)
  {
    *sIds[i].ids = RegisterString(sIds[i].name);
  }
  

}
