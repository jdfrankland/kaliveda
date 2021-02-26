#ifndef KVCSI_H
#define KVCSI_H

#include "KVINDRADetector.h"
#include "KVINDRACsITotLightReconstructor.h"

/**
\class KVCsI
\brief CsI(Tl) scintillation detectors of the INDRA multidetector array
\ingroup INDRAGeometry

Type of detector: "CSI"
*/

class KVCsI: public KVINDRADetector {

   Char_t  fPinLaser;//number of pin laser used to control stability of crystal

protected:
   void init();

public:
   KVCsI();
   KVCsI(Float_t thick, Float_t thickAl = 0 /* um */);
   virtual ~ KVCsI();

   void SetAlThickness(Float_t thickAl /* um */);


   Double_t GetCorrectedEnergy(KVNucleus*, Double_t lum = -1., Bool_t transmission = kTRUE);

   void SetPinLaser(Int_t n)
   {
      if (n > 0 && n < 255) fPinLaser = (Char_t)n;
   }
   Int_t GetPinLaser()
   {
      // Returns number of Pin Laser used to control stability of CsI detector.
      return (Int_t)fPinLaser;
   }

   void DeduceACQParameters(KVEvent*, KVNumberList&);

   virtual KVDetectorSignal* GetDetectorSignal(const KVString& type) const
   {
      // Overrides KVDetector method for special case of `TotLight` signal.
      //
      // If we try to add a calibration which requires `TotLight` as input before the signal is defined,
      // then we add it here **IF** the required `L_PedCor` and `R_PedCor` signals exist, i.e. if pedestals
      // have been set for fast and slow acquisition parameters

      if (type == "TotLight" && !KVDetector::GetDetectorSignal(type)) {
         if (KVDetector::GetDetectorSignal("L_PedCor") && KVDetector::GetDetectorSignal("R_PedCor")) {
            const_cast<KVCsI*>(this)->AddDetectorSignal(new KVINDRACsITotLightReconstructor(this));
         }
      }
      return KVDetector::GetDetectorSignal(type);
   }
   void RemoveCalibrators()
   {
      // Overrides KVDetector method for special case of `TotLight` signal.
      //
      // If `TotLight` was added to calculate from `L_PedCor` and `R_PedCor` signals we
      // have to remove it from the list of signals by hand (no associated calibrator).

      KVDetectorSignal* totlight = GetListOfDetectorSignals().get_object<KVDetectorSignal>("TotLight");
      if (totlight && !totlight->IsRaw() && GetDetectorSignal("L_PedCor") && GetDetectorSignal("R_PedCor")) {
         const_cast<KVSeqCollection&>(GetListOfDetectorSignals()).Remove(totlight);
         delete totlight;
      }
      KVDetector::RemoveCalibrators();
   }

   ClassDef(KVCsI, 5)           // The CsI(Tl) detectors of the INDRA array
};

#endif
