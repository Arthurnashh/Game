#ifndef REF_H_
#define REF_H_

namespace BredyLibs
{

    template<class T>
    class Ref
    {
        mutable T *_ptr;
    public:
        Ref():_ptr(0) {}
        Ref(T *ptr):_ptr(ptr) {if (_ptr) _ptr->AddRef();}
        ~Ref() {if (_ptr) _ptr->Release();}
        
        Ref(const Ref<T> &other):_ptr(other._ptr) {if (_ptr) _ptr->AddRef();}
        Ref &operator=(const Ref<T> &other)
        {
            if (other._ptr) other._ptr->AddRef();
            if (_ptr) _ptr->Release();
            _ptr=other._ptr;
            return *this;
        }
        
        operator T *() const {return _ptr;}
        T *operator->() const {return _ptr;}
        T &operator *() const {return *_ptr;}
        T *GetRef() const {return _ptr;}   
    };

}
#endif /*REF_H_*/
