#ifndef __KVFILTERGROUPRECONSTRUCTOR_H
#define __KVFILTERGROUPRECONSTRUCTOR_H

#include "KVGroupReconstructor.h"
#include <unordered_map>
/**
 \class KVFilterGroupReconstructor
 \brief Reconstruct simulated events after filtering

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Fri Jul  2 11:44:58 2021
*/

class KVFilterGroupReconstructor : public KVGroupReconstructor {
   std::unique_ptr<KVEvent> fSimEvent;//! particles of simulated event detected in this group
   std::unordered_map<std::string, int> hits; //! count simulated particles in stopping detectors
   std::unordered_map<KVReconstructedNucleus*, TList> part_correspond; //! correspondence between reconstructed and simulated particles
   KVReconstructedNucleus* current_nuc_recon;//! temporary, store argument to ReconstructParticle

   std::unordered_map<std::string, double> energy_loss; //! energy losses in detectors
   std::unordered_map<std::string, int> number_uncalibrated; //! number of particles for which the energy contribution of detector has not yet been set
   std::unordered_map<std::string, int> number_unidentified; //! number of particles hitting detector aas yet unidentified
protected:
   KVReconNucTrajectory* get_recon_traj_for_particle(const KVGeoDNTrajectory*, const KVGeoDetectorNode* node);
   void identify_particle(KVIDTelescope* idt, KVIdentificationResult* IDR, KVReconstructedNucleus& nuc);

   void PerformSecondaryAnalysis();

public:
   KVFilterGroupReconstructor() {}
   virtual ~KVFilterGroupReconstructor() {}

   void SetSimEvent(KVEvent* e)
   {
      // Reset simevent pointer with new event of correct class

      fSimEvent.reset(static_cast<KVEvent*>(e->IsA()->New()));
   }
   void AddSimParticle(const KVNucleus* n)
   {
      // Add copy of simulated particle to internal list of particles detected by this group
      //
      // Increment hit count in each detector on particle's trajectory

      KVNucleus* nuc;
      n->Copy(*(nuc = fSimEvent->AddParticle()));
      auto traj = GetGroup()->FindReconTraj(n->GetParameters()->GetStringValue("TRAJECTORY"));
      traj->IterateFrom();
      while (auto node = traj->GetNextNode()) {
         ++hits[node->GetName()];
         ++number_unidentified[node->GetName()];
      }
   }
   void Clear(Option_t* = "")
   {
      // Clear copies of simulated particles from internal list
      fSimEvent->Clear();
      hits.clear();
      part_correspond.clear();
      energy_loss.clear();
      number_uncalibrated.clear();
      number_unidentified.clear();
   }
   void ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);
   void IdentifyParticle(KVReconstructedNucleus& PART);
   void CalibrateParticle(KVReconstructedNucleus* PART);

   ClassDef(KVFilterGroupReconstructor, 1) //Reconstruct simulated events after filtering
};

#endif
