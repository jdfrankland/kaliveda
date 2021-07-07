#ifndef __KVGROUPRECONSTRUCTOR_H
#define __KVGROUPRECONSTRUCTOR_H

#include "KVBase.h"
#include "KVGroup.h"
#include "KVReconstructedEvent.h"
#include <unordered_map>
#include <string>

/**
  \class KVGroupReconstructor
  \ingroup Reconstruction
  \brief Base class for particle reconstruction in one group of a detector array

KVGroupReconstructor is the basic working unit of event
reconstruction. A KVEventReconstructor will use many KVGroupReconstructor objects in order to
reconstruct an event from data detected by the array (let us recall that a group of detectors
is the largest part of any array which can be treated independently of all other detectors).

Daughter classes of KVGroupReconstructor can be specialised for event reconstruction in
specific arrays (or even specific parts of specific arrays).

\sa KVEventReconstructor, KVGroup, KVMultiDetArray
 */
class KVGroupReconstructor : public KVBase {

   static bool fDoIdentification;
   static bool fDoCalibration;

   KVGroup*              fGroup;//!        the group where we are reconstructing
   KVReconstructedEvent* fGrpEvent;//!     event containing particles reconstructed in this group
   TString               fPartSeedCond;//! condition for seeding reconstructed particles
protected:
   mutable int nfireddets;//! number of fired detectors in group for current event
   KVIdentificationResult partID;//! identification to be applied to current particle
   KVIDTelescope* identifying_telescope;//! telescope which identified current particle
   std::unordered_map<std::string, KVIdentificationResult*> id_by_type; //! identification results by type for current particle

   virtual KVReconstructedNucleus* ReconstructTrajectory(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);
   void ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);
   virtual void PostReconstructionProcessing();
   virtual void IdentifyParticle(KVReconstructedNucleus& PART);
   virtual void CalibrateParticle(KVReconstructedNucleus*)
   {
      AbstractMethod("CalibrateParticle(KVReconstructedNucleus*)");
   }
   virtual void CalibrateCoherencyParticle(KVReconstructedNucleus*)
   {
      AbstractMethod("CalibrateCoherencyParticle(KVReconstructedNucleus*)");
   }
   void SetCalibrationStatus(KVReconstructedNucleus& PART, UShort_t code)
   {
      // Set status of particle to 'IsCalibrated' and give the quality code corresponding to the calibration
      PART.SetIsCalibrated();
      PART.SetECode(code);
   }

   Double_t GetTargetEnergyLossCorrection(KVReconstructedNucleus* ion);
   TString GetPartSeedCond() const
   {
      return fPartSeedCond;
   }
   void TreatStatusStopFirstStage(KVReconstructedNucleus&);

   /**
     \struct particle_to_add_from_coherency_analysis
     \brief informations required to add a particle to the event which is revealed by an inconsistency between the different identifications for an existing particle
    */
   struct particle_to_add_from_coherency_analysis {
      KVReconstructedNucleus* original_particle;  ///< particle whose identification/calibration revealed presence of pile-up
      KVGeoDetectorNode* stopping_detector_node;  ///< detector node in which new particle stopped
      KVGeoDNTrajectory* stopping_trajectory;     ///< trajectory on which new particle stopped
      KVIDTelescope* identifying_telescope;       ///< the identification telescope used to identify the particle
      Int_t first_id_result_to_copy;              ///< number of KVIdentificationResult (in original_particle's list) corresponding to identification of the particle
      Int_t max_id_result_index;                  ///< last KVIdentificationResult in original_particle's list
   };
   std::vector<particle_to_add_from_coherency_analysis> coherency_particles;
   virtual void AddCoherencyParticles() {};

   virtual KVReconNucTrajectory* get_recon_traj_for_particle(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);
   virtual void identify_particle(KVIDTelescope* idt, KVIdentificationResult* IDR, KVReconstructedNucleus&)
   {
      idt->Identify(IDR);
   }

public:
   KVGroupReconstructor();
   virtual ~KVGroupReconstructor();

   void SetReconEventClass(TClass* c);
   int GetNFiredDets() const
   {
      return nfireddets;
   }

   KVReconstructedEvent* GetEventFragment() const
   {
      return fGrpEvent;
   }
   virtual void SetGroup(KVGroup* g);
   KVGroup* GetGroup() const
   {
      return fGroup;
   }

   static KVGroupReconstructor* Factory(const TString& plugin = "");

   void Process();
   void Reconstruct();
   virtual void Identify();
   void Calibrate();

   void AnalyseParticles();
   Int_t GetNIdentifiedInGroup()
   {
      //number of identified particles reconstructed in group
      Int_t n = 0;
      if (GetEventFragment()->GetMult()) {
         for (KVReconstructedEvent::Iterator it = GetEventFragment()->begin(); it != GetEventFragment()->end(); ++it) {
            KVReconstructedNucleus& nuc = it.get_reference();
            n += (Int_t) nuc.IsIdentified();
         }
      }
      return n;
   }
   Int_t GetNUnidentifiedInGroup()
   {
      //number of unidentified particles reconstructed in group
      return (GetEventFragment()->GetMult() - GetNIdentifiedInGroup());
   }
   static void SetDoIdentification(bool on = kTRUE)
   {
      // Enable/Disable identification step in KVGroupReconstructor::Process
      fDoIdentification = on;
   }
   static void SetDoCalibration(bool on = kTRUE)
   {
      // Enable/Disable calibration step in KVGroupReconstructor::Process
      fDoCalibration = on;
   }
   void Clear(Option_t* = "")
   {
      // Reset before treating a new event
   }

   ClassDef(KVGroupReconstructor, 0) //Base class for handling event reconstruction in detector groups
};

#endif
