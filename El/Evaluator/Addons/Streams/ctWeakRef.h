#ifndef _CT_WEAK_REF_TEMPLATE_H_12FDA002115753XXX_ONDREJ_NOVAK_
#define _CT_WEAK_REF_TEMPLATE_H_12FDA002115753XXX_ONDREJ_NOVAK_

#define CTMAGIC 0x1A
#ifndef CTRANGECHECK
#ifdef _DEBUG
#define CTRANGECHECK 1
#else
#define CTRANGECHECK 0
#endif
#endif

#ifndef CTRANGEERRORCALL 
#define CTRANGEERRORCALL ASSERT(false)
#endif

typedef struct tagWeakRefDesc //popisovac bitoveho pole informace pro referencni citac
{
  unsigned int refcount:24;   //16 777 216 referenci max
  unsigned int data:1;			//1 oznamuje, ze datovy blok je platny
  unsigned int redirect:1;		 //1 oznamuje, ze nasleduje informace o presmerovani - 0 oznamuje NULL
  unsigned int array:1;			//1 informuje, ze prvni 4 bajty dat je pocet prvku
  unsigned int magic:5;			//6-bitove magic cislo pro zachytavani chyb
} WEAKREFDESC;

typedef struct tagWeakRefProxy //popisovac bitoveho pole informace pro referencni citac
{
  WEAKREFDESC desc; //vlajky
  void *redirect;				//ukazatel na presmerovani
} WEAKREFPROXY;

enum ctWeakRefEnum 
{ctWeakRefAlloc};

inline void *operator new[](size_t sz,ctWeakRefEnum enm) //vlastni operator pro alokaci weakref poli
{
  sz+=2*sizeof(WEAKREFDESC);		//zvets velikost o 2*popisovac
  void *mem=malloc(sz);		//alokuj pamet
  WEAKREFDESC *p1=(WEAKREFDESC  *)mem; //ziskej adresu popisovace
  p1->array=0;		  //prvni popisovac informuje, ze to neni pole
  p1->data=1;		  //neni to null
  p1->redirect=0;	  //zadny redirect
  p1->refcount=0;	//zatim zadna reference 
  p1->magic=CTMAGIC; //magic cislo
  p1[1]=p1[0];		  //zkopiruj do druheho popisovace
  p1[1].array=1;	  //ten popisuje pole
  return (void *)(p1+2);
}

inline void operator delete[](void *ptr,ctWeakRefEnum enm)
{
  int *x=(int *)ptr;x--;
  free(x);
}

inline void *operator new(size_t sz,ctWeakRefEnum enm) //vlastni operator pro jednoduchou alokaci
{
  sz+=2*sizeof(WEAKREFDESC);		//zvets velikost o 2*popisovac
  void *mem=malloc(sz);		//alokuj pamet
  WEAKREFDESC *p1=(WEAKREFDESC *)mem; //ziskej adresu popisovace
  p1->array=0;		  //prvni popisovac informuje, ze to neni pole
  p1->data=1;		  //neni to null
  p1->redirect=0;	  //zadny redirect
  p1->refcount=0;	//zatim zadna reference 
  p1->magic=CTMAGIC; //magic cislo
  p1[1]=p1[0];		  //zkopiruj do druheho popisovace
  p1[1].array=1;	  //ten popisuje pole
  return (void *)(p1+2);
}

inline void operator delete(void *ptr,ctWeakRefEnum enm)
{
  int *x=(int *)ptr;x--;
  free(x);
}

#pragma warning (disable:4284)

/*
v pripade jednoduche alokace
| proxy-s | xxxx | objekt |
v pripade pole s destruktorem
| xxxx | proxy-a |  count | objekt |
*/


template <class T>
class ctWeakRef
{
  mutable T *ptr;   //Ukazatel na referencovany objekt
  void Construct(const ctWeakRef &other); //konstrukce ukazatele z jineho ukazatele;
  void Construct(const T *other); //konstrukce ukazatele z jineho ukazatele;
  void RawDelete(); //volani destruktoru
  
  public:
    //Konstruktory
    ctWeakRef():ptr(NULL) 
    {} //Kontruuje prazdny ukazatel (ukazuje na NULL)
    ctWeakRef(const ctWeakRef& other) 
    {Construct(other);} //Konstruuje ukazatel z jineho ukazatele
    ctWeakRef(const T *other) 
    {Construct(other);} //Konstrukce z prime reference (zajistuje dedeni)
    //Destruktor
    ~ctWeakRef() 
    {Release();}
    
    //Operatory
    ctWeakRef& operator=(const ctWeakRef &other) 
    {Release();Construct(other);return *this;}
    ctWeakRef& operator=(const T * other) 
    {Release();Construct(other);return *this;}
    
    operator T*() const
    {return GetRef();} //vrat ukazatel
    T *operator->() 
    {T *p=GetRef();ASSERT(p!=NULL);return p;} //vrat referenci s testem na NULL
    #if CTRANGECHECK
    T &operator[] (int index);
    #else
    T &operator[] (int index) 
    {T *p=GetRef();return *(p+index);}
    #endif
    
    bool operator==(const T *other) const
    {return GetRef()==other;}
    bool operator!=(const T *other) const
    {return GetRef()!=other;}
    T *operator+(int value) const
    {return GetRef()+value;}
    T *operator-(int value) const 
    {return GetRef()-value;}  
    
    //Metody
    WEAKREFPROXY *GetProxy() const //vraci ukazatel na proxy 
    {return (WEAKREFPROXY *)((char *)ptr-2*sizeof(int));}
    
    T *GetRef() const  //vraci ukazatel na objekt
    {					//zajistuje navrat NULL, presmerovani a podobne
      retry:
      if (ptr==NULL) return NULL;  //pokud je ukazatel NULL - vrat NULL
      WEAKREFPROXY *pp=GetProxy(); //ziskej proxy objekt
      if (pp->desc.data)  return ptr;  //pokud je prazdny - vrat NULL;
      if (pp->desc.redirect)   //pokud je presmerovani
      {
        T *x=(T *)pp->redirect; //ziskej adresu presmerovani
        pp->desc.refcount--;	  //sniz citac referenci presmerovani
        if (pp->desc.refcount==0) 
        {free(pp);ptr=x;}else const_cast<ctWeakRef<T> *>(this)->Construct(x); //pokud je nulovy, uvolni presmerovani
        //jinak pridej referenci na novy objekt (pri uvolneni se nepridava, protoze stale ukazuje jeden ukazatel)
        goto retry;			  //opakuj
      } 																			//pokud je presmerovani, nyni jej proved a zkus to znova
      return NULL; //jinak vrat ukazatel	
    }
    bool IsNull() const
    {return (ptr==NULL || GetProxy()->desc.data==0);}		
    bool IsArray() const 
    {return (ptr!=NULL && GetProxy()->desc.array);}
    bool IsRedirect() const
    {return (ptr!=NULL && GetProxy()->desc.redirect);}
    void Delete();
    static bool TestValid(const T *object);
    void Redirect(T *newptr);
    int GetRefCount() 
    {return ptr!=NULL?GetProxy()->desc.refcount:0;}
    void AddRef() 
    { WEAKREFPROXY *pp=GetProxy();
      pp->desc.refcount++;} //zvys citac
    
    void Release(bool nosetnull=false); //uvolneni reference, nastaveni ukazatele na NULL  
};

template<class T>
void ctWeakRef<T>::Construct(const T *other)
{
  if (other==NULL) 
  {ptr=(T *)other;return;}
  ASSERT(TestValid(other)); //pokud neni objekt specialne alokovan
  ptr=(T *)other;	  //kopiruj referenci 
  AddRef();
}

template<class T>
void ctWeakRef<T>::Construct(const ctWeakRef &other)
{
  if (other.ptr==NULL) 
  {ptr=other.ptr;return;}
  ASSERT(TestValid(other.ptr)); //pokud neni objekt specialne alokovan
  ptr=other.ptr;  //kopiruj referenci 
  WEAKREFPROXY *pp=GetProxy();
  pp->desc.refcount++; //zvys citac
}

template<class T>
void ctWeakRef<T>::Release(bool nosetnull) //uvolneni objektu
{
  if (ptr==NULL) return;
  retry:				  
  WEAKREFPROXY *pp=GetProxy(); //ziskej popisovac proxy
  pp->desc.refcount--;	  //sniz citac referenci
  if (pp->desc.refcount==0)	//pokud jiz dosahl nuly
  { 
    void *begin=(void *)pp;
    if (pp->desc.array) begin=(void *)((int *)begin-1);
    if (pp->desc.redirect)		  //je-li presmerovani 
    {
      T *nx=(T *)pp->redirect;		//ziskej adresu presmerovani
      free(begin);									//uvolni tento popisovac
      ptr=nx;								//nastav ukazatel na novy popisovac
      goto retry;						//a uvolni jej tez
    }
    else if (pp->desc.data==0) free(begin);	  //je-li prazdny, pouze jej uvolni
    else 
    {
      RawDelete();					  //zavolej destruktory objektu
      free(begin);							  //uvolni celou pamet
    }
  }
  if (!nosetnull) ptr=NULL;				//nastav ukazatel na NULL
}


template<class T>
void ctWeakRef<T>::RawDelete()
{
  WEAKREFPROXY *pp=GetProxy(); //ziskej popisovac proxy
  if (pp->desc.array) //znic pole
  {
    int cnt=*((int *)ptr-1);
    for (int i=0;i<cnt;i++) (ptr+i)->~T();
  }
  else
    ptr->~T();
}

template<class T>
void ctWeakRef<T>::Delete()
{
  T *x=GetRef();
  if (x==NULL) return;  
  WEAKREFPROXY *pp=GetProxy(); //ziskej popisovac proxy
  RawDelete();		//znic objekt
  void *begin=(void *)pp; //ziskej zacatek bloku
  if (pp->desc.array) begin=(void *)((int *)begin-1); //pokud je to pole, posun se o int zpet
  _expand(begin,16); //realokuj velikost na nejmensi nutnou
  pp->desc.data=0;	//nastav, ze je to NULL  
}

template<class T>
bool ctWeakRef<T>::TestValid(const T *object)
{
  WEAKREFPROXY *pp=(WEAKREFPROXY *)((char *)object-2*sizeof(int));; //ziskej popisovac proxy
  if (pp->desc.magic!=CTMAGIC) return false;
  if (pp->desc.data && pp->desc.redirect) return false;
  return true;
}

template<class T>
void ctWeakRef<T>::Redirect(T *newptr)
{
  if (ptr!=NULL)
  {
    WEAKREFPROXY *pp=GetProxy(); //ziskej popisovac proxy
    if (pp->desc.data) //pokud neni proxy prazdny - natrvdo zlikviduj objekt (bez volani destruktoru)
    {
      void *begin=(void *)pp; //ziskej zacatek bloku
      if (pp->desc.array) begin=(void *)((int *)begin-1); //pokud je to pole, posun se o int zpet
      _expand(begin,16); //realokuj velikost na nejmensi nutnou
      pp->desc.data=0;	//nastav, ze je to NULL
    }	
    ASSERT(TestValid(newptr)); //kontrola, zda newptr ukazuje na spravne alokovany objekt
    pp->desc.redirect=1;
    pp->desc.data=0;
    pp->redirect=newptr;  
    Release();
    Construct(newptr);  
    Construct(newptr);  
  }
  else
  {
    Construct(newptr);  
  }
}

#if CTRANGECHECK
template <class T>
T &ctWeakRef<T>::operator[] (int index)
{
  WEAKREFPROXY *pp=GetProxy();
  GetRef(); //make redirect
  if (pp->desc.data==0) 
  {CTRANGEERRORCALL;return *GetRef();} //is is null, assert error
  else 
    if (pp->desc.array)	//is array - check ranges
  {
    int *cnt=(int *)ptr-1; //get array count
    if (index<0 || index>=*cnt) 
    {CTRANGEERRORCALL;return *GetRef();}
  }
  return *(GetRef()+index);
}

#endif

template <class T>
class wkRefCount: public T
{  
  wkRefCount()  { }
public:
  void AddRef() const
  {
    ctWeakRef<T> p=const_cast<wkRefCount<T> *>(this);
    p.AddRef();
  }
  void Release() const
  {
    ctWeakRef<T> p=const_cast<wkRefCount<T> *>(this);
    p.Release(true);
  }
  static wkRefCount *Attach(T *instance)
  {
    return static_cast<wkRefCount<T> *>(instance);
  }
};


//deklarace pro ctArchive5

#define wknew new(ctWeakRefAlloc) 


#if defined(AFX_CTARCHIVE_H__F66DAB8E_9620_11D5_B39F_00C0DFAE7D0A__INCLUDED_)
#if defined(AFX_CTCLASSINFO_H__E26BCC3F_4C55_4F9D_AD6B_93A882EFECB7__INCLUDED_)


template <class T>
ctArchive& operator>>=(ctArchive& arch, ctWeakRef<T>& ptr)
{
  if (+arch)
  {
    CTLPSC zz=ptr;
    arch>>=zz;
  }
  else
  {
    CTLPSC zz;
    arch>>=zz;
    ptr=dynamic_cast<T *>(zz);
    if (ptr==NULL) ctLostData.Add(zz);
  }
  return arch;
}

//deklarace pro ctClassInfo.h


#define ctRegisterClassWeak(classname,idoffset)\
static CTLPSC _classcreat_##classname() {return new(ctWeakRefAlloc) ##classname;}\
static void _classdestroy_##classname(CTLPSC what) {ctWeakRef<ctSerializeClass> zz=what;zz.Delete();}\
static ctClassInfo _classinfo_##classname((long)(typeid(##classname).name()),0, _classcreat_##classname,_classdestroy_##classname,#classname);\

#define ctRegisterClassFullWeak(classname,idoffset,params)\
static CTLPSC _classcreat_##classname() {return new(ctWeakRefAlloc) classname params;}\
static void _classdestroy_##classname(CTLPSC what) {ctWeakRef<ctSerializeClass> zz=what;what.Delete();}\
static ctClassInfo _classinfo_##classname((long)(typeid(classname).name()),idoffset, _classcreat_##classname,_classdestroy_##classname,#classname);

#endif
#endif
#endif