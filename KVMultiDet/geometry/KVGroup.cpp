#include "Riostream.h"
#include "KVGroup.h"
#include "KVNucleus.h"
#include "KVList.h"
#include "KVDetector.h"
#include "TROOT.h"
#include "KVNameValueList.h"

using namespace std;

ClassImp(KVGroup)


KVGroup::KVGroup() : fReconstructedNuclei(kFALSE)
{
   init();
}

//_________________________________________________________________________________

void KVGroup::init()
{
    // Default initialisation
    // KVGroup does not own the structures which it groups together

   fReconstructedNuclei.SetCleanup();
   SetType("GROUP");
   SetOwnsDaughters(kFALSE);
}

//_____________________________________________________________________________________

KVGroup::~KVGroup()
{
   fReconstructedNuclei.Clear();
}

//______________________________________________________________________________

void KVGroup::Reset()
{
   //Reset the group, i.e. wipe the list of reconstructed nuclei and call "Clear" method of
   //each and every detector in the group.

   fReconstructedNuclei.Clear();
   //reset energy loss and KVDetector::IsAnalysed() state
   //plus ACQParams set to zero
   ClearHitDetectors();
}

//_________________________________________________________________________________

void KVGroup::AddHit(KVNucleus * kvd)
{
   fReconstructedNuclei.Add(kvd);
}

//_________________________________________________________________________________

void KVGroup::RemoveHit(KVNucleus * kvd)
{
   //Remove reconstructed nucleus from group's list of reconstructed
   //particles.

   fReconstructedNuclei.Remove(kvd);
}

void KVGroup::ClearHitDetectors()
{
   // Loop over all detectors in group and clear their list of 'hits'
   // i.e. the lists of particles which hit each detector
   const_cast<KVSeqCollection*>(GetDetectors())->R__FOR_EACH(KVDetector, ClearHits)();
}


