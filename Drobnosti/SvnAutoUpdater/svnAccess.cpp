// SvnAccess.cpp : 
//

#include <windows.h>
#include "resource.h"
#include "svnAccess.hpp"

#include <string.h>
#include <assert.h>

#include "svn_pools.h"
#include "svn_fs.h"
#include "cl.h"
#include "svn_private_config.h"


/* This implements 'svn_auth_ssl_server_trust_prompt_func_t'.
  Don't actually prompt.  Instead, set *CRED_P to valid credentials
  iff FAILURES is empty or is exactly SVN_AUTH_SSL_UNKNOWNCA.  If
  there are any other failure bits, then set *CRED_P to null (that
  is, reject the cert).

  Ignore MAY_SAVE; we don't save certs we never prompted for.

  Ignore BATON, REALM, and CERT_INFO,
*/
static svn_error_t *ssl_trust_unknown_server_cert
  (svn_auth_cred_ssl_server_trust_t **cred_p,
  void *baton,
  const char *realm,
  apr_uint32_t failures,
  const svn_auth_ssl_server_cert_info_t *cert_info,
  svn_boolean_t may_save,
  apr_pool_t *pool)
{
  *cred_p = NULL;

  if (failures == 0 || failures == SVN_AUTH_SSL_UNKNOWNCA)
  {
    *cred_p = reinterpret_cast<svn_auth_cred_ssl_server_trust_t *>( apr_pcalloc(pool, sizeof(**cred_p)) );
    (*cred_p)->may_save = FALSE;
    (*cred_p)->accepted_failures = failures;
  }

  return SVN_NO_ERROR;
}

//provider accepting server certificate permanently without any user input confirmation
static svn_error_t *my_svn_cmdline_auth_ssl_server_trust_prompt
 (svn_auth_cred_ssl_server_trust_t **cred_p,
  void *baton,
  const char *realm,
  apr_uint32_t failures,
  const svn_auth_ssl_server_cert_info_t *cert_info,
  svn_boolean_t may_save,
  apr_pool_t *pool)
{
  svn_cmdline_prompt_baton2_t *pb = (svn_cmdline_prompt_baton2_t *)baton;
  svn_stringbuf_t *buf = svn_stringbuf_createf
    (pool, _("Error validating server certificate for '%s':\n"), realm);

  if (failures & SVN_AUTH_SSL_UNKNOWNCA)
  {
    svn_stringbuf_appendcstr
      (buf,
      _(" - The certificate is not issued by a trusted authority. Use the\n"
      "   fingerprint to validate the certificate manually!\n"));
  }

  if (failures & SVN_AUTH_SSL_CNMISMATCH)
  {
    svn_stringbuf_appendcstr
      (buf, _(" - The certificate hostname does not match.\n"));
  }

  if (failures & SVN_AUTH_SSL_NOTYETVALID)
  {
    svn_stringbuf_appendcstr
      (buf, _(" - The certificate is not yet valid.\n"));
  }

  if (failures & SVN_AUTH_SSL_EXPIRED)
  {
    svn_stringbuf_appendcstr
      (buf, _(" - The certificate has expired.\n"));
  }

  if (failures & SVN_AUTH_SSL_OTHER)
  {
    svn_stringbuf_appendcstr
      (buf, _(" - The certificate has an unknown error.\n"));
  }

  /*
  cert_info->hostname,
  cert_info->valid_from,
  cert_info->valid_until,
  cert_info->issuer_dname,
  cert_info->fingerprint;
  */

  {
    *cred_p = (svn_auth_cred_ssl_server_trust_t *)apr_pcalloc(pool, sizeof(**cred_p));
    (*cred_p)->may_save = FALSE;
    (*cred_p)->accepted_failures = failures;
  }

  return SVN_NO_ERROR;
}

static svn_error_t * my_svn_cmdline_create_auth_baton(svn_auth_baton_t **ab,
  svn_boolean_t non_interactive,
  const char *auth_username,
  const char *auth_password,
  const char *config_dir,
  svn_boolean_t no_auth_cache,
  svn_boolean_t trust_server_cert,
  svn_config_t *cfg,
  svn_cancel_func_t cancel_func,
  void *cancel_baton,
  apr_pool_t *pool)
{
  svn_boolean_t store_password_val = TRUE;
  svn_boolean_t store_auth_creds_val = TRUE;
  svn_auth_provider_object_t *provider;
  svn_cmdline_prompt_baton2_t *pb = NULL;

  /* The whole list of registered providers */
  apr_array_header_t *providers;
  providers = apr_array_make(pool, 12, sizeof(svn_auth_provider_object_t *));

  /* If we have a cancellation function, cram it and the stuff it
  needs into the prompt baton. */
  if (cancel_func)
  {
    pb = reinterpret_cast<svn_cmdline_prompt_baton2_t *>(apr_palloc(pool, sizeof(*pb)));
    pb->cancel_func = cancel_func;
    pb->cancel_baton = cancel_baton;
    pb->config_dir = config_dir;
  }

  svn_auth_get_simple_provider2(&provider, NULL, NULL, pool);
  APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

  if (non_interactive == FALSE)
  {
    /* Three ssl prompt providers, for server-certs, client-certs,
    and client-cert-passphrases.  */
    svn_auth_get_ssl_server_trust_prompt_provider
      (&provider, my_svn_cmdline_auth_ssl_server_trust_prompt, pb, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
  }
  else if (trust_server_cert)
  {
    /* Remember, only register this provider if non_interactive. */
    svn_auth_get_ssl_server_trust_prompt_provider
      (&provider, ssl_trust_unknown_server_cert, NULL, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
  }

  /* Build an authentication baton to give to libsvn_client. */
  svn_auth_open(ab, providers, pool);

  /* Place any default --username or --password credentials into the
  auth_baton's run-time parameter hash. */
  if (auth_username)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_DEFAULT_USERNAME,
    auth_username);
  if (auth_password)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_DEFAULT_PASSWORD,
    auth_password);

  /* Same with the --non-interactive option. */
  if (non_interactive)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_NON_INTERACTIVE, "");

  if (config_dir)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_CONFIG_DIR,
    config_dir);

  /* Determine whether storing passwords in any form is allowed.
  * This is the deprecated location for this option, the new
  * location is SVN_CONFIG_CATEGORY_SERVERS. The RA layer may
  * override the value we set here. */
  SVN_ERR(svn_config_get_bool(cfg, &store_password_val,
    SVN_CONFIG_SECTION_AUTH,
    SVN_CONFIG_OPTION_STORE_PASSWORDS,
    SVN_CONFIG_DEFAULT_OPTION_STORE_PASSWORDS));

  if (! store_password_val)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_DONT_STORE_PASSWORDS, "");

  /* Determine whether we are allowed to write to the auth/ area.
  * This is the deprecated location for this option, the new
  * location is SVN_CONFIG_CATEGORY_SERVERS. The RA layer may
  * override the value we set here. */
  SVN_ERR(svn_config_get_bool(cfg, &store_auth_creds_val,
    SVN_CONFIG_SECTION_AUTH,
    SVN_CONFIG_OPTION_STORE_AUTH_CREDS,
    SVN_CONFIG_DEFAULT_OPTION_STORE_AUTH_CREDS));

  if (no_auth_cache || ! store_auth_creds_val)
    svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_NO_AUTH_CACHE, "");

#ifdef SVN_HAVE_GNOME_KEYRING
  svn_auth_set_parameter(*ab, SVN_AUTH_PARAM_GNOME_KEYRING_UNLOCK_PROMPT_FUNC,
    &svn_cmdline__auth_gnome_keyring_unlock_prompt);
#endif /* SVN_HAVE_GNOME_KEYRING */

  return SVN_NO_ERROR;
}

/*** Main. ***/

/* Display a prompt and read a one-line response into the provided buffer,
removing a trailing newline if present. */
static svn_error_t* prompt_and_read_line(const char *prompt, char *buffer, size_t max)
{
  int len;
  printf("%s: ", prompt);
  if (fgets(buffer, max, stdin) == NULL) return svn_error_create(0, NULL, "error reading stdin");
  len = strlen(buffer);
  if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = 0;
  return SVN_NO_ERROR;
}

bool SvnAccess::Checkout(const char *url, const char *destPath)
{
  svn_opt_revision_t rev; rev.kind = svn_opt_revision_head;
  (void) CheckoutEx(url, destPath, rev, true, true, rev);
  return !GetLastError();
}

long int SvnAccess::CheckoutEx(const char * url, const char * destPath, const svn_opt_revision_t revision, bool recurse, bool ignore_externals, const svn_opt_revision_t peg_revision)
{
  apr_pool_t *apr_pool = svn_pool_create(NULL);
  svn_revnum_t revnum = 0;

  svn_error_t *err = svn_client_checkout2(&revnum, url, destPath, &peg_revision, &revision, recurse, ignore_externals, _ctx, apr_pool);
  SetLastError(err);

  svn_pool_destroy(apr_pool);
  return revnum;
}

bool SvnAccess::Update(const char *path, int &result_rev)
{
  apr_pool_t * scratch_pool = svn_pool_create(NULL);
  apr_array_header_t *result_revs = apr_array_make(scratch_pool, 0, 0);
  apr_array_header_t *targets = apr_array_make(scratch_pool, 1, sizeof(const char *));
  APR_ARRAY_PUSH(targets, const char *) = path;
  svn_opt_revision_t revision; revision.kind = svn_opt_revision_head;

  svn_error_t *error = svn_client_update4(&result_revs, targets,
    &(revision),
    svn_depth_infinity, //recurse
    false,
    true, //ignore externals
    true, TRUE /* adds_as_modification */,
    false, //parents
    _ctx, scratch_pool);

  if (result_revs->nelts)
    result_rev = APR_ARRAY_IDX(result_revs, 0, int);
  else
    result_rev = -1;
  svn_pool_destroy(scratch_pool);
  return error==NULL; //true when no error
}

static svn_error_t *check_status(void *baton,
  const char *path,
  const svn_client_status_t *status,
  apr_pool_t *pool)
{
  static int count = 1;
  int rev = status->changed_rev;
  int newRev = status->ood_changed_rev;
#ifdef _DEBUG
  const char *stxt = NULL;
  if (status->versioned)
  {
    if (status->node_status!=svn_wc_status_normal) stxt = "!svn_wc_status_normal";
    else if (status->repos_node_status==svn_wc_status_deleted) stxt = "DELETED";
    else if (status->repos_node_status==svn_wc_status_modified) stxt = "MODIFIED";
    else if (status->repos_node_status==svn_wc_status_added) stxt = "ADDED";
    else if (status->repos_node_status==svn_wc_status_conflicted) stxt = "CONFLICTED";
  }
  else
  {
    stxt = "NOT VERSIONED";
  }
  if (stxt) printf("%d->%d ... %s %s\n", rev, newRev, path, stxt);
#endif
  bool *changed = reinterpret_cast<bool *>(baton);
  if (status->versioned && 
    (  status->repos_node_status!=svn_wc_status_none ) )
    *changed = true;
  return EXIT_SUCCESS;
}

static svn_error_t *dummy_status(void *baton,
  const char *path,
  const svn_client_status_t *status,
  apr_pool_t *pool)
{
  return EXIT_SUCCESS;
}

bool SvnAccess::UpdateAvailable(const char *path, bool &upd)
{
  upd = false; //for possible fail

  apr_pool_t *scratch_pool = svn_pool_create(NULL);

  svn_opt_revision_t rev;
  /* Get the working directory current rev */
  rev.kind = svn_opt_revision_committed; // revision of the most recent change
  _repos_rev = SVN_INVALID_REVNUM;
  apr_pool_t *iterpool1 = svn_pool_create(scratch_pool);
  svn_error_t *err = svn_client_status5(&_repos_rev, _ctx, path, &rev,
    svn_depth_infinity,
    false,  //get_all
    true,  //update
    false,  //noignore
    true,
    FALSE /* depth_as_sticky */,
    NULL,   //changelist
    dummy_status, 
    NULL, //check_status uses it as return value to fill in
    iterpool1);

  SetLastError(err);
  if (!err)
  {
    /* We want our -u statuses to be against HEAD. */
    rev.kind = svn_opt_revision_head; // revision of the most recent change

    bool sb = false;
    svn_revnum_t head_rev = SVN_INVALID_REVNUM;
    apr_pool_t *iterpool = svn_pool_create(scratch_pool);
    svn_error_t *err = svn_client_status5(&head_rev, _ctx, path, &rev,
      svn_depth_infinity,
      true,  //verbose
      true,   //update
      false,  //noignore
      true,
      FALSE /* depth_as_sticky */,
      NULL,   //changelist
      check_status, 
      &sb, //check_status uses it as return value to fill in
      iterpool);

    upd = sb;
    SetLastError(err);
  }
  svn_pool_destroy(scratch_pool);
  return (!err);
}

svn_error_t *summarize_diff(
  const svn_client_diff_summarize_t *diff,
  void *baton,
  apr_pool_t *pool)
{
  const char *kind = "";
  switch (diff->summarize_kind)
  {
  case svn_client_diff_summarize_kind_normal: /** An item with no text modifications */
    break;
  case svn_client_diff_summarize_kind_added: /** An added item */
    kind = "ADDED"; break;
  case svn_client_diff_summarize_kind_modified: /** An item with text modifications */
    kind = "MODIFIED"; break;
  case  svn_client_diff_summarize_kind_deleted: /** A deleted item */
    kind = "DELETED"; break;
  }
  //if (diff->summarize_kind==svn_client_diff_summarize_kind_deleted)
    printf("%s ... %s\n", diff->path, kind);
  return EXIT_SUCCESS;
}

bool SvnAccess::DiffSummarize(const char *path, int rev1, int rev2)
{
  apr_pool_t *scratch_pool = svn_pool_create(NULL);

  svn_opt_revision_t revision1;
  revision1.kind = svn_opt_revision_number;
  revision1.value.number = rev1;
  svn_opt_revision_t revision2;
  revision2.kind = svn_opt_revision_number;
  revision2.value.number = rev2;

  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  svn_error_t *err = svn_client_diff_summarize2(path, &revision1, path, &revision2,
    svn_depth_infinity,
    true,            //ignore
    NULL,             //changelist
    summarize_diff,   //callback
    NULL,
    _ctx,
    iterpool);

  SetLastError(err);
  svn_pool_destroy(scratch_pool);
  return (!err);
}

bool SvnAccess::IsVersioned(const char *path)
{
  apr_pool_t *result_pool = svn_pool_create(NULL);
  apr_pool_t *scratch_pool = svn_pool_create(NULL);
  
  const char *url;
  svn_error_t *err = svn_client_url_from_path2(&url, path, _ctx, result_pool, scratch_pool);
  SetLastError(err);
  
  svn_pool_destroy(scratch_pool);
  svn_pool_destroy(result_pool);

  return ( !err && url!=NULL ); 
}

SvnAccess::SvnAccess(const char *auth_username, const char *auth_password, svn_wc_notify_func2_t notifyFunc, void *notifyBaton)
  : _fail(false), _repos_rev(SVN_INVALID_REVNUM)
{
#define CHECK_ERR(err) if (err) { _fail=true; SetLastError(err); return; }

  _lastError[0] = 0;  //empty

  svn_error_t *err;
  const char *myPath = NULL;

  /* Initialize the app.  Send all error messages to 'stderr'.  */
  if (svn_cmdline_init ("minimal_client", NULL) != EXIT_SUCCESS)
  {
    _fail = true;
    return;
  }

  /* Create top-level memory pool. Be sure to read the HACKING file to
  understand how to properly use/free subpools. */
  _pool = svn_pool_create(NULL);

  /* Initialize the FS library. */
  err = svn_fs_initialize (_pool);
  CHECK_ERR(err)

  /* Make sure the ~/.subversion run-time config files exist */
  err = svn_config_ensure (NULL, _pool);
  CHECK_ERR(err)

  /* All clients need to fill out a client_ctx object. */
  {
    /* Initialize and allocate the client_ctx object. */
    err = svn_client_create_context (&_ctx, _pool);
    CHECK_ERR(err)

    _ctx->notify_func2 = notifyFunc;
    _ctx->notify_baton2 = notifyBaton;

    /* Load the run-time config file into a hash */
    err = svn_config_get_config (&(_ctx->config), NULL, _pool);
    CHECK_ERR(err)

#ifdef WIN32
    /* Set the working copy administrative directory name. */
    if (getenv ("SVN_ASP_DOT_NET_HACK")) 
    {
      err = svn_wc_set_adm_dir ("_svn", _pool);
      CHECK_ERR(err)
    }
#endif

/*
#ifdef _CONSOLE
      // Make the client_ctx capable of authenticating users
      prompt_and_read_line("Login", auth_username, sizeof(auth_username));
      prompt_and_read_line("Password", auth_password, sizeof(auth_password));
#endif
*/
    Init(auth_username, auth_password);
  } /* end of client_ctx setup */
}

SvnAccess::~SvnAccess()
{
  svn_pool_destroy(_pool);
}

void SvnAccess::Init(const char *auth_username, const char *auth_password)
{
  _lastError[0] = 0;  //empty

  svn_error_t *err;  err = my_svn_cmdline_create_auth_baton(&_ctx->auth_baton, false, auth_username, auth_password, NULL, true, false, NULL /*TODO*/, NULL, NULL, _pool);
  CHECK_ERR(err)
}
