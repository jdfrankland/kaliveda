//Created by KVClassFactory on Mon Oct 19 09:33:44 2015
//Author: John Frankland,,,

#include "KVEventReconstructor.h"
#include "KVDetectorEvent.h"
#include "KVGroupReconstructor.h"
#include "KVTarget.h"

#include <TThread.h>

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

KVEventReconstructor::KVEventReconstructor(KVMultiDetArray* a, KVReconstructedEvent* e, Bool_t threaded)
   : KVBase("KVEventReconstructor", Form("Reconstruction of events in array %s", a->GetName())),
     fArray(a), fEvent(e), fGroupReconstructor(nullptr), fThreaded(threaded)
{
   // Default constructor
}

KVEventReconstructor::~KVEventReconstructor()
{
   // Destructor
   if (fGroupReconstructor) {
      fGroupReconstructor->Delete();
      delete fGroupReconstructor;
   }
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
   fGroupReconstructor = new TClonesArray(plugin_class, N / 5);
}

void KVEventReconstructor::ReconstructEvent(TSeqCollection* fired)
{
   // Reconstruct current event based on state of detectors in array
   //
   // The list pointer, if given, can be used to supply a list of fired
   // acquisition parameters to KVMultiDetArray::GetDetectorEvent

   KVDetectorEvent detev;
   GetArray()->GetDetectorEvent(&detev, fired);

   fNGrpRecon = 0;

   TIter it(detev.GetGroups());
   KVGroup* group;
   while ((group = (KVGroup*)it())) {
      KVGroupReconstructor* grec = (KVGroupReconstructor*)fGroupReconstructor->ConstructedAt(fNGrpRecon);
      if (!grec->GetEventFragment()) {
         grec->SetReconEventClass(GetEvent()->IsA());
         grec->SetEventReconstructor(this);
      }
      grec->SetGroup(group);
      if (fThreaded) {
         TThread* t = new TThread(Form("grp_rec_%d", fNGrpRecon), (TThread::VoidRtnFunc_t)&ThreadedReconstructor, (void*)grec);
         fThreads.Add(t);
         t->Run();
      } else
         grec->Reconstruct();
      ++fNGrpRecon;
   }
   if (fThreaded) {
      // wait for threads to finish
      TThread* th;
      TIter itr(&fThreads);
      while ((th = (TThread*)itr())) th->Join();
      // cleanup threads
      fThreads.Delete();
   }

   // merge resulting event fragments
   MergeGroupEventFragments();
}

void KVEventReconstructor::ThreadedReconstructor(void* arg)
{
   // Group reconstruction in separate threads

   KVGroupReconstructor* grec = (KVGroupReconstructor*)arg;
   grec->Reconstruct();
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
   GetEvent()->MergeEventFragments(&to_merge, "NGR");// "NGR" = no group reset
   for (int i = 0; i < fNGrpRecon; i++) {

      ((KVGroupReconstructor*)(*fGroupReconstructor)[i])->GetEventFragment()->Clear("NGR");// "NGR" = no group reset

   }
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

