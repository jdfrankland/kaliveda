#ifndef KVGROUP_H
#define KVGROUP_H

#include "KVGeoStrucElement.h"
#include "KVList.h"

#include <KVGeoDNTrajectory.h>

class KVDetector;
class KVNucleus;
class KVNameValueList;

class KVGroup : public KVGeoStrucElement {

protected:
    KVList fReconstructedNuclei;        // Particles reconstructed in this group
    KVHashList fTrajectories;           // Trajectories passing through group

public:
   KVGroup();
   void init();
   virtual ~ KVGroup();
   virtual void SetNumber(UInt_t num)
   {
      // Setting number for group also sets name to "Group_n"
      SetName(Form("Group_%u", num));
      KVGeoStrucElement::SetNumber(num);
   };

   void Reset();

    UInt_t GetHits() { return fReconstructedNuclei.GetSize(); }
    void ClearHitDetectors();
    KVList *GetParticles() {
        return &fReconstructedNuclei;
   }
   void AddHit(KVNucleus* kvd);
   void RemoveHit(KVNucleus* kvd);

    const TCollection* GetTrajectories() const { return &fTrajectories; }
    void AddTrajectory(KVGeoDNTrajectory* t) { fTrajectories.Add(t); }
    void AddTrajectories(const TCollection* c) { fTrajectories.AddAll(c); }

   ClassDef(KVGroup, 1)//Group of detectors having similar angular positions.
};

#endif
