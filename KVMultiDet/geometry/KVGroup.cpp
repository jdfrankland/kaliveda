#include "Riostream.h"
#include "KVGroup.h"
#include "KVDetector.h"

ClassImp(KVGroup)

KVGroup::KVGroup()
{
   init();
}

//_________________________________________________________________________________

void KVGroup::init()
{
   // Default initialisation
   // KVGroup does not own the structures which it groups together

   SetType("GROUP");
   SetOwnsDaughters(kFALSE);
   fReconTraj.SetOwner();
}

//______________________________________________________________________________

void KVGroup::Reset()
{
   //Reset the group: call "Clear" method of
   //each and every detector in the group.

   //reset energy loss and KVDetector::IsAnalysed() state
   //plus ACQParams set to zero
   ClearHitDetectors();
}

void KVGroup::ClearHitDetectors()
{
   // Loop over all detectors in group and clear their list of 'hits'
   // i.e. the lists of particles which hit each detector
   const_cast<KVSeqCollection*>(GetDetectors())->R__FOR_EACH(KVDetector, ClearHits)();
}

void KVGroup::CalculateReconstructionTrajectories()
{
   // Calculate all possible (sub-)trajectories
   // for particle reconstruction (GetReconTrajectories())

   fReconTraj.Clear();
   TIter next_traj(GetTrajectories());
   KVGeoDNTrajectory* traj;
   while ((traj = (KVGeoDNTrajectory*)next_traj())) {   // loop over all trajectories

      traj->IterateFrom();   // from furthest-out to closest-in detector

      KVGeoDetectorNode* N;
      while ((N = traj->GetNextNode())) {
         fReconTraj.Add(KVGeoDNTrajectory::Factory("KVReconNucTrajectory", traj, N));
      }
   }

   // There may be trajectories with different names but identical titles
   // (=physically same trajectories)
   // We find the duplicates, delete them, and set up a map between the names of the
   // duplicates and the name of the one remaining trajectory in the list
   TList toRemove;
   KVUniqueNameList unique_trajectories(kFALSE);//no replace
   unique_trajectories.SetOwner();
   fReconTrajMap.Clear();
   TIter nxtRT(GetReconTrajectories());
   KVGeoDNTrajectory* rnt;
   while ((rnt = (KVGeoDNTrajectory*)nxtRT())) {

      TNamed* n = new TNamed(rnt->GetTitle(), rnt->GetName());
      unique_trajectories.Add(n);
      TNamed* orig = n;
      if (!unique_trajectories.ObjectAdded()) {
         orig = (TNamed*)unique_trajectories.FindObject(rnt->GetTitle());
         toRemove.Add(rnt);
         delete n;
      }
      // set up mapping from duplicate trajectory name to orginal trajectory name
      fReconTrajMap.SetValue(rnt->GetName(), orig->GetTitle());

   }

   // now remove & delete the duplicates
   TIter nxtDel(&toRemove);
   while ((rnt = (KVGeoDNTrajectory*)nxtDel())) {
      fReconTraj.Remove(rnt);
      delete rnt;
   }
}
