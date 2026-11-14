#include "El/elementpch.hpp"
#include "httpRequest.h"
#include <Es/Strings/bString.hpp>

void GeoLocationFromIP(float &latitude, float &longitude, const char *ip)
{
  longitude = FLT_MAX;
  latitude = FLT_MAX;
  const char geoLocURL[] = "http://services.ipaddresslabs.com/iplocation/locateip?key=SAKF9WKS23364926J8NZ&ip=";
  RString responseStr;

  BString<1024> url;
  url.PrintF("%s%s",geoLocURL,ip);

  long http_code = GetResponse(EHTTPREQ_GET,responseStr,url,NULL,NULL);
  if (http_code<400)
  {
    const char *latStr = strstr(responseStr,"<latitude>");
    const char *lngStr = strstr(responseStr,"<longitude>");
    if (latStr) latitude = (float)strtod(latStr+sizeof("<latitude>")-1,NULL);
    if (lngStr) longitude = (float)strtod(lngStr+sizeof("<longitude>")-1,NULL);
  }

  /*
  const char geoLocURL[] = "http://api.ipinfodb.com/v3/ip-city/?key=4966b333a712481b32f65cc4c68426a656a2eb821b53f876f8e234938b18c955&format=json&ip=";

  RString responseStr;
  long http_code = GetResponse(responseStr, geoLocURL,ip,NULL);
  if (http_code<400)
  {
    rapidjson::DocumentValue response;
    response->Parse<rapidjson::kParseDefaultFlags>(responseStr);
    if (!response->Empty())
    {
      const rapidjson::Value &lat = response["latitude"];
      const rapidjson::Value &lng = response["longitude"];
      if (!lng.Empty())
      {
        if (lng.IsNumber()) longitude = (float)lng.GetDouble();
        else longitude = (float)strtod(lng.GetString(),NULL);
      }
      if (!lat.Empty())
      {
        if (lat.IsNumber()) latitude = (float)lat.GetDouble();
        else latitude = (float)strtod(lat.GetString(),NULL);
      }
    }
  }
  */
}
