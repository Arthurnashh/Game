#include <El/Common/randomGen.hpp>
#include "XA2CustomAPO.h"

//--------------------------------------------------------------------------------------
// Name: CSimpleAPO::CSimpleAPO
// Desc: Constructor
//--------------------------------------------------------------------------------------
CompressorAPO::CompressorAPO()
  : CSampleXAPOBase<CompressorAPO, CompressorAPOParams>(),
  _treshold(1.f),
  _r(1.f),
  _dt(0.0f),
  _attackPhase(0.0f),
  _releasePhase(0.0f)
{
  ZeroMemory(&_ratios[0], sizeof(_ratios));
}

void CompressorAPO::OnSetParameters(const void* pParams, UINT32 cbParams)
{
  CompressorAPOParams *params = (CompressorAPOParams *) pParams;

  _treshold = floatMax(0.01f, params->_gain);
  _attackPhase = floatMax(params->_attackPhase, 1) * 0.001f;
  _releasePhase = floatMax(params->_releasePhase, 1) * 0.001f;
  _r = floatMin(1.f, params->_ratio);

  const WAVEFORMATEX& wave = base::WaveFormat();
  _dt = (1.0f / (float) (wave.nSamplesPerSec * wave.nChannels));
}

void CompressorAPO::DoProcess( const CompressorAPOParams& params, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels )
{
  if (absfxa2(_r - 1.f) < 0.05f)
    return;

  float dtAttack[] = { _dt * ((_ratios[0] - _r) / _attackPhase), _dt * ((_ratios[1] - _r) / _attackPhase) };
  float dtRelease[] = { _dt * ((1.f - _ratios[0]) / _releasePhase), _dt * ((1.f - _ratios[1]) / _releasePhase) };
  
  for (UINT32 i = 0; i < cFrames * cChannels; i += cChannels)
  {
    // for each channel
    for (UINT32 c = 0; c < cChannels; c++)
    {
      int idx = i + c;
      float val = pData[idx];
      float sample = absfxa2(val);

      float & r = _ratios[c];

      // attack
      if (sample > _treshold)
      {
        /*if (ratioUp)
        {
          r += dtAttack;
          r = min(r, R);
        }
        else*/
        {
          r -= dtAttack[c];
          r = floatMax(r, _r);
        }

        float delta = sample - _treshold;
        pData[idx] = signfxa2(val) * (_treshold + delta * r);
      }
      else
      { 
       /* if (ratioUp)
        {
          r -= dtRelease;
          r = max(r, 1.f);
        }
        else*/
        {
          r += dtRelease[c];
          r = floatMin(r, 1.f);
        }

       //pData[idx] *= r;
      }
    }  
  }
}

void Discretizer256APO::OnSetParameters(const void* pParams, UINT32 cbParams)
{
  //const double dBMin = -100;

  for (int i = 0; i < lenof(coefs); i++)
  {
    coefs[i] = i / 256.0f;
  }
}

void Discretizer256APO::DoProcess(const Discretizer256Pars& pars, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels)
{
  int count = cFrames * cChannels;
  float quality = (pars.voiceQuality * pars.voiceQuality)*32; //max 32 levels
  int Levels = toInt(quality);
  saturate(Levels, 2,Coef2Div);
  int multCoef = Coef2Div / Levels;

  while (count--)
  {
    FLOAT32 s = signfxa2(*pData);
    FLOAT32 v = absfxa2(*pData);
    int idx = toInt(v * Levels) * multCoef;
    saturate(idx, 0, Coef2Div);
    *pData = s * coefs[idx];
    pData++;
  }
}

bool GaussianNoiceAPO::initialized = false;
float GaussianNoiceAPO::GaussNoise[GaussianNoiceAPO::NoiseCount];

void GaussianNoiceAPO::OnSetParameters(const void* pParams, UINT32 cbParams)
{
  /* Setup constants */
  const static int q = 15;
  const static float c1 = (1 << q) - 1;
  const static float c2 = ((int)(c1 / 3)) + 1;
  const static float c3 = 1.f / c1;
  
  if (!GaussianNoiceAPO::initialized)
  {
    for (int i = 0; i < GaussianNoiceAPO::NoiseCount; i++)
    {
      GaussianNoiceAPO::GaussNoise[i] = (2.f * ((GRandGen.RandomValue() * c2) + (GRandGen.RandomValue() * c2) + (GRandGen.RandomValue() * c2)) - 3.f * (c2 - 1.f)) * c3;
    }
  
    for (int i = 0; i < lenof(GaussianNoiceAPO::GaussNoise); i++)
    GaussianNoiceAPO::initialized = true;
  }
}

void GaussianNoiceAPO::DoProcess(const GaussianNoisePars&pars, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels)
{
  int count = cFrames * cChannels;

  float voiceCoef = (1.25 * pars.voiceQuality); saturateMin(voiceCoef, 1.f); //no voice degradation first 25% of the distance
  float noiseCoef = 0.4*(1-pars.voiceQuality)*(1-pars.voiceQuality) + 0.02;  //some noice should be always used

  while (count--)
  {
    // interpolate between the voice and noise
    *pData = voiceCoef*(*pData) + noiseCoef*GaussianNoiceAPO::GaussNoise[(sampleIx++)%GaussianNoiceAPO::NoiseCount];
    pData++;
  }
}

void LimiterAPO::OnSetParameters(const void* pParams, UINT32 cbParams) 
{
  LimiterAPOPars *pars = (LimiterAPOPars*) pParams;
  
  _limit = pars->limit;

  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    _envelopes[i].Setup(pars->attackMs, pars->releaseMs, pars->sampleRate);
  }
}

void LimiterAPO::DoProcess(const LimiterAPOPars&, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels)
{
  int count = cFrames;// * cChannels;

  if (cChannels == 1) 
  {
    EnvelopeFollower &e = _envelopes[0];
    while (count--) 
    {
      e.Process<FLOAT32, 1>(1, pData);
      if (e.envelope > _limit)
        *pData = *pData / e.envelope;
      pData++;
    }
  }
  else if (cChannels == 2)
  {
    EnvelopeFollower &e0 = _envelopes[0];
    EnvelopeFollower &e1 = _envelopes[1];

    while (count--)
    {
      e0.Process<FLOAT32, 1>(1, pData);
      if (e0.envelope > _limit)
        *pData = *pData / e0.envelope;
      pData++;

      e1.Process<FLOAT32, 1>(1, pData);
      if (e1.envelope > _limit)
        *pData = *pData / e1.envelope;
      pData++;
    }
  }

}

MonoAPO::MonoAPO()
  : CSampleXAPOBase<MonoAPO, MonoAPOParams>(),
  _ratio(1.f)
{
}

void MonoAPO::OnSetParameters(const void* pParams, UINT32 cbParams)
{
  MonoAPOParams *params = (MonoAPOParams *) pParams;

  _ratio = params->_ratio;
}

void MonoAPO::DoProcess(const MonoAPOParams&, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels)
{ 
  if (cChannels == 1)
    return;

  if (_ratio < 0.02f)
    return;

  if (cChannels == 2)
  {
    if (_ratio > .980f)
    {
      // TODO: SSE2
      for (UINT32 i = 0; i < cFrames * cChannels; i += cChannels)
      {
        float avg = (pData[i] + pData[i+1]) * .5f;

        pData[i] = avg;
        pData[i+1] = avg;
      }
    }
    else
    {
      float t0 = _ratio;
      float t1 = 1.f - t0;

      for (UINT32 i = 0; i < cFrames * cChannels; i += cChannels)
      {
        float avg = (pData[i] + pData[i+1]) * .5f;

        pData[i] = pData[i]*t0 + avg*t1;
        pData[i+1] = pData[i+1]*t0 +avg*t1;
      }
    }
  }
  else
  {
    float invcChannels = 1.f / cChannels;

    // almost mono
    if (_ratio > .980f)
    {
      for (UINT32 i = 0; i < cFrames * cChannels; i += cChannels)
      {
        float sum = pData[i];

        for (UINT32 c = 1; c < cChannels; c++) 
        {
          sum += pData[i+c];
        }

        float avg = sum * invcChannels;

        for (UINT32 c = 0; c < cChannels; c++)
        {
          pData[i+c] = avg;
        }
      }
    }
    else
    {
      float t0 = _ratio;
      float t1 = 1.f - t0;

      for (UINT32 i = 0; i < cFrames * cChannels; i += cChannels)
      {
        float sum = pData[i];

        for (UINT32 c = 1; c < cChannels; c++) 
        {
          sum += pData[i+c];
        }

        float avg = sum * invcChannels;

        for (UINT32 c = 0; c < cChannels; c++)
        {
          pData[i+c] = pData[i+c]*t0 + avg*t1;
        }
      }
    }
  }
}