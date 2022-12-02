#include "KVFilterGroupReconstructor.h"
#include "KVIDZAGrid.h"
#include "KVMultiDetArray.h"
#include "KVNucleusEvent.h"

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
   if (!Rtraj) {
      Info("get_recon_traj_for_particle", "noRtraj for this: %s %s", node->GetName(), traj->GetPathString().Data());
   }
   return Rtraj;
}

void KVFilterGroupReconstructor::identify_particle(KVIDTelescope* idt, KVIdentificationResult* IDR, KVReconstructedNucleus& nuc)
{
   // check associated simulated particle passes identification threshold
   //
   // if more than one simulated particle stopped in the same detector, we add the Z, A and E of each particle together
   // and merge the lists of parameters into one.

   int multihit;
   if ((multihit = part_correspond[&nuc].GetEntries()) > 1) {
      KVNucleus sum_part;
      TIter next(&part_correspond[&nuc]);
      KVNucleus* N;
      KVNameValueList q;
      // the parameter lists are added to q because the KVNucleus operator+ does
      // not retain any parameters from either of its arguments
      while ((N = (KVNucleus*)next())) {
         sum_part += *N;
         q.Merge(*N->GetParameters());
      }
      sum_part.GetParameters()->Merge(q);
      auto sim_part = (KVNucleus*)part_correspond[&nuc].First();
      // replace properties of first nucleus in list with summed properties
      part_correspond[&nuc].Clear();
      sum_part.Copy(*sim_part);
      part_correspond[&nuc].Add(sim_part);
      sim_part->SetParameter("PILEUP", Form("%d particles in %s", multihit, nuc.GetStoppingDetector()->GetName()));
      nuc.GetParameters()->Concatenate(*sim_part->GetParameters());
   }
   auto sim_part = (KVNucleus*)part_correspond[&nuc].First();
   IDR->SetIDType(idt->GetType());
   IDR->IDattempted = true;

   if (idt->CanIdentify(sim_part->GetZ(), sim_part->GetA())) {
      IDR->Z = sim_part->GetZ();
      IDR->A = sim_part->GetA();
      if (idt->CheckTheoreticalIdentificationThreshold((KVNucleus*)sim_part)) {
         IDR->IDOK = true;
         IDR->IDcode = idt->GetIDCode();
         IDR->IDquality = KVIDZAGrid::kICODE0;
         // set mass identification status depending on telescope, Z, A & energy of simulated particle
         idt->SetIdentificationStatus(IDR, sim_part);
      }
      else {
         IDR->IDOK = false;
         IDR->IDcode = GetGroup()->GetParentStructure<KVMultiDetArray>()->GetIDCodeForParticlesStoppingInFirstStageOfTelescopes();
         IDR->IDquality = KVIDZAGrid::kICODE8;
         IDR->SetComment("below threshold for identification");
      }
   }
   else {
      IDR->IDOK = false;
   }

   // special case: if particle ends up in a deadzone or punches through all detector layers, having a residual energy,
   // it would not be possible to identify it correctly.
   if (sim_part->GetParameters()->HasDoubleParameter("RESIDUAL ENERGY")) {
      IDR->IDOK = false;
      IDR->IDcode = GetGroup()->GetParentStructure<KVMultiDetArray>()->GetBadIDCode();
      IDR->IDquality = KVIDZAGrid::kICODE8;
      IDR->SetComment(Form("particle incompletely detected, DETECTED=%s", sim_part->GetParameters()->GetStringValue("DETECTED")));
   }
}

void KVFilterGroupReconstructor::PerformSecondaryAnalysis()
{
   // After first round of identification in group, try to identify remaining particles

   int num_ident_0, num_ident;
   int num_unident_0, num_unident;
   do {
      AnalyseParticles();
      num_ident = num_ident_0 = GetNIdentifiedInGroup();
      num_unident_0 = GetNUnidentifiedInGroup();
      Reconstruct();
      num_unident = GetNUnidentifiedInGroup();
      if (num_unident > num_unident_0) {
         Identify();
         num_ident = GetNIdentifiedInGroup();
         if (num_ident > num_ident_0) Calibrate();
      }
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

void KVFilterGroupReconstructor::IdentifyParticle(KVReconstructedNucleus& PART)
{
   KVGroupReconstructor::IdentifyParticle(PART);

   if (PART.IsIdentified()) {
      PART.GetReconstructionTrajectory()->IterateFrom();
      KVGeoDetectorNode* node;
      while ((node = PART.GetReconstructionTrajectory()->GetNextNode())) {
         --number_unidentified[node->GetName()];
      }
   }
}

void KVFilterGroupReconstructor::CalibrateParticle(KVReconstructedNucleus* PART)
{
   // Calculate and set the energy of a (previously identified) reconstructed particle,
   // including an estimate of the energy loss in the target.

   PART->SetIsCalibrated();
   PART->SetECode(GetGroup()->GetParentStructure<KVMultiDetArray>()->GetNormalCalibrationCode()); // => general code for calibration with no problems, no calculated energy losses
   double Einc = 0;
   PART->GetReconstructionTrajectory()->IterateFrom();
   while (auto node = PART->GetReconstructionTrajectory()->GetNextNode()) {
      double edet, dE;
      auto det = node->GetDetector();
      //Info("Calib", "det=%s", det->GetName());
      //Info("Calib", "number_uncalibrated=%d, energy_loss=%f", number_uncalibrated[det->GetName()], energy_loss[det->GetName()]);
      // deal with pileup in detectors
      if (number_uncalibrated[det->GetName()] > 1 || (det != PART->GetStoppingDetector() && number_unidentified[det->GetName()] > 0)) {
         //Info("Calib", "number_uncalib[%s]=%d, Einc=%f", det->GetName(), number_uncalibrated[det->GetName()], Einc);
         // there are other uncalibrated particles which hit this detector
         // the contribution for this particle must be calculated from the residual energy (Einc)
         // deposited in all detectors so far (if any)
         if (Einc > 0) {
            // calculate dE in active layer for particle
            dE = det->GetDeltaEFromERes(PART->GetZ(), PART->GetA(), Einc);
            if (dE == 0) {
               //Info("Calib", "dE from Eres gives 0! dead");
               PART->SetECode(GetGroup()->GetParentStructure<KVMultiDetArray>()->GetNoCalibrationCode());
               PART->SetIsUncalibrated();
               break;
            }
            det->SetEResAfterDetector(Einc);
            edet = det->GetCorrectedEnergy(PART, dE);
            //Info("Calib", "calculated edet=%f from dE=%f", edet, dE);
            // negative parameter for calculated contribution
            PART->SetParameter(Form("%s.E%s", GetGroup()->GetParentStructure<KVMultiDetArray>()->GetName(), det->GetLabel()), -edet);
            PART->SetECode(GetGroup()->GetParentStructure<KVMultiDetArray>()->GetCalculatedCalibrationCode()); // calculated
         }
         else {
            //Info("Calib", "Einc=0, dead");
            // nothing to do here
            PART->SetECode(GetGroup()->GetParentStructure<KVMultiDetArray>()->GetNoCalibrationCode());
            PART->SetIsUncalibrated();
            break;
         }
      }
      else if (det->GetNHits() > 1) {
         // other particles hit this detector, but their contributions have already been calculated
         // and subtracted. use remaining energy calculated in detector.
         //Info("Calib", "det->nhits>1");
         dE = energy_loss[det->GetName()];
         det->SetEResAfterDetector(Einc);
         edet = det->GetCorrectedEnergy(PART, dE);
         // negative parameter for calculated contribution
         PART->SetParameter(Form("%s.E%s", GetGroup()->GetParentStructure<KVMultiDetArray>()->GetName(), det->GetLabel()), -edet);
         PART->SetECode(GetGroup()->GetParentStructure<KVMultiDetArray>()->GetCalculatedCalibrationCode()); // calculated
      }
      else {
         dE = det->GetEnergyLoss();
         det->SetEResAfterDetector(Einc);
         edet = det->GetCorrectedEnergy(PART, -1., (Einc > 0));
         //Info("Calib", "Simple normal dE=%f edet=%f", dE, edet);
         PART->SetParameter(Form("%s.E%s", GetGroup()->GetParentStructure<KVMultiDetArray>()->GetName(), node->GetDetector()->GetLabel()), edet);
      }
      Einc += edet;
      --number_uncalibrated[det->GetName()];
      energy_loss[det->GetName()] -= dE;
      det->SetEnergyLoss(energy_loss[det->GetName()]);
      //Info("Calib", "After: number_uncalibrated=%d, energy_loss=%f", number_uncalibrated[det->GetName()], energy_loss[det->GetName()]);
      //Info("Calib", "After: number_unidentified=%d", number_unidentified[det->GetName()]);
   }
   if (PART->GetECode() > 0) {
      PART->SetEnergy(Einc);
      // set particle momentum from telescope dimensions (random)
      PART->GetAnglesFromReconstructionTrajectory();
      //Info("CalibrateParticle","Calibrated particle with %f MeV",Einc);
      //add correction for target energy loss - moving charged particles only!
      Double_t E_targ = 0.;
      if (PART->GetZ() && PART->GetEnergy() > 0) {
         E_targ = GetTargetEnergyLossCorrection(PART);
         //Info("CalibrateParticle","Target energy loss %f MeV",E_targ);
         PART->SetTargetEnergyLoss(E_targ);
      }
      Double_t E_tot = PART->GetEnergy() + E_targ;
      PART->SetEnergy(E_tot);
      //Info("CalibrateParticle","Total energy now %f MeV",E_tot);
   }
}

ClassImp(KVFilterGroupReconstructor)


