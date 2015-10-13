//Created by KVClassFactory on Sat Oct 10 09:37:42 2015
//Author: John Frankland,,,

#ifndef __KVDETECTIONSIMULATOR_H
#define __KVDETECTIONSIMULATOR_H

#include "KVBase.h"
#include "KVMultiDetArray.h"
#include "KVEvent.h"
#include "KVDetectorEvent.h"
#include "KVTarget.h"

class KVDetectionSimulator : public KVBase {
private:
   KVMultiDetArray* fArray;//           array used for detection
   KVDetectorEvent fHitGroups;//       list of hit groups in simulation
   Bool_t fCalcTargELoss;//              whether to include energy loss in target, if defined
   const Double_t fMinKECutOff;//       particles with KE below cut off are considered stopped

public:
   KVDetectionSimulator() : KVBase(), fArray(nullptr), fCalcTargELoss(kTRUE), fMinKECutOff(1.e-3) {}
   KVDetectionSimulator(KVMultiDetArray* a) :
      KVBase(Form("DetectionSimulator_%s", a->GetName()),
             Form("Simulate detection of particles or events in detector array %s", a->GetTitle())),
      fArray(a), fCalcTargELoss(kTRUE), fMinKECutOff(1.e-3)
   {
      a->SetSimMode(kTRUE);
   }
   virtual ~KVDetectionSimulator() {}

   void SetArray(KVMultiDetArray* a)
   {
      fArray = a;
   }
   KVMultiDetArray* GetArray() const
   {
      return fArray;
   }
   KVTarget* GetTarget() const
   {
      return fArray->GetTarget();
   }
   void SetIncludeTargetEnergyLoss(Bool_t y = kTRUE)
   {
      fCalcTargELoss = y;
   }
   Bool_t IncludeTargetEnergyLoss() const
   {
      return fCalcTargELoss;
   }
   Double_t GetMinKECutOff() const
   {
      return fMinKECutOff;
   }

   void DetectEvent(KVEvent* event, const Char_t* detection_frame = "");
   KVNameValueList DetectParticle(KVNucleus*);
   KVNameValueList DetectParticleIn(const Char_t* detname, KVNucleus* kvp);


   ClassDef(KVDetectionSimulator, 0) //Simulate detection of particles or events in a detector array
};

#endif
