//Created by KVClassFactory on Mon Oct 19 14:11:26 2015
//Author: John Frankland,,,

#include "KVGroupReconstructor.h"

#include <TPluginManager.h>
using std::cout;
using std::endl;

ClassImp(KVGroupReconstructor)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVGroupReconstructor</h2>
<h4>Base class for handling event reconstruction in detector groups</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVGroupReconstructor::KVGroupReconstructor()
   : KVBase("KVGroupReconstructor", "Reconstruction of particles in detector groups"),
     fGroup(nullptr), fGrpEvent(nullptr)
{
   // Default constructor

}

KVGroupReconstructor::~KVGroupReconstructor()
{
   // Destructor

   SafeDelete(fGrpEvent);
}

//________________________________________________________________

void KVGroupReconstructor::Copy(TObject& obj) const
{
   // This method copies the current state of 'this' object into 'obj'
   // You should add here any member variables, for example:
   //    (supposing a member variable KVGroupReconstructor::fToto)
   //    CastedObj.fToto = fToto;
   // or
   //    CastedObj.SetToto( GetToto() );

   KVBase::Copy(obj);
   //KVGroupReconstructor& CastedObj = (KVGroupReconstructor&)obj;
}

void KVGroupReconstructor::SetReconEventClass(TClass* c)
{
   // Instantiate event fragment object

   fGrpEvent = (KVReconstructedEvent*)c->New();
}

KVGroupReconstructor* KVGroupReconstructor::Factory(const char* plugin)
{
   // Create a new KVGroupReconstructor plugin instance

   TPluginHandler* ph = LoadPlugin("KVGroupReconstructor", plugin);
   if (ph) {
      return (KVGroupReconstructor*)ph->ExecPlugin(0);
   }
   return nullptr;
}

void KVGroupReconstructor::Reconstruct()
{
   // Reconstruct the particles in the group

   GetEventFragment()->Clear();

   TIter nxt_traj(GetGroup()->GetTrajectories());
   KVGeoDNTrajectory* traj;

   // loop over trajectories
   while ((traj = (KVGeoDNTrajectory*)nxt_traj())) {
      //cout << "on trajectory:" << traj->GetTitle() << endl;

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
         // if d has fired, has not already been analysed, and is not the last
         // node of several trajectories, reconstruct particle
         if ((d->Fired(GetEventFragment()->GetPartSeedCond()) && !d->IsAnalysed()
               && !(traj->EndsAt(node) && node->GetBackwardTrajectories()->GetEntries() > 1))) {
            //cout << d->GetName() << " fired: reconstructing particle" << endl;

            KVReconstructedNucleus* kvdp = GetEventFragment()->AddParticle();
            //add all active detector layers in front of this one
            //to the detected particle's list
            ReconstructParticle(kvdp, traj, node);

            //set detector state so it will not be used again
            d->SetAnalysed(kTRUE);
         }
      }

   }

   if (GetEventFragment()->GetMult()) AnalyseParticles();

}

void KVGroupReconstructor::ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   // Reconstruction of a detected nucleus from the successive energy losses
   // measured in a series of detectors/telescopes along a given trajectory

   const KVReconNucTrajectory* Rtraj = (const KVReconNucTrajectory*)GetGroup()->GetTrajectoryForReconstruction(traj, node);
   part->SetReconstructionTrajectory(Rtraj);

   Rtraj->IterateFrom();// iterate over trajectory
   KVGeoDetectorNode* n;
   while ((n = Rtraj->GetNextNode())) {

      KVDetector* d = n->GetDetector();
      //part->AddDetector(d);
      d->AddHit(part);  // add particle to list of particles hitting detector
      d->SetAnalysed(kTRUE);   //cannot be used to seed another particle

   }

   //part->ResetNSegDet();
   //cout << "part->ResetNSegDet()=" << part->GetNSegDet() << endl;
   //cout << "independent = " << Rtraj->GetNumberOfIndependentIdentifications() << endl;
}

void KVGroupReconstructor::AnalyseParticles()
{
   //cout << "GetNUnidentifiedInGroup()=" << GetNUnidentifiedInGroup() << endl;

   if (GetNUnidentifiedInGroup() > 1) { //if there is more than one unidentified particle in the group

      Int_t n_nseg_1 = 0;
      KVReconstructedNucleus* nuc;
      //loop over particles counting up different cases
      while ((nuc = GetEventFragment()->GetNextParticle())) {
         //ignore identified particles
         if (nuc->IsIdentified())
            continue;

         if (nuc->GetNSegDet() >= 2) {
            //all part.s crossing 2 or more independent detectors are fine
            nuc->SetStatus(KVReconstructedNucleus::kStatusOK);
         } else if ((nuc->GetNSegDet() <= 1) && (nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications())) {
            //1 or less independent detector hit => depends on what's in the rest of the group
            ++n_nseg_1;
         } else {
            //part.s crossing 0 independent detectors (i.E. arret ChIo)
            //can not be reconstructed
            nuc->SetStatus(KVReconstructedNucleus::kStatusStopFirstStage);
         }
      }
      //loop again, setting status
      while ((nuc = GetEventFragment()->GetNextParticle())) {
         if (nuc->IsIdentified())
            continue;           //ignore identified particles

         if ((nuc->GetNSegDet() <= 1) && (nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications())) {
            if (n_nseg_1 == 1) {
               //just the one ? then we can get it no problem
               //after identifying the others and subtracting their calculated
               //energy losses from the "dependent"/"non-segmented" detector
               nuc->SetStatus(KVReconstructedNucleus::kStatusOKafterSub);
            } else {
               //more than one ? then we can make some wild guess by sharing the
               //"non-segmented" contribution between them, but
               //I wouldn't trust it as far as I can spit
               nuc->SetStatus(KVReconstructedNucleus::kStatusOKafterShare);
            }
            //one possibility remains: the particle may actually have stopped e.g.
            //in the DE detector of a DE-E telescope, in which case AnalStatus = 3
            if (!nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications()) {
               //no ID telescopes with which to identify particle
               nuc->SetStatus(KVReconstructedNucleus::kStatusStopFirstStage);
            }
         }
      }
   } else if (GetNUnidentifiedInGroup() == 1) {
      //only one unidentified particle in group: just need an idtelescope which works

      //loop over particles looking for the unidentified one
      KVReconstructedNucleus* nuc;
      while ((nuc = GetEventFragment()->GetNextParticle())) {
         if (!nuc->IsIdentified()) break;
      }
      //cout << "nuc->GetNSegDet()=" << nuc->GetNSegDet() << endl;
      if (nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications()) {
         //OK no problem
         nuc->SetStatus(KVReconstructedNucleus::kStatusOK);
      } else {
         //dead in the water
         nuc->SetStatus(KVReconstructedNucleus::kStatusStopFirstStage);
      }
   }
}

void KVGroupReconstructor::IdentifyParticle(KVReconstructedNucleus* PART)
{
   // Try to identify this nucleus by calling the Identify() function of each
   // ID telescope crossed by it, starting with the telescope where the particle stopped, in order
   //      -  only attempt identification in ID telescopes containing the stopping detector.
   //      -  only telescopes which have been correctly initialised for the current run are used,
   //         i.e. those for which KVIDTelescope::IsReadyForID() returns kTRUE.
   // This continues until a successful identification is achieved or there are no more ID telescopes to try.
   // The identification code corresponding to the identifying telescope is set as the identification code of the particle.


   const KVSeqCollection* idt_list = PART->GetReconstructionTrajectory()->GetIDTelescopes();

   if (idt_list->GetEntries() > 0) {

      KVIDTelescope* idt;
      TIter next(idt_list);
      Int_t idnumber = 1;
      Int_t n_success_id = 0;//number of successful identifications
      while ((idt = (KVIDTelescope*) next())) {

         KVIdentificationResult* IDR = PART->GetIdentificationResult(idnumber++);

         if (idt->IsReadyForID()) { // is telescope able to identify for this run ?

            IDR->IDattempted = kTRUE;
            idt->Identify(IDR);

            if (IDR->IDOK) n_success_id++;
         } else
            IDR->IDattempted = kFALSE;

         if (n_success_id < 1 &&
               ((!IDR->IDattempted) || (IDR->IDattempted && !IDR->IDOK))) {
            // the particle is less identifiable than initially thought
            // we may have to wait for secondary identification
            Int_t nseg = PART->GetNSegDet();
            PART->SetNSegDet(TMath::Max(nseg - 1, 0));
            //if there are other unidentified particles in the group and NSegDet is < 2
            //then exact status depends on segmentation of the other particles : reanalyse
            if (PART->GetNSegDet() < 2 && GetNUnidentifiedInGroup() > 1) {
               AnalyseParticles();
               return;
            }
            //if NSegDet = 0 it's hopeless
            if (!PART->GetNSegDet()) {
               AnalyseParticles();
               return;
            }
         }

      }

   }

}

//_________________________________________________________________________________

void KVGroupReconstructor::CalibrateParticle(KVReconstructedNucleus* PART)
{
   //Calculate and set the energy of a (previously identified) reconstructed particle,
   //including an estimate of the energy loss in the target.
   //
   //Starting from the detector in which the particle stopped, we add up the
   //'corrected' energy losses in all of the detectors through which it passed.
   //Whenever possible, for detectors which are not calibrated or not working,
   //we calculate the energy loss. Measured & calculated energy losses are also
   //compared for each detector, and may lead to new particles being seeded for
   //subsequent identification. This is done by KVIDTelescope::CalculateParticleEnergy().
   //
   //For particles whose energy before hitting the first detector in their path has been
   //calculated after this step we then add the calculated energy loss in the target,
   //using gMultiDetArray->GetTargetEnergyLossCorrection().

   KVIDTelescope* idt = PART->GetIdentifyingTelescope();
   idt->CalculateParticleEnergy(PART);
   if (idt->GetCalibStatus() != KVIDTelescope::kCalibStatus_NoCalibrations) {
      PART->SetIsCalibrated();
      //add correction for target energy loss - moving charged particles only!
      Double_t E_targ = 0.;
      if (PART->GetZ() && PART->GetEnergy() > 0) {
         E_targ = GetEventReconstructor()->GetTargetEnergyLossCorrection(PART);
         PART->SetTargetEnergyLoss(E_targ);
      }
      Double_t E_tot = PART->GetEnergy() + E_targ;
      PART->SetEnergy(E_tot);
      // set particle momentum from telescope dimensions (random)
      PART->GetAnglesFromStoppingDetector();
   }
}

//_________________________________________________________________________________

void KVGroupReconstructor::Identify()
{
   //All particles which have not been previously identified (IsIdentified=kFALSE), and which
   //may be identified independently of all other particles in their group according to the 1st
   //order coherency analysis (KVReconstructedNucleus::GetStatus=0), will be identified.
   //Particles stopping in first member of a telescope (KVReconstructedNucleus::GetStatus=3) will
   //have their Z estimated from the energy loss in the detector (if calibrated).

   KVReconstructedNucleus* d;
   while ((d = GetEventFragment()->GetNextParticle())) {
      if (!d->IsIdentified()) {
         if (d->GetStatus() == KVReconstructedNucleus::kStatusOK) {
            // identifiable particles
            IdentifyParticle(d);
         } else if (d->GetStatus() == KVReconstructedNucleus::kStatusStopFirstStage) {
            // particles stopped in first member of a telescope
            // estimation of Z (minimum) from energy loss (if detector is calibrated)
            Int_t zmin = d->GetStoppingDetector()->FindZmin(-1., d->GetMassFormula());
            if (zmin) {
               d->SetZ(zmin);
               d->SetIsIdentified();
               // "Identifying" telescope is taken from list of ID telescopes
               // to which stopping detector belongs
               //d->SetIdentifyingTelescope( (KVIDTelescope*)d->GetStoppingDetector()->GetIDTelescopes()->At(0) );
            }
         }
      }
   }
}

//_____________________________________________________________________________

void KVGroupReconstructor::Calibrate()
{
   // Calculate and set energies of all identified particles in event.
   //
   // This will call the KVReconstructedNucleus::Calibrate() method of each
   // uncalibrated particle (those for which KVReconstructedNucleus::IsCalibrated()
   // returns kFALSE).

   KVReconstructedNucleus* d;

   while ((d = GetEventFragment()->GetNextParticle())) {

      if (d->IsIdentified() && !d->IsCalibrated()) {
         CalibrateParticle(d);
      }

   }

}

