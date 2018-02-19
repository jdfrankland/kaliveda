#ifndef KVGROUP_H
#define KVGROUP_H

#include "KVGeoStrucElement.h"
#include "KVGeoDNTrajectory.h"

class KVGroup : public KVGeoStrucElement {

protected:
   KVHashList fTrajectories;           //! Trajectories passing through group
   KVHashList fReconTraj;              //! list of all possible trajectories for reconstructed particles
   KVNameValueList fReconTrajMap;      //! map names of duplicate trajectories for reconstructed particles

public:
   KVGroup();
   void init();
   virtual ~KVGroup() {}
   virtual void SetNumber(UInt_t num)
   {
      // Setting number for group also sets name to "Group_n"
      SetName(Form("Group_%u", num));
      KVGeoStrucElement::SetNumber(num);
   }
   void Reset();
   void ClearHitDetectors();
   const TCollection* GetTrajectories() const
   {
      return &fTrajectories;
   }
   void AddTrajectory(KVGeoDNTrajectory* t)
   {
      fTrajectories.Add(t);
   }
   void AddTrajectories(const TCollection* c)
   {
      fTrajectories.AddAll(c);
   }

   void CalculateReconstructionTrajectories();
   const TSeqCollection* GetReconTrajectories() const
   {
      // Get list of all possible trajectories for particle reconstruction in array
      return &fReconTraj;
   }
   const KVGeoDNTrajectory* GetTrajectoryForReconstruction(const KVGeoDNTrajectory* t, const KVGeoDetectorNode* n) const
   {
      TString mapped_name = fReconTrajMap.GetStringValue(Form("%s_%s", t->GetName(), n->GetName()));
      const KVGeoDNTrajectory* tr = (const KVGeoDNTrajectory*)fReconTraj.FindObject(mapped_name);
      return tr;
   }

   ClassDef(KVGroup, 1)//Group of detectors having similar angular positions.
};

#endif
