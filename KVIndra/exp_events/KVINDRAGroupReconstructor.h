//Created by KVClassFactory on Wed Feb 21 13:42:47 2018
//Author: John Frankland,,,

#ifndef __KVINDRAGROUPRECONSTRUCTOR_H
#define __KVINDRAGROUPRECONSTRUCTOR_H

#include "KVGroupReconstructor.h"
#include "KVINDRA.h"

/**
\class KVINDRAGroupReconstructor
\brief Reconstruct particles in INDRA groups
\ingroup INDRAReconstruction
*/

class KVINDRAGroupReconstructor : public KVGroupReconstructor {

   friend class KVINDRA;

protected:
   KVDetector* theChio;                 // the ChIo of the group
   Double_t fECsI, fESi, fEChIo;
   bool print_part;//debug
   static TString CSI_ID_TYPE;

   void SetBadCalibrationStatus(KVReconstructedNucleus* n)
   {
      n->SetIsUncalibrated();
      n->SetECode(KVINDRA::ECodes::BAD_CALIBRATION);
      n->SetEnergy(0);
   }
   void SetNoCalibrationStatus(KVReconstructedNucleus* n)
   {
      n->SetIsUncalibrated();
      n->SetECode(KVINDRA::ECodes::NO_CALIBRATION_ATTEMPTED);
      n->SetEnergy(0);
   }

   double DoBeryllium8Calibration(KVReconstructedNucleus* n);
   void CheckCsIEnergy(KVReconstructedNucleus* n);
   KVDetector* GetCsI(KVReconstructedNucleus* n)
   {
      return n->GetReconstructionTrajectory()->GetDetector("CSI");
   }

   Bool_t CalculateChIoDEFromResidualEnergy(KVReconstructedNucleus* n, Double_t ERES);

   /// TO BE IMPLEMENTED!!!
   void CalibrateCoherencyParticle(KVReconstructedNucleus*) {}

public:
   KVINDRAGroupReconstructor(const KVGroup* g = nullptr) : KVGroupReconstructor(g)
   {
      if (g) theChio = g->GetDetectorByType("CI");
   }
   virtual ~KVINDRAGroupReconstructor() {}

   void SetGroup(KVGroup* g)
   {
      KVGroupReconstructor::SetGroup(g);
      theChio = g->GetDetectorByType("CI");
   }

   KVReconstructedNucleus* ReconstructTrajectory(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);

   void Identify();
   void IdentifyParticle(KVReconstructedNucleus& PART);
   void CalibrateParticle(KVReconstructedNucleus* PART);

   virtual bool DoCoherencyAnalysis(KVReconstructedNucleus&) = 0;

   virtual void DoCalibration(KVReconstructedNucleus*) {}

   ClassDef(KVINDRAGroupReconstructor, 1) //Reconstruct particles in INDRA groups
};

#endif
