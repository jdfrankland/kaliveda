#include "KVFilterEventReconstructor.h"
#include "KVFilterGroupReconstructor.h"
#include "KVNucleusEvent.h"

KVFilterEventReconstructor::KVFilterEventReconstructor(KVMultiDetArray* arg1, KVReconstructedEvent* arg2, KVEvent* E, const TString& det_frame)
   : KVEventReconstructor(arg1, arg2), fSimEvent(E), fDetectionFrame(det_frame)
{
   // set up simulated events in reconstructors
   TIter it(GetReconstructors());
   KVFilterGroupReconstructor* gr;
   while ((gr = (KVFilterGroupReconstructor*)it())) gr->SetSimEvent(E);
}

void KVFilterEventReconstructor::ReconstructEvent(const TSeqCollection* fired)
{
   // Copy particles of simulated event to group reconstructors before reconstruction.
   //
   // The particle default kinematics are in the detection frame.

   for (auto& n : EventIterator(fSimEvent)) {
      if (n.GetParameters()->HasIntParameter("GROUP")) {
         auto gr = GetReconstructor(n.GetParameters()->GetIntValue("GROUP"));
         dynamic_cast<KVFilterGroupReconstructor*>(gr)->AddSimParticle((const KVNucleus*)n.GetFrame(fDetectionFrame));
      }
   }

   KVEventReconstructor::ReconstructEvent(fired);
}

ClassImp(KVFilterEventReconstructor)


