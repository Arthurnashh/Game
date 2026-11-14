// SvnAutoUpdater.cpp : Defines the entry point for the console application.
//

#include "../SvnAutoUpdater/svnAccess.hpp"

const char *SvnActionFormat(svn_wc_notify_action_t action)
{
  char buf[1024];
  switch (action)
  {
  case svn_wc_notify_update_started: return "Update started for %s\n";
  case svn_wc_notify_update_update: return "  %s - Updated\n";
  case svn_wc_notify_update_delete: return "  %s - Deleted\n";
  case svn_wc_notify_update_add:    return "  %s - Added\n";
  case svn_wc_notify_update_completed: return "Update of %s completed\n";
  case svn_wc_notify_skip_conflicted: return "CONFLICTED: %s\n";
  }
  sprintf(buf, "(%d) %%s\n", action); //something different
  return buf;
}

void SvnNotifyCallback(void *baton, const svn_wc_notify_t *notify, apr_pool_t *pool)
{
  printf(SvnActionFormat(notify->action), notify->path);
}

int main(int argc, const char *argv[])
{
  const char *myPath;
  if (argc <= 1) {
    printf ("Usage:  %s working copy folder\n", argv[0]);
    return EXIT_FAILURE;
  }
  else myPath = argv[1];

  //SvnAccess svnAcc("arma2OA", "Arma2OA-24915472",SvnNotifyCallback, NULL);  //test case for O:\tmp\bebulek
  SvnAccess svnAcc("dayzUpdater", ",6u-bvDCwotS", SvnNotifyCallback, NULL);  //DayZ dist working copy
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
    //const char *URL = "https://dev.bistudio.com/svn/pgm/trunk/Drobnosti/SvnAutoUpdater/bebulek";
    const char *URL = "https://of.bistudio.com/svn/dist/DayZ";
    svnAcc.Checkout(URL, myPath);
  }

  //check status (test new version)
  bool update = false;
  bool ok = svnAcc.UpdateAvailable(myPath, update);
  printf("The \"%s\" is%s necessary to update!\n", myPath, update ? "" : " not");
  char buf[1024];
  sprintf(buf, "The \"%s\" is%s necessary to update!\n", myPath, update ? "" : " not");
  //OutputDebugString(buf);

  bool ok2 = svnAcc.DiffSummarize(myPath, 630,750);

  //update
  if (!ok)
  {
    printf("svn error: %s\n", svnAcc.GetLastError());
  }
  else if (update)
  {
    int rev;
    if ( svnAcc.Update(myPath, rev) )
      printf("The \"%s\" was updated to revision %d.\n", myPath, rev);
    else
      printf("Update of \"%s\" has failed!\n", myPath);
  }

  return EXIT_SUCCESS;
}
