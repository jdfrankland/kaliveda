//Created by KVClassFactory on Wed Oct 21 12:57:36 2015
//Author: John Frankland,,,

/////////////////////////////////////////////////////////////////
// SIMPLE SMART POINTER CLASS
//
// Sometimes we have methods which instantiate an object (via 'new')
// and then return a pointer to this object which the user is supposed
// to 'delete' after use to avoid memory leaks.
// This presupposes: (1) that the instruction to delete the returned
// object pointer is clearly written in the method's comments/doc;
// (2) that the user has read the comments/doc; (3) that the user doesn't
// forget to apply what he/she read in the comments/doc.
//
// For this reason, smart pointers were invented. A smart pointer is
// a wrapper around a 'raw' pointer which will delete the pointed-to
// object as soon as it (the smart pointer) goes out of scope:
//
// smart_pointer<TObject> func2()
// {
//   return new TObject;                    // create & return TObject through smart pointer
// }
//
// int main()
// {
//
//    smart_pointer<TObject> p = func2();   // retrieve smart pointer to created TObject
//    p->Draw();                            // use as simple pointer to object
//
// } <---------------------smart pointer goes out of scope: object deleted

#include "smart_pointer.h"

