#ifndef __KVINDRAFILTERGROUPRECONSTRUCTOR_H
#define __KVINDRAFILTERGROUPRECONSTRUCTOR_H

#include "KVFilterGroupReconstructor.h"
#include "KVChIo.h"
/**
 \class KVINDRAFilterGroupReconstructor
 \brief Reconstruct simulated events after filtering with INDRA

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Fri Jul  2 12:06:08 2021
*/

class KVINDRAFilterGroupReconstructor : public KVFilterGroupReconstructor {

   KVChIo* theChIo = nullptr;//! pointer to ChIo in group, if present

   KVReconstructedNucleus* ReconstructTrajectory(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);

protected:
   void identify_particle(KVIDTelescope* idt, KVIdentificationResult* IDR, KVReconstructedNucleus& nuc);

public:
   KVINDRAFilterGroupReconstructor(const KVGroup* g = nullptr)
      : KVFilterGroupReconstructor(g)
   {
      if (g) theChIo = dynamic_cast<KVChIo*>(g->GetDetectorByType("CI"));
   }

   void Reconstruct();
   void IdentifyParticle(KVReconstructedNucleus&);

   ClassDef(KVINDRAFilterGroupReconstructor, 1) //Reconstruct simulated events after filtering with INDRA
};

#endif
