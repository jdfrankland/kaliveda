//Created by KVClassFactory on Mon Oct 19 09:33:44 2015
//Author: John Frankland,,,

#include "KVEventReconstructor.h"

ClassImp(KVEventReconstructor)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVEventReconstructor</h2>
<h4>Base class for handling event reconstruction</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVEventReconstructor::KVEventReconstructor()
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

void KVEventReconstructor::ReconstructEvent(KVReconstructedEvent* recev, KVDetectorEvent* kvde)
{
   // Use the KVDetectorEvent (list of hit groups) in order to fill the
   // KVReconstructedEvent with reconstructed nuclei
   //
   // See KVReconstructedEvent::AnalyseTrajectory for details of particle reconstruction

   KVGroup* grp_tch;
   TIter nxt_grp(kvde->GetGroups());
   // loop over hit groups
   while ((grp_tch = (KVGroup*) nxt_grp())) {

      TIter nxt_traj(grp_tch->GetTrajectories());
      KVGeoDNTrajectory* traj;
      // loop over trajectories
      while ((traj = (KVGeoDNTrajectory*)nxt_traj())) {

         // Work our way along the trajectory, starting from furthest detector from target,
         // start reconstruction of new detected particle from first fired detector.
         //
         // More precisely: If detector has fired*,
         // making sure fired detector hasn't already been used to reconstruct
         // a particle, then we create and fill a new detected particle.
         //
         // *change condition by calling SetPartSeedCond("any"): in this case,
         // particles will be reconstructed starting from detectors with at least 1 fired parameter.

         traj->IterateFrom();
         KVGeoDetectorNode* node;
         while ((node = traj->GetNextNode())) {

            KVDetector* d = node->GetDetector();
            /*
                  If detector has fired,
                  making sure fired detector hasn't already been used to reconstruct
                  a particle, then we create and fill a new detected particle.
              */
            if ((d->Fired(recev->GetPartSeedCond()) && !d->IsAnalysed())) {

               KVReconstructedNucleus* kvdp = recev->AddParticle();
               //add all active detector layers in front of this one
               //to the detected particle's list
               ReconstructParticle(kvdp, traj, node);

               //set detector state so it will not be used again
               d->SetAnalysed(kTRUE);
            }
         }

      }

      KVReconstructedNucleus::AnalyseParticlesInGroup(grp_tch);
   }
}

void KVEventReconstructor::ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   // Reconstruction of a detected nucleus from the successive energy losses
   // measured in a series of detectors/telescopes along a given trajectory

   const KVReconNucTrajectory* Rtraj = gMultiDetArray->GetTrajectoryForReconstruction(traj, node);
   part->SetReconstructionTrajectory(Rtraj);
   node->GetDetector()->GetGroup()->AddHit(part);

   Rtraj->IterateFrom();// iterate over trajectory
   KVGeoDetectorNode* n;
   while ((n = Rtraj->GetNextNode())) {

      KVDetector* d = n->GetDetector();
      part->AddDetector(d);
      d->AddHit(part);  // add particle to list of particles hitting detector
      d->SetAnalysed(kTRUE);   //cannot be used to seed another particle

   }

   part->ResetNSegDet();
}

//_________________________________________________________________________________
