//Created by KVClassFactory on Wed Jun  5 17:00:08 2019
//Author: John Frankland,,,

#ifndef __KVDETECTORSIGNAL_H
#define __KVDETECTORSIGNAL_H

#include "KVBase.h"

class KVDetector;

class KVDetectorSignal : public KVBase {

   KVDetector* fDetector;//! associated detector

public:
   KVDetectorSignal()
      : KVBase(), fDetector(nullptr)
   {}
   KVDetectorSignal(const Char_t* type)
      : KVBase(type), fDetector(nullptr)
   {}
   virtual ~KVDetectorSignal()
   {}

   virtual Double_t GetValue() const
   {
      return 0;
   }
   virtual Double_t GetInverseValue(Double_t, const TString&) const
   {
      return 0;
   }

   void SetDetector(KVDetector*);
   KVDetector* GetDetector() const;

   ClassDef(KVDetectorSignal, 1) //Signal associated with detector
};

#endif
