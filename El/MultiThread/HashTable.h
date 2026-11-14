#ifndef _BREDY_LIBS_TYPES_HASHTABLE_
#define _BREDY_LIBS_TYPES_HASHTABLE_

#pragma warning( disable : 4290 ) 

namespace HashTable
{

  template<class T>
  class ItemBaseBase
  {
  public:
    struct HashItem
    {
      char itemdata[sizeof(T)];
      unsigned int collisions;
      int hashcache;
      HashItem () {}

      T &GetStorage() {return *reinterpret_cast<T *>(itemdata);}	
      const T &GetStorage() const {return *reinterpret_cast<const T *>(itemdata);}	
    };
  };

  template<class T, class Bs>
  class Base: public Bs
  {
  protected:

    typename ItemBaseBase<T>::HashItem *_table;
    int _size;
    int _n;
  protected:

    int FindIndex(const T &what, int &start) const
    {
      if (_size==0) return -1;
      int index=what%_size;
      const HashItem *item=_table+index;
      start=index;
      if (item->hashcache<0 || item->GetStorage()!=what)
      {
        if (item->collisions==0) return -1;
        else
        {
          int col=item->collisions;
          index=(index+1)%_size;
          while (start!=index && col)
          {
            item=_table+index;
            const T &kk=item->GetStorage();
            if (item->hashcache==start)
            {
              if (kk==what) return index;
              col--;
            }
            index=(index+1)%_size;
          }
          return -1;
        }
      }
      return index;
    }

  public:
    Base():_table(0),_size(0),_n(0)
    {
    }

    ~Base()
    {
      Clear();
      ReleaseTable(_table);
    }

    void Clear()
    {
      for (int i=0;i<_size;i++) if (_table[i].hashcache>=0)
      {
        T &item=_table[i].GetStorage();
        item.~T();
        _table[i].hashcache=-1;
        _table[i].collisions=0;
      }
      _n=0;
    }

    T *Find(const T &what) const
    {
      int start;
      int z=FindIndex(what,start);
      if (z==-1) return 0;
      return &(_table[z].GetStorage());
    }

    bool Add(const T &what)
    {
      if (_n>=_size/2) Expand();
      if (_n>=_size) return false;
      int index=what%_size;
      int start=index;
      HashItem *item;
      do
      {
        item=_table+index;
        if (item->hashcache<0)
        {
          item->hashcache=start;
          if (index!=start)
            _table[start].collisions++;
          new(&(item->GetStorage())) T(what);
          _n++;
          return true;		
        }
        index=(index+1)%_size;
      }
      while (index!=start);
      _n=_size;
      return false;
    }

    bool Remove(const T &what)
    {
      int start;
      int idx=FindIndex(what,start);
      if (idx!=-1)
      {
        if (start!=idx && _table[start].collisions) _table[start].collisions--;
        _table[idx].GetStorage().~T();
        _table[idx].hashcache=-1;
        return true;
      }
      else
        return false;


    }

    void Expand()
    {
      HashItem *oldtable=_table;
      int oldsize=_size;

      if (ExpandBegin(_table,_size))
      {
        int i;
        for (i=0;i<_size;i++) 
        {
          _table[i].collisions=0;
          _table[i].hashcache=-1;
        }
        _n=0;
        for (i=0;i<oldsize;i++) if (oldtable[i].hashcache>=0)
          Add(oldtable[i].GetStorage());
        ExpandEnd(oldtable,oldsize);
      }	
    }

    template<class Functor>
      bool ForEach(const Functor &f)
    {
      for (int i=0;i<_size;i++) if (_table[i].hashcache>=0)
      {
        if (f(_table[i].GetStorage())) return true;
      }
      return false;
    }

    template<class Functor>
      bool ForEach(const Functor &f) const
    {
      return const_cast<Base *>(this)->ForEach(f);
    }

    template<class Functor>
      bool ForEach(const T &search, const Functor &f)
    {
      int start;
      int t=FindIndex(search,start);
      if (t!=-1)
      {
        if (f(_table[t].GetStorage())) return true;
        int col=_table[start].collisions;
        if (t!=start) col--;
        while (col)
        {
          t=(t+1)%_size;
          if (_table[t].hashcache==start)
          {
            if (_table[t].GetStorage()==search)
            {
              if (f(_table[t].GetStorage())) return true;
            }
            col--;
          }
        }
      }
      return false;
    }

    template<class Functor>
      bool ForEach(const T &search, const Functor &f) const
    {
      return const_cast<HashTableBase *>(this)->ForEach(search,f);
    }


    enum NotFoundException {NotFoundExceptionValue};

    const T &operator[](const T &find) const throw(NotFoundException)
    {
      const T *res=Find(find);
      if (res==0) throw NotFoundExceptionValue;
      return *res;
    }
  };

  template<class T, int size>
  class StaticBase: public ItemBaseBase<T>
  {
    HashItem _staticTable[size];
  public:

    bool ExpandBegin(HashItem *& table, int &sz)
    {
      if (table) return false;
      table=_staticTable;
      sz=size;
      return true;
    }

    void ExpandEnd(HashItem *, int sz)
    {

    }
    void ReleaseTable(HashItem *table)
    {
    }
  };

  template<class T>
  class ExpandableBase: public ItemBaseBase<T>
  {
  public:

    bool ExpandBegin(HashItem *& table, int &sz)
    {
      int newsz=sz;
      if (newsz==0) newsz=101; else newsz=newsz*2-1;
      table=new HashItem[newsz];
      sz=newsz;
      return true;
    }

    void ExpandEnd(HashItem *old, int sz)
    {
      delete [] old;
    }

    void ReleaseTable(HashItem *table)
    {
      delete [] table;
    }
  };



  template<class T>
  class Expandable: public Base<T, ExpandableBase<T> >
  {
    class CopyFunctor
    {
      Expandable *container;
    public:
      CopyFunctor(Expandable *container):container(container) {}
      bool operator()(const T *zz) const
      {
        container->Add(*zz);
      }
    };
  public:

    Expandable() {}
    Expandable(const Expandable &other)
    {
      other.ForEach(CopyFunctor(*this));
    }
    Expandable &operator=(const Expandable &other)
    {
      Clear();
      other.ForEach(CopyFunctor(*this));
    }
    template<class EnumClass>
      Expandable &operator=(const EnumClass &other)
    {
      Clear();
      other.ForEach(CopyFunctor(*this));
    }

  };

  template<class T, int size>
  class Static: public Base<T, StaticBase<T, size> >
  {
    class CopyFunctor
    {
      Static *container;
    public:
      CopyFunctor(Static *container):container(container) {}
      bool operator()(const T *zz) const
      {
        container->Add(*zz);
      }
    };
  public:

    Static() {}
    Static(const Static &other)
    {
      other.ForEach(CopyFunctor(*this));
    }
    Static &operator=(const Static &other)
    {
      Clear();
      other.ForEach(CopyFunctor(*this));
    }
    template<class EnumClass>
      Static &operator=(const EnumClass &other)
    {
      Clear();
      other.ForEach(CopyFunctor(*this));
    }

  };

};
#endif