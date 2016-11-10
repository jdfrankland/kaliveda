//Created by KVClassFactory on Mon Oct 19 14:11:26 2015
//Author: John Frankland,,,

#ifndef __KVGROUPRECONSTRUCTOR_H
#define __KVGROUPRECONSTRUCTOR_H

#include "KVBase.h"
#include "KVGroup.h"
#include "KVReconstructedEvent.h"
#include "KVEventReconstructor.h"

class KVGroupReconstructor : public KVBase {
   friend class KVEventReconstructor;

   KVGroup*              fGroup;//!        the group where we are reconstructing
   KVReconstructedEvent* fGrpEvent;//!     event containing particles reconstructed in this group
   KVEventReconstructor* fEvRecon;//!      parent class for reconstructing event

   void SetReconEventClass(TClass* c);

protected:
   virtual void ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);
   virtual void IdentifyParticle(KVReconstructedNucleus* PART);
   virtual void CalibrateParticle(KVReconstructedNucleus* PART);

public:
   KVGroupReconstructor();
   virtual ~KVGroupReconstructor();

   void Copy(TObject& obj) const;

   KVReconstructedEvent* GetEventFragment() const
   {
      return fGrpEvent;
   }
   void SetGroup(KVGroup* g)
   {
      fGroup = g;
   }
   KVGroup* GetGroup() const
   {
      return fGroup;
   }
   void SetEventReconstructor(KVEventReconstructor* evr)
   {
      // Set parent event reconstructor.
      // Also initializes reconstructed event fragment if not already done
      fEvRecon = evr;
      if (!fGrpEvent) SetReconEventClass(evr->GetEvent()->IsA());
   }
   KVEventReconstructor* GetEventReconstructor() const
   {
      return fEvRecon;
   }

   static KVGroupReconstructor* Factory(const char*);

   void Reconstruct();
   void Identify();
   void Calibrate();

   void AnalyseParticles();
   Int_t GetNIdentifiedInGroup()
   {
      //number of identified particles reconstructed in group
      Int_t n = 0;
      if (GetEventFragment()->GetMult()) {
         KVReconstructedNucleus* nuc;
         GetEventFragment()->ResetGetNextParticle();
         while ((nuc = GetEventFragment()->GetNextParticle())) n += (Int_t) nuc->IsIdentified();
      }
      return n;
   }
   Int_t GetNUnidentifiedInGroup()
   {
      //number of unidentified particles reconstructed in group
      return (GetEventFragment()->GetMult() - GetNIdentifiedInGroup());
   }

   ClassDef(KVGroupReconstructor, 0) //Base class for handling event reconstruction in detector groups
};

#endif
