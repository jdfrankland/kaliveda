#ifndef KVFAZIAGROUPRECONSTRUCTOR_H
#define KVFAZIAGROUPRECONSTRUCTOR_H

#include "KVGroupReconstructor.h"
#include "KVFAZIADetector.h"

/**
\class KVFAZIAGroupReconstructor
\brief Reconstruction of particles detected in FAZIA telescopes
\ingroup FAZIARecon

*/

class KVFAZIAGroupReconstructor : public KVGroupReconstructor {

   KVGeoDNTrajectory* theTrajectory;//! 1 FAZIA group = 1 telescope with 1 unique trajectory SI1 - SI2 - CSI
   KVFAZIADetector* si1, *si2, *csi;
   KVIDTelescope* fSi1Si2IDTelescope;//! SI1-SI2 ID Telescope

protected:

   void CalibrateParticle(KVReconstructedNucleus* PART);
   void CalibrateCoherencyParticle(KVReconstructedNucleus* PART);
   void PostReconstructionProcessing();
   void IdentifyParticle(KVReconstructedNucleus& PART);

   void  ChangeReconstructedTrajectory(KVReconstructedNucleus& PART);

   void HandleSI1SI2PunchThrough(KVIdentificationResult*, KVReconstructedNucleus&);

   void AddCoherencyParticles();
public:
   KVFAZIAGroupReconstructor(const KVGroup* g = nullptr) : KVGroupReconstructor(g)
   {
      if (g) SetGroup(g);
   }
   void SetGroup(const KVGroup* g);

   ClassDef(KVFAZIAGroupReconstructor, 1) // Reconstruct particles in FAZIA telescopes
};

#endif // KVFAZIAGROUPRECONSTRUCTOR_H
