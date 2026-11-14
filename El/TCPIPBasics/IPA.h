// IPA.h: interface for the CIPA class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IPA_H__DA6818E8_23F2_11D4_90D5_00C0DFAE7D0A__INCLUDED_)
#define AFX_IPA_H__DA6818E8_23F2_11D4_90D5_00C0DFAE7D0A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define ADRTYPE unsigned long
#define PORTTYPE unsigned short

#include <winsock.h>

#ifndef _WINSOCK2API_
typedef struct sockaddr FAR *LPSOCKADDR;
#endif

/** This simple class is designed to easy manipulates with IP address and port **/

class CIPA  
{
  public:    
    ADRTYPE addr;  ///< R/W variable with addres ordered in network order (BIG-ENDIAN)
    PORTTYPE port; ///< R/W variable with port ordered in x86 order (LITTLE-ENDIAN)
    
    ///Constructs CIPA object.
    /**
        @param adr assigned address, default value creates object without address assigned
        @param port assigned port, default value creates object without port assigned
        */
    CIPA(ADRTYPE adr=INADDR_NONE, PORTTYPE p=0): addr(adr),port(p) 
    {}
    
    ///Constructs CIPA object from SOCKADDR struct
    /**
        @param adr address and port in SOCKADDR. It MUST have AF_INET family set.
        */
    CIPA(SOCKADDR& adr): addr(((SOCKADDR_IN &)adr).sin_addr.s_addr),port(htons(((SOCKADDR_IN &)adr).sin_port)) 
    {}
    
    ///Constructs CIPA object from SOCKADDR_IN struct
    /** SOCKADDR_IN is particular definition for SOCKADDR. Common practice is type SOCKADDR_IN
        to SOCKADDR. This constructor can be used without need typing
        @param adr address and port in SOCKADDR_IN. It MUST have AF_INET familt set.
        */
    CIPA(SOCKADDR_IN& adr): addr(adr.sin_addr.s_addr),port(htons(adr.sin_port)) 
    {}
    
    ///Constructs CIPA object from address in text form and port
    /**
        @param adr Address entered in text form. It can contain doted numbers e.g. 207.46.248.109 
          or valid DNS name (e.g. msdn.microsoft.com). String can also contain the colon (:) which 
          split string into two pairs. The First pair is address (descripted above), the second part
          contain port number in decimal form. In this case, port number in string overides parameter
          p. This feature is useful, when address is entered by user, and allow to user change port without
          any additional support (for example: in dialogs)
        @param p Port number, or default port, if is not contained in adr.
        @return if address is valid and exists, constructs valid object. To test object validity, use
          IsAddrValid() member function. Function need access the network.
        */
    CIPA(const char *adr, PORTTYPE p);
    
    ///Constructs CIPA object from another.
    /**
        @param other source object.
        */
    CIPA(const CIPA& other) 
    {addr=other.addr;port=other.port;}
    
    ///This operator is useful everywhere pointer to SOCKADDR is need. 
    /**NOTE: The result pointer points at static variable shared with all instances of CIPA.
        Copy content of variable into local variable immediatelly.<p>
        @example Example: connect(socket,IPA,sizeof(SOCKADDR)); //variable IPA is type of CIPA
        */
    operator LPSOCKADDR() const;
    
    ///Fills SOCKADDR structure with current values
    /**
        @param sa SOCKADDR type variable, that receives data from object
        */
    void GetSockAddres(SOCKADDR& sa)  const ;
    ///Fills SOCKADDR_IN structure with current values
    /**
        @param sa SOCKADDR_IN type variable, that receives data from object
        */
    void GetSockAddres(SOCKADDR_IN &sin) const;
    
    ///Compares two CIPA objects
    bool operator!= (const CIPA& other) const 
    {return !(*this==other);}
    ///Compares two CIPA objects
    bool operator== (const CIPA& other) const;
    ///Provides basic assigment
    CIPA& operator= (const CIPA& other);
    ///Function gets host name from DNS server.
    /**
        @param buffer string buffer that receives host name
        @param size size of that buffer in characters
        @return true, if buffer receives address, false if buffer is too small. If 
        DNS record were not found, function fills buffer in form "dotted numbers" e.g. 207.46.248.109 
        If address is not valid, buffer is filled with "dotted question marks" (???.???.???.???).
        Function need access the network.
        */
    bool GetHostName(char *buffer, int size) const;
    ///Function gets the host name and the port from object.
    /**Both values is formated as string in form address:port.
        @param buffer string buffer that receives the host name and the port
        @param size size of the buffer in characters
        @return true, if buffer receives the address, or false if buffer were to small.
        If DNS record were not found, function fills buffer in form "dotted numbers" e.g. 207.46.248.109 
        If address is not valid, buffer is filled with "dotted question marks" (???.???.???.???).
        Function need access the network.
        */
    bool GetFullAdrSpec(char *buffer, int size) const;
    ///Function gets the port number        
    PORTTYPE GetPort() const 
    {return port;}
    ///Tests, whether address is valid. Invalid address contain value INADDR_NONE (defined in WinSock)
    /**
        @return true, if address is valid
        */
    bool IsAddrValid() const 
    {return addr!=INADDR_NONE;}
    ///Tests, whether port is valid. Invalid port contain value zero.
    /**
        @return true, if port is valid
        */
    bool IsPortValid() const 
    {return port!=0;}
    ///This function only sets address with specified value
    /**
        @param addr new address
        */
    void SetAddr(ADRTYPE a) 
    {addr=a;}
    ///This function only sets port with specified value
    /**
        @param P new port
        */
    void SetPort(PORTTYPE p) 
    {port=p;}
};

#endif // !defined(AFX_IPA_H__DA6818E8_23F2_11D4_90D5_00C0DFAE7D0A__INCLUDED_)
