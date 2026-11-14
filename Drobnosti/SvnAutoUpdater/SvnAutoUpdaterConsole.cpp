// SvnAutoUpdater.cpp : Defines the entry point for the console application.
//

#include "svnAccess.hpp"

int main(int argc, const char *argv[])
{
  const char *myPath;
  if (argc <= 1) {
    printf ("Usage:  %s working copy folder\n", argv[0]);
    return EXIT_FAILURE;
  }
  else myPath = argv[1];

  SvnAccess svnAcc;
  if (svnAcc.Failed()) 
    return EXIT_FAILURE;

  /* Now do the real work. */

  /* Main call into libsvn_client does all the work. */
#if TEST_LIST_CMD
  // example to call list command
  const char *URL = "https://dev.bistudio.com/svn/pgm/Colabo/trunk/test";
  err = svn_client_list2(URL, &revision, &revision, svn_depth_infinity, SVN_DIRENT_ALL, FALSE, MyFunc, NULL, ctx, pool);
  if (err) {
    return EXIT_FAILURE;
  }
#endif

  //test wc path
  //set doCheckout if the myPath folder is not under source control
  bool doCheckout = !svnAcc.IsVersioned(myPath);

  //checkout test
  if (doCheckout)
  {
    const char *URL = "https://dev.bistudio.com/svn/pgm/trunk/Drobnosti/SvnAutoUpdater/bebulek";
    svnAcc.Checkout(URL, myPath);
  }

  //check status (test new version)
  
  bool updateAvailable = false;
  bool updateWc = svnAcc.UpdateAvailable(myPath, updateAvailable);
  printf("The \"%s\" is%s necessary to update!\n", myPath, updateWc ? "" : " not");

  //update wc
  if (updateWc)
  {
    int rev;
    if ( svnAcc.Update(myPath, rev) )
      printf("The \"%s\" was updated to revision %d.\n", myPath, rev);
    else
      printf("Update of \"%s\" has failed!\n", myPath);
  }

  return EXIT_SUCCESS;
}
