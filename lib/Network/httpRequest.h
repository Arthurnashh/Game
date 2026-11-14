#ifdef _MSC_VER
#pragma once
#endif

#ifndef http_request_hpp
#define http_request_hpp

#include <Es/Strings/rString.hpp>
#include <Es/Memory/normalNew.hpp>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <Es/Memory/debugNew.hpp>

#ifdef _WIN32
#define NOTHROW __declspec(nothrow)
#else
#define NOTHROW __attribute__((nothrow))
#endif

#include <memory>
using std::shared_ptr;

enum EHttpRequestType
{
	EHTTPREQ_INVALID = 0,
	EHTTPREQ_GET,
	EHTTPREQ_POST,
};

//! Type of target servlet
enum EServletType
{
	ESERVLET_UNKNOWN = 0,

	ESERVLET_CHARACTER,
	ESERVLET_WORLD,
	ESERVLET_SYSTEM,
};

namespace rapidjson
{
  struct DocumentValue
  {
    shared_ptr<Document> val;
    DocumentValue():val(std::make_shared<Document>())
    {
      val->SetObject();
    }
    Document &operator * () {return *val.get();}
    Document *operator -> () {return val.get();}

    Value &operator [] (const char *index){return (*val.get())[index];}

	  template <typename T>
	  void AddMember(const UTF8<>::Ch* name, T value) {val->AddMember(name, value, val->GetAllocator());}

	  void AddMemberString(const UTF8<>::Ch* name, const UTF8<>::Ch* value)
    {
      val->AddMember(name, 0, val->GetAllocator());
      (*val)[name].SetString(value, val->GetAllocator());
    }
  };
};

///*TO REMOVE*/int NOTHROW GetResponse(RString &response, const char *site, const char *query = NULL, const char *postString = NULL, const char *adv = NULL);
///*TO REMOVE*/rapidjson::DocumentValue NOTHROW GetResponseJSON(const char *site, const char *query, rapidjson::DocumentValue &data);

int NOTHROW GetResponse( EHttpRequestType eRequest, RString &response, const char* sServerPath, const char* sArgumentList, const char* sPostData );
rapidjson::DocumentValue NOTHROW GetResponseJSON( const char *site, const char *query, rapidjson::DocumentValue &data );


extern void SetInstanceId( const char* id );
extern void SetInstanceKey( const char* key );

extern const char* GetInstanceId( void );
extern const char* GetInstanceKey( void );


#endif
