#include "KVFilterEventReconstructor.h"
#include "KVFilterGroupReconstructor.h"
#include "KVNucleusEvent.h"

KVFilterEventReconstructor::KVFilterEventReconstructor(KVMultiDetArray* arg1, KVReconstructedEvent* arg2, KVEvent* E, const TString& det_frame)
   : KVEventReconstructor(arg1, arg2), fSimEvent(E), fDetectionFrame(det_frame)
{
   if (E) SetSimEvent(E);
}

void KVFilterEventReconstructor::ReconstructEvent(const TSeqCollection*)
{
   // Copy particles of simulated event to group reconstructors before reconstruction.
   //
   // The particle default kinematics are in the detection frame.

   TList fired; // list of stopping detectors for all particles
   for (auto& n : EventIterator(fSimEvent)) {
      if (n.GetParameters()->HasIntParameter("GROUP")) {
         auto gr = GetReconstructor(n.GetParameters()->GetIntValue("GROUP"));
         if (!gr) {
            Error("ReconstructEvent", "No reconstructor for group %d", n.GetParameters()->GetIntValue("GROUP"));
            n.Print();
            assert(gr);
         }
         dynamic_cast<KVFilterGroupReconstructor*>(gr)->AddSimParticle((const KVNucleus*)n.GetFrame(fDetectionFrame));
         fired.Add(GetArray()->GetDetector(n.GetParameters()->GetStringValue("STOPPING DETECTOR")));
      }
   }

   KVEventReconstructor::ReconstructEvent(&fired);
}

ClassImp(KVFilterEventReconstructor)


