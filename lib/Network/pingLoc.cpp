#if _ENABLE_CHEATS && _PROFILE
# pragma optimize("",off)
#endif

#include "El/elementpch.hpp"
#include "pingLoc.hpp"
#include "Es/Algorithms/qsort.hpp"
#include "El/Math/mathOpt.hpp"


void PingLocalize::Add(RString country, int region, float ping)
{
  if (!country.IsEmpty())
  {
    _country.AddSample(country,ping);
  }
  // region list based on http://docs.poweredbygamespy.com/wiki/Region_Codes_for_Game_Sessions
  // lowest bit set is always the main region
  bool firstBit = true;
  for (int i=0; i<sizeof(int)*8; i++)
  {
    int mask = 1U<<i;
    if  (region&mask)
    {
      if (firstBit) _region.AddSample(mask,ping),firstBit = false;
      _subRegion.AddSample(mask,ping);
    }
  }
}

void PingLocalize::Best(RString &country, int &region, int &subRegion)
{
  // find country and region with the lowest ping value
  if (_country.IsValid()) country = _country.Best();
  if (_region.IsValid()) region = _region.Best();
  if (_subRegion.IsValid()) subRegion = _subRegion.Best();
}


void PingGeoLocalize::Add(int latitude, int longitude, float ping, const char *id)
{
  _items.Add(Item(float(latitude),float(longitude),ping,id));
}

/// angular distance on the Globe (input and result in degrees)

static float AngularDistOnGlobe(float lat1, float lon1, float lat2, float lon2)
{
  const float toRad = 3.14156363f/180;
  const float toDeg = 1/toRad;
  lat1 = toRad*lat1;
  lat2 = toRad*lat2;
  lon1 = toRad*lon1;
  lon2 = toRad*lon2;
  return acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon2-lon1))*toDeg;
}

  // adapt the position to maximize a function: sum of abs(avgSpeed * dist(x,i.pos)-i.ping)
  // a function to maximize

#if _PROFILE
#pragma optimize("",off)
#endif


void PingGeoLocalize::Best(int &latitude, int &longitude)
{
  latitude = longitude = INT_MAX;
  // select a few fastest servers only
  const int maxInput = 40;
  if (_items.Size()>maxInput)
  {
    struct CompareItem
    {
      int operator () (const Item *i1, const Item *i2) const
      {
        return sign(i1->ping-i2->ping);
      }
    } compare;
    QSort(_items, compare);
    _items.Resize(maxInput);
  }


  const float globeRadius = 6371; // km
  const float pi = 3.1415926536f;
  const float speedOfLight = 300000; // km/s
  const float degToKm = globeRadius*2*pi/360;

  // find fastest server
  int best = -1;
  float bestPenalty = FLT_MAX;
  int nBest = 0;
  float sumBestLng = 0;
  float sumBestLat = 0;
  for (int i=0; i<_items.Size(); i++)
  {
    const Item &item = _items[i];
    float penalty = item.ping;
    if (penalty>bestPenalty) continue; // fast path - skip contradiction computation when we already know we are worse then best
    for (int j=0; j<_items.Size(); j++)
    {
      const Item &itemJ = _items[j];
      float dist = AngularDistOnGlobe(itemJ.lat,itemJ.lng,item.lat,item.lng);
      float speed = dist*degToKm/(itemJ.ping*0.001f);
      // we expect to be under half of speed of light, as we need to travel there and back
      if (speed>speedOfLight*0.6f)
      {
        penalty += 100; // one contradiction is worth 100 ping
        if (penalty>bestPenalty) break; // fast path - skip contradiction computation when we already know we are worse then best
      }
    }

    if (bestPenalty>penalty)
    {
      bestPenalty = penalty;
      best = i;
      sumBestLng = item.lng;
      sumBestLat = item.lat;
      nBest = 1;
    }
    else if (bestPenalty==penalty)
    {
      sumBestLng += item.lng;
      sumBestLat += item.lat;
      nBest++;
      
    }
  }
  if (best<0) return;

  float bestLat = sumBestLng/nBest;
  float bestLng = sumBestLat/nBest;

  latitude = toInt(bestLat);
  longitude = toInt(bestLng);

}
