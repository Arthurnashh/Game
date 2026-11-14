// WSA.h: interface for the CWSA class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WSA_H__7717B5F3_AC31_4D1D_85E0_9A6320F841D8__INCLUDED_)
#define AFX_WSA_H__7717B5F3_AC31_4D1D_85E0_9A6320F841D8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/// This class is very simple. Only ensures 

class CWSA :public WSADATA
{
  int result;
  public:
    CWSA(unsigned short version)
    {
      result=WSAStartup(version,this);
    }
    virtual ~CWSA()
    {
      WSACleanup();
    }
    int GetResult() 
    {return result;}
    
    
};

#endif // !defined(AFX_WSA_H__7717B5F3_AC31_4D1D_85E0_9A6320F841D8__INCLUDED_)
