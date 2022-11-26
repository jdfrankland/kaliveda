//Created by KVClassFactory on Tue Feb 27 11:35:10 2018
//Author: John Frankland,,,

#ifndef __KVINDRAFORWARDGROUPRECONSTRUCTOR_H
#define __KVINDRAFORWARDGROUPRECONSTRUCTOR_H

#include "KVINDRAGroupReconstructor.h"

/**
\class KVINDRAForwardGroupReconstructor
\brief Reconstruct data in rings 1-9 of INDRA
\ingroup INDRAReconstruction
*/

class KVINDRAForwardGroupReconstructor : public KVINDRAGroupReconstructor {

protected:
   void DoNeutronCalibration(KVReconstructedNucleus* PART);
   Bool_t CoherencyChIoSiCsI(KVReconstructedNucleus& PART);
   Bool_t CoherencySiCsI(KVReconstructedNucleus& PART);
   Bool_t CalculateSiliconDEFromResidualEnergy(KVReconstructedNucleus* n, KVDetector* si);
   KVDetector* GetSi(KVReconstructedNucleus* n)
   {
      return n->GetReconstructionTrajectory()->GetDetector("SI");
   }
public:
   KVINDRAForwardGroupReconstructor(const KVGroup* g = nullptr) : KVINDRAGroupReconstructor(g) {}
   virtual ~KVINDRAForwardGroupReconstructor() {}

   bool DoCoherencyAnalysis(KVReconstructedNucleus& PART);

   void DoCalibration(KVReconstructedNucleus* PART);

   ClassDef(KVINDRAForwardGroupReconstructor, 1) //Reconstruct data in rings 1-9 of INDRA
};

#endif
