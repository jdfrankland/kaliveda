//Created by KVClassFactory on Mon Oct 19 09:33:44 2015
//Author: John Frankland,,,

#include "KVEventReconstructor.h"
#include "KVDetectorEvent.h"
#include "KVGroupReconstructor.h"
#include "KVTarget.h"

// for parallelisation
#ifdef _OPENMP
#include <omp.h>
#endif

ClassImp(KVEventReconstructor)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVEventReconstructor</h2>
<h4>Base class for handling event reconstruction</h4>
<!-- */
// --> END_HTML
// Event reconstruction begins with a KVMultiDetArray containing hit
// detectors (either following the simulated detection of a simulated event,
// or after reading some experimental data corresponding to an experimental
// event) and ends with a KVReconstructedEvent object filled with
// KVReconstructedNucleus objects corresponding to the reconstructed
// (and possibly identfied & calibrated) particles.
//
// 1. Get list of hit groups (KVDetectorEvent)
// 2. For each hit group, perform reconstruction/identification/ etc. in the group
//    using a KVGroupReconstructor-derived object (uses plugins)
// 3. Merge together the different event fragments into the output reconstructed
//    event object
////////////////////////////////////////////////////////////////////////////////

KVEventReconstructor::KVEventReconstructor(KVMultiDetArray* a, KVReconstructedEvent* e)
   : KVBase("KVEventReconstructor", Form("Reconstruction of events in array %s", a->GetName())),
     fArray(a), fEvent(e), fGroupReconstructor(nullptr)
{
   // Default constructor
}

KVEventReconstructor::~KVEventReconstructor()
{
   // Destructor

}

//________________________________________________________________

void KVEventReconstructor::Copy(TObject& obj) const
{
   // This method copies the current state of 'this' object into 'obj'
   // You should add here any member variables, for example:
   //    (supposing a member variable KVEventReconstructor::fToto)
   //    CastedObj.fToto = fToto;
   // or
   //    CastedObj.SetToto( GetToto() );

   KVBase::Copy(obj);
   //KVEventReconstructor& CastedObj = (KVEventReconstructor&)obj;
}

void KVEventReconstructor::SetGroupReconstructorPlugin(const char* p)
{
   // Set up TClonesArray of group reconstructors for detector array

   unique_ptr<KVGroupReconstructor> gr(KVGroupReconstructor::Factory(p));
   TClass* plugin_class = gr->IsA();

   unique_ptr<KVSeqCollection> groups(GetArray()->GetStructureTypeList("GROUP"));
   Int_t N = groups->GetEntries();
   fGroupReconstructor = new TClonesArray(plugin_class, N);
   for (int i = 0; i < N; ++i) {
      KVGroupReconstructor* grec = (KVGroupReconstructor*)fGroupReconstructor->ConstructedAt(i);
      grec->SetReconEventClass(GetEvent()->IsA());
      grec->SetEventReconstructor(this);
   }
}

void KVEventReconstructor::ReconstructEvent(TSeqCollection* fired)
{
   // Reconstruct current event based on state of detectors in array
   //
   // The list pointer, if given, can be used to supply a list of fired
   // acquisition parameters to KVMultiDetArray::GetDetectorEvent

   KVDetectorEvent detev;
   GetArray()->GetDetectorEvent(&detev, fired);

   fGroupReconstructor->Clear();
   fNGrpRecon = 0;

   KVGroup* g;
   TIter it(detev.GetGroups());
   // set up group reconstructors for this event
   int j = 0;
   while ((g = (KVGroup*)it())) {
      ((KVGroupReconstructor*)(*fGroupReconstructor)[j++])->SetGroup(g);
   }
   // perform reconstruction in groups - in parallel if OpenMP is enabled
//#ifdef _OPENMP
//#pragma omp parallel for
//#endif
   for (int i = 0; i < fNGrpRecon; i++) {

      ((KVGroupReconstructor*)(*fGroupReconstructor)[i])->Reconstruct();
      ((KVGroupReconstructor*)(*fGroupReconstructor)[i])->Identify();

   }

   // merge resulting event fragments
   MergeGroupEventFragments();
}

void KVEventReconstructor::IdentifyEvent()
{
   // Identify current event based on state of detectors in array

   for (int i = 0; i < fNGrpRecon; i++) {

      ((KVGroupReconstructor*)(*fGroupReconstructor)[i])->Identify();

   }

}

void KVEventReconstructor::CalibrateEvent()
{
   // Calibrate current event based on state of detectors in array


   if (GetArray()->GetTarget()) {
      GetArray()->GetTarget()->SetIncoming(kFALSE);
      GetArray()->GetTarget()->SetOutgoing(kTRUE);
   }
   for (int i = 0; i < fNGrpRecon; i++) {

      ((KVGroupReconstructor*)(*fGroupReconstructor)[i])->Calibrate();

   }

}

void KVEventReconstructor::MergeGroupEventFragments()
{
   // After processing has finished in groups, call this method to produce
   // a final merged event containing particles from all groups

   TList to_merge;
   for (int i = 0; i < fNGrpRecon; i++) {

      to_merge.Add(((KVGroupReconstructor*)(*fGroupReconstructor)[i])->GetEventFragment());

   }
   GetEvent()->MergeEventFragments(&to_merge);
}

Double_t KVEventReconstructor::GetTargetEnergyLossCorrection(KVReconstructedNucleus* ion)
{
   // Calculate the energy loss in the current target of the multidetector
   // for the reconstructed charged particle 'ion', assuming that the current
   // energy and momentum of this particle correspond to its state on
   // leaving the target.
   //
   // WARNING: for this correction to work, the target must be in the right 'state':
   //
   //      gMultiDetArray->GetTarget()->SetIncoming(kFALSE);
   //      gMultiDetArray->GetTarget()->SetOutgoing(kTRUE);
   //
   // (see KVTarget::GetParticleEIncFromERes).
   //
   // The returned value is the energy lost in the target in MeV.
   // The energy/momentum of 'ion' are not affected.

   if (!GetArray()->GetTarget() || !ion) return 0.0;
   return (GetArray()->GetTarget()->GetParticleEIncFromERes(ion) - ion->GetEnergy());
}

