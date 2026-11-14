#ifndef registration_h__
#define registration_h__

#pragma warning( push )
#pragma warning(default:4530)

#include <Es/Strings/rString.hpp>

#ifdef _WIN32
#define NOTHROW __declspec(nothrow)
#else
#define NOTHROW __attribute__((nothrow))
#endif

struct SecureKeyList
{
  FindArray<RString> keys; // blocked keys list
  AutoArray<char> signature; // signature of a blocked keys list
};

struct UserCertificate
{
  AutoArray<char> certificate;
  AutoArray<char> privateKey;
  SecureKeyList blockedKeys;
  RString errorMessage;

  UserCertificate(){}
  explicit UserCertificate(RString errorMessage):errorMessage(errorMessage){}
};

UserCertificate NOTHROW GetUserCertificate(RString url, RString productId, RString playerId, const Array<unsigned char> &cdKey);

enum SignValidationResult
{
  SignOK,
  SignBadUserName,
  SignBlockedUser,
  SignBadSignature,
  SignBadCertificate,
  SignUnknownError,
};

SignValidationResult NOTHROW VerifySignedData(RString playerId, long long challenge, const Array<char> &signature, const Array<char> &certificate);
const AutoArray<unsigned char> NOTHROW SignData(long long challenge, const Array<unsigned char> &privateKey);
bool NOTHROW VerifySignedKeys(const Array<RString> &data, const Array<char> &signature);

RString NOTHROW PrintSignedKeys(const Array<RString> &data, const Array<char> &signature);
void NOTHROW ParseSignedKeys(FindArray<RString> &data, AutoArray<char> &signature, const char *input);

long long NOTHROW GetSecureRandom();

RString NOTHROW GetConfigAppFromRegistry();

#endif // registration_h__

#pragma warning( pop )