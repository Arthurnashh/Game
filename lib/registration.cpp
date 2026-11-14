#ifdef _WIN32
#include <crtdbg.h>
#endif
#include "registration.hpp"
#include "Network/httpRequest.h"
#include <Es/Strings/bString.hpp>
#include <Es/Memory/normalNew.hpp>
#include <El/Credits/credits.hpp>

#ifdef _WIN32
#include <Botan/botan_all.h>
#else
#include <Botan/botan_all_linux.h>
#endif

REGISTER_CREDITS("Botan","1999-2012 Jack Lloyd, 2001 Peter J Jones, 2004-2007 Justin Karneges, 2004 Vaclav Ovsik, 2005 Matthew Gregan, 2005-2006 Matt Johnston, 2006 Luca Piccarreta, 2007 Yves Jerschow, " \
"2007-2008 FlexSecure GmbH, 2007-2008 Technische Universitat Darmstadt, 2007-2008 Falko Strenzke, 2007-2008 Martin Doering, 2007 Manuel Hartl, 2007 Christoph Ludwig, 2007 Patrick Sona, 2010 Olivier de Gaalon",STANDARD_DISCLAIMER)

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
REGISTER_CREDITS("rapidjson","2011 Milo Yip","")


#include <Es/Memory/debugNew.hpp>

#include <sstream>

Botan::LibraryInitializer GBotanInit; 

#ifdef _WIN32
static RString QueryRegistryString(HKEY key, const char *value)
{
  char data[2048];
  DWORD size = sizeof(data);
  DWORD type = REG_SZ;
  HRESULT result = ::RegQueryValueEx(key, value, 0, &type, (BYTE *)data, &size);
  if (result!=ERROR_SUCCESS) return RString();
  return data;
}

static bool StoreRegistryString(HKEY key, const char *value, const char *content)
{
  HRESULT result = ::RegSetValueEx(key, value, 0, REG_SZ, reinterpret_cast<const BYTE *>(content), strlen(content)+1);
  return result==ERROR_SUCCESS;
}
#endif

static Botan::SecureVector<Botan::byte> Base64Decode(const char *encoded_string)
{
  int in_len = strlen(encoded_string);

  // remove [" "] encapsulation when needed
  if (in_len>=4 && encoded_string[0]=='[' && encoded_string[1]=='"' &&  encoded_string[in_len-1]==']' && encoded_string[in_len-2]=='"')
  {
    encoded_string += 2;
    in_len -= 4;
  }

  return Botan::base64_decode(encoded_string,in_len);
}

static const char *serverCertBase64="MIIDQzCCAiugAwIBAgIRAKX5DEA7Zknzpa6WWilcw6UwDQYJKoZIhvcNAQEFBQAwXDELMAkGA1UEBhMCQ1oxEzARBgNVBAoTCklERUEgR2FtZXMxGjAYBgNVBAsTEVVzZXIgUmVnaXN0cmF0aW9uMRwwGgYDVQQDExNSZWdpc3RyYXRpb24gU2VydmVyMCAXDTEyMDYyNzIxMDMwOFoYDzQ5OTkxMjMxMjIwMzA4WjBcMQswCQYDVQQGEwJDWjETMBEGA1UEChMKSURFQSBHYW1lczEaMBgGA1UECxMRVXNlciBSZWdpc3RyYXRpb24xHDAaBgNVBAMTE1JlZ2lzdHJhdGlvbiBTZXJ2ZXIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCmo2THdTKH1Lh5iTPpDblzsGUX+qB6x2KomkzvktXUg7kPT7PMPBr3M3W/hhrcpfXzgt1MH9FQCEf6K6VWk6tCQzYnDWYBl+fOmO3pAwY7kVjcOxWeILtaxWy1b6HVVqjX8i6bTDg/V8R5r1bqpuyE2FInkT1slH2Z05UzUMhQglSTeKx7hdT3g5Ed0CBCo0tAZFi44ipC3PfbkiB2tR5BKXz3BJM/qqbEALrA388cw9YOxNhJtfNzZoSSoIUEZNGyEQ3bCyzsl/4h8wgyROjjWiX1X4Dvmbj+EA0T7rk+WUVtqNw/fxGVq09kFDY2V69gaLsVqC45aXgOJ/jErWrvAgMBAAEwDQYJKoZIhvcNAQEFBQADggEBAHKtfhWctneNSMM5jE2kd/MlqYWZiuzqcqDPol5ygTiTqekP+mc/fNAkqQMgsbvh0okzXaZOPpcU28GRxSGEU0e45iMXXSPiY7ox5xWfRLkwakiaznWaXl+CKYeHVboJCuyb2PpQJy6B4TPrAts8TO0DzeIMUs9YLihMqXX9iSxT6p0br+hTv0vpOZPpiBh30q8uPpPVW632mRCRYFxHHQeyJGoMkZJyeQFX5Zgsx+vkp9BE0+2wIVRk+z4PEB1gAdO0tD1fXpsDLoAtiUf2XHab4ZEju63BY9QSEZcnwa4EO3uEe/m5GEKZN6qLpyTZ+VvR/BVlzBkrpw2esvXtwXw=";
static const std::string SignaturePadding = "EMSA3(SHA-256)";
static const Botan::Signature_Format SignatureFormat = Botan::IEEE_1363;


static Botan::X509_Code VerifyCertSig(const Botan::X509_Certificate &subjCert, Botan::X509_Certificate &caCert)
{
    std::auto_ptr<Botan::Public_Key> caPublicKey = std::auto_ptr<Botan::Public_Key>(caCert.subject_public_key());

    try {
      std::vector<std::string> sig_info =
          Botan::split_on(Botan::OIDS::lookup(subjCert.signature_algorithm().oid), '/');

      if(sig_info.size() != 2 || sig_info[0] != caPublicKey->algo_name())
          return Botan::SIGNATURE_ERROR;

      std::string padding = sig_info[1];
      Botan::Signature_Format format = Botan::IEEE_1363;

      Botan::PK_Verifier verifier(*caPublicKey.get(), padding, format);

      bool valid = verifier.verify_message(subjCert.tbs_data(), subjCert.signature());

      if(valid)
          return Botan::VERIFIED;
      else
          return Botan::SIGNATURE_ERROR;
      }
    catch(Botan::Lookup_Error)   { return Botan::CA_CERT_CANNOT_SIGN; }
    catch(Botan::Decoding_Error) { return Botan::CERT_FORMAT_ERROR; }
    catch(Botan::Exception)      {}

    return Botan::UNKNOWN_X509_ERROR;
}

static Botan::X509_Certificate GetCACertificate()
{
  Botan::SecureVector<Botan::byte> caCertBytes = Base64Decode(serverCertBase64);
  Botan::DataSource_Memory dsm(caCertBytes);
  return Botan::X509_Certificate(dsm);
}

static SignValidationResult ValidateUserCert(RString playerId, const Botan::X509_Certificate &subject_cert)
{
  try
  {
    // verify the certificate belongs to the ID and it is valid
    Botan::X509_Certificate caCert = GetCACertificate();
    Botan::X509_Code code = VerifyCertSig(subject_cert,caCert);

    if(code != Botan::VERIFIED) 
    {
      RptF("Certificate signature not valid (error code %d).",code);
      return SignBadSignature;
    }

    std::vector<std::string> dn = subject_cert.subject_dn().get_attribute("Name");
    std::string expectDn,blockedDn;
    {
      std::ostringstream userIdStream;
      userIdStream << "User" << playerId;
      expectDn = userIdStream.str();
    }
    {
      std::ostringstream userIdStream;
      userIdStream << "Blocked" << playerId;
      blockedDn = userIdStream.str();
    }

    if (dn.size()!=1 || dn[0]!=expectDn)
    {
      if (dn.size()==1 && dn[0]==blockedDn)
      {
        return SignBlockedUser; 
      }
      RptF("Certificate does not belong to the user. Name '%s', expected '%s'",dn[0].c_str(),expectDn.c_str());
      return SignBadUserName;
    }

    return SignOK;
  }
  catch (...)
  {
    RptF("Exception during the certificate validation");
  }
  return SignUnknownError;
}

static bool VerifySignedKeys(const Array<RString> &keys, const Botan::SecureVector<Botan::byte> &sigBytes)
{
  // now parse and verify the signature
  Botan::X509_Certificate caCert = GetCACertificate();
  std::unique_ptr<Botan::Public_Key> publicKey(caCert.subject_public_key());

  Botan::PK_Verifier verifier(*publicKey.get(), SignaturePadding, SignatureFormat);

  for (int i=0; i<keys.Size(); i++)
  {
    RString key = keys[i];
    verifier.update(reinterpret_cast<const unsigned char *>(key.Data()),key.GetLength()+1); // include zero terminator
  }
  return verifier.check_signature(sigBytes);
}
      
static SecureKeyList ParseKeyList(const char *json)
{
  SecureKeyList ret;
  rapidjson::Document doc;
  doc.Parse<rapidjson::kParseDefaultFlags>(json);
  if (doc.IsObject())
  {
    const rapidjson::Value &keys = doc["keys"];
    const rapidjson::Value &signature = doc["signature"];
    if (!keys.Empty() && !signature.Empty())
    {
      for (rapidjson::Value::ConstValueIterator key = keys.Begin(); key!=keys.End(); key++)
      {
        const char *keyValue = key->GetString();
        ret.keys.Add(keyValue);
      }
      // now parse and verify the signature
      const char *sigBase64 = signature.GetString();
      Botan::SecureVector<Botan::byte> sigBytes = Base64Decode(sigBase64);

      bool valid = VerifySignedKeys(ret.keys,sigBytes);
      if (!valid)
      {
        ret.keys.Clear();
      }
      else
      {
        ret.signature.Copy(reinterpret_cast<const char *>(sigBytes.begin()),sigBytes.size());
      }
    }
  }

  return ret;
}
static SignValidationResult ValidateUserCert(RString playerId, RString certificate)
{
  try
  {
    // verify the certificate belongs to the ID and it is valid
    Botan::SecureVector<Botan::byte> ret = Base64Decode(certificate);
    if (ret.size()>0)
    {
      Botan::DataSource_Memory dsm(ret);
      return ValidateUserCert(playerId,Botan::X509_Certificate(dsm));
    }
  }
  catch (...)
  {
    RptF("Exception during the certificate validation");
  }
  return SignUnknownError;
}

#ifdef _WIN32
struct AutoHKEY
{
  HKEY key;

  AutoHKEY(HKEY key):key(key){}
  ~AutoHKEY()
  {
    ::RegCloseKey(key);
  }
  operator HKEY () const {return key;}
};

UserCertificate GetUserCertificate(RString url, RString productId, RString playerId, const Array<unsigned char> &cdKey)
{
  try
  {
    // TODO: use SHGetFolderPath(CSIDL_COMMON_APPDATA)
    HRESULT FindConfigAppRegistry(HKEY &hKey);
    HKEY regKey;
    if (!SUCCEEDED(FindConfigAppRegistry(regKey))) return UserCertificate("Cannot access registry");

    int protInURL = url.Find(':');
    while (protInURL>=0 && protInURL<url.GetLength() && strchr(":/",url[protInURL])) protInURL++;
    // strip protocol from URL to keep the name nice
    RString registryUrl = protInURL>=0 ? url.Substring(protInURL,url.GetLength()) : url;
    
    HKEY subKey;
    // in case name for URL cannot be created (could happen when URL is too long or contains nonprintable characters)
    HRESULT keyOk = RegCreateKeyEx(regKey,registryUrl,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, NULL, &subKey, NULL);
    if (SUCCEEDED(keyOk))
    {
      RegCloseKey(regKey);
      regKey = subKey;
    }

    AutoHKEY closeKey = regKey;

    RString certificate = QueryRegistryString(regKey,"certificate");
    RString privateKey = QueryRegistryString(regKey,"signingKey");
    RString blockedKeys = QueryRegistryString(regKey,"blockedKeys");

    if (!certificate.IsEmpty() && !privateKey.IsEmpty())
    {
      SignValidationResult signOK = ValidateUserCert(playerId,certificate);
      if (signOK==SignOK || signOK==SignBlockedUser)
      {
        Botan::SecureVector<Botan::byte> certificateBytes = Base64Decode(certificate);
        Botan::SecureVector<Botan::byte> privateKeyBytes = Base64Decode(privateKey);
        UserCertificate cert;
        cert.certificate = AutoArray<char>(reinterpret_cast<const char *>(certificateBytes.begin()),certificateBytes.size());
        cert.privateKey = AutoArray<char>(reinterpret_cast<const char *>(privateKeyBytes.begin()),privateKeyBytes.size());
        cert.blockedKeys = ParseKeyList(blockedKeys);
        return cert;
      }
      // silently attempt to recover
      certificate = RString();
      privateKey = RString();
    }

    if (certificate.IsEmpty() || privateKey.IsEmpty())
    {

      // generate a private / public key pair

      Botan::AutoSeeded_RNG rng;

      Botan::RSA_PrivateKey key(rng, 1024);

      Botan::MemoryVector<Botan::byte> publicEncoded = Botan::X509::BER_encode(key);

      Botan::PK_Signer rsa(key,"EMSA3(SHA-256)"); // Java was using: Signature rsa = Signature.getInstance("SHA256withRSA");
      rsa.update(cdKey.Data(),cdKey.Size());

      Botan::SecureVector<Botan::byte> cdKeySignature = rsa.signature(rng);
      Botan::SecureVector<Botan::byte> cdKeyBytes(cdKey.Data(),cdKey.Size());

      rapidjson::DocumentValue registerPars;
      registerPars.AddMemberString("cdKeySignature", Botan::base64_encode(cdKeySignature).c_str());
      registerPars.AddMemberString("publicKey", Botan::base64_encode(publicEncoded).c_str());
      registerPars.AddMemberString("cdKey", Botan::base64_encode(cdKeyBytes).c_str());

      RString query = productId.IsEmpty() ? Format("register/%s",cc_cast(playerId)) : Format("register/%s/%s",cc_cast(productId),cc_cast(playerId));
      rapidjson::DocumentValue registerResponse = GetResponseJSON(url,query,registerPars);
      const rapidjson::Value &cert = registerResponse["certificate"];
      if (!cert.Empty())
      {
        RString newCertificate = cert.GetString();
        Botan::SecureVector<Botan::byte> certBytes = Base64Decode(newCertificate);

        Botan::X509_Certificate verifyCert = Botan::DataSource_Memory(certBytes); 

        Botan::SecureVector<Botan::byte> privateKeyBytes = Botan::PKCS8::BER_encode(key,rng,std::string());

        privateKey = Botan::base64_encode(privateKeyBytes).c_str();

        // we have the id and the certificate - store somewhere, including the private key (use restrictive ACL?)
        certificate = RString(newCertificate);
        SignValidationResult signOK=ValidateUserCert(playerId,certificate);
        if (signOK!=SignOK && signOK!=SignBlockedUser)
        {
          Fail("Newly created certificate not valid");
          certificate = RString();
          privateKey = RString();
          switch (signOK)
          {
            case SignBadSignature:
            case SignBadCertificate: return UserCertificate("New certificate not valid - invalid signature");
            default: return UserCertificate("New certificate not valid");
          }
        }
        // store to the registry
        const rapidjson::Value &blocked = registerResponse["blocked"];

        rapidjson::StringBuffer blockedDataBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(blockedDataBuffer);
        blocked.Accept(writer);

        blockedKeys = blockedDataBuffer.GetString();
        StoreRegistryString(regKey,"certificate",certificate);
        StoreRegistryString(regKey,"signingKey",privateKey);
        StoreRegistryString(regKey,"blockedKeys",blockedKeys);
      }
      else
      {
        RString error = registerResponse["error"].GetString();
        if (!error.IsEmpty())
        {
          return UserCertificate(cc_cast(error));
        }
        else
        {
          return UserCertificate("Unknown error");
        }
      }
    }
    if (!certificate.IsEmpty() && !privateKey.IsEmpty())
    {
      Botan::SecureVector<Botan::byte> certificateBytes = Base64Decode(certificate);
      Botan::SecureVector<Botan::byte> privateKeyBytes = Base64Decode(privateKey);
      UserCertificate cert;
      cert.certificate = AutoArray<char>(reinterpret_cast<const char *>(certificateBytes.begin()),certificateBytes.size());
      cert.privateKey = AutoArray<char>(reinterpret_cast<const char *>(privateKeyBytes.begin()),privateKeyBytes.size());
      cert.blockedKeys = ParseKeyList(blockedKeys);
      return cert;
    }
  }
  catch (...)
  {
    return UserCertificate("Unhandled exception");
  }

  return UserCertificate();
}
#endif

long long GetSecureRandom()
{
  long long ret;
  Botan::AutoSeeded_RNG rng;
  rng.randomize(reinterpret_cast<Botan::byte *>(&ret),sizeof(ret));
  return ret;
}



static SignValidationResult VerifySignedDataInternal(RString playerId, long long challenge, const Array<char> &signature, const Array<char> &certificate)
{
  try {
    Botan::DataSource_Memory dsm(reinterpret_cast<const unsigned char *>(certificate.Data()),certificate.Size());
    Botan::X509_Certificate verifyCert(dsm); 

    std::unique_ptr<Botan::Public_Key> publicKey(verifyCert.subject_public_key());


    Botan::PK_Verifier verifier(*publicKey.get(), SignaturePadding, SignatureFormat);

    verifier.update(reinterpret_cast<const unsigned char *>(&challenge),sizeof(challenge));
    bool valid = verifier.check_signature(reinterpret_cast<const unsigned char *>(signature.Data()),signature.Size());

    if(!valid)
      return SignUnknownError;
    // verify we accept the certificate
    return ValidateUserCert(playerId,verifyCert);
  }
  catch(Botan::Lookup_Error)   {}
  catch(Botan::Decoding_Error) { return SignBadCertificate; }
  catch(Botan::Exception)      {}
  catch(...)      {}

  return SignUnknownError;
}

SignValidationResult VerifySignedData(RString playerId, long long challenge, const Array<char> &signature, const Array<char> &certificate)
{
  // verify the signature is correct - based on VerifyCertSig
  return VerifySignedDataInternal(playerId,challenge,signature,certificate);
}

bool VerifySignedKeys(const Array<RString> &data, const Array<char> &signature)
{
  try
  {
    Botan::SecureVector<Botan::byte> signatureBytes(reinterpret_cast<const Botan::byte *>(signature.Data()),signature.Size());

    return VerifySignedKeys(data,signatureBytes);
  }
  catch(...)
  {
    return false;
  }
}

RString PrintSignedKeys(const Array<RString> &data, const Array<char> &signature)
{
  try
  {
    rapidjson::Document list;
    list.SetObject();
    list.AddMember("keys",0,list.GetAllocator());
    list.AddMember("signature",0,list.GetAllocator());
    rapidjson::Value &keys = list["keys"];
    keys.SetArray();
    for (int i=0; i<data.Size(); i++)
    {
      const char *key = data[i];
      keys.PushBack(key,list.GetAllocator()); // we do not care if strings are not copied, they stay live long enough
    }
    std::string signatureBase64 = Botan::base64_encode(reinterpret_cast<const Botan::byte *>(signature.Data()),signature.Size());
    list["signature"].SetString(signatureBase64.c_str(),list.GetAllocator());

    rapidjson::StringBuffer blockedDataBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(blockedDataBuffer);
    list.Accept(writer);
    return blockedDataBuffer.GetString();
  }
  catch (...)
  {
    return RString();
  }
}

void ParseSignedKeys(FindArray<RString> &data, AutoArray<char> &signature, const char *input)
{
  try
  {
    SecureKeyList keys = ParseKeyList(input);
    data = keys.keys;
    signature = keys.signature;
  }
  catch (...)
  {
  }
}


const AutoArray<unsigned char> SignData(long long challenge, const Array<unsigned char> &privateKey)
{
  try
  {
    Botan::DataSource_Memory src(privateKey.Data(),privateKey.Size());
    Botan::AutoSeeded_RNG rng;
    std::unique_ptr<Botan::PKCS8_PrivateKey> key(Botan::PKCS8::load_key(src, rng, std::string()));

    Botan::PK_Signer signer(*key.get(),SignaturePadding, SignatureFormat);
    signer.update(reinterpret_cast<Botan::byte *>(&challenge),sizeof(challenge));
    Botan::SecureVector<Botan::byte> signature = signer.signature(rng);
    return AutoArray<unsigned char>(signature.begin(),signature.size());
  }
  catch (...)
  {
    return AutoArray<unsigned char>();
  }
}