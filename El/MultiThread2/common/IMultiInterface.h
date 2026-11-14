#pragma once

///This class enables you to create multiinterfaces classes
/**
* Example: You have a class hierarchy, that defines a set of virtual functions.
* You want to add new virtual function into the base, but this function has no
* relation with the class hierarchy - you only need to create special implementation
* of this function for each type of the derived class.
*
* To allow this, use IMultiInterfaceBase as very base class of your class hierarchy.
* Using this class, you are able to obtain any other interfaces attached to the derived
* class, that is invisible at base. Any additional functions can be declared
* at this extra interface with no connection to base class.
*
*/

namespace BredyLibs
{

  class IMultiInterface
  {
  public:
    ///General function to get pointer to additional interface
    /**
    * @param ifcuid interface unique identifier - To get interface, you need
    *   to know ifcuid, that identifies specific interface. There is no
    *   rules in creating unique id. Each interface available in class
    *   hiearchy should have an unique uid.
    *
    * @retval pointer to the new interface. You need to cast result to the right pointer type. 
    * @retval 0 in this case, the interface with the specified ifcuid is not available
    * @note There is safe version of this function, that uses the C++ template. see GetInterface()
    */
    virtual void *GetInterfacePtr(unsigned int ifcuid) {return 0;}

    ///Queries object for another interface.
    /**
    * Interface type is specified as InterfaceClass. Each InterfaceClass must declare public constant
    * IFCUID, that contains ifcuid for that interface
    *
    * @retval pointer to the new interface.
    * @retval 0 requested interface is not available
    */
    template<class InterfaceClass>
    InterfaceClass *GetInterface() 
    {
      return reinterpret_cast<InterfaceClass *>(GetInterfacePtr(InterfaceClass::IFCUID));
    }  
    template<class InterfaceClass>
    const InterfaceClass *GetInterface() const
    {
      return reinterpret_cast<InterfaceClass *>(const_cast<IMultiInterface *>(this)->GetInterfacePtr(InterfaceClass::IFCUID));
    }  
  };

};
#define DECLARE_IFCUID(num) static const unsigned int IFCUID=(num)
#define DECLARE_IFCUIDC(a,b,c,d) static const unsigned int IFCUID=(((a)<<24)|((b)<<16)|((c)<<8)|(d))
#define DECLARE_IFCUIDA(a,b,c,d,e,f) static const unsigned int IFCUID=(((a-'A')<<25)+((b-'A')<<20)+((c-'A')<<15)+((d-'A')<<10)+((e-'A')<<10)+(f-'A'))
#define GET_CLASS_IFCUID(clsname) (clsname::IFCUID)
