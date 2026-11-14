#include "../wpch.hpp"

#if _VBS2 && !_VBS2_LITE
#include <El/QStream/qbStream.hpp>
#include <Es/Common/win.h>

#include "../../../extern/HASP/hasp_hl.h"


enum Customers           {C_Invalid=-1,C_Trial, C_Developer, C_General, C_USMC, C_USSF, C_ADF, C_UK, C_USARMY, C_LS       , C_CAF,   C_End};
static char *customer_String[C_End] = {"Trial", "Developer", "General", "USMC", "USSF", "ADF", "UK", "USArmy", "Lasershot", "Canada"};
static bool isTrialKey[C_End] =       {true,     true,       false,     false,  false,  false, false, false,   false,       false};

static Customers HASPCustomerId = C_Invalid;

static const char *vendor_code[C_End] = {
    "ugW5A3r+757YaQvcikZyVyWm2SP8ADPnlBxc+rcrma3U7KgR5Ehpk1ds8uVZLRnnFsLSHTqxaxQtL2wD"
    "nwOcR9UFoJXqaEIEuUeROMAIns0vrqSSjFqvPQQyG8F4ucvfPfXFFHPCkEeEL8wyjuyD72LYQtCsObKU"
    "82nEpYuGABIqaM2xYtM49UZYHsKiqosFPOJ4ILzneIdD0e7z7VHXXImtuG7Ar4w1wcKOYij7sUp3rikt"
    "G6mI8PNnMA9FpCSQAgqQcZxIeL5/CthRc8FQzRgm9mBKBJEeTMQnrApuNGwrFZr/SJNur0KRjbKM8JuY"
    "optfP0xdGe1/QtiPYUjPdYsMbkQJQavknX0ssMf+pPkyWB5XOABhSxIIsebi5nYitSpJt0TNWbCSQRws"
    "H/P2bPFB0KlNDZajjbZVkqaRphYK7ffLgF1GnaE6FvDvShgJdpWDAUJja1xs5Dl6XTJ3DemhgiFVIudG"
    "wr+fkJgaDgRNP62FkyUaZMCiGS0AdfbBui+/jKv/uYiaTaGes65xsjgyRi+Fv/d7S/LsITErqCs3n+sY"
    "qXjJ2g4TPDH40EcFSLbt4ebYXgpxCcR0vsSmjJr2zV7uyvg1W+bwzAn8DQGMn+eJm5UbTLFHQNoCuy5P"
    "1HawnMf5H8pjQ4cyrYKfZtJfZW9VtFZsygaMQHYWaudaMaQTK1bMVsuZg9uwo56a4y6EhwGObjIOT/pa"
    "N80SJM1ObQdZ+w=="
    ,
    "QsKzAzs7l0mOhGt3FLue/Cpq0sByIw0TJubdoFxZWIH1whTBe/VgAy8lvZYRl2Fy4u4nqtLeqqLMulgT"
    "yqWpKmPgHJcgWNZtLqz8xjarhAPGe1Jg4ntH+3MbzvQhuq1myaWdNajKC6L3aA0iNyW/hxJV9Q0iD1Nc"
    "y98iIcIjaayt4GPZNUZr5Gv5Do805UOtz8K/xwwnc+jGpUgyCDnm3WINJGzZer8mwFkrd1affcL++h13"
    "/cwwdJbNkkf6FfpZyVwrQcj65RJOe2996iP71QfM5oGjoyeLyqrByuvRcgf/FRgAGhZOC/QavImiXjNT"
    "vJzuj7Lkl6np9LLupMvcyerILwVAO9IQo4lgDbuYgpiZ5GAlIqNQFLpl5bYrBiWsumxPjfFQRO5f9Vp9"
    "rnvDC60EAB4Oglo9XKkMDAZ/pRb6xCbI9ED2UWvmoCpgBZ+d6QphS6GAx+BSZDNw2DKHhP66Al9Qt9PW"
    "Aq/farTU4513V5djF1Gz6BDgQol27RcjFSqDXPUPfFvDAa9PZCjCZjyafGEoR9N5upMEGm2FsEXGsb4X"
    "L8lSjwbPdY1yeaHXmnG/yFJOHpm1KYl4lrqNJ/ALhmfBMwAkqYhFGwFcfln0tjeBpOKp0ljQxS4bSZ+T"
    "boD2b6BEiOgZzHhq0LlJqytQ2dxFXlFdlddt6oFga/FQ8E5zRC2Lgv4QaRB03dQX38FIzG1UrbuLY7AP"
    "deXRNpekOdQtdg=="
    ,
    "+wrFKWYj5xWX+abFaYRZUjwPpnWaQ7GoqbTNLzaM/ZyS5mqJoLzqiT9s1BPwPSJO36wvsS9Su+KsHrr5"
    "kUdO9OM1V4dtZinoZQA5/qflUnBnE0nWcnsXlFHyjoE0IvmFxOVqvvOWmPYJ89IiaMnkk7MC80hMvdek"
    "wZ2KgVgUhZXHiZTODBJZgcuH7pG8mHy3vqUfTk9GVy08sztYyBWgY4nHpbF7bmSrjoLTrt7P2bF+UoBg"
    "NX6XwzNDuH5vllHc3xav0ofLCqkYqxocnWnm+z2XtRU+lkitIPkRi1zzehLNODWACPfCGpAfuYJt26t/"
    "2aT0QJInk1VYRmQE1HdnmXPD/nzNdbzh6AneompERmPUjQRUrlB3Quw7ZP1afSx3IQh5klXsu4N1luK9"
    "5S7FHakHAOYWFqgew8WVmh04kStPMtKBp7hA3kGnBvdE5cG+sqCdbXeXONIoDjBI8aJqGIOo9Bk8vpmP"
    "GF4r6GxE4pDSiou0chpqO/uXpTKyv9x/lzimmZyDoKWUUCfeJfrKYbtdRGI3FzpGJbR8JJwL4+EnIc2+"
    "am0f8v6AmiW2QiIhjOhaVilzw1etKoIFgq3kcen/beqKrtx2A/8+sAHOorVHRH1n/B9Mq6p543w1hGx5"
    "YzHLwQltq/Oytjjw7yErt+fwBRf0kS78/JAQZRI5YzQglhm5WEIHkIgp/a7rPcWOXy+QSf2skEe4HVC9"
    "5W9tq7ZM6U0osg=="
    ,
    "NtACBliH6zjOSbcp6OexwPjhn6FB1vuI9ub6xAHVOo6KKDYhSdnQl+fkEG7RRX/rFtRtZHQu2D/i0wxV"
    "wMuZt7CZUDbCi+9LmsCDWh+ZtMClECiI706uNS6GMoPi2MDZZM6b3wzibnFLuWXTfgE9Cq7+U29Tccc1"
    "ZqSw5LEThw0nNJZxCBUGelEd6/CwrxnLqzldorYWXeYD8cG2HnGJ7/kvFqT51q37D58VdhO865MVqFLW"
    "mskzIqWla8wHbCc4ALprq7x08caM6pHioPcEUI250BK+HjDGZR0ePn3LcudvAMSGJWKHQ3oBJZ+QTUiZ"
    "NO2212XfHSCqbnc+ARv6xAQknjWc0ppk+XIs78KJGTWCQaVzhxFH1jPEOtFK4KA//sTyAt04ysXHM4N7"
    "j7qGNR16mMAv2Ez59fls94UjVAJ+sySvcJZ+c+Mu87BEeL817XAiujQKNtHAWY1vSkViIkVj+MRrlqOp"
    "APDMDAj6zNk4ZDhKqT5uqTM/X3QfHHV77v/kPz8Jof5lBJCd3yeC2rjc6weXUcm7nHEJB3f5kfsJ5UI+"
    "KfjXa8RgTv+lIgfCOWGOfB2kjxRSkv/tAjdOmMjRFHCUW3ck9ttkdXxjJw3zgkTO80Gaem6Zt4rPYOqQ"
    "BumugDM1vY94vtDyjXX/bbUNGK3/LrW+AjlB8/S1WNUj2yPTv4YEfWNz4PM7BINVHa67CYgbeQt9Oq4l"
    "d335ed7wrEZXPA=="
    ,
    "adRySBYJ8wd0JI2WIq2clRYNQR3J5SjVIVEN3RcnaeCi0/7o/6j6flzIXZUTaPwC0oxovfZOaOKvhtBp"
    "aXwfYKZCa4eEqGqxRmLu+Rr/zRNGnMXHROlJTerLIfyhhtGEByMyfXlw3RCMw/GWCilIhRU7PqeWhikm"
    "DpK6lHcEpCqlWFZmycx1d9uo1QMTVB8UaqFcyBvC0QM/XEJdGnujYLwO4qgB+Cmfe0kCEVKhSyTcfQSG"
    "uYyefa7VUdbxioiFxzFUBiHLgkUZAwH4lD/loAG1NcS5lVzuTDzqxziSIjRDP8xqkCwQx1yWjXcZSOMH"
    "mUUbd0dABp+TjcwqymtrBf/1ltPH+Tf/rSm8g2mzX/W8MQME79xpVEzWDdMmzTbiJqiZFzarOcqdcFYL"
    "LFQ715cLnOUm43qAu80IpzdNG6aIBiJRFbRes1KT7Yjq/Xe9y59s6wnsXO1/2R6KdqZwOyIuMuSGIkU4"
    "NdcnmwM8xA0+FJUMu8uTvCBCBko5vkTJguarQYEzDmxmSOqFjLaygSGt7tIWAyN0SQ2mEKPhZbVBXNje"
    "MXqNq30M/41Kv6K4XEwngm3vqPWBA6V2jDpP1qVGuhL+xksjhPmmzhyHvvXPKVjWYvdCRh/D3NXK/dWu"
    "Fx80NJexyw1SbyGUIJr/okP4Bjj7/dogTEjzvoREVWNwWp+8mP+a1xCe0J6Am9VM4l/ODvR6djGq3XbO"
    "gVTrBAtYp85RRg=="
    ,
    "eIaqWCB+TESTVgYoB3G1htciAhIX1ReWj5oHH1vNcnie8AyblDwlksMSzMViQ551Xa03yiKs/tqF7wGG"
    "yk4/3UoUY57ekci8ScMeplQUAeTvQoPo/lQEq5Vhn+Rvuv/W0m4n00vb7DBE2LFiqBwpCXBmbrpoUMED"
    "ZvP6Q7SyxH+q+CmDqNlScPeBnkfjLgrcHfmkHw0bl58VvoHzLz0aOw8mjnTXZ1vX4XodwVxFACXqpAur"
    "TMQyj4f3+1ILvQGp0/8PkQw4CtO/NRGrF8Vfzn1t7bH6mlTqcNhl5XWE9KmzNEllEfZKXIfbFc3CL05T"
    "JHIRf9B0C4e9D2VYwYoYIzQ5Z4nTUn9u7WJ7epcsvBwmnzBa5UOrvpgJrW0xLLkZzSk/fPP3EKDIlEqs"
    "ohuXE5tIG78ZuKbyzG0a8nvfkBJ92nDqO1XcXPIpgNehiXRD+opWF051LDsfjFezC1yt/uVbAJZh9moT"
    "vgaRBvXYP4l9HsQzV5P0BZfprCJwHfeZgK+wltOyveLy5U6W98JZSWCDIGUX5T4iXOTvRO5RVCPByxay"
    "sIo/nR6W3xeO2a1xy+rsYZDyyH9TljJ0V+D6p7GVn+H1PDYAgq0HiUF/7vJ5leHF1WX8P2loZiroTdM2"
    "KZmKPCiPeQRRNtLSIbc/62zK+srhi8/7Xsq6WW6YrfTfeGdm8oBUqO22o8/2bUs8ohUJoFm4z344HisJ"
    "o8M4xVxL6d8pLQ=="
    ,
    "9Ok0aGFYQamVpLyjVi/GRya8puhmqpKPARXSvDG0IIhxvjyBgD/XaIchh6g+5KqELw+jI5Ird6pc0j5N"
    "GcHFq/axQV+ovo1vuEF5qlj8+yG7dUmwHtDnry8fDlnNLA0L0L9yND1FyFbcM/4QdtY9wiTGmbPSQ/8m"
    "Mx61/OM5ulYUllftwW7r3Id0oLl2IZLXAbI2PHG5WfACQXgeQ2JFcZXcPeSdEMkryIPm1jSqJiUGtHv3"
    "xxyrGHH/0B72UrCGW5ox1kKHCX7+Zk9ukpOFbq4yrxAuHHZEUABDsXgIyiTe1popLDNqQQO+LQxfCfJS"
    "cxpE6uX6LYrJ5xlZeCvqTKA6m54AkLi6Qh8Z+6/Q9O6jK5j/5KpO6qlbX/uQHMqtcqV4JwnoIc1USXbm"
    "zvzurRzqltRGWS+2dSONLCIcbTFXLLHQZdHrcDhuYQgtFtIFx8F+5itz90qnIVAIH+yvoELkXDDzO2Mp"
    "zpmGZSWlW8XT3607lqz4rRSD7rYVHVmvSwlyicYgXITV+tCNz5uJvMGscY0jsBp0pkYCpS80J/PS7Kt2"
    "4lLNG9wEtRvReL/8LBTtl2xeqmkD0a4KWyRH84iAFdzI9+a8+U2ZCXdSgD1qJen0IK37ECNC6DC7KDvs"
    "AmTBfjgTm4EQ5I52Myt/arDRXCy67WdZ6PXFWq2pwKnlJCIK7oxAY3GUz/jXjM5F7d9TuWhGOvR1xjUp"
    "2kFpXDFr30y0Nw=="
    ,
    "MxYy3Yk7/UzJJvyn30cpWlaU/pgrmNa7ryy3IC9b5aII704Zgrd1sEfqOfMdUUNgjD5r4murh2EUUTrc"
    "+B5GEjalEkm1xgva/6buN1bSn4V9CuxfF1lLPSkxD7a6fiur4uwT69N8Iwx77vkm58tUfIMSmVglMkrb"
    "QizmS0BA7pYVGFgh13pIOqV32fzFlnwpJ2hGjTWEyUmSfhXwqi7uEy2yPLpa4T+Z3nrplNCG9HbP8KyR"
    "bjfmztY0Nk4fMIRs44/4EMJz4fTJ97y8314Ch8sLqVNn9OCGhzmVPk4H1OgIrskShShDgUuiSKh9FJ5y"
    "VyABO36Wjk2Dmt2U0GZq1tsF+Zy+hrlSwZEi54zr9jjkQoKBuJrS+xZeM1FitXrSqqzhgvY+/9/57vkN"
    "4ErNtirC+pXr++rSjLKahXdXPqqZm7HFpFyf2nQRd14FQzOwx94n8pE6qRr9u40JG09TLCyYVRbMoGKD"
    "YFKXqvYS/s7EhCnPTkGhPbCTdGBLEApnx+Z262WE0pPyvctZKdjGcpbI6eGieQIaF1hL6Nk7XeVqyHfb"
    "Y7spvuOjSdR6pdysmo7iagDOFhiIb0137zbAijNIavlR8S2BP0zXQS36z88fKzZId9BcHyNgoHTXexXQ"
    "OgnganJ0D7F7fwJUjdmZat51JKl+uTELb6rRva/ch2o/iNuasKU4EhQGDGLgLODTAzZxGx9Tapxhc6mI"
    "sc0aRkZkKSClKQ=="
    ,
    "UO8mUc+Zddd6xZzxmNrQF2ka7jJae7516ySL0R5t/SD+E22vrDlJ5smovov2JalDxSZd/KwpctCN2Ly8"
    "8AaNCCePRVtzp8aySBro9hD1RvF/ZXlsGNa9Q4XXJrxr9HJ1p9Eoa3JGRgdF3epDPe1zhJr4iJYFMXPb"
    "XQv123uZGXQDCL/RpUhwpbMYuYU377RLUbOPD5K8gIeb2xWYsGbv76gnbssI9r17A81WTEPlnRcZEra3"
    "LQ4YoYXmM+pElvUzvqpAnPpmeCF1acLKPE0QSsK93MRlwUtwV2lYvvK6tAtS5erIeKqJYedwERjRZAh4"
    "mC25fC9Fq5Z4np/x6l9Z0zhppFJN2nfztKrBNDMlL2aAks3tpaIgsw5Mg/B0oPqUYCdu3vWN3w39JiIX"
    "owhO4mygMm2QCJJqyNSyeqibkjUNSmm4ckmoGEH+yMPaEHeJ784sRm4hrZfjrQ/VYKGorVAsD1Epkadn"
    "JPvz6XRt7XTeLQ0QBmRaAkQZCIHxCFaP0gA1uc2d2tCVcIgCNXDh+9rqjA2xubyVmhnHNxsIMvE6/TYi"
    "lOpbEIX7ZOCASV3W49gH+XDXPS3AEyR6VBNobbvZTyQ33YD81qRvOnceSqAYpr1owhJ1JDh75ZUkoeHY"
    "eMZtcyVbA9p9MuFJ5Qfb53myGXxtJurXF5d6uspNKdIY5D5sXsPvnWuuno2hSkeZY4l0FVJANOgxKkSw"
    "XbWXtSfJR4fM"
    ,
    "+yo5QOCzymKL/S3/ECc8PqfT3lsz1ynMcf8CUqLsm/XENc3yeSKHyBr7rJLmTCYUCQvyorT7owxQ1tRG"
    "Vm049nZZmolsvIRGZC041+ZHbk5jCGaG1oilg28If145CpP5mqgTTzMH2tj2pLhg5Co8+zxHYshvNijs"
    "Dlc1FsY2FDaYQPOFSlRoK4s9FuuYFTJX0MhM/hxkLJbRRPsBTXOwSSW8BWAuAs3T7okzU13snjb+h7CR"
    "bzolfYmJeSIDBnTu1pk/Db/B0ggiuFIiMrR13MiXJWc3JFkyL9Z2DTU1Z2Fzuoc499pSDAA6MML3RU//"
    "CnscXPPcnSk+KNjfTx4rw+6SLZ2Qf9GX+Nl00YwldChENyQ8LOoqssFtXvnzyrr/rV+fDzl0d6rAT1u2"
    "coSrfPwSH2lIaen/xPvTBHZZ6saoBp4DdNswmNj1YJ2m3EUhomkKgn6/mWL6Z5+Gx3OIlVqCnx7/yCe1"
    "YLEKenU6zyU7XmH1aPXK9JqpvFIlErMAtvhoZEOTZbtc6XKobXQGA1k/rS50PzJ7ixtPfSwCv4Kbae2V"
    "dxmqpoYkQEZPpectY9pv3Qplcf40mJlcAbOzzIkZHuU9FftlW9o9NKnwUC4HeCsvIZ7X+qGTZ2l2r/9C"
    "Pvyfm1D3n8hJWn2PWQGK2vrlF6nSbK5WhsQO1/8fEV/B/qOSu19QnpVT3L1OzaP+TjnfH9j0FmXg7BRz"
    "Q6sjL5/O8NpseA=="
  };
static const char *vendor_codeSRM[C_End] = {
  "Trial"
  ,
  "Dev"
  ,
  "Gen"
  ,
  "t/O/CwTBR7B72ZnAcuqUD/cPRAS+/weS7Ko239zS4CqiEIAdplb75K6sY6guxq9kTIFXZwYU8xFA/4A5"
    "kTgQogu7Vx+KimLwKdFTz9Zx7pzK1cozehAVfiSbpkiu1773PPnw6G7hL6Sbe8dBRZcH0lWlrGzawUda"
    "fjEFQ+Eub04vLFE6fzbhfsnJqT8LPKmN6xvKYvi5KPuW1iHM1mXxMB/wCEHnBVSGmUFyzmkTDxK6WFpS"
    "iu96Ui7xlUPmJH0nY5DdJRhjAGLZevExF+t24AW87X+S7oL8M1P8GioxZB2mGX1OLxsOF+DY69Ovw62D"
    "xfI+yF7pFHyUQFKsnT6h1oVVTPP06JSA9wZ+hUkCRVFlvWITzX8BiMnyKfCk5s5/aLkZxQi2WMpbqmbZ"
    "byODVoe0p4o1A2kavIgoClTWQfOatST7c75OEUUR29D4OLlxJhzwDypmRyt33idpFfjbxwYFBHSjfzRi"
    "ozkPlzVmARaWTFDxfSxtGztQpeQl6gk9CSpT+6mk2cmLQl6fGKz/nrPEZrBYskDfffjmg77MYAaKrZyI"
    "JkWERJzV5NT8lh2fSoiGdnxcP6LRx7fT9TfUZOpUxJGZtywtX5Y2uPI+TgPvkHDgI7mlV8s/1EBXY6dv"
    "n9PyPYmqLTNUjhcLGM4JQMlEFcKoQWLqiKun87KLGWWyJAZ2MgmX8Af0PxVZbrAa3ROjwnNmQq0njbsV"
    "MVTNa6FfWEmeJKX5d2uGCxJx+SIz+TLE8LEsSPr15LOxQxxYD5M9PM/ZoSuBZPql3RA5rr+x79rCtcGt"
    "ZqWkowWxg10KPTqpkF1Ub/TnyNX96PJLg3v8oTvJH9s/luNU0k+1TNVrYxGvT9ZY5d1QnYbbCMD8+Mx5"
    "HJQ8Af8QPyOHt/2X9ftvfaAWTFA3K3VtgospP6S0I4AsKOS8Wi51blzkPAI="
    ,
    "USSF"
    ,
    "ADF"
    , 
    "UK"
    ,
    "USARMY"
    , 
    "LS"
    ,
    "CAF"
  };
// Check the error code returned from hasp.
bool CheckHASPError(hasp_status_t error)
{
  switch (error)
  {   
    case HASP_MEM_RANGE: 
      ErrorMessage("invalid memory address");break;
    case HASP_INV_PROGNUM_OPT:           
      ErrorMessage("unknown/invalid feature id option");break;
    case HASP_INSUF_MEM:       
      ErrorMessage("memory allocation failed");break;
    case HASP_TMOF:           
      ErrorMessage("too many open features");break;
    case HASP_ACCESS_DENIED:          
      ErrorMessage("feature access denied");break;
    case HASP_INCOMPAT_FEATURE:    
      ErrorMessage("incompatible feature");break;
    case HASP_CONTAINER_NOT_FOUND:
      ErrorMessage("license container not found");break;
    case HASP_TOO_SHORT:
      ErrorMessage("en-/decryption length too short");break;
    case HASP_INV_HND:
      ErrorMessage("invalid handle");break;
    case HASP_INV_FILEID:
      ErrorMessage("invalid file id");break;
    case HASP_OLD_DRIVER:
      ErrorMessage("driver or support daemon version too old");break;
    case HASP_NO_TIME:
      ErrorMessage("real time support not available");break;
    case HASP_SYS_ERR:
      ErrorMessage("generic error from host system call");break;
    case HASP_NO_DRIVER:
      ErrorMessage("hardware key driver not found");break;
    case HASP_INV_FORMAT:
      ErrorMessage("unrecognized info format");break;
    case HASP_REQ_NOT_SUPP:
      ErrorMessage("request not supported");break;
    case HASP_INV_UPDATE_OBJ:
      ErrorMessage("invalid update object");break;
    case HASP_KEYID_NOT_FOUND:
      ErrorMessage("key with requested id was not found");break;
    case HASP_INV_UPDATE_DATA:
      ErrorMessage("update data consistency check failed");break;
    case HASP_INV_UPDATE_NOTSUPP:
      ErrorMessage("update not supported by this key");break;
    case HASP_INV_UPDATE_CNTR:   
      ErrorMessage("update counter mismatch");break;
    case HASP_INV_VCODE:
      ErrorMessage("invalid vendor code");break;
    case HASP_ENC_NOT_SUPP:
      ErrorMessage("requested encryption algorithm not supported");break;
    case HASP_INV_TIME:
      ErrorMessage("invalid date");break;
    case HASP_NO_BATTERY_POWER:
      ErrorMessage("clock has no power");break;
    case HASP_NO_ACK_SPACE:
      ErrorMessage("update requested acknowledgement, but no area to return it");break;
    case HASP_TS_DETECTED:
      ErrorMessage("terminal services (remote terminal) detected");break;
    case HASP_FEATURE_TYPE_NOT_IMPL:
      ErrorMessage("feature type not implemented");break;
    case HASP_UNKNOWN_ALG:
      ErrorMessage("unknown algorithm");break;
    case HASP_INV_SIG:               
      ErrorMessage("signature check failed");break;
    case HASP_FEATURE_NOT_FOUND:
      ErrorMessage("feature not found");break;
    case HASP_NO_LOG:
      ErrorMessage("trace log is not enabled");break;
    default:
      ErrorMessage("Unknow error:%d",error);
  }

  return false;
}

struct Validation
{
  char       expire;  // expiration date
  SYSTEMTIME preTime; // Previous time we ran
  SYSTEMTIME expDate; // expirary date for system
};


const __int64 nanoSecInWeek = (__int64)10000000*60*60*24*7;
const __int64 nanoSecInDay  = (__int64)10000000*60*60*24;
const __int64 nanoSecInHour = (__int64)10000000*60*60;
const __int64 nanoSecInMin  = (__int64)10000000*60;
const __int64 nanoSecInSec  = (__int64)10000000; 


double SubSysTime( const __int64 datepart,const SYSTEMTIME* minend,const SYSTEMTIME* subtrahend )
{
  FILETIME ft1,ft2;
  __int64 *pi1,*pi2;

  SystemTimeToFileTime (minend,&ft1);
  SystemTimeToFileTime (subtrahend,&ft2); 
  pi1 = (__int64*)&ft1;
  pi2 = (__int64*)&ft2;

  return (((*pi1)-(*pi2))/(double)datepart);
} 

SYSTEMTIME AddSysTime( const __int64 datepart, const SYSTEMTIME* pst )
{
  FILETIME ft;
  SYSTEMTIME st;
  __int64* pi; 

  SystemTimeToFileTime (pst,&ft); 
  pi     = (__int64*)&ft; 
  (*pi) += datepart; 

  FileTimeToSystemTime (&ft,&st); 

  return st;
}


#define RETRYATTEMPTS 4
 
inline bool CheckFeature(int customer, int feature)
{
  //Overriding the key check due to new HASP SRM encryption!
//  if(customer == C_USMC) return true;

  hasp_handle_t handle = 0;
  const hasp_feature_t hfeature = feature;
  bool ret = false;

  if(HASPCustomerId > C_Invalid && HASPCustomerId < C_End)
    customer = HASPCustomerId;

  if(customer > C_Invalid && customer < C_End)
  {
    //check HL first
    ret = (HASP_STATUS_OK == hasp_login(hfeature| HASP_PROGNUM_FEATURETYPE, (hasp_vendor_code_t)vendor_code[customer], &handle));
    if(!ret) //check SRM
    {
      LogF("Can't find old HASP HL key");
      ret = (HASP_STATUS_OK == hasp_login(hfeature, (hasp_vendor_code_t)vendor_codeSRM[customer], &handle));
    }
  }
#if _ENABLE_CHEATS //always try developer as well
    if(!ret)
    {
      ret = (HASP_STATUS_OK == hasp_login(hfeature, (hasp_vendor_code_t)vendor_code[C_Developer], &handle));
    }
#endif
  return ret;
}

bool CheckIfKeyIsInserted()
{
  if(HASPCustomerId <= C_Invalid || HASPCustomerId >= C_End) //we don't know which key it is, continue
    return false; // function to fail if key code is unknown, because therefore no key is inserted

  //Overriding the key check due to new HASP SRM encryption!
  if(HASPCustomerId == C_USMC) return true;

  hasp_handle_t handle = 0;
  hasp_status_t status = HASP_CONTAINER_NOT_FOUND;
  const hasp_feature_t feature = 1 | HASP_PROGNUM_FEATURETYPE; //feature 1 is read out incorrectly for net keys if they are local

  int retries = 3;
  while(status != HASP_STATUS_OK && retries-- > 0)
  {
    status = hasp_login(feature, (hasp_vendor_code_t)vendor_code[HASPCustomerId], &handle); 
    Sleep(100);
    LogF("[m] Check key...");
  }

  if(status == HASP_STATUS_OK)
  {
    return true;
  }
  
  return false;
}

//returns true if the key is valid, changes the customerId
bool CheckTrialVersion(int customerId)
{
  if(customerId == C_USMC)
  {
    HASPCustomerId = (Customers)customerId;
    return true;
  }

  hasp_handle_t handle = 0;
  hasp_status_t status = HASP_CONTAINER_NOT_FOUND;
  hasp_status_t memSize = 0;
  const hasp_feature_t feature = 1 | HASP_PROGNUM_FEATURETYPE; //feature 1 is read out incorrectly for net keys if they are local
  
  //due to the switch to HASP SRM exclude USMC
  Customers customer = C_Trial;

  LogF("[m] config stored key: %d", customerId);

  if(customerId >= 0 && customerId < C_End) //we got an Id passed, check it first!
  {
    LogF("[m] Check config stored key: %s", customer_String[customerId]);
    status = hasp_login(feature, (hasp_vendor_code_t)vendor_code[customerId], &handle);
    if (status == HASP_STATUS_OK) //key found
      customer = Customers(customerId);
  }

  while(status == HASP_CONTAINER_NOT_FOUND && customer < C_End)
  {
    LogF("[m] Check for HASP key: %s", customer_String[customer]);

    // Open the connection to the dongle using the signature. Try to find customer.
    // Get the size of the avaliable memory
    status = hasp_login(feature, (hasp_vendor_code_t)vendor_code[customer], &handle);

    if (status == HASP_CONTAINER_NOT_FOUND) //no key or different customer
      customer = Customers(int(customer) + 1);
  }
  
  if (status == HASP_CONTAINER_NOT_FOUND || customer == C_End) //key is not listed in expiry keys
  {
    LogF("HASP: Customer key not found in list. No software expiry");
    return true; //no expiry check
  }

  if(status != HASP_STATUS_OK) //something went wrong
  {
    ErrorMessage("HASP: Internal Error: %d", status); //no expiry message, but exit program
    return true;
  }

  HASPCustomerId = customer; //store the customer key for periodic checks

  LogF("HASP: Customer key: %s", customer_String[customer]);

  if(!isTrialKey[customer])
    return true;

  // only execute for trial keys
  hasp_time_t htime; 
  status = hasp_get_rtc(handle, &htime);

  if (status == HASP_STATUS_OK) //Hardware time expiry possible, no software expiry required
  {
    char *info = 0;

    status = hasp_get_sessioninfo(handle, HASP_SESSIONINFO, &info);

    /* check if operation was successful */
    if (status != HASP_STATUS_OK)
    {
      switch (status)
      {
      case HASP_FEATURE_NOT_FOUND:
        break;
      case HASP_INV_HND:
        break;
      case HASP_INV_FORMAT:
        break;
      default:
        break;
      }
    }

    RString haspInfo(info);

    hasp_free(info);

    int start = haspInfo.Find("<expirationdate>");
    int end = haspInfo.Find("</expirationdate>");

    if(start != -1 && end != -1)
    {
      hasp_time_t expTime = atol(cc_cast(haspInfo.Substring(start+16, end)));
      unsigned int day, month, year, hour, minute, second;
      hasp_hasptime_to_datetime(expTime, &day, &month, &year, &hour, &minute, &second);
      char buf[50]; 
      sprintf(buf,"expires: %d/%d/%d", day, month, year);
      Glob.expiryDate = RString(buf);
      LogF("HASP: Hardware expiry detected: %s", cc_cast(Glob.expiryDate));
    }
    else
      LogF("[m] Error in key Read: start = %d, end = %d");
    return true;
  }

  // get the amount of memory available on the dongle
  status = hasp_get_size(handle, HASP_FILEID_MAIN, &memSize);
  if( status != HASP_STATUS_OK)
    return CheckHASPError(status);
    
  // check if the size of memory is sufficient 
  if(memSize < sizeof(Validation))
    return CheckHASPError(HASP_INSUF_MEM);

  // resize the buffer to the memory size
  AutoArray<char> buffer;
  buffer.Resize(sizeof(Validation)); 

  bool cleanDongal = true;

  status = hasp_read(handle, HASP_FILEID_MAIN, 0 , buffer.Size(), buffer.Data());
  if( status != HASP_STATUS_OK)
    return CheckHASPError(status);
  
  for( int i = 0; i < buffer.Size(); i++ ) //see if it's a virgin
  {
    if(buffer[i]>0)
    {
      cleanDongal = false;
      break;
    }
  }

  SYSTEMTIME sysTime;
  GetLocalTime(&sysTime);

  if(cleanDongal)
  {
    int trialPeroidDays = 60;

    SYSTEMTIME newTime = AddSysTime(nanoSecInDay*trialPeroidDays,&sysTime);
    
    Validation* validation = (Validation*)buffer.Data();
    validation->preTime = sysTime;
    validation->expDate = newTime;
    validation->expire  = 0;

    char buf[50]; 
    sprintf_s(buf, 50, "expires: %u/%u/%u"
      , validation->expDate.wDay
      , validation->expDate.wMonth
      , validation->expDate.wYear
      );
    Glob.expiryDate = RString(buf);

    status = hasp_encrypt(handle, buffer.Data(), buffer.Size()); //encrypt expiry date
    if( status != HASP_STATUS_OK)
      return CheckHASPError(status);

    status = hasp_write(handle, HASP_FILEID_MAIN, 0, buffer.Size(), buffer.Data()); //store expiry date
    if( status != HASP_STATUS_OK)
      return CheckHASPError(status);

//    LogF("HASP: Software expiry detected: %s", cc_cast(Glob.expiryDate));

    return true;
  }
  else
  {
    // we need to decyrpt the data now
    status = hasp_decrypt(handle, buffer.Data(), buffer.Size());
    if( status != HASP_STATUS_OK)
      return CheckHASPError(status);

    Validation* validation = (Validation*)buffer.Data();

    // difftime should always be ahead of time, if it
    // isn't its and indication somebody has played with
    // the system time, in regards to this, we abort and write
    // out that the expire is true.

    // Resolution is in days, we give one day buffer in case of network computer systems
    // time clock being extremely slow or in-accurate. if its bellow -0.5 it means they have
    // tried to get free time from the trial.

    char buf[50]; 
    sprintf_s(buf, 50, "expires: %u/%u/%u"
      , validation->expDate.wDay
      , validation->expDate.wMonth
      , validation->expDate.wYear
      );

    Glob.expiryDate = RString(buf);

    if( SubSysTime(nanoSecInDay,&sysTime,&validation->preTime) < -1.0 )  
    {
      if(validation->expire < RETRYATTEMPTS)
      {
        char remaining = ++validation->expire;
        status = hasp_encrypt(handle, buffer.Data(), buffer.Size());
        if( status != HASP_STATUS_OK)
          return CheckHASPError(status);

        status = hasp_write(handle, HASP_FILEID_MAIN, 0, buffer.Size(), buffer.Data());
        if( status != HASP_STATUS_OK)
          return CheckHASPError(status);

        int startPos = buffer.Size();
        
        // write out, the system time that caused the system to fail.
        // no need to check returned values, 
        hasp_write(handle, HASP_FILEID_MAIN, startPos, sizeof(SYSTEMTIME),&sysTime);
        
        // get the computer name
        char compName[MAX_COMPUTERNAME_LENGTH+1] = {0};
        DWORD bufSize = MAX_COMPUTERNAME_LENGTH+1;
        GetComputerName(compName,&bufSize);
        compName[MAX_COMPUTERNAME_LENGTH] = '\0';
      
        hasp_write(handle,HASP_FILEID_MAIN,startPos+sizeof(SYSTEMTIME),bufSize,&compName);        
 

        ErrorMessage( "The workstation:%s system time(DMY:%d/%d/%d) is not sychronised. Please adjust " \
                      "workstation system time to be the same on all VBS2 machines. Attempts remaining:%d", 
                      compName,
                      sysTime.wDay,
                      sysTime.wMonth,
                      sysTime.wYear,
                     (RETRYATTEMPTS-remaining));
      }

      return false;
    }

    // Check if we're past our expiry date
    // or, we've reached the max number of retrys allowed
    if((SubSysTime(nanoSecInDay,&sysTime,&validation->expDate) >= 0.0f) ||
       (validation->expire >= RETRYATTEMPTS))
    {
      return false;
    }
    else
    {
      double daysRemaining = SubSysTime(nanoSecInDay,&sysTime,&validation->expDate);
      if(daysRemaining >= -5)
        ErrorMessage(EMNote,"%d Days remaining on trial key", (int) abs(daysRemaining));

      validation->preTime = sysTime;
      status = hasp_encrypt(handle, buffer.Data(), buffer.Size());
      if( status != HASP_STATUS_OK)
        return CheckHASPError(status);

      status = hasp_write(handle, HASP_FILEID_MAIN, 0, buffer.Size(), buffer.Data());
      if( status != HASP_STATUS_OK)
        return CheckHASPError(status);

      return true; // everthing ok
    }  
  }

//  return false;
}

#endif