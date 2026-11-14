#ifndef SVN_ACCESS_H
#define SVN_ACCESS_H

#include <svn_client.h>

class SvnAccess
{
private:
  char _lastError[1024];
  svn_client_ctx_t *_ctx;
  apr_pool_t *_pool;
  bool _fail;
  svn_revnum_t _repos_rev;
public:
  // Initialize ctx (auth batton etc.)
  SvnAccess(const char *auth_username=NULL, const char *auth_password=NULL, svn_wc_notify_func2_t notifyFunc=NULL, void *notifyBaton=NULL);
  ~SvnAccess();

  void Init(const char *auth_username, const char *auth_password);

  // Test whether given folder is under svn version control
  bool IsVersioned(const char *path);

  // Checkout to head revision, returns its revision, false when failed
  bool Checkout(const char *url, const char *destPath);

  // Checkout to with additional parameters, returns its revision
  long int CheckoutEx(const char *url, const char *destPath, const svn_opt_revision_t revision, bool recurse, bool ignore_externals, const svn_opt_revision_t peg_revision);

  // Set upd according to whether new revision of working folder is available using svn status
  // returns true when svn repository access is available 
  bool UpdateAvailable(const char *path, bool &upd);

  bool DiffSummarize(const char *path, int rev1, int rev2);

  // True when update was successful
  bool Update(const char *path, int &result_rev);

  // True when something went wrong
  bool Failed() const { return _fail; }

  long RepositoryRevision() const { return _repos_rev; }

  //
  void SetLastError(svn_error_t *err)
  {
    const char *errmsg;
    if (err)
    {
      errmsg = svn_err_best_message(err, _lastError, 1024);
      strncpy(_lastError, errmsg, 1023);
      _lastError[1023]=0;
    }
    else
     _lastError[0] = 0;  //empty
  }
  const char *GetLastError() const 
  { 
    if ( *_lastError )
      return _lastError; 
    else
      return NULL;
  }
};

#endif
