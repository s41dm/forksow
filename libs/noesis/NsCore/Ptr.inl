////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Error.h>
#include <NsCore/Hash.h>
#include <NsCore/CompilerTools.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T> 
Ptr<T>::Ptr(): mPtr(0) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T> 
Ptr<T>::Ptr(NullPtrT): mPtr(0) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T> 
Ptr<T>::Ptr(T* ptr): mPtr(ptr)
{
    if (mPtr != 0)
    {
        mPtr->AddReference();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>::Ptr(T& ptr): mPtr(&ptr) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>::Ptr(const Ptr& ptr): mPtr(ptr.mPtr)
{
    if (mPtr != 0)
    {
        mPtr->AddReference();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
template<class S> Ptr<T>::Ptr(const Ptr<S>& ptr): mPtr(ptr.GetPtr())
{
    if (mPtr != 0)
    {
        mPtr->AddReference();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>::Ptr(Ptr&& ptr): mPtr(ptr.mPtr)
{
    ptr.GiveOwnership();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
template<class S> Ptr<T>::Ptr(Ptr<S>&& ptr): mPtr(ptr.GetPtr())
{
    ptr.GiveOwnership();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>::~Ptr()
{
    if (mPtr != 0)
    {
        mPtr->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>& Ptr<T>::operator=(const Ptr& ptr)
{
    Reset(ptr.mPtr);
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
template<class S> Ptr<T>& Ptr<T>::operator=(const Ptr<S>& ptr)
{
    Reset(ptr.GetPtr());
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>& Ptr<T>::operator=(Ptr&& ptr)
{
    if (mPtr != ptr.mPtr)
    {
        if (mPtr != 0)
        {
            mPtr->Release();
        }

        mPtr = ptr.mPtr;
        ptr.GiveOwnership();
    }

    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
template<class S> Ptr<T>& Ptr<T>::operator=(Ptr<S>&& ptr)
{
    // GiveOwnership() done at the beginning to avoid a bug in MSVC2015 Update3 when compiling with 
    // Whole Program Optimization and Omit Frame Pointers enabled
    S* obj = ptr.GetPtr();
    ptr.GiveOwnership();

    if (mPtr != 0)
    {
        mPtr->Release();
    }

    mPtr = obj;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>& Ptr<T>::operator=(T& ptr)
{
    if (mPtr != &ptr)
    {
        if (mPtr != 0)
        {
            mPtr->Release();
        }

        mPtr = &ptr;
    }

    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
void Ptr<T>::Reset()
{
    if (mPtr != 0)
    {
        mPtr->Release();
    }

    mPtr = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
void Ptr<T>::Reset(T* ptr)
{
    if (mPtr != ptr)
    {
        if (ptr != 0)
        {
            ptr->AddReference();
        }
        
        if (mPtr != 0)
        {
            mPtr->Release();
        }

        mPtr = ptr;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
T* Ptr<T>::GiveOwnership()
{
    T* ptr = mPtr;
    mPtr = 0;
    return ptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
T* Ptr<T>::operator->() const
{
    NS_ASSERT(mPtr != 0);
    return mPtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
T* Ptr<T>::GetPtr() const
{
    return mPtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
Ptr<T>::operator T*() const
{
    return mPtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T, class... Args>
Ptr<T> MakePtr(Args&&... args)
{
    return *new T(Forward<Args>(args)...);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Hash function
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T> struct HashKeyInfo;
template<typename T> struct HashKeyInfo<Ptr<T>>
{
    static bool IsEmpty(const Ptr<T>& key) { return key.GetPtr() == (T*)0x1; }
    static void MarkEmpty(Ptr<T>& key) { *(T**)(&key) = (T*)0x1; }
    static uint32_t HashValue(const Ptr<T>& key) { return Hash(key.GetPtr()); }
    static uint32_t HashValue(const T* key) { return Hash(key); }
    static bool IsEqual(const Ptr<T>& lhs, const Ptr<T>& rhs) { return lhs == rhs; }
    static bool IsEqual(const Ptr<T>& lhs, const T* rhs) { return lhs == rhs; }
};

}