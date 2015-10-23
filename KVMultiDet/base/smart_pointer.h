//Created by KVClassFactory on Wed Oct 21 12:57:36 2015
//Author: John Frankland,,,

#ifndef __SMART_POINTER_H
#define __SMART_POINTER_H

#include "Rtypes.h"

template<typename T>
class smart_pointer {
   T* ptr;

public:
   smart_pointer() : ptr(nullptr) {}
   smart_pointer(T* _ptr) : ptr(_ptr) {}
   smart_pointer(const smart_pointer<T>& other) : ptr(other.ptr) {}
   ~smart_pointer()
   {
      if (ptr != nullptr) delete ptr;
   }

   operator T* () const
   {
      return ptr;
   }

   smart_pointer<T>& operator=(const smart_pointer<T>& other)
   {
      ptr = other.ptr;
      return (*this);
   }

   T* operator->()
   {
      return ptr;
   }
};

#endif
