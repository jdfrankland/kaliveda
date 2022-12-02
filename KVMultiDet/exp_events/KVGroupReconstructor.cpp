#include "KVGroupReconstructor.h"
#include "KVMultiDetArray.h"
#include "KVTarget.h"

#include <TPluginManager.h>
using std::cout;
using std::endl;

ClassImp(KVGroupReconstructor)

bool KVGroupReconstructor::fDoIdentification = kTRUE;
bool KVGroupReconstructor::fDoCalibration = kTRUE;

KVGroupReconstructor::KVGroupReconstructor(const KVGroup* g)
   : KVBase("KVGroupReconstructor", "Reconstruction of particles in detector groups"),
     fGrpEvent(nullptr)
{
   // Default constructor
   SetGroup(g);
}

KVGroupReconstructor::~KVGroupReconstructor()
{
   // Destructor

   SafeDelete(fGrpEvent);
}

void KVGroupReconstructor::SetGroup(const KVGroup* g)
{
   // Set the group to be reconstructed
   //
   // Set condition for seeding reconstructed particles
   fGroup = const_cast<KVGroup*>(g);
   fPartSeedCond = fGroup->GetParentStructure<KVMultiDetArray>()->GetPartSeedCond();
}

void KVGroupReconstructor::SetReconEventClass(TClass* c)
{
   // Instantiate event fragment object

   if (!fGrpEvent) fGrpEvent = (KVReconstructedEvent*)c->New();
}

KVGroupReconstructor* KVGroupReconstructor::Factory(const TString& plugin, const KVGroup* g)
{
   // Create a new object of a class derived from KVGroupReconstructor defined by a plugin
   //
   // If plugin="" this is just a new KVGroupReconstructor instance

   if (plugin == "") return new KVGroupReconstructor(g);
   TPluginHandler* ph = LoadPlugin("KVGroupReconstructor", plugin);
   if (ph) {
      return (KVGroupReconstructor*)ph->ExecPlugin(1, g);
   }
   return nullptr;
}

void KVGroupReconstructor::Process()
{
   // Perform full reconstruction of particles detected in group.
   //
   // This will call in order Reconstruct(), Identify(), Calibrate() and AddCoherencyParticles()
   //
   //   - identification can be inhibited (for all groups) by calling KVGroupReconstructor::SetDoIdentification(false);
   //   - calibration can be inhibited (for all groups) by calling KVGroupReconstructor::SetDoCalibration(false);

   nfireddets = 0;
   coherency_particles.clear();
   Reconstruct();
   if (GetEventFragment()->GetMult() == 0) {
      return;
   }
   if (fDoIdentification) Identify();
   if (fDoCalibration) {
      Calibrate();
      if (!coherency_particles.empty()) AddCoherencyParticles();
   }
}

void KVGroupReconstructor::Reconstruct()
{
   // Reconstruct the particles in the group from hit trajectories
   //
   // We work our way along each trajectory, starting from the furthest detector from the target,
   // and start reconstruction of a new detected particle depending on decision made in ReconstructTrajectory()
   // for each detector we try (normally just the first fired detector we find).
   //
   // The reconstruction is then handled by ReconstructParticle().

   TIter nxt_traj(GetGroup()->GetTrajectories());
   KVGeoDNTrajectory* traj;

   auto old_mult = GetEventFragment()->GetMult();

   // loop over trajectories
   while ((traj = (KVGeoDNTrajectory*)nxt_traj())) {

      // Work our way along the trajectory, starting from furthest detector from target,
      // start reconstruction of new detected particle from first fired detector.
      traj->IterateFrom();
      KVGeoDetectorNode* node;
      while ((node = traj->GetNextNode())) {

         KVReconstructedNucleus* kvdp;
         if ((kvdp = ReconstructTrajectory(traj, node))) {
            ReconstructParticle(kvdp, traj, node);
            break; //start next trajectory
         }

      }

   }

   if (GetEventFragment()->GetMult() > old_mult) {
      AnalyseParticles();
      PostReconstructionProcessing();
   }
}

void KVGroupReconstructor::PostReconstructionProcessing()
{
   // This method will be called after reconstruction and first-order coherency analysis
   // of all particles in the group (if there are any reconstructed particles).
   // By default it does nothing.
}

KVReconstructedNucleus* KVGroupReconstructor::ReconstructTrajectory(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   // \param traj trajectory currently being scanned
   // \param node current detector on trajectory to test
   // \returns pointer to a new reconstructed particle added to this group's event; nullptr if nothing is to be done
   //
   // The condition which must be fulfilled for us to begin the reconstruction of a new particle starting from
   // a fired detector on the trajectory is either:
   //   - the trajectory is the only one which passes through this detector; or
   //   - the detector directly in front of this one on this trajectory also fired.

   KVDetector* d = node->GetDetector();
   nfireddets += d->Fired();
   // if d has fired, has not been used to seed a reconstructed particle (IsAnalysed),
   // is not on the trajectory of a reconstructed particle (GetHits()->GetEntries()),
   // and only if the detector belongs to a unique trajectory (i.e. neither multiple trajectories
   // arrive from behind nor continue in front)
   if (!d->IsAnalysed() && d->Fired(fPartSeedCond)
         && (!d->GetHits() || !d->GetHits()->GetEntries())
         && (node->GetNTrajForwards() <= 1 && node->GetNTrajBackwards() <= 1)) {
      return GetEventFragment()->AddParticle();
   }

   return nullptr;
}

KVReconNucTrajectory* KVGroupReconstructor::get_recon_traj_for_particle(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   return (KVReconNucTrajectory*)GetGroup()->GetTrajectoryForReconstruction(traj, node);
}

void KVGroupReconstructor::ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   // Reconstruction of a detected nucleus from the successive energy losses
   // measured in a series of detectors/telescopes along a given trajectory.
   //
   // The particle is associated with a reconstruction trajectory which starts from the detector in
   // which the particle stopped and leads up through the detection layers towards the target.
   // These can be accessed in later analysis using the two methods
   //    - KVReconstructedNucleus::GetStoppingDetector()
   //    - KVReconstructedNucleus::GetReconstructionTrajectory()
   //
   // \sa KVReconNucTrajectory

   auto Rtraj = get_recon_traj_for_particle(traj, node);
   if (!Rtraj)
      throw std::runtime_error(Form("<KVGroupReconstructor::ReconstructParticle>: Failed to obtain reconstruction trajectory for node %s on trajectory %s",
                                    node->GetName(), traj->GetPathString().Data()));
   part->SetReconstructionTrajectory(Rtraj);
   part->SetParameter("ARRAY", GetGroup()->GetParentStructure<KVMultiDetArray>()->GetName());
   // only the stopping detector is set 'analysed' (to stop any new particles
   // being reconstructed starting from it) in case other particles stopped
   // in other detectors further up the reconstruction trajectory
   part->GetStoppingDetector()->SetAnalysed();
   Rtraj->IterateFrom();// iterate over trajectory
   KVGeoDetectorNode* n;
   while ((n = Rtraj->GetNextNode())) {

      KVDetector* d = n->GetDetector();
      d->AddHit(part);  // add particle to list of particles hitting detector
   }

}

void KVGroupReconstructor::AnalyseParticles()
{
   // Analyse and set status of reconstructed particles in group, to decide whether they can be identified
   // and further treated straight away.
   //
   // If there is more than one reconstructed particle in the group on different trajectories which
   // have a detector in common, then it may be necessary to identify one of the particles before the
   // others in order to try to subtract its contribution from the common detector and allow the
   // identification of the other particles.

   if (GetNUnidentifiedInGroup() > 1) { //if there is more than one unidentified particle in the group

      Int_t n_nseg_1 = 0;
      //loop over particles counting up different cases
      for (KVReconstructedEvent::Iterator it = GetEventFragment()->begin(); it != GetEventFragment()->end(); ++it) {
         KVReconstructedNucleus* nuc = it.get_pointer();
         //ignore identified particles
         if (nuc->IsIdentified())
            continue;
         // The condition for a particle to be identifiable straight away is that the first
         // identification method that will be used must be independent
         if (nuc->GetReconstructionTrajectory()->GetIDTelescopes()->First()
               && ((KVIDTelescope*)nuc->GetReconstructionTrajectory()->GetIDTelescopes()->First())->IsIndependent()) {
            //all particles whose first identification telescope is independent are fine
            nuc->SetStatus(KVReconstructedNucleus::kStatusOK);
         }
         else if (nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications()) {
            //no independent identification telescope => depends on what's in the rest of the group
            ++n_nseg_1;
         }
         else {
            //no identification available
            nuc->SetStatus(KVReconstructedNucleus::kStatusStopFirstStage);
         }
      }
      //loop again, setting status
      for (KVReconstructedEvent::Iterator it = GetEventFragment()->begin(); it != GetEventFragment()->end(); ++it) {
         KVReconstructedNucleus* nuc = it.get_pointer();
         if (nuc->IsIdentified())
            continue;           //ignore identified particles

         if (!(nuc->GetReconstructionTrajectory()->GetIDTelescopes()->First()
               && ((KVIDTelescope*)nuc->GetReconstructionTrajectory()->GetIDTelescopes()->First())->IsIndependent())
               && nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications()) {
            //particles with no independent identification possibility
            if (n_nseg_1 == 1) {
               //just the one ? then we can get it no problem
               //after identifying the others and subtracting their calculated
               //energy losses from the other detectors
               nuc->SetStatus(KVReconstructedNucleus::kStatusOKafterSub);
            }
            else {
               //more than one ? then we can make some wild guess by sharing the
               //contribution between them, but I wouldn't trust it as far as I can spit
               nuc->SetStatus(KVReconstructedNucleus::kStatusOKafterShare);
            }
            //one possibility remains: the particle may actually have stopped e.g.
            //in the DE detector of a DE-E telescope
            if (!nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications()) {
               //no ID telescopes with which to identify particle
               nuc->SetStatus(KVReconstructedNucleus::kStatusStopFirstStage);
            }
         }
      }
   }
   else if (GetNUnidentifiedInGroup() == 1) {
      //only one unidentified particle in group: just need an idtelescope which works

      //loop over particles looking for the unidentified one
      KVReconstructedNucleus* nuc(nullptr);
      for (KVReconstructedEvent::Iterator it = GetEventFragment()->begin(); it != GetEventFragment()->end(); ++it) {
         nuc = it.get_pointer();
         if (!nuc->IsIdentified()) break;
      }
      //cout << "nuc->GetNSegDet()=" << nuc->GetNSegDet() << endl;
      if (nuc->GetReconstructionTrajectory()->GetNumberOfIdentifications()) {
         //OK no problem
         nuc->SetStatus(KVReconstructedNucleus::kStatusOK);
      }
      else {
         //dead in the water
         nuc->SetStatus(KVReconstructedNucleus::kStatusStopFirstStage);
      }
   }
}

void KVGroupReconstructor::IdentifyParticle(KVReconstructedNucleus& PART)
{
   // Try to identify this nucleus by calling the KVIDTelescope::Identify() method of each
   // identification telescope on its reconstruction trajectory, starting with the telescope
   // where the particle stopped (i.e. containing its stopping detector), in order.
   //
   // Only identifications which have been correctly initialised for the current run are used,
   // i.e. those for which KVIDTelescope::IsReadyForID() returns kTRUE.
   //
   // Note that *all* identifications along the trajectory are tried, even those far in front
   // of the detector where the particle stopped. The results of all identifications (KVIdentificationResult)
   // are stored with the particle (they can be retrieved later using KVReconstructedNucleus::GetIdentificationResult() ).
   // They are later used for consistency checks ("coherency") between the identifications in the different
   // stages.
   //
   // The first successful identification obtained in this way, if one exists, for a KVIDTelescope which includes
   // the detector where the particle stopped is then attributed to the particle (see KVReconstructedNucleus::SetIdentification()).

   const KVSeqCollection* idt_list = PART.GetReconstructionTrajectory()->GetIDTelescopes();
   identifying_telescope = nullptr;
   id_by_type.clear();

   if (idt_list->GetEntries() > 0) {

      KVIDTelescope* idt;
      TIter next(idt_list);
      Int_t idnumber = 1;
      Int_t n_success_id = 0;//number of successful identifications
      while ((idt = (KVIDTelescope*) next())) {
         KVIdentificationResult* IDR = PART.GetIdentificationResult(idnumber++);

         if (idt->IsReadyForID()) { // is telescope able to identify for this run ?
            id_by_type[idt->GetType()] = IDR;// map contains only attempted identifications
            if (!IDR->IDattempted) { // the identification may already have been attempted, e.g. in gamma particle
               // rejection in CSI: see KVINDRAGroupReconstructor::ReconstructTrajectory
               identify_particle(idt, IDR, PART);
            }
            if (IDR->IDOK) n_success_id++;
         }
         else
            IDR->IDattempted = kFALSE;

         if (n_success_id < 1 &&
               ((!IDR->IDattempted) || (IDR->IDattempted && !IDR->IDOK))) {
            // the particle is less identifiable than initially thought
            // we may have to wait for secondary identification
            Int_t nseg = PART.GetNSegDet();
            if (nseg == 0) {
               // the last independent identification method just failed
               AnalyseParticles();
               return;
            }
            PART.SetNSegDet(TMath::Max(nseg - 1, 0));
            //if there are other unidentified particles in the group and NSegDet is < 1
            //then exact status depends on segmentation of the other particles : reanalyse
            if (PART.GetNSegDet() < 1 && GetNUnidentifiedInGroup() > 1) {
               AnalyseParticles();
               if (PART.GetStatus() != KVReconstructedNucleus::kStatusOK) return;
            }
         }

      }
      // set first successful identification as particle identification
      // as long as the id telescope concerned contains the stopping detector!
      Int_t id_no = 1;
      Bool_t ok = kFALSE;
      KVIdentificationResult* pid = PART.GetIdentificationResult(id_no);
      next.Reset();
      idt = (KVIDTelescope*)next();
      while (idt->HasDetector(PART.GetStoppingDetector())) {
         if (pid->IDattempted && pid->IDOK) {
            ok = kTRUE;
            partID = *pid;
            identifying_telescope = idt;
            break;
         }
         ++id_no;
         pid = PART.GetIdentificationResult(id_no);
         idt = (KVIDTelescope*)next();
      }
      if (ok) {
         PART.SetIsIdentified();
         PART.SetIdentification(&partID, identifying_telescope);
      }

   }

}

//_________________________________________________________________________________


//_________________________________________________________________________________

void KVGroupReconstructor::TreatStatusStopFirstStage(KVReconstructedNucleus& d)
{
   // particles stopped in first member of a telescope
   // estimation of Z (minimum) from energy loss (if detector is calibrated)
   Int_t zmin = d.GetStoppingDetector()->FindZmin(-1., d.GetMassFormula());
   if (zmin) {
      KVIdentificationResult idr;
      idr.Z = zmin;
      idr.IDcode = GetGroup()->GetParentStructure<KVMultiDetArray>()->GetIDCodeForParticlesStoppingInFirstStageOfTelescopes();
      idr.Zident = false;
      idr.Aident = false;
      idr.PID = zmin;
      d.SetIsIdentified();
      KVGeoDNTrajectory* t = (KVGeoDNTrajectory*)d.GetStoppingDetector()->GetNode()->GetTrajectories()->First();
      d.SetIdentification(&idr, (KVIDTelescope*) t->GetIDTelescopes()->Last());
   }
}

void KVGroupReconstructor::Identify()
{
   // Identify all particles reconstructed so far in this group which
   // may be identified independently of all other particles in the group according to the 1st
   // order coherency analysis (see AnalyseParticles() ). This is done by calling the method
   // IdentifyParticle() for each particle in turn.
   //
   // Particles stopping in the first member of a telescope will
   // have their Z estimated from the energy loss in the detector (if calibrated):
   // in this case the Z is a minimum value.

   for (auto& d : ReconEventIterator(GetEventFragment())) {
      if (!d.IsIdentified()) {
         if (d.GetStatus() == KVReconstructedNucleus::kStatusOK) {
            // identifiable particles
            IdentifyParticle(d);
         }
         else if (d.GetStatus() == KVReconstructedNucleus::kStatusStopFirstStage) {
            // particles stopped in first member of a telescope
            // estimation of Z (minimum) from energy loss (if detector is calibrated)
            TreatStatusStopFirstStage(d);
         }
      }
   }
}

//_____________________________________________________________________________

void KVGroupReconstructor::Calibrate()
{
   // Calculate and set energies of all identified but uncalibrated particles in event.

   for (auto& d : ReconEventIterator(GetEventFragment())) {

      if (d.IsIdentified() && !d.IsCalibrated()) {
         CalibrateParticle(&d);
      }

   }

}

Double_t KVGroupReconstructor::GetTargetEnergyLossCorrection(KVReconstructedNucleus* ion)
{
   // Calculate the energy loss in the current target of the multidetector
   // for the reconstructed charged particle 'ion', assuming that the current
   // energy and momentum of this particle correspond to its state on
   // leaving the target.
   //
   // WARNING: for this correction to work, the target must be in the right 'state':
   //
   //      gMultiDetArray->GetTarget()->SetOutgoing(kTRUE);
   //
   // (see KVTarget::GetParticleEIncFromERes).
   //
   // The returned value is the energy lost in the target in MeV.
   // The energy/momentum of 'ion' are not affected.

   auto array = GetGroup()->GetParentStructure<KVMultiDetArray>();
   if (!array->GetTarget() || !ion) return 0.0;
   return (array->GetTarget()->GetParticleEIncFromERes(ion) - ion->GetEnergy());
}

