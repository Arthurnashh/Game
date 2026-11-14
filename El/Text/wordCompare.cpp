// Preproc.cpp: implementation of the Preproc class.
//
//////////////////////////////////////////////////////////////////////

#include <El/elementpch.hpp>
#include <Es/Common/fltopts.hpp>

#include "wordCompare.hpp"

struct SimCharDesc
{
	const char *chars;
	float sim;
};

//@{
/*!
\name Word related penalties
*/
struct WordPenalties
{
	//! a string contains more characters
	float aMorePenalty;
	//! b string contains more characters
	float bMorePenalty;
	//! characters become less and less important as going through the string
	float charCurve;
};


const float LowUpPenalty = 0.005; //!< not matching upper/lower
const float LangPenalty = 0.01; //!< not matching languague style
const float DiffPenalty = 0.5; //!< characters are completely different

 //= 0.9
//@}

//@{
/*!
\name Substring related penalties
*/
//! penalty for skipping starting character
const float SkipPenalty = 0.03;
//! skipping characters become less and less important as we skip more of them
const float SkipCurve = 0.67;
//@}

static char LangUp[]= {"ÁÈÏÉÌÍÒÓØŠÚÙÝŽ"};
static char LangLo[]= {"áèïéìíòóøšúùýž"};

static const SimCharDesc Simi[]=
{
	{"áa",LangPenalty},
	{"èc",LangPenalty},
	{"ïd",LangPenalty},
	{"éìe",LangPenalty},
	{"íi",LangPenalty},
	{"òn",LangPenalty},
	{"óo",LangPenalty},
	{"ør",LangPenalty},
	{"šs",LangPenalty},
	{"t",LangPenalty},
	{"ùúu",LangPenalty},
	{"ýy",LangPenalty},
	{"žz",LangPenalty},
};

static float DiffLang(char a, char b)
{
	if (a==0 || b==0) return 1000;
	const int nSimi = sizeof(Simi)/sizeof(*Simi);
	for (int i=0; i<nSimi; i++)
	{
		const SimCharDesc &sim = Simi[i];
		if (strchr(sim.chars,a) && strchr(sim.chars,b))
		{
			return sim.sim;
		}
	}
	return 1000;
}

inline char ToLowerLang(char c)
{
	const char *langUp = strchr(LangUp,c);
	if (!langUp) return c;
	return LangLo[langUp-LangUp];
}

class DiffMatrix
{
	unsigned char _diff[256][256];

	public:
	DiffMatrix();
	float operator () (char a, char b)
	{
		Assert(a!=0);
		Assert(b!=0);
		unsigned char ret = _diff[unsigned char(a)][unsigned char(b)];
		return ret*(1.0f/255);
	}
};

DiffMatrix::DiffMatrix()
{
	for (int i=0; i<256; i++)
	{
		for (int j=0; j<i; j++)
		{
			float diff = 0;

			char il = tolower(i);
			char jl = tolower(j);
			if (il==jl)
			{
				diff += LowUpPenalty;
			}
			else
			{
				il = ToLowerLang(il);
				jl = ToLowerLang(jl);
				// matching upper / lower case
				if (il==jl)
				{
					diff += LowUpPenalty;
				}
				else
				{
					// try to match language signs
					float diffL = DiffLang(il,jl);
					if (diffL<100) diff += diffL;
					else diff += DiffPenalty;
				}
			}
			
			int diffI = toInt(diff*255);
			saturate(diffI,0,255);
			_diff[i][j] = diffI;
			_diff[j][i] = diffI;
		}
		_diff[i][i] = 0;
	}
}

static DiffMatrix GDiffMatrix;

float WordDifference
(
	const char *a, const char *b, float maxAcceptable, const WordPenalties &pen
)
{
	#if _DEBUG
	// store compared words so that they are visible in debugger
	const char *a0 = a, *b0 = b;
	#endif
	int lb = strlen(b);
	int la = strlen(a);
	float dif = 0;
	float difFactor = 1;
	while (*a && *b)
	{
		char ca = *a;
		char cb = *b;
		float difAB = 0;
		if (ca!=cb)
		{
			difAB = GDiffMatrix(ca,cb);
		}

		a++,b++;
		// each following letter is less significant
		dif += difAB*difFactor;
		if (dif>maxAcceptable) return dif;
		difFactor *= pen.charCurve;
	}
	while (*a)
	{
		dif += pen.aMorePenalty*difFactor;
		if (dif>maxAcceptable) return dif;
		difFactor *= pen.charCurve;
		a++;
	}
	while (*b)
	{
		dif += pen.bMorePenalty*difFactor;
		if (dif>maxAcceptable) return dif;
		difFactor *= pen.charCurve;
		b++;
	}
	// if the pattern is short, any difference is more significant
	if (dif<maxAcceptable)
	{
		// we want to avoid matching very short words, as they may be many
		static const float shortEnhancerA[]=
		{
			1000,6,5,2,1.08,1.04,
		};
		int nShortEnhancerA = sizeof(shortEnhancerA)/sizeof(*shortEnhancerA);
		static const float shortEnhancerB[]=
		{
			1000,4,2,1.04,1.01,
		};
		int nShortEnhancerB = sizeof(nShortEnhancerB)/sizeof(*shortEnhancerB);
		if (la<nShortEnhancerA)
		{
			float shortEnhancer = shortEnhancerA[la];
			dif *= shortEnhancer;
		}
		if (lb<nShortEnhancerB)
		{
			float shortEnhancer = shortEnhancerB[lb];
			dif *= shortEnhancer;
		}
	}
	return dif;
}

float WordDifference(const char *a, const char *b, float maxAcceptable)
{
	WordPenalties defWordPenalties={0.1,0.05,0.8};
	return WordDifference(a,b,maxAcceptable,defWordPenalties);
}

float WordDifferenceSubstring(const char *a, const char *b, float maxAcceptable)
{
	float maxDifference = maxAcceptable;
	WordPenalties substrWordPenalties={0.01,0.2,0.8};
	float skipPenalty = 0;
	float skipPenaltyFactor = 1;
	for (const char *aStart=a; *aStart; aStart++)
	{
		float diff = WordDifference(aStart,b,maxDifference,substrWordPenalties);
		diff += skipPenalty;
		if (diff<maxDifference)
		{
			maxDifference = diff;
		}
		skipPenalty += SkipPenalty*skipPenaltyFactor;
		skipPenaltyFactor *= SkipCurve;
	}
	/*
	// reverse substring is desired only when similiariry is significant
	// we do not think a 
	skipPenalty = 0;
	skipPenaltyFactor = 1;
	for (const char *bStart=b; *bStart; bStart++)
	{
		skipPenalty += SkipPenalty*skipPenaltyFactor;
		float diff = WordDifference(a,bStart,maxDifference);
		if (diff<maxDifference)
		{
			maxDifference = diff;
		}
		skipPenaltyFactor *= SkipCurve;
	}
	*/
	// if everything failed, we could try to find a common substring
	return maxDifference;
}
