#include "KVFilterGroupReconstructor.h"

KVReconNucTrajectory* KVFilterGroupReconstructor::get_recon_traj_for_particle(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   // We look for a simulated particle stopped in the same node with the same trajectory.
   // We use the actual trajectory of the simulated particle to get the right reconstruction trajectory

   KVReconNucTrajectory* Rtraj{nullptr};

   for (auto& n : EventIterator(fSimEvent.get())) {
      TString stop = n.GetParameters()->GetTStringValue("STOPPING DETECTOR");
      if (stop == node->GetName()) {
         Rtraj = (KVReconNucTrajectory*)GetGroup()->GetTrajectoryForReconstruction(traj, node);
         if (Rtraj && n.GetParameters()->GetTStringValue("TRAJECTORY") == Rtraj->GetPathString()) {
            // correspondence recon <-> simu
            part_correspond[current_nuc_recon].Add(&n);
            // copy simulated particle parameters to reconstructed particle
            n.GetParameters()->Copy(*current_nuc_recon->GetParameters());
         }
      }
   }
   return Rtraj;
}

void KVFilterGroupReconstructor::identify_particle(KVIDTelescope* idt, KVIdentificationResult* IDR, KVReconstructedNucleus& nuc)
{
   // check associated simulated particle passes identification threshold

   if (part_correspond[&nuc].GetEntries() > 1) {
      Warning("identify_particle", "This reconstructed particle is associated to >1 simulated particles:");
      TIter next(&part_correspond[&nuc]);
      KVNucleus* N;
      while ((N = (KVNucleus*)next())) N->Print();

      IDR->IDOK = false;
   }
   else {
      auto sim_part = (KVNucleus*)part_correspond[&nuc].First();
      if (idt->IsReadyForID()
            && idt->CanIdentify(sim_part->GetZ(), sim_part->GetA())
            && idt->CheckTheoreticalIdentificationThreshold((KVNucleus*)sim_part)) {
         IDR->IDOK = true;
         IDR->Z = sim_part->GetZ();
         IDR->A = sim_part->GetA();
         IDR->IDcode = idt->GetIDCode();
         IDR->IDquality = 0;
         // set mass identification status depending on telescope, Z, A & energy of simulated particle
         idt->SetIdentificationStatus(IDR, sim_part);
      }
      else {
         IDR->IDOK = false;
      }
   }
}

void KVFilterGroupReconstructor::PerformSecondaryAnalysis()
{
   // After first round of identification in group, try to identify remaining particles

   int num_ident_0, num_ident;
   do {
      AnalyseParticles();
      num_ident_0 = GetNIdentifiedInGroup();
      Identify();
      num_ident = GetNIdentifiedInGroup();
      if (num_ident > num_ident_0) Calibrate();
   }
   while (num_ident > num_ident_0); // continue until no more new identifications occur
}

void KVFilterGroupReconstructor::ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   current_nuc_recon = part;

   KVGroupReconstructor::ReconstructParticle(part, traj, node);

   part->GetReconstructionTrajectory()->IterateFrom();
   while (auto Nd = part->GetReconstructionTrajectory()->GetNextNode()) {
      // energy_loss will contain the sum of energy losses (dE) measured in the *active* layer of each detector
      // the contribution to the energy of each particle may be greater than this after correcting for
      // dead layers etc.
      energy_loss[Nd->GetName()] = Nd->GetDetector()->GetEnergyLoss();
      ++number_uncalibrated[Nd->GetName()];
   }
}

void KVFilterGroupReconstructor::CalibrateParticle(KVReconstructedNucleus* PART)
{
   // Calculate and set the energy of a (previously identified) reconstructed particle,
   // including an estimate of the energy loss in the target.

   PART->SetIsCalibrated();
   PART->SetECode(1); // => general code for calibration with no problems, no calculated energy losses
   double Einc = 0;
   PART->GetReconstructionTrajectory()->IterateFrom();
   while (auto node = PART->GetReconstructionTrajectory()->GetNextNode()) {
      double edet, dE;
      auto det = node->GetDetector();
      Info("Calib", "det=%s", det->GetName());
      Info("Calib", "number_uncalibrated=%d, energy_loss=%f", number_uncalibrated[det->GetName()], energy_loss[det->GetName()]);
      // deal with pileup in detectors
      if (number_uncalibrated[det->GetName()] > 1) {
         Info("Calib", "number_uncalib[%s]=%d, Einc=%f", det->GetName(), number_uncalibrated[det->GetName()], Einc);
         // there are other uncalibrated particles which hit this detector
         // the contribution for this particle must be calculated from the residual energy (Einc)
         // deposited in all detectors so far (if any)
         if (Einc > 0) {
            // calculate dE in active layer for particle
            dE = det->GetDeltaEFromERes(PART->GetZ(), PART->GetA(), Einc);
            if (dE == 0) {
               Info("Calib", "dE from Eres gives 0! dead");
               PART->SetECode(0);
               PART->SetIsCalibrated();
               break;
            }
            det->SetEResAfterDetector(Einc);
            edet = det->GetCorrectedEnergy(PART, dE);
            Info("Calib", "calculated edet=%f from dE=%f", edet, dE);
            // negative parameter for calculated contribution
            PART->SetParameter(Form("%s.E%s", GetGroup()->GetArray()->GetName(), det->GetLabel()), -edet);
            PART->SetECode(2); // calculated
         }
         else {
            Info("Calib", "Einc=0, dead");
            // nothing to do here
            PART->SetECode(0);
            PART->SetIsCalibrated();
            break;
         }
      }
      else if (det->GetNHits() > 1) {
         // other particles hit this detector, but their contributions have already been calculated
         // and subtracted. use remaining energy calculated in detector.
         Info("Calib", "det->nhits>1");
         dE = energy_loss[det->GetName()];
         det->SetEResAfterDetector(Einc);
         edet = det->GetCorrectedEnergy(PART, dE);
         // negative parameter for calculated contribution
         PART->SetParameter(Form("%s.E%s", GetGroup()->GetArray()->GetName(), det->GetLabel()), -edet);
         PART->SetECode(2); // calculated
      }
      else {
         dE = det->GetEnergyLoss();
         det->SetEResAfterDetector(Einc);
         edet = det->GetCorrectedEnergy(PART, -1., (Einc > 0));
         Info("Calib", "Simple normal dE=%f edet=%f", dE, edet);
         PART->SetParameter(Form("%s.E%s", GetGroup()->GetArray()->GetName(), node->GetDetector()->GetLabel()), edet);
      }
      Einc += edet;
      --number_uncalibrated[det->GetName()];
      energy_loss[det->GetName()] -= dE;
      Info("Calib", "After: number_uncalibrated=%d, energy_loss=%f", number_uncalibrated[det->GetName()], energy_loss[det->GetName()]);
   }
   if (PART->GetECode() > 0) {
      PART->SetEnergy(Einc);
      //add correction for target energy loss - moving charged particles only!
      Double_t E_targ = 0.;
      if (PART->GetZ() && PART->GetEnergy() > 0) {
         E_targ = GetTargetEnergyLossCorrection(PART);
         PART->SetTargetEnergyLoss(E_targ);
      }
      Double_t E_tot = PART->GetEnergy() + E_targ;
      PART->SetEnergy(E_tot);
      // set particle momentum from telescope dimensions (random)
      PART->GetAnglesFromReconstructionTrajectory();
   }
}

ClassImp(KVFilterGroupReconstructor)


