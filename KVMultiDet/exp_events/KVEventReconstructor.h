#ifndef __KVEVENTRECONSTRUCTOR_H
#define __KVEVENTRECONSTRUCTOR_H

#include <vector>
#include "KVBase.h"
#include "KVDetectorEvent.h"
#include "KVMultiDetArray.h"
#include "KVReconstructedEvent.h"

/**
  \class KVEventReconstructor
  \ingroup Reconstruction
  \brief Base class for event reconstruction from array data

 Event reconstruction begins with a KVMultiDetArray containing hit
 detectors (either following the simulated detection of a simulated event,
 or after reading some experimental data corresponding to an experimental
 event) and ends with a KVReconstructedEvent object filled with
 KVReconstructedNucleus objects corresponding to the reconstructed
 (and possibly identfied & calibrated) particles.

 The actual reconstruction is done by KVGroupReconstructor objects. Each group in a detector
 array is attributed a reconstructor at initialisation, which may be a specific implementation
 depending on the array (see classes derived from KVGroupReconstructor).

 The reconstruction of each event proceeds as follows:

 -# Get list of hit groups in the array
 -# For each hit group, perform reconstruction/identification/ etc. in the group
 -# Merge together the different event fragments into the output reconstructed event object
*/
class KVEventReconstructor : public KVBase {

private:
   KVMultiDetArray*       fArray;//!       Array for which events are to be reconstructed
   KVReconstructedEvent*  fEvent;//!       The reconstructed event
   TObjArray       fGroupReconstructor;//! array of group reconstructors
   Int_t           fNGrpRecon;//!          number of group reconstructors for current event
   std::vector<int> fHitGroups;//!         group indices in current event
   KVDetectorEvent detev;//!               list of hit groups in event

protected:
   KVMultiDetArray* GetArray()
   {
      return fArray;
   }
   void MergeGroupEventFragments();
   TObjArray* GetReconstructors()
   {
      return &fGroupReconstructor;
   }
   Int_t get_number_reconstructors_current_event() const
   {
      return fNGrpRecon;
   }
   Int_t get_group_index_for_current_event_reconstructor(Int_t i) const
   {
      return fHitGroups[i];
   }

public:
   KVEventReconstructor(KVMultiDetArray*, KVReconstructedEvent*);
   virtual ~KVEventReconstructor() {}

   void Copy(TObject& obj) const;

   virtual void ReconstructEvent(const TSeqCollection* = nullptr);

   KVReconstructedEvent* GetEvent()
   {
      return fEvent;
   }

   KVGroupReconstructor* GetReconstructor(int group_number)
   {
      return (KVGroupReconstructor*)fGroupReconstructor[group_number];
   }
   ClassDef(KVEventReconstructor, 0) //Base class for handling event reconstruction
};

#endif
