
/*===============================================================*/
/*  Copyright (C) Microsoft Corporation.  All Rights Reserved.   */
/*===============================================================*/
/*
    WinLive.h
        - Live public header
*/

#ifndef __WINLIVE__
#define __WINLIVE__


#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif


#define LIVE_ON_WINDOWS

#include <wtypes.h>
#include <unknwn.h>
#include <xtl.h>


#ifndef XUSER_INDEX_FOCUS
#define XUSER_INDEX_FOCUS               0x000000FD
#endif
#ifndef XUSER_INDEX_NONE
#define XUSER_INDEX_NONE                0x000000FE
#endif
#ifndef XUSER_INDEX_ANY
#define XUSER_INDEX_ANY                 0x000000FF
#endif

#define E_DEBUGGER_PRESENT MAKE_HRESULT(1, 4, 0x317)

#ifdef __cplusplus
extern "C" {
#endif

typedef void* XLIVE_PROTECTED_BUFFER;
typedef void** XLIVE_PROTECTED_BUFFER_PTR;

HRESULT WINAPI XLivePBufferAllocate(__in ULONG ulSize, __deref_out XLIVE_PROTECTED_BUFFER_PTR pxebBuffer);
HRESULT WINAPI XLivePBufferFree(__in XLIVE_PROTECTED_BUFFER xebBuffer);
HRESULT WINAPI XLivePBufferGetByte(__in XLIVE_PROTECTED_BUFFER xebBuffer, __in ULONG ulOffset, __out UCHAR* pucValue);
HRESULT WINAPI XLivePBufferSetByte(__in XLIVE_PROTECTED_BUFFER xebBuffer, __in ULONG ulOffset, __in UCHAR ucValue);
HRESULT WINAPI XLivePBufferGetDWORD(__in XLIVE_PROTECTED_BUFFER xebBuffer, __in ULONG ulOffset, __out DWORD* pdwValue);
HRESULT WINAPI XLivePBufferSetDWORD(__in XLIVE_PROTECTED_BUFFER xebBuffer, __in ULONG ulOffset, __in DWORD dwValue);
HRESULT WINAPI XLivePBufferGetByteArray(__in XLIVE_PROTECTED_BUFFER xebBuffer, __in ULONG ulOffset, __out_ecount(dwSize) BYTE* pbValues, __in DWORD dwSize);
HRESULT WINAPI XLivePBufferSetByteArray(__in XLIVE_PROTECTED_BUFFER xebBuffer, __in ULONG ulOffset, __in_ecount(dwSize) BYTE* pbValues, __in DWORD dwSize);
HRESULT WINAPI XNetGetCurrentAdapter(__out_ecount_opt(*pcchBuffer) PCHAR pszAdapter, __inout ULONG* pcchBuffer);
HRESULT WINAPI XLiveLoadLibraryEx(__in PWSTR pszModuleFileName, __out HMODULE* phModule, __in DWORD dwFlags);
HRESULT WINAPI XLiveFreeLibrary(__in HMODULE hModule);
HRESULT WINAPI XLiveVerifyDataFile(__in PWSTR pszDataFileName);


#define XLIVE_INITFLAG_USE_ADAPTER_NAME          0x00000001
#define XLIVE_INITFLAG_NO_AUTO_LOGON             0x00000002

typedef struct _XLIVE_INITIALIZE_INFO
{
    UINT cbSize;
    DWORD dwFlags;
    IUnknown* pD3D;
    VOID* pD3DPP;
    LANGID langID;  //Language the Game UI is in.  Live UI will map to this if we support it, otherwise default to English
    WORD wReserved1;
    PCHAR pszAdapterName;
    WORD wLivePortOverride;
    WORD wReserved2;
} XLIVE_INITIALIZE_INFO;

typedef struct _XLIVE_INPUT_INFO
{
    UINT cbSize;
    HWND hWnd;
    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
    BOOL fHandled;
    LRESULT lRet;
} XLIVE_INPUT_INFO;

HRESULT WINAPI XLiveInitialize(__in XLIVE_INITIALIZE_INFO* pXii);
void WINAPI XLiveUnInitialize();
HRESULT WINAPI XLiveRender();
BOOL WINAPI XLivePreTranslateMessage(__in const LPMSG lpMsg);
HRESULT WINAPI XLiveInput(__inout XLIVE_INPUT_INFO* pXii);
HRESULT WINAPI XLiveOnCreateDevice(__in IUnknown* pD3D, __in VOID* pD3DPP);
HRESULT WINAPI XLiveOnDestroyDevice();
HRESULT WINAPI XLiveOnResetDevice(__in VOID* pD3DPP);
BOOL WINAPI XCloseHandle(__in HANDLE hObject);

HRESULT WINAPI XLiveProtectData(
    __in_ecount(dwSizeOfDataToProtect) BYTE* pabDataToProtect,
    __in DWORD dwSizeOfDataToProtect,
    __out_ecount_part_opt(*pdwSizeOfProtectedData, *pdwSizeOfProtectedData) BYTE* pabProtectedData,
    __inout DWORD* pdwSizeOfProtectedData,
    __in HANDLE hProtectedData
);

HRESULT WINAPI XLiveUnprotectData(
    __in_ecount(dwSizeOfProtectedData) BYTE* pabProtectedData,
    __in DWORD dwSizeOfProtectedData,
    __out_ecount_part_opt(*pdwSizeOfData, *pdwSizeOfData) BYTE* pabData,
    __inout DWORD* pdwSizeOfData,
    __out PHANDLE phProtectedData
);

//
// The following flag is used upon input to XLiveCreateProtectedDataContext to
// signal that the resultant context handle will always operate on data that is
// not associated with a LIVE-enabled profile.  Thus, regardless of whether the
// user is signed into a LIVE-enabled profile at the time XLiveProtectData is
// called, the protected data will be marked as offline-only (hence, ineligible
// for use in garnering achievements).
//
// The flag is set upon output from XLiveQueryProtectedDataInformation to
// indicate whether data protected using the handle is flagged as offline-only.
// This may be the result of the offline-only designation mandated by the
// caller of XLiveCreateProtectedDataContext, as described above.  Alternately, 
// it may be due to the fact that the handle was used in a call to 
// XLiveProtectData when the user was not signed into a LIVE-enabled profile.  
// Regardless of the reason, once this flag is set, it can never be cleared.
//
#define XLIVE_PROTECTED_DATA_FLAG_OFFLINE_ONLY    0x1

typedef struct _XLIVE_PROTECTED_DATA_INFORMATION {
  DWORD cbSize;   // must be initialized to sizeof(XLIVE_PROTECTED_DATA_INFORMATION)
  DWORD dwFlags;  // XLIVE_PROTECTED_DATA_FLAG_* values
} XLIVE_PROTECTED_DATA_INFORMATION, *PXLIVE_PROTECTED_DATA_INFORMATION;

HRESULT WINAPI XLiveCreateProtectedDataContext(
  __in PXLIVE_PROTECTED_DATA_INFORMATION pProtectedDataInfo,
  __out PHANDLE phProtectedData
);

HRESULT WINAPI XLiveQueryProtectedDataInformation (
  __in HANDLE hProtectedData,
  __out PXLIVE_PROTECTED_DATA_INFORMATION pProtectedDataInfo
);

HRESULT WINAPI XLiveCloseProtectedDataContext (
  __in HANDLE hProtectedData
);

typedef struct _XLIVEUPDATE_INFORMATION {
    DWORD cbSize;        // on input, must be set to sizeof(XLIVEUPDATE_INFORMATION)
    BOOL  bSystemUpdate; // TRUE for system update, FALSE for title update
    DWORD dwFromVersion; // version of system/title updated by this title update package
    DWORD dwToVersion;   // version that the system/title will be updated to
    WCHAR szUpdateDownloadPath[MAX_PATH];        // directory containing update
} XLIVEUPDATE_INFORMATION, *PXLIVEUPDATE_INFORMATION;

HRESULT
WINAPI
XLiveGetUpdateInformation(
    __inout PXLIVEUPDATE_INFORMATION pXLiveUpdateInfo
);

HRESULT
WINAPI
XLiveUpdateSystem (
    __in_opt LPCWSTR lpszRelaunchCmdLine
);

typedef enum _XLIVE_DEBUG_LEVEL
{
    XLIVE_DEBUG_LEVEL_OFF = 0,      // No debug output
    XLIVE_DEBUG_LEVEL_ERROR,        // Error only debug output
    XLIVE_DEBUG_LEVEL_WARNING,      // Warnings and error debug output
    XLIVE_DEBUG_LEVEL_INFO,         // Info, warning and error debug output
    XLIVE_DEBUG_LEVEL_DEFAULT,      // Reset level to whatever is in registry
}XLIVE_DEBUG_LEVEL;

HRESULT WINAPI XLiveSetDebugLevel(__in XLIVE_DEBUG_LEVEL xdlLevel, __out_opt XLIVE_DEBUG_LEVEL* pxdlOldLevel);


// maximum number of players that can be signed in at the same time.
#define XSIGNIN_MAX_COUNT           1

#define LIVEID_MEMBERNAME_MAX       113
#define LIVEID_PASSWORD_MAX         16

#define XLMGRCREDS_FLAG_SAVE                0x00000001
#define XLMGRCREDS_FLAG_DELETE              0x00000002

#define XLSIGNIN_FLAG_SAVECREDS             0x00000001
#define XLSIGNIN_FLAG_ALLOWTITLEUPDATES     0x00000002
#define XLSIGNIN_FLAG_ALLOWSYSTEMUPDATES    0x00000004

//Warning: Once the credentials have been used the memory containing the strings will be Zeroed
HRESULT WINAPI XLiveManageCredentials(
    __nullterminated LPWSTR lpszLiveIdName,
    __nullterminated LPWSTR lpszLiveIdPassword,
    __in DWORD dwCredFlags,
    __inout_opt PXOVERLAPPED pXOverlapped);

//Warning: Once the credentials have been used the memory containing the strings will be Zeroed
HRESULT WINAPI XLiveSignin(
    __nullterminated LPWSTR lpszLiveIdName,
    __nullterminated LPWSTR lpszLiveIdPassword,
    __in DWORD dwFlags,
    __inout_opt PXOVERLAPPED pXOverlapped);

HRESULT WINAPI XLiveSignout (__inout_opt PXOVERLAPPED pXOverlapped);


HRESULT WINAPI XLiveGetLiveIdError(
    __out HRESULT *phrAuthState,
    __out HRESULT *phrRequestState,
    __out_ecount_part(*pdwUrlLen,*pdwUrlLen) LPWSTR   wszWebFlowUrl,
    __inout LPDWORD  pdwUrlLen);


// HRESULTs returned from XLiveGetLiveIdError

#define LIVEID_HRESULT_BASE    0x8800

#define LIVEID_SUCCESS_CODE(x) MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, LIVEID_HRESULT_BASE|(x))
#define LIVEID_ERROR_CODE(x) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, LIVEID_HRESULT_BASE|(x))

//
// Authentication State
//

//
// Failure codes are 0x800488XX, success codes are 0x000488XX
//

#define LIVEID_AUTHSTATE_E_UNAUTHENTICATED                           LIVEID_ERROR_CODE(0x00)  // 0x80048800

// returned by STS in as auth state, when the DA ticket is expired
#define LIVEID_AUTHSTATE_E_EXPIRED                                   LIVEID_ERROR_CODE(0x01) 
#define LIVEID_AUTHSTATE_S_AUTHENTICATED_OFFLINE                     LIVEID_SUCCESS_CODE(0x02) // Offline login from hashed creds
#define LIVEID_AUTHSTATE_S_AUTHENTICATED_PASSWORD                    LIVEID_SUCCESS_CODE(0x03) // Online login with password, Returned by the server?

#define LIVEID_AUTHREQUIRED_E_PASSWORD                               LIVEID_ERROR_CODE(0x10) // password is required for authentication
//Reserved
#define LIVEID_AUTHREQUIRED_E_SECURITY_KEY                           LIVEID_ERROR_CODE(0x11)
#define LIVEID_AUTHREQUIRED_E_CERTIFICATE                            LIVEID_ERROR_CODE(0x13)

// The auth required status is unknown, in case, we cannot retrieve it from the token bag
#define LIVEID_AUTHREQUIRED_E_UNKNOWN                                LIVEID_ERROR_CODE(0x14)

#define LIVEID_REQUEST_E_AUTH_SERVER_ERROR                           LIVEID_ERROR_CODE(0x20) // Request failure, IDCRL did not get back a response
#define LIVEID_REQUEST_E_BAD_MEMBER_NAME_OR_PASSWORD                 LIVEID_ERROR_CODE(0x21)
#define LIVEID_REQUEST_E_PASSWORD_LOCKED_OUT                         LIVEID_ERROR_CODE(0x23)
#define LIVEID_REQUEST_E_PASSWORD_LOCKED_OUT_BAD_PASSWORD_OR_HIP     LIVEID_ERROR_CODE(0x24)
#define LIVEID_REQUEST_E_TOU_CONSENT_REQUIRED                        LIVEID_ERROR_CODE(0x25) // not used
#define LIVEID_REQUEST_E_FORCE_RENAME_REQUIRED                       LIVEID_ERROR_CODE(0x26)
#define LIVEID_REQUEST_E_FORCE_CHANGE_PASSWORD_REQUIRED              LIVEID_ERROR_CODE(0x27)
#define LIVEID_REQUEST_E_STRONG_PASSWORD_REQUIRED                    LIVEID_ERROR_CODE(0x28)
#define LIVEID_REQUEST_E_NO_CERTIFICATES_AVAILABLE                   LIVEID_ERROR_CODE(0x29)
#define LIVEID_REQUEST_E_PARTNER_NOT_FOUND                           LIVEID_ERROR_CODE(0x2a)
#define LIVEID_REQUEST_E_PARTNER_HAS_NO_ASYMMETRIC_KEY               LIVEID_ERROR_CODE(0x2b)
#define LIVEID_REQUEST_E_INVALID_POLICY                              LIVEID_ERROR_CODE(0x2c)
#define LIVEID_REQUEST_E_INVALID_MEMBER_NAME                         LIVEID_ERROR_CODE(0x2d)
#define LIVEID_REQUEST_E_MISSING_PRIMARY_CREDENTIAL                  LIVEID_ERROR_CODE(0x2e)
#define LIVEID_REQUEST_E_PENDING_NETWORK_REQUEST                     LIVEID_ERROR_CODE(0x2f)
#define LIVEID_REQUEST_E_FORCE_CHANGE_SQSA                           LIVEID_ERROR_CODE(0x30)
#define LIVEID_REQUEST_E_PASSWORD_EXPIRED                            LIVEID_ERROR_CODE(0x31)
#define LIVEID_REQUEST_E_PENDING_USER_INPUT                          LIVEID_ERROR_CODE(0x32)
#define LIVEID_REQUEST_E_MISSING_HIP_SOLUTION                        LIVEID_ERROR_CODE(0x33)
#define LIVEID_REQUEST_E_PROFILE_ACCRUE_REQUIRED                     LIVEID_ERROR_CODE(0x34)
#define LIVEID_REQUEST_S_PROFILE_ACCRUE_DONE                         LIVEID_SUCCESS_CODE(0x35)
#define LIVEID_REQUEST_E_EMAIL_VALIDATION_REQUIRED                   LIVEID_ERROR_CODE(0x36)
#define LIVEID_REQUEST_E_PARTNER_NEED_STRONGPW                       LIVEID_ERROR_CODE(0x37)
#define LIVEID_REQUEST_E_PARTNER_NEED_STRONGPW_EXPIRY                LIVEID_ERROR_CODE(0x38)
//Login server sends this request status when DA ticket is expired, apps should use this request
// status to determine if the ticket is expired.
#define LIVEID_REQUEST_E_AUTH_EXPIRED                                LIVEID_ERROR_CODE(0x39)

#define LIVEID_REQUEST_E_USER_REQUESTED_HELP                         LIVEID_ERROR_CODE(0x40)
#define LIVEID_REQUEST_E_USER_FORGOT_PASSWORD                        LIVEID_ERROR_CODE(0x41)
#define LIVEID_REQUEST_E_USER_CANCELED                               LIVEID_ERROR_CODE(0x42)
#define LIVEID_REQUEST_E_USER_EDIT_PASSPORT                          LIVEID_ERROR_CODE(0x43)
#define LIVEID_REQUEST_E_USER_PASSPORTLOGO                           LIVEID_ERROR_CODE(0x44)
#define LIVEID_REQUEST_E_USER_SHOW_PRIVACY_STATEMENT                 LIVEID_ERROR_CODE(0x45)
#define LIVEID_REQUEST_E_USER_SHOW_TERMS_OF_USE                      LIVEID_ERROR_CODE(0x46)
#define LIVEID_REQUEST_S_IO_PENDING                                  LIVEID_SUCCESS_CODE(0x47)
#define LIVEID_REQUEST_E_NO_NETWORK                                  LIVEID_ERROR_CODE(0x48)
// The request status is unknown, in case, we cannot retrieve it from the token bag
#define LIVEID_REQUEST_E_UNKNOWN                                     LIVEID_ERROR_CODE(0x49)

// reserved
#define LIVEID_REQUEST_E_TOKEN_BEYOND_LIFTIME                        LIVEID_ERROR_CODE(0x50)

// reserved
#define LIVEID_REQUEST_E_TOKEN_TARGETS_MISMATCH                      LIVEID_ERROR_CODE(0x51)

//  indicate the client needs to re-post to another STS
//  STS url should be part of the response
#define LIVEID_REQUEST_E_WRONG_DA                                    LIVEID_ERROR_CODE(0x52)

// parent consent for the KID is required
#define LIVEID_REQUEST_E_KID_HAS_NO_CONSENT                          LIVEID_ERROR_CODE(0x53)

// new error code 3.2
#define LIVEID_REQUEST_E_RSTR_MISSING_REFERENCE_URI                  LIVEID_ERROR_CODE(0x54)
//reserved
#define LIVEID_REQUEST_E_RSTR_FAULT                                  LIVEID_ERROR_CODE(0x55)
#define LIVEID_REQUEST_E_RSTR_MISSING_REFERENCED_TOKEN               LIVEID_ERROR_CODE(0x56)
#define LIVEID_REQUEST_E_RSTR_MISSING_BASE64CERT                     LIVEID_ERROR_CODE(0x57)
#define LIVEID_REQUEST_E_RSTR_MISSING_TOKENTYPE                      LIVEID_ERROR_CODE(0x58)
#define LIVEID_REQUEST_E_RSTR_MISSING_SERVICENAME                    LIVEID_ERROR_CODE(0x59)
#define LIVEID_REQUEST_E_RSTR_INVALID_TOKENTYPE                      LIVEID_ERROR_CODE(0x5a)
#define LIVEID_REQUEST_E_RSTR_MISSING_PRIVATE_KEY                    LIVEID_ERROR_CODE(0x5b)

// This error is returned if the STS cannot verify the timestamp on the request. In this case,
// STS will also send back the server time and IDCRL can adjust 
#define LIVEID_REQUEST_E_INVALID_SERVICE_TIMESTAMP                   LIVEID_ERROR_CODE(0x5c) //0x8004885c
#define LIVEID_REQUEST_E_INVALID_PKCS10_TIMESTAMP                    LIVEID_ERROR_CODE(0x5d) //0x8004885d
#define LIVEID_REQUEST_E_INVALID_PKCS10                              LIVEID_ERROR_CODE(0x5e) //0x8004885e
// NOTE: 2nd range for LIVEID_REQUEST_E errors starts at 0x800488E0. This one is full!

#define LIVEID_S_NO_MORE_IDENTITIES                                  LIVEID_SUCCESS_CODE(0x60)
#define LIVEID_S_TOKEN_TYPE_DOES_NOT_SUPPORT_SESSION_KEY             LIVEID_SUCCESS_CODE(0x61)
#define LIVEID_S_NO_SUCH_CREDENTIAL                                  LIVEID_SUCCESS_CODE(0x62)
#define LIVEID_S_NO_AUTHENTICATION_REQUIRED                          LIVEID_SUCCESS_CODE(0x63)

#define LIVEID_E_AUTH_CONTEXT_ALREADY_IN_USE                         LIVEID_ERROR_CODE(0x60)
#define LIVEID_E_IDENTITY_NOT_AUTHENTICATED                          LIVEID_ERROR_CODE(0x61)
#define LIVEID_E_UNABLE_TO_RETRIEVE_SERVICE_TOKEN                    LIVEID_ERROR_CODE(0x62)

#define LIVEID_E_CERTIFICATE_AUTHENTICATION_NOT_SUPPORTED            LIVEID_ERROR_CODE(0x68)
#define LIVEID_E_AUTH_SERVICE_UNAVAILABLE                            LIVEID_ERROR_CODE(0x69)
#define LIVEID_E_INVALID_AUTH_SERVICE_RESPONSE                       LIVEID_ERROR_CODE(0x6a)
#define LIVEID_E_UNABLE_TO_INITIALIZE_CRYPTO_PROVIDER                LIVEID_ERROR_CODE(0x6b)
#define LIVEID_E_NO_MEMBER_NAME_SET                                  LIVEID_ERROR_CODE(0x6c)
#define LIVEID_E_CALLBACK_REQUIRED                                   LIVEID_ERROR_CODE(0x6d)
#define LIVEID_E_DISCONTINUE_AUTHENTICATION                          LIVEID_ERROR_CODE(0x6e)
#define LIVEID_E_INVALIDFLAGS                                        LIVEID_ERROR_CODE(0x6f)
#define LIVEID_E_UNABLE_TO_RETRIEVE_CERT                             LIVEID_ERROR_CODE(0x70)
#define LIVEID_E_INVALID_RSTPARAMS                                   LIVEID_ERROR_CODE(0x71)
#define LIVEID_E_MISSING_FILE                                        LIVEID_ERROR_CODE(0x72)
#define LIVEID_E_ILLEGAL_LOGONIDENTITY_FLAG                          LIVEID_ERROR_CODE(0x73)

#define LIVEID_E_CERT_NOT_VALID_FOR_MINTTL                           LIVEID_ERROR_CODE(0x74) // the certificate is found, but not valid for the minTTL, minTTL could be too big
#define LIVEID_S_OK_CLIENTTIME                                       LIVEID_SUCCESS_CODE(0x75)

#define LIVEID_E_CERT_INVALID_ISSUER                                 LIVEID_ERROR_CODE(0x76) // the certificate is found, but not valid for the minTTL, minTTL could be too big
#define LIVEID_E_NO_CERTSTORE_FOR_ISSUERS                            LIVEID_ERROR_CODE(0x77)

// Indicates there was a problem with offline login, more information can be had by calling GetAuthState API.
#define LIVEID_E_OFFLINE_AUTH                                        LIVEID_ERROR_CODE(0x78)

// GetCertificate failed because POP message could not be signed. This is an unexepcted error.
#define LIVEID_E_SIGN_POP_FAILED                                    LIVEID_ERROR_CODE(0x79)

// VerifyCertificate failed because POP was invalid. Sender may not possess the private key
// for the certificate
#define LIVEID_E_CERT_INVALID_POP                                    LIVEID_ERROR_CODE(0x80)

// The calling application is not signed. In order to call GetCertificate API, the application exe
// must be signed with Microsoft Code signing certificate (https://codesignaoc for more details)
// 
#define LIVEID_E_CALLER_NOT_SIGNED                                   LIVEID_ERROR_CODE(0x81) ////0x80048881

// Identity is processing another API call. Please wait for some time and retry the call
#define LIVEID_E_BUSY                                                LIVEID_ERROR_CODE(0x82) ////0x80048882

// There was a failure while downloading the config or ui dlls from passport
// config servers. Most likely internet connection is not setup correctly.
#define LIVEID_E_DOWNLOAD_FILE_FAILED                                LIVEID_ERROR_CODE(0x83) ////0x80048883

// There was an error generating the certificate request. The error could be due
// to bad parameters supplied by the application (usually out of range keylength)
// See CryptGenKey API for allowed key lengths. 0 and 384-16,384 will usually
// work. Trace will provide more info on exact cause.
#define LIVEID_E_BUILD_CERT_REQUEST_FAILED                           LIVEID_ERROR_CODE(0x84) ////0x80048884

// Returned by GetCertificate API if it cannot find a certificate for the identity
#define LIVEID_E_CERTIFICATE_NOT_FOUND                               LIVEID_ERROR_CODE(0x85) ////0x80048885

// Returned by ExportAuthState API
#define LIVEID_E_AUTHBLOB_TOO_LARGE                                  LIVEID_ERROR_CODE(0x86) ////0x80048886
#define LIVEID_E_AUTHBLOB_NOT_FOUND                                  LIVEID_ERROR_CODE(0x87) ////0x80048887
#define LIVEID_E_AUTHBLOB_INVALID                                    LIVEID_ERROR_CODE(0x88) ////0x80048888

#define LIVEID_E_EXTPROP_NOTFOUND                                    LIVEID_ERROR_CODE(0x89) ////0x80048889

#define LIVEID_E_RESPONSE_TOO_LARGE                                   LIVEID_ERROR_CODE(0x8A) ////0x8004888A

#define LIVEID_E_EXTENDED_ERROR_NOT_SET                              LIVEID_ERROR_CODE(0x8B) ////0x8004888B

#define LIVEID_E_USER_NOTFOUND                                       LIVEID_ERROR_CODE(0x8C) ////0x8004888C

// Added in 4.0. Returned by Initialize(Ex) if signature on ppcrlconfig.dll or
// ppcrlui.dll cannot be validated.
#define LIVEID_E_SIGCHECK_FAILED                                     LIVEID_ERROR_CODE(0x8D) ////0x8004888D

// added in 4.5. Returned by AuthIdentitytoService if flag SERVICE_TOKEN_FROM_CACHE 

// invalid credential target name( too long, empty )
#define LIVEID_E_CREDTARGETNAME_INVALID                                LIVEID_ERROR_CODE(0x8F) ////0x8004888F

// persisted credential invalid 
#define LIVEID_E_CREDINFO_CORRUPTED                                LIVEID_ERROR_CODE(0x90) ////0x8004888F

//******************************************************************************
// 2nd range of request status codes starts with 0x800488E0

// User requested offline login but there was no hashed password. They must 
// create hashed password first by doing a successful online login.
#define LIVEID_REQUEST_E_MISSING_HASHED_PASSWORD                   LIVEID_ERROR_CODE(0xE0) ////0x800488E0

// The client issuing this request is no longer supported by passport login server. 
// User must upgrade to a newer version of the application built with a supported 
// version of IDCRL. It may be returned in request status by LogonIdentity* APIs.
#define LIVEID_REQUEST_E_CLIENT_DEPRECATED                         LIVEID_ERROR_CODE(0xE1) ////0x800488E1

// The request was cancelled by calling CancelPendingRequest API.
#define LIVEID_REQUEST_E_CANCELLED                                 LIVEID_ERROR_CODE(0xE2) ////0x800488E2

// Application provided incorrect key length for certificate request.
#define LIVEID_REQUEST_E_INVALID_PKCS10_KEYLEN                     LIVEID_ERROR_CODE(0xE3) ////0x800488E3

// Application tried to submit a request with duplicate service target names in PCMultiRSTParams parameter. 
// This is not supported. In this case no network call is made. This error can be returned by the 
// LogonIdentityEx or AuthIdentityToServiceEx fuctions.
#define  LIVEID_REQUEST_E_DUPLICATE_SERVICETARGET                  LIVEID_ERROR_CODE(0xE4) ////0x800488E4

// The authentication token has expired, re-authentication is required by the partner. Different partners
// choose different time windows for authentication. In a multiple request, some requests may succeed
// and others fail with this error.
#define LIVEID_REQUEST_E_FORCE_SIGNIN                              LIVEID_ERROR_CODE(0xE5) ////0x800488E5

// Partner needs a certificate to login.
#define LIVEID_REQUEST_E_PARTNER_NEED_CERTIFICATE                  LIVEID_ERROR_CODE(0xE6) ////0x800488E6

// Partner needs a PIN to login.
#define LIVEID_REQUEST_E_PARTNER_NEED_PIN                          LIVEID_ERROR_CODE(0xE7) ////0x800488E7

// Partner needs a password to login.
#define LIVEID_REQUEST_E_PARTNER_NEED_PASSWORD                     LIVEID_ERROR_CODE(0xE8) ////0x800488E8

// Following two error codes are returned by LogonIdentityEx, AuthIdentityToServiceEx
// if we are unable to generate the SLC request (this will happen for guest users)
// In this case IDCRL automatically submits the request without the SLC and returned
// one of these return codes. LIVEID_S_OK_NO_SLC for sync and LIVEID_S_IO_PENDING_NO_SLC for async
#define LIVEID_REQUEST_S_OK_NO_SLC                                           LIVEID_SUCCESS_CODE(0xE9)
#define LIVEID_REQUEST_S_IO_PENDING_NO_SLC                                   LIVEID_SUCCESS_CODE(0xEA)


// There was an schannel failure while sending the request. Make sure SSL is enabled in IE.
#define LIVEID_REQUEST_E_SCHANNEL_ERROR                            LIVEID_ERROR_CODE(0xEB) ////0x800488EB

// There was an error parsing the certificate blob in the response.
#define LIVEID_REQUEST_E_CERT_PARSE_ERROR                       LIVEID_ERROR_CODE(0xEC) ////0x800488EC


#define MID_SPONSOR_TOKEN_SIZE             29

HRESULT WINAPI XLiveSetSponsorToken(
    __in_ecount(MID_SPONSOR_TOKEN_SIZE) LPCWSTR pwszToken,
    __in DWORD dwTitleId);

HRESULT WINAPI XLiveUninstallTitle(DWORD dwTitleId);

HRESULT WINAPI XLiveGetLocalOnlinePort(__out WORD *pwPort);

#ifndef NONET

//-----------------------------------------------------------
// LocatorService for dedicated servers
//-----------------------------------------------------------

// Predefined dedicated server types
#define XLOCATOR_SERVERTYPE_PUBLIC          0   // dedicated server is for all players.
#define XLOCATOR_SERVERTYPE_GOLD_ONLY       1   // dedicated server is for Gold players only.
#define XLOCATOR_SERVERTYPE_PEER_HOSTED     2   // dedicated server is a peer-hosted game server.
#define XLOCATOR_SERVICESTATUS_PROPERTY_START     0x100

// Property IDs for locator service status

// Total online servers.
#define X_PROPERTY_SERVICESTATUS_SERVERCOUNT_TOTAL          XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_SERVICESTATUS_PROPERTY_START + 1)

// public servers (allow any user to play)
#define X_PROPERTY_SERVICESTATUS_SERVERCOUNT_PUBLIC         XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_SERVICESTATUS_PROPERTY_START + 2)

// gold-only servers (only allow gold users to play)
#define X_PROPERTY_SERVICESTATUS_SERVERCOUNT_GOLDONLY       XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_SERVICESTATUS_PROPERTY_START + 3)

// peer hosted servers (for invite only)
#define X_PROPERTY_SERVICESTATUS_SERVERCOUNT_PEERHOSTED     XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_SERVICESTATUS_PROPERTY_START + 4)

// Compare operators for dedicated server search.
typedef enum _XTS_FILTER_COMPARE_OPERATOR
{
    XTS_FILTER_COMPARE_OPERATOR_None = 0,               // this operator is reserved.
    XTS_FILTER_COMPARE_OPERATOR_Equals = 1,             // property = value
    XTS_FILTER_COMPARE_OPERATOR_NotEquals = 2,          // property <> value
    XTS_FILTER_COMPARE_OPERATOR_GreaterThan = 4,        // property > value
    XTS_FILTER_COMPARE_OPERATOR_GreaterEqualThan = 8,   // property >= value
    XTS_FILTER_COMPARE_OPERATOR_LessThan = 16,          // property < value
    XTS_FILTER_COMPARE_OPERATOR_LessEqualThan = 32,     // property <= value
    XTS_FILTER_COMPARE_OPERATOR_Contains = 64,          // CONTAINS(property, value)
}XTS_FILTER_COMPARE_OPERATOR;

// A filter for dedicated server search.
typedef struct _XLOCATOR_FILTER {
    XUSER_PROPERTY userProperty;                        // The property ID and the value are combined with eComparator
    XTS_FILTER_COMPARE_OPERATOR eComparator;
} XLOCATOR_FILTER, *PXLOCATOR_FILTER;

// A group of filters for dedicated server search.
// The filters in one filter group are combined with 'OR' operator.
// Each filter group are combined with other groups with 'AND' operator.
typedef struct _XLOCATOR_FILTER_GROUP {
    UINT cFilterCount;                                  // Number of filters in this group
    PXLOCATOR_FILTER pFilters;
} XLOCATOR_FILTER_GROUP, *PXLOCATOR_FILTER_GROUP;

#define XLOCATOR_SORT_ASCENDING     0       // sorted in ascending order, from lowest value to highest value
#define XLOCATOR_SORT_DESCENDING    1       // sorted in descending order, from highest value to lowest value

typedef struct _XLOCATOR_SORTER {
    DWORD dwPropertyId;                     // id of the field to be sorted.
    DWORD dwSortDirection;                  // should be XLOCATOR_SORT_ASCENDING or XLOCATOR_SORT_DESCENDING
} XLOCATOR_SORTER, *PXLOCATOR_SORTER;

// result of a dedicated server search.
typedef struct _XLOCATOR_SEARCHRESULT {
    ULONGLONG serverID;                     // the ID of dedicated server
    DWORD dwServerType;                     // see XLOCATOR_SERVERTYPE_PUBLIC, etc
    XNADDR serverAddress;                   // the address dedicated server
    XNKID xnkid;
    XNKEY xnkey;
    DWORD dwMaxPublicSlots;
    DWORD dwMaxPrivateSlots;
    DWORD dwFilledPublicSlots;
    DWORD dwFilledPrivateSlots;
    DWORD cProperties;                      // number of custom properties.
    PXUSER_PROPERTY pProperties;            // an array of custom properties.
} XLOCATOR_SEARCHRESULT, *PXLOCATOR_SEARCHRESULT;

#define SERVER_MASK_MAX         32
typedef struct _XLOCATOR_INIT_INFO
{
    DWORD dwTitleId;
    DWORD dwServiceId;
    WORD  wDefaultPort;
    char  szServiceMask[SERVER_MASK_MAX];
}XLOCATOR_INIT_INFO, *PXLOCATOR_INIT_INFO;

// fields that supports search in a dedicated server search
enum XTS_SEARCH_FIELD
{
    XTS_SEARCH_FIELD_ServerIdentity = 1,
    XTS_SEARCH_FIELD_ServerType = 2,
    XTS_SEARCH_FIELD_MaxPublicSlots = 3,
    XTS_SEARCH_FIELD_MaxPrivateSlots = 4,
    XTS_SEARCH_FIELD_AvailablePublicSlots = 5,
    XTS_SEARCH_FIELD_AvailablePrivateSlots = 6,
    XTS_SEARCH_FIELD_ReservedInt1 = 7,
    XTS_SEARCH_FIELD_ReservedInt2 = 8,
    XTS_SEARCH_FIELD_ReservedInt3 = 9,
    XTS_SEARCH_FIELD_ReservedInt4 = 10,
    XTS_SEARCH_FIELD_ReservedInt5 = 11,
    XTS_SEARCH_FIELD_ReservedInt6 = 12,
    XTS_SEARCH_FIELD_ReservedInt7 = 13,
    XTS_SEARCH_FIELD_ReservedInt8 = 14,
    XTS_SEARCH_FIELD_ReservedInt9 = 15,
    XTS_SEARCH_FIELD_ReservedInt10 = 16,
    XTS_SEARCH_FIELD_ReservedInt11 = 17,
    XTS_SEARCH_FIELD_ReservedInt12 = 18,
    XTS_SEARCH_FIELD_ReservedInt13 = 19,
    XTS_SEARCH_FIELD_ReservedInt14 = 20,
    XTS_SEARCH_FIELD_ReservedInt15 = 21,
    XTS_SEARCH_FIELD_ReservedInt16 = 22,
    XTS_SEARCH_FIELD_ReservedInt17 = 23,
    XTS_SEARCH_FIELD_ReservedInt18 = 24,
    XTS_SEARCH_FIELD_ReservedInt19 = 25,
    XTS_SEARCH_FIELD_ReservedInt20 = 26,
    XTS_SEARCH_FIELD_ReservedLongLong1 = 27,
    XTS_SEARCH_FIELD_ReservedLongLong2 = 28,
    XTS_SEARCH_FIELD_ReservedLongLong3 = 29,
    XTS_SEARCH_FIELD_ReservedLongLong4 = 30,
    XTS_SEARCH_FIELD_ReservedLongLong5 = 31,
    XTS_SEARCH_FIELD_ReservedLongLong6 = 32,
    XTS_SEARCH_FIELD_ReservedLongLong7 = 33,
    XTS_SEARCH_FIELD_ReservedLongLong8 = 34,
    XTS_SEARCH_FIELD_ReservedLongLong9 = 35,
    XTS_SEARCH_FIELD_ReservedLongLong10 = 36,
    XTS_SEARCH_FIELD_ReservedString1 = 37,
    XTS_SEARCH_FIELD_ReservedString2 = 38,
    XTS_SEARCH_FIELD_ReservedString3 = 39,
    XTS_SEARCH_FIELD_ReservedString4 = 40,
    XTS_SEARCH_FIELD_ReservedString5 = 41,
    XTS_SEARCH_FIELD_ReservedString6 = 42,
    XTS_SEARCH_FIELD_ReservedString7 = 43,
    XTS_SEARCH_FIELD_ReservedString8 = 44,
    XTS_SEARCH_FIELD_ReservedString9 = 45,
    XTS_SEARCH_FIELD_ReservedString10 = 46,
    XTS_SEARCH_FIELD_OwnerXuid = 47,
    XTS_SEARCH_FIELD_OwnerGamerTag = 48,
    XTS_SEARCH_FIELD_RegionID = 49,
    XTS_SEARCH_FIELD_LanguageID = 50,
    XTS_SEARCH_FIELD_FilledPublicSlots = 51,
    XTS_SEARCH_FIELD_FilledPrivateSlots = 52,
};

#define XLOCATOR_DEDICATEDSERVER_PROPERTY_START     0x200

// These properties are used for search only.
// The search result header should already contains the information, and the query should not request these properties again.
#define X_PROPERTY_DEDICATEDSERVER_IDENTITY             XPROPERTYID(1, XUSER_DATA_TYPE_INT64,  XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ServerIdentity)   // server id. supports '=' operator only.
#define X_PROPERTY_DEDICATEDSERVER_TYPE                 XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ServerType)
#define X_PROPERTY_DEDICATEDSERVER_MAX_PUBLIC_SLOTS     XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_MaxPublicSlots)
#define X_PROPERTY_DEDICATEDSERVER_MAX_PRIVATE_SLOTS    XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_MaxPrivateSlots)
#define X_PROPERTY_DEDICATEDSERVER_AVAILABLE_PUBLIC_SLOTS   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_AvailablePublicSlots)
#define X_PROPERTY_DEDICATEDSERVER_AVAILABLE_PRIVATE_SLOTS  XPROPERTYID(1, XUSER_DATA_TYPE_INT32,  XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_AvailablePrivateSlots)
#define X_PROPERTY_DEDICATEDSERVER_FILLED_PUBLIC_SLOTS      XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_FilledPublicSlots)
#define X_PROPERTY_DEDICATEDSERVER_FILLED_PRIVATE_SLOTS     XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_FilledPrivateSlots)

// These properties are used for both advertise and search.
// To retrieve these properties, please set the property IDs in the pRequiredPropertyIDs parameter.
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT1   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt1)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT2   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt2)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT3   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt3)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT4   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt4)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT5   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt5)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT6   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt6)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT7   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt7)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT8   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt8)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT9   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt9)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT10   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt10)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT11   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt11)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT12   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt12)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT13   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt13)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT14   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt14)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT15   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt15)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT16   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt16)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT17   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt17)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT18   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt18)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT19   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt19)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_INT20   XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedInt20)

#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG1   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong1)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG2   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong2)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG3   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong3)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG4   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong4)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG5   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong5)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG6   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong6)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG7   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong7)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG8   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong8)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG9   XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong9)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_LONGLONG10  XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedLongLong10)

#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING1   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString1)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING2   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString2)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING3   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString3)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING4   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString4)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING5   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString5)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING6   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString6)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING7   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString7)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING8   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString8)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING9   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString9)
#define X_PROPERTY_DEDICATEDSERVER_RESERVED_STRING10   XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_ReservedString10)

// the following properties only support XTS_FILTER_COMPARE_OPERATOR_Equals operator
#define X_PROPERTY_DEDICATEDSERVER_OWNER_XUID           XPROPERTYID(1, XUSER_DATA_TYPE_INT64,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_OwnerXuid)
#define X_PROPERTY_DEDICATEDSERVER_OWNER_GAMERTAG       XPROPERTYID(1, XUSER_DATA_TYPE_UNICODE,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_OwnerGamerTag)
#define X_PROPERTY_DEDICATEDSERVER_REGIONID             XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_RegionID)
#define X_PROPERTY_DEDICATEDSERVER_LANGUAGEID           XPROPERTYID(1, XUSER_DATA_TYPE_INT32,   XLOCATOR_DEDICATEDSERVER_PROPERTY_START + XTS_SEARCH_FIELD_LanguageID)

// Error code from XLocator APIs. Typically title may only see XTS_E_TIMEOUT if the code is correct.
#define XTS_E_FAIL                              ((HRESULT)0x80500001) // Unspecified failure
#define XTS_E_MEMORY                            ((HRESULT)0x80500002) // Memory error
#define XTS_E_NULLARG                           ((HRESULT)0x80500003) // Argument was NULL where NULL not allowed
#define XTS_E_INVALIDARG                        ((HRESULT)0x80500004) // Same as E_INVALIDARG
#define XTS_E_CONNECTION_LOST                   ((HRESULT)0x80500005) // Title server connection lost.
#define XTS_E_NULL_TRANSPORT                    ((HRESULT)0x80500008) // Check for XLocatorServiceInitialize success before other calls.
#define XTS_E_INVALID_REQUEST                   ((HRESULT)0x80500009) // Request not recognized
#define XTS_E_UNSUPPORTED_UNICODE_GAMERTAG      ((HRESULT)0x8050000A) // If gamertag has unicode characters that is bigger than WCHAR(16)
#define XTS_E_DATA_SIZE                         ((HRESULT)0x8050000C) // Data too big to fit in buffer.
#define XTS_E_WEB_FAILURE                       ((HRESULT)0x8050000D) // Server error. The server may be busy or offline.
#define XTS_E_SOAP_EXCEPTION                    ((HRESULT)0x8050000E) // Server error.
#define XTS_E_READ_BINDING                      ((HRESULT)0x8050000F) // Error in web method read binding
#define XTS_E_TIMEOUT                           ((HRESULT)0x80500010) // Task timed out. Usually because the server is not installed for this title, or XLocatorServiceInitialize has incorrect parameter.
#define XTS_E_TASK_CLOSED                       ((HRESULT)0x80500011) // Task was closed. 
#define XTS_E_INVALID_PROTOCOL_STATE            ((HRESULT)0x80500012) // Internal error
#define XTS_E_NO_FREE_REQUEST_CONTEXTS          ((HRESULT)0x80500013) // Internal error, request context busy
#define XTS_E_INVALID_MESSAGE_TYPE              ((HRESULT)0x80500014) // Invalid message type. 
#define XTS_E_INVALID_SERVICE                   ((HRESULT)0x80500015) // Invalid service id.
#define XTS_E_SOCKET_ERROR                      ((HRESULT)0x80500016) // General socker error.
#define XTS_E_XONLINE_FAILURE                   ((HRESULT)0x8050001B) // Xonline check failure
#define XTS_E_ALREADY_REGISTERED                ((HRESULT)0x8050001C) // Internal error.
#define XTS_E_ENCODING_FAILURE                  ((HRESULT)0x8050001D) // Text string encoding failed.
#define XTS_E_UNREQUESTED_RESPONSE              ((HRESULT)0x8050001E) // Internal error. 
#define XTS_E_NO_MATCHING_CONNECTION            ((HRESULT)0x8050001F) // No connection available. Check server availability.
#define XTS_E_INVALID_PENDING                   ((HRESULT)0x80500020) // Internal error.
#define XTS_E_SEND_TRUNCATED                    ((HRESULT)0x80500021) // Internal error. 
#define XTS_E_NOT_OVERLAPPED                    ((HRESULT)0x80500022) // Internal error. 
#define XTS_E_INVALID_BUFFER_SIZE               ((HRESULT)0x80500024) // Internal error. Buffer too small.
#define XTS_E_NOT_CONNECTED                     ((HRESULT)0x80500025) // Not connected to title server. Check the title server availability. Check XLocatorServiceInitialize return code.
#define XTS_E_TOPOLOGY_FAILURE                  ((HRESULT)0x80500026) // Internal error. 
#define XTS_E_TOPOLOGY_INVALID_MESSAGE_TYPE     ((HRESULT)0x80500027) // Internal error. 
#define XTS_E_TOPOLOGY_ITEM_NOT_FOUND           ((HRESULT)0x80500028) // Internal error. 
#define XTS_E_BIND_UNSUPPORTED_TYPE             ((HRESULT)0x8050002A) // Internal error. The data format is incorrect.
#define XTS_E_SYSTEM_LISTENER_FAILED            ((HRESULT)0x8050002B) // Internal error. 
#define XTS_E_INVALID_LOGON_USERS               ((HRESULT)0x8050002C) // The user has logged off.
#define XTS_E_USING_LIVE_USERS                  ((HRESULT)0x8050002D) // Tried to explicitly set user data when live-enabled
#define XTS_E_COMPRESSION_ERROR                 ((HRESULT)0x8050002E) // Internal error. The data compression failed.
#define XTS_E_DECOMPRESSION_ERROR               ((HRESULT)0x8050002F) // Internal error. The data decompression failed.
#define XTS_E_LIVE_NOT_SUPPORTED                ((HRESULT)0x80500030) // Tried to use live features from win32
#define XTS_E_USERS_REQUIRED                    ((HRESULT)0x80500031) // Internal initialize error.
#define XTS_E_DEFAULT_MASK_REQUIRED             ((HRESULT)0x80500032) // Internal initialize error.
#define XTS_E_UNSECURE_ADDRESS_REQUIRED         ((HRESULT)0x80500033) // Internal initialize error.
#define XTS_E_INVALID_COMPRESSION_TYPE          ((HRESULT)0x80500034) // Internal error. The data compression type is not supported.
#define XTS_E_NULL_PROTOCOL_FACTORY             ((HRESULT)0x80500035) // Internal initialize error.
#define XTS_E_BANDWIDTH_FULL                    ((HRESULT)0x80500036) // Internal error.
#define XTS_E_FEED_INIT_FAILED                  ((HRESULT)0x80500037) // Internal error.
#define XTS_E_OVERLAPPED_NOT_SUPPORTED          ((HRESULT)0x80500038) // Internal error.
#define XTS_E_QUERY_SERVICE_NOT_SUPPORTED       ((HRESULT)0x80500039) // Internal error.
#define XTS_E_INVALID_TITLEID                   ((HRESULT)0x8050003A) // Internal error.
#define XTS_E_COULD_NOT_CONNECT                 ((HRESULT)0x8050003B) // Connection error. Check title server availability.
#define XTS_E_CONNECTION_DEAD                   ((HRESULT)0x8050003C) // Connection error. Check title server availability.
#define XTS_E_TICKER_FORMAT_CALLBACK            ((HRESULT)0x8050003D) // Internal error.
#define XTS_E_NO_SERVERS                        ((HRESULT)0x8050003E) // Connection error. Check title server availability.
#define XTS_E_SERVICE_UNAVAILABLE               ((HRESULT)0x8050003F) // Connection error. Check title server availability.
#define XTS_E_STREAM_READ                       ((HRESULT)0x80500040) // Internal error.
#define XTS_E_STREAM_WRITE                      ((HRESULT)0x80500041) // Internal error.
#define XTS_E_INVALID_FEED_FORMAT               ((HRESULT)0x80500042) // Internal error.
#define XTS_E_INVALID_WEB_RESULT                ((HRESULT)0x80500043) // Server error.
#define XTS_E_INVALID_METHOD                    ((HRESULT)0x80500044) // means GenXts has a bug, or someone is calling transacted calls via ungenerated code
#define XTS_E_NULL_LOGON_PROVIDER               ((HRESULT)0x80500045) // Internal initialization error.
#define XTS_E_NO_DATA_FILE_PROVIDER             ((HRESULT)0x80500047) // A two phase commit method was called and no transaction log manager exists
#define XTS_E_NO_DATA_FILE_FOR_USER             ((HRESULT)0x80500048) // Could not obtain data file for user
#define XTS_E_INVALID_TRANSACTION_LOG_VERSION   ((HRESULT)0x80500049) // The user transaction log read from disk does not have the correct version
#define XTS_E_INVALID_MSG_TRANSACTION_STATE     ((HRESULT)0x8050004A) // An invalid state has occurred in transacted message protocol
#define XTS_E_TRANSACTION_FILE_IO_PENDING       ((HRESULT)0x8050004B) // An IO call was made to the default transaction log manager while another IO was pending.
#define XTS_E_UNEXPECTED_STATE                  ((HRESULT)0x8050004C) // Internal error.
#define XTS_E_NULL_TRANSPORT_PROVIDER           ((HRESULT)0x8050004D) // Initialization error. Check XLocatorServiceInitialize result.
#define XTS_E_BUFFER_TOO_SMALL                  ((HRESULT)0x8050004E) // Buffer too small.
#define XTS_E_TRANSACTION_LOG_BROKEN            ((HRESULT)0x8050004F) // Internal error. Data file is corrupted.
#define XTS_E_TRANSPORT_HAS_BEEN_SHUTDOWN       ((HRESULT)0x80500050) // Some APIs were called after XLocatorServiceUninitialize.
#define XTS_E_NO_NEED_TO_REINITIALIZE           ((HRESULT)0x80500051) // Internal error.
#define XTS_E_INVALID_PROVIDERS                 ((HRESULT)0x80500052) // Internal error.
#define XTS_E_CONCURRENT_DATAFILE_IO            ((HRESULT)0x80500054) // Internal error. 
#define XTS_E_TRANSACTION_ENTRY_NOT_FOUND       ((HRESULT)0x80500056) // Internal error. In memory transaction log doesn't have expected entry
#define XTS_E_INVALID_TRANSPORT_STATE           ((HRESULT)0x80500057) // Internal error. Detected user logon change and found transport in unknown state
#define XTS_E_INVALID_DIRECTORY_PATH            ((HRESULT)0x80500058) // Internal error. 
#define XTS_E_TRANSACTION_LOG_MISSING           ((HRESULT)0x80500059) // Internal error. In memory transaction log missing for otherwise valid controller index
#define XTS_E_TRANSACTION_ABORTED               ((HRESULT)0x8050005A) // Internal error. In memory transaction log missing for otherwise valid controller index
#define XTS_E_SECURITY                          ((HRESULT)0x805103E9) // The request was forbidden by server.
#define XTS_E_TITLE_SERVER                      ((HRESULT)0x805103EA) // Title server connection error. 
#define XTS_E_TITLE_SERVER_OFFLINE              ((HRESULT)0x805103EB) // Title server is going offline.
#define XTS_E_WRITE_CONFLICT                    ((HRESULT)0x805107D0) // Internal error. Should retry the request. 
#define XTS_E_INVALIDID                         ((HRESULT)0x805107D1) // Internal error. Should retry the request.
#define XTS_E_UNIQUE_FIELD                      ((HRESULT)0x805107D2) // Internal error.
#define XTS_E_ALREADY_LOCKED                    ((HRESULT)0x805107D3) // Internal error.
#define XTS_E_FIELD_LENGTH                      ((HRESULT)0x805107D4) // Internal error.
#define XTS_E_VERSION_MISMATCH                  ((HRESULT)0x805107D5) // Internal error.
#define XTS_E_USER_BANNED                       ((HRESULT)0x805109C4) // The user was banned for frequent requests.
#define XTS_E_DS_INVALIDCOMPAREOPERATOR         ((HRESULT)0x80517564) // Filter compare operator is not supported for specific field.
#define XTS_E_DS_INSUFFICIENTRESULTBUFFER       ((HRESULT)0x80517566) // Result buffer size is not enough.
#define XTS_E_DS_INVALIDFILTER                  ((HRESULT)0x80517567) // The given filter is not valid.
#define XTS_E_DS_DATATYPENOTSUPPORTED           ((HRESULT)0x80517568) // The data type is not supported.
#define XTS_E_DS_INVALIDXNETKID                 ((HRESULT)0x80517569) // The XnetKID is not valid. Please use XLocatorCreateKey.
#define XTS_E_DS_STRINGTOOLONG                  ((HRESULT)0x8051756A) // The given string is long.

// Advertise a dedicated server to locator service.
HRESULT WINAPI XLocatorServerAdvertise(
    __in DWORD dwUserIndex,
    __in DWORD dwServerType,             // see XLOCATOR_SERVERTYPE_PUBLIC, etc
    __in XNKID xnkid,
    __in XNKEY xnkey,
    __in DWORD dwMaxPublicSlots,
    __in DWORD dwMaxPrivateSlots,
    __in DWORD dwFilledPublicSlots,
    __in DWORD dwFilledPrivateSlots,
    __in DWORD cProperties,                                     // number of custom properties
    __in_ecount_opt(cProperties) PXUSER_PROPERTY pProperties,   // array of properties
    __inout_opt PXOVERLAPPED pXOverlapped
);

// Unadvertise a server from locator service.
HRESULT WINAPI XLocatorServerUnAdvertise(
    __in        DWORD           dwUserIndex,
    __inout_opt PXOVERLAPPED    pXOverlapped
);

HRESULT WINAPI XLocatorGetServiceProperty(
    DWORD dwUserIndex,
    DWORD cNumProperties,
    __inout_ecount(cNumProperties) PXUSER_PROPERTY pProperties, // an array of user properties.
                                                                // pass in the property id like X_PROPERTY_SERVICESTATUS_SERVERCOUNT_TOTAL etc
                                                                // and returns the property value
    __inout_opt PXOVERLAPPED pXOverlapped
);

// Create an enumerator for dedicated servers search.
// The filters in one filter group are combined with 'OR' operator,
// and then the filter groups are combined with 'AND' operator.
DWORD WINAPI XLocatorCreateServerEnumerator(
    __in DWORD dwUserIndex,
    __in DWORD cItems,
    __in DWORD cRequiredPropertyIDs,
    __in_ecount_opt(cRequiredPropertyIDs) DWORD* pRequiredPropertyIDs,
    __in DWORD cFilterGroupItems,
    __in_ecount_opt(cFilterGroupItems) PXLOCATOR_FILTER_GROUP pxlFilterGroups,
    __in DWORD cSorterItems,
    __in_ecount_opt(cSorterItems) PXLOCATOR_SORTER pxlSorters,
    __out_opt DWORD* pcbBuffer,
    __deref_out PHANDLE phEnum
);

// Search the dedicated servers by ID.
DWORD WINAPI XLocatorCreateServerEnumeratorByIDs(
    __in DWORD dwUserIndex,
    __in DWORD cItems,
    __in DWORD cRequiredPropertyIDs,
    __in_ecount_opt(cRequiredPropertyIDs) DWORD* pRequiredPropertyIDs,
    __in DWORD cIDS,
    __in_ecount(cIDS) PULONGLONG pIDs,
    __out_opt DWORD* pcbBuffer,
    __deref_out PHANDLE phEnum
);

// Initialize locator service.
HRESULT WINAPI XLocatorServiceInitialize(
    __in XLOCATOR_INIT_INFO * pXii,
    __deref_out PHANDLE phLocatorService
);

HRESULT WINAPI XLocatorServiceUnInitialize(
    __in HANDLE hLocatorService
);

HRESULT WINAPI XLocatorCreateKey(__inout XNKID * pxnkid,
                                 __inout XNKEY * pxnkey);

#endif  // NONET

/****************************************************
The following functions have been deprecated and should no longer be used. 
They are provided in this header purely for backward compatibility purposes. 
The list of deprecated functions are:
  1. XLiveRegisterDataSection
  2. XLiveUnregisterDataSection
  3. XLiveUpdateHashes
****************************************************/
HRESULT WINAPI XLiveRegisterDataSection(__in PCWSTR pszDataModuleName, __in LPVOID lpvBaseAddress, __in SIZE_T nSize);
HRESULT WINAPI XLiveUnregisterDataSection(__in PCWSTR pszDataModuleName);
HRESULT WINAPI XLiveUpdateHashes(__in PCWSTR pszFilename, __in BOOL fAppend);
/***************************************************
End deprecated function list
***************************************************/


#ifdef __cplusplus
}
#endif // __cplusplus

#endif  /* __WINLIVE__ */

