#ifndef __KVFILTEREVENTRECONSTRUCTOR_H
#define __KVFILTEREVENTRECONSTRUCTOR_H

#include "KVEventReconstructor.h"
#include "KVFilterGroupReconstructor.h"

/**
 \class KVFilterEventReconstructor
 \brief Reconstruct events after filtering a simulation

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Fri Jul  2 14:18:05 2021
*/

class KVFilterEventReconstructor : public KVEventReconstructor {
   KVEvent* fSimEvent = nullptr; //! the simulated event used as input to the filter
   TString fDetectionFrame; //! the frame used in the call to KVDetectionSimulator::DetectEvent()

public:
   KVFilterEventReconstructor(KVMultiDetArray* arg1, KVReconstructedEvent* arg2,
                              KVEvent* E = nullptr, const TString& det_frame = "");

   void ReconstructEvent(const TSeqCollection* = nullptr);

   void SetSimEvent(KVEvent* E)
   {
      // \param[in] E pointer to the event which is being filtered
      //
      // uses pointer to simulated event in order to create events of same class inside the group reconstructors
      //
      // if it has already been called (fSimEvent != nullptr), does nothing

      if (fSimEvent) return;

      fSimEvent = E;
      TIter it(GetReconstructors());
      KVFilterGroupReconstructor* gr;
      while ((gr = (KVFilterGroupReconstructor*)it())) gr->SetSimEvent(E);
   }
   void SetDetectionFrame(const TString& det_frame)
   {
      // \param[in] det_frame name of kinematical frame used in call to KVDetectionSimulator::DetectEvent()
      //
      // must be called at least once before calling ReconstructEvent() (unless default kinematical frame of particles is used)

      fDetectionFrame = det_frame;
   }

   ClassDef(KVFilterEventReconstructor, 1) //Reconstruct events after filtering a simulation
};

#endif
