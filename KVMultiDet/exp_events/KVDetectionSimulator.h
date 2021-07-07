//Created by KVClassFactory on Sat Oct 10 09:37:42 2015
//Author: John Frankland,,,

#ifndef __KVDETECTIONSIMULATOR_H
#define __KVDETECTIONSIMULATOR_H

#include "KVBase.h"
#include "KVMultiDetArray.h"
#include "KVNucleusEvent.h"
#include "KVDetectorEvent.h"
#include "KVTarget.h"
#include "KVRangeTableGeoNavigator.h"

/**
  \class KVDetectionSimulator
  \ingroup Simulation
  \brief Simulate detection of events in a detector array
 */
class KVDetectionSimulator : public KVBase {

   KVMultiDetArray* fArray = nullptr;        //  array used for detection
   KVDetectorEvent  fHitGroups;              //  used to reset hit detectors in between events
   Bool_t           fCalcTargELoss = kTRUE;  //  whether to include energy loss in target, if defined
   Bool_t           fGeoFilter = kFALSE;     //  when true, only consider geometry, not particle energies

   KVRangeTableGeoNavigator* get_array_navigator() const
   {
      return static_cast<KVRangeTableGeoNavigator*>(fArray->GetNavigator());
   }
   KVNameValueList PropagateParticle(KVNucleus*);
public:
   KVDetectionSimulator() {}
   KVDetectionSimulator(KVMultiDetArray* a, Double_t cut_off = 1.e-3);
   virtual ~KVDetectionSimulator() {}

   void SetGeometricFilterMode()
   {
      // In geometric filter mode, energy loss in target is not calculated and particles
      // do not need to have sufficient energy to reach the detectors
      SetIncludeTargetEnergyLoss(kFALSE);
      fGeoFilter = kTRUE;
   }

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
      return get_array_navigator()->GetCutOffKEForPropagation();
   }
   void SetMinKECutOff(Double_t cutoff)
   {
      get_array_navigator()->SetCutOffKEForPropagation(cutoff);
   }

   void DetectEvent(KVEvent* event, const Char_t* detection_frame = "");

   void ClearHitGroups()
   {
      // Reset any detectors/groups hit by previous detection
      fHitGroups.Clear();
   }

   ClassDef(KVDetectionSimulator, 0) //Simulate detection of particles or events in a detector array
};

#endif
