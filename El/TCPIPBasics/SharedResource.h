#ifndef __SHAREDRESURCE_H_TEMPLATE_ID_14489465308740ABC_BREDY_
#define __SHAREDRESURCE_H_TEMPLATE_ID_14489465308740ABC_BREDY_

#include <Es/Types/pointers.hpp>

class SharedResourceInfo: public RefCount
{
  bool _closed;
public:
    SharedResourceInfo() {Reopen();}
    void CloseResource() {_closed=1;}
    bool IsClosed() const {return _closed;}
    void Reopen() {_closed=0;}
};

template<class T, class Traits>
class SharedResource: public Traits
{
  T _resource;                        ///<promìná rukojeti
  mutable Ref<SharedResourceInfo> _counter;   ///<ukazatel na èítaè

    void ReleaseResource()
    {
      if (_counter.IsNull()) return;
      if (_counter->RefCounter()==1) Traits::Release(_resource);
      _counter=0;
    }
  public:
    
    ///Konstrukce tøídy z rukojeti
    /**POZOR: Tøída nepozná, e rukoje je nìkde sdílena, a vdy pro ní zaloí novı èítaè.
      Pokud je potøeba zajistit sdílení všude, je tøeba pøedávat rukoje zabalenou do tøídy
      SharedResource, nikoliv samostatnì.
      V pøípadì, e je tøeba zabránit uzavøení rukojeti po ukonèení sdílìní, lze pøesvìdèit
      tøídu SharedResource, e rukoje je ji uzavøená pomocí SetClosed(). Nedojde k uzavøení
      resource, ale bude ji povaovaná za uzavøenou.
      @param src rukoje kterou je tøeba sdílet. Rukoje je kopírována do _resource, a nemìla
      by pøejit do stavu "uzavøená" po znièení src
      */
    SharedResource(const T& src):_resource(src),_counter(new  SharedResourceInfo()) {}
    
    
 
    ///Konstrukce tøídy operátorem pøiøazení
    /** Zaloí novou sdílenou rukoje pomocí operátoru pøiøazení
      @param other reference na zdrojovou sdílenou rukoje. Funkce sníí referenci
       souèasné rukojeti a poté pøepíše rukoje a zaloí jí èítaè s referencí 1
      */
    SharedResource<T,Traits>& operator=(const T& other)
    {
       ReleaseResource();
       _resource=other;
      _counter=new SharedResourceInfo();
      return *this;
    }
    
    ///Umoznuje zmìnit natvrdo vlastni resource, coz muze nastat v pripade, ze 
    /**doslo k presunu resource pod jinou referenci, nebo v jinych speciálních pøípadech */
    void SetResource(const T& value)
    {
      _resource=value;
    }

    ///Konstrukce tøídy operátorem pøiøazení
    /** Duplikuje rukoje a zvıší referenèní èítaè 
      @param other reference na zdrojovou sdílenou rukoje. Funkce sníí referenci
       souèasné rukojeti a poté duplikuje rukoje a zvıší ji èítaè referencí
      */
    SharedResource<T,Traits>& operator=(const SharedResource<T,Traits>&other)
    {
      if (_counter==other._counter) return *this;
       ReleaseResource();
       _resource=other._resource;
      _counter=other._counter;
      return *this;
    }
    
    ///Operátor umoòuje pøímı pøístup k rukojeti resource
    operator T& () 
      {return _resource;}
    ///Operátor umoòuje pøímı pøístup k rukojeti resource avšak jako konstantní objekt
    operator const T&() const 
      {return _resource;}
    ///Operátor umoòuje porovnat dve rukojeti. Musí to rukoje podporovat!
    bool operator==(const SharedResource<T,Traits>&other) const
      {return _resource==other._resource;}
    ///Operátor umoòuje porovnat dve rukojeti. Musí to rukoje podporovat!
    bool operator!=(const SharedResource<T,Traits>&other) const
      {return _resource!=other._resource;}
    ///Operátor umoòuje porovnat dve rukojeti. Musí to rukoje podporovat!
    bool operator==(const T& other) const
      {return _resource==other;}
    ///Operátor umoòuje porovnat dve rukojeti. Musí to rukoje podporovat!
    bool operator!=(const T& other) const
      {return _resource!=other;}
    ///Operátor je zaveden pro rukoje typu ukazatel, nebo pokud podporuje operátor ->
    T operator->() const 
      {return _resource;}
    ///Sníí referenci rukojei a pøípadnì ji uzavøe, poté zlikviduje tento objekt
    ~SharedResource()
      {ReleaseResource();}
    ///Funkce uzavírá rukoje
    /** Po zavolání této funkce pøejde rukoje do stavu "uzavøená". To se docílí
      zavoláním funkce Traits::Release. Poté se v èítaèi nastaví stav "uzavøená" tak
      aby všechny další objekty, jen rukoje sdílí, se dozvìdìli, e rukoje ji byla uzavøena.
      Zabrání se tím také dalšímu vícenásobné volání Release.
      */
    void CloseResource() 
      {if (!IsClosed()) 
        {_counter->CloseResource(); Traits::Release(_resource); }}
    ///Funkce nastaví èítaè tak aby informoval, e rukoje je ji ve stavu "uzavøená".
    /**Pouívá se pokud rukoje pøešla do stavu "uzavøená" jinou cestou (napøíklad chybou).
      V tomto okamiku se u nesmí volat Release. Funkce sesynchronizuje tøídu, aby rukoje byla
      vedena jako "uzavøená". Tuto funkci lze také vyuit v pøípadì, e z nìjakého dùvodu
      si nepøejeme po ukonèení sdílení aby se rukoje uzavøela. Má to však nevıhodu, e
      pak funkce IsClosed vrací true a pokud s tím ostatní objekty nepoèítají, mohou odmítnout
      pracovat s uzavøenou rukojetí.*/
    void SetClosed() 
      {_counter->CloseResource();}
    ///Funkce vrací true, pokud tøída eviduje rukoje jako uzavøenou.
    bool IsClosed() const
      {return _counter->IsClosed();}
    ///Funkce obnoví stav "platná" pro rukoje. 
    /** Pouívá se u rukojetí, jen mohou bıt nové otevøeny. Ovšem pozor, platí pro
      rukojeti, které po otevøení nezmìní svou hodnotu! (napøíklad re-use socket). Vıhodou
      je, e po novém otevøení ne neztratí dosavadní sdílení, jak by tomu došlo otevøením
      nové rukojeti (kterou po otevøení nikdo nesdílí).<p>
      POZNÁMKA: Funkce neotevírá rukoje, pouze sesynchronizuje tøídu SharedResource
      */
    void Reopen() 
      {_counter->Reopen();}
};



#endif
