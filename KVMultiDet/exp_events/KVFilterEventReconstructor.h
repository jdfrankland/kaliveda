#ifndef __KVFILTEREVENTRECONSTRUCTOR_H
#define __KVFILTEREVENTRECONSTRUCTOR_H

#include "KVEventReconstructor.h"

/**
 \class KVFilterEventReconstructor
 \brief Reconstruct events after filtering a simulation

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Fri Jul  2 14:18:05 2021
*/

class KVFilterEventReconstructor : public KVEventReconstructor {
   KVEvent* fSimEvent; //! the simulated event used as input to the filter
   TString fDetectionFrame; //! the frame used in the call to KVDetectionSimulator::DetectEvent()

public:
   KVFilterEventReconstructor(KVMultiDetArray* arg1, KVReconstructedEvent* arg2, KVEvent* E, const TString& det_frame);
   virtual ~KVFilterEventReconstructor() {}

   void ReconstructEvent(const TSeqCollection* = nullptr);

   ClassDef(KVFilterEventReconstructor, 1) //Reconstruct events after filtering a simulation
};

#endif
