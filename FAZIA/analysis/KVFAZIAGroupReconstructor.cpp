#include "KVFAZIAGroupReconstructor.h"

#include <KVFAZIA.h>
#include <KVFAZIADetector.h>
#include <KVSignal.h>
#include <KVLightEnergyCsIFull.h>
#include <KVLightEnergyCsI.h>
#include <KVCalibrator.h>
#include <KVIDGCsI.h>
#include <KVCalibratedSignal.h>

ClassImp(KVFAZIAGroupReconstructor)

void KVFAZIAGroupReconstructor::CalibrateParticle(KVReconstructedNucleus* PART)
{
   // Perform energy calibration of (previously identified) charged particle

   KVFAZIADetector* det = (KVFAZIADetector*)PART->GetStoppingDetector();

   PART->SetIsUncalibrated();
   PART->SetECode(KVFAZIA::ECodes::NO_CALIBRATION_ATTEMPTED);

   if (det == csi && PART->GetIDCode() == KVFAZIA::IDCodes::ID_GAMMA) { // gammas
      // "Gamma" particles identified in CsI are given the equivalent energy of proton,
      // if the CsI detector is calibrated
      if (det->IsCalibrated()) {
         double Ep = det->GetDetectorSignalValue("Energy", "Z=1,A=1");
         if (Ep > 0) {
            PART->SetEnergy(Ep);
            SetCalibrationStatus(*PART, KVFAZIA::ECodes::NORMAL_CALIBRATION);
            PART->SetParameter("FAZIA.ECSI", Ep);
         }
      }
   }

   if (PART->GetIDCode() == KVFAZIA::IDCodes::ID_STOPPED_IN_FIRST_STAGE) {  // stopped in SI1, no PSA identification
      if (det->IsCalibrated()) {
         double Ep = det->GetEnergy();
         PART->SetEnergy(Ep);
         SetCalibrationStatus(*PART, KVFAZIA::ECodes::NORMAL_CALIBRATION);
         PART->SetParameter("FAZIA.ESI1", Ep);
      }
   }

   // particle identified in Si1 PSA, detector is calibrated
   if (PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI1_PSA) {
      if (si1->IsCalibrated()) {
         double e1 = si1->GetCorrectedEnergy(PART, -1., kFALSE);
         if (e1 <= 0) {
            Warning("CalibrateParticle",
                    "IDCODE=11 Z=%d A=%d calibrated SI1 E=%f",
                    PART->GetZ(), PART->GetA(), e1);
            return;
         }
         PART->SetParameter("FAZIA.ESI1", e1);
         PART->SetEnergy(e1);
         SetCalibrationStatus(*PART, KVFAZIA::ECodes::NORMAL_CALIBRATION);
      }
   }

   // particle identified in Si1-Si2
   if (PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI1_SI2
         || PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI1_SI2_MAYBE_PUNCH_THROUGH
         || PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI1_SI2_PUNCH_THROUGH) {
      if (si1->IsCalibrated() && si2->IsCalibrated()) { //  with both detectors calibrated
         double e1(0), e2(0);
         bool calc_e1(false), calc_e2(false);
         if ((e2 = si2->GetEnergy()) > 0) {
            if (!((e1 = si1->GetEnergy()) > 0)) { // if SI1 not fired, we calculate from SI2 energy
               e1 = si1->GetDeltaEFromERes(PART->GetZ(), PART->GetA(), e2);
               calc_e1 = true;
            }
         }
         else if ((e1 = si1->GetEnergy()) > 0) {
            // if SI2 not fired, we calculate from SI1 energy
            e2 = si1->GetEResFromDeltaE(PART->GetZ(), PART->GetA());
            calc_e2 = true;
         }

         PART->SetParameter("FAZIA.ESI1", (calc_e1 ? -e1 : e1));
         PART->SetParameter("FAZIA.ESI2", (calc_e2 ? -e2 : e2));
         if (e1 > 0 && e2 > 0) {
            PART->SetEnergy(e1 + e2);
            SetCalibrationStatus(*PART, (calc_e1 || calc_e2) ? KVFAZIA::ECodes::SOME_ENERGY_LOSSES_CALCULATED : KVFAZIA::ECodes::NORMAL_CALIBRATION);
         }
      }
   }

   // particle identified in Si2-CsI or in (Si1+Si2)-CsI
   if (PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI2_CSI || PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI12_CSI) {

      KVNameValueList part_id(Form("Z=%d,A=%d", PART->GetZ(), PART->GetA()));

      if (csi->IsCalibrated(part_id) && si1->IsCalibrated() && si2->IsCalibrated()) {
         // treat case of all detectors calibrated
         double esi1 = si1->GetEnergy();
         double esi2 = si2->GetEnergy();
         double ecsi = csi->GetDetectorSignalValue("Energy", part_id);
         PART->SetParameter("FAZIA.ESI1", esi1);
         PART->SetParameter("FAZIA.ESI2", esi2);
         PART->SetParameter("FAZIA.ECSI", ecsi);
         PART->SetEnergy(esi1 + esi2 + ecsi);
         SetCalibrationStatus(*PART, KVFAZIA::ECodes::NORMAL_CALIBRATION); // all energies calibrated
      }
      else {
         // treat case of uncalibrated CsI detector
         // case where SI1 && SI2 are calibrated & present in event
         // and if NOTHING ELSE STOPPED IN SI1
         //
         // For Z<=2 the calibration code KVFAZIA::ECodes::ENERGY_LOSSES_TENTATIVELY_CALCULATED is given
         // (because silicon energy losses may be too small to give correct estimation for Z=1 and Z=2):
         bool si1_pileup = PART->GetParameters()->GetBoolValue("si1_pileup");

         if (si1->IsCalibrated() && si1->GetEnergy() && si2->IsCalibrated() && si2->GetEnergy() && !si1_pileup) {
            // calculate total delta-E in (SI1+SI2) then use to calculate CsI energy
            double deltaE = si1->GetEnergy() + si2->GetEnergy();
            KVDetector si1si2("Si", si1->GetThickness() + si2->GetThickness());
            double ecsi = si1si2.GetEResFromDeltaE(PART->GetZ(), PART->GetA(), deltaE);
            PART->SetParameter("FAZIA.ESI1", si1->GetEnergy());
            PART->SetParameter("FAZIA.ESI2", si2->GetEnergy());
            PART->SetParameter("FAZIA.ECSI", -ecsi);
            PART->SetEnergy(deltaE + ecsi);
            if (PART->GetZ() > 2) SetCalibrationStatus(*PART, KVFAZIA::ECodes::SOME_ENERGY_LOSSES_CALCULATED); // CsI energy calculated
            else                  SetCalibrationStatus(*PART, KVFAZIA::ECodes::ENERGY_LOSSES_TENTATIVELY_CALCULATED); // CsI energy calculated
         }
      }
   }

   // particle identified in CsI
   if (PART->GetIDCode() == KVFAZIA::IDCodes::ID_CSI_PSA) {

      bool si1_pileup = PART->GetParameters()->GetBoolValue("si1_pileup");
      bool si2_pileup = PART->GetParameters()->GetBoolValue("si2_pileup");

      KVNameValueList part_id(Form("Z=%d,A=%d", PART->GetZ(), PART->GetA()));

      double esi1 = -1;
      double esi2 = -1;
      double ecsi = -1;
      std::unordered_map<std::string, bool> is_calculated;
      is_calculated["csi"] = is_calculated["si2"] = is_calculated["si1"] = false;
      if (csi->IsCalibrated(part_id)) ecsi = csi->GetDetectorSignalValue("Energy", part_id);
      if (!si2_pileup && si2->IsCalibrated()) esi2 = si2->GetEnergy();
      if (!si1_pileup && si1->IsCalibrated()) esi1 = si1->GetEnergy();
      // pile-up in Si2 with calibrated CsI: calculate esi1 & esi2
      if (si2_pileup && csi->IsCalibrated(part_id) && ecsi > 0) {
         esi2 = si2->GetDeltaEFromERes(PART->GetZ(), PART->GetA(), ecsi);
         is_calculated["si2"] = true;
         esi1 = si1->GetDeltaEFromERes(PART->GetZ(), PART->GetA(), esi2 + ecsi);
         is_calculated["si1"] = true;
      }
      // pile-up in Si1 with calibrated CsI & calibrated Si2: calculate esi1
      else if (si1_pileup && csi->IsCalibrated(part_id) && si2->IsCalibrated() && ecsi > 0 && esi2 > 0) {
         esi1 = si1->GetDeltaEFromERes(PART->GetZ(), PART->GetA(), esi2 + ecsi);
         is_calculated["si1"] = true;
      }
      // treat case of uncalibrated CsI detector
      // case where SI1 && SI2 are calibrated & present in event & no pileup detected in Si1/Si2
      // and only for Z>2 (because silicon energy losses are too small to give correct
      // estimation for Z=1 and Z=2):
      if (!csi->IsCalibrated(part_id) &&
            (si1->IsCalibrated() && si1->GetEnergy() && si2->IsCalibrated() && si2->GetEnergy())
            && !si1_pileup && !si2_pileup && PART->GetZ() > 2) {
         // calculate total delta-E in (SI1+SI2) then use to calculate CsI energy
         double deltaE = esi1 + esi2;
         KVDetector si1si2("Si", si1->GetThickness() + si2->GetThickness());
         ecsi = si1si2.GetEResFromDeltaE(PART->GetZ(), PART->GetA(), deltaE);
         is_calculated["csi"] = true;
      }

      if (ecsi > 0 && esi1 > 0 && esi2 > 0) {
         if (is_calculated["si1"] || is_calculated["si2"] || is_calculated["csi"])
            SetCalibrationStatus(*PART, KVFAZIA::ECodes::SOME_ENERGY_LOSSES_CALCULATED);
         else
            SetCalibrationStatus(*PART, KVFAZIA::ECodes::NORMAL_CALIBRATION);
         PART->SetEnergy(esi1 + esi2 + ecsi);
         PART->SetParameter("FAZIA.ESI1", is_calculated["si1"] ? -esi1 : esi1);
         PART->SetParameter("FAZIA.ESI2", is_calculated["si2"] ? -esi2 : esi2);
         PART->SetParameter("FAZIA.ECSI", is_calculated["csi"] ? -ecsi : ecsi);
      }
   }

   if (PART->IsCalibrated()) {

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

      // check for energy loss coherency
      KVNucleus avatar;
      avatar.SetZAandE(PART->GetZ(), PART->GetA(), PART->GetKE());

      int ndet = 0;
      KVGeoDetectorNode* node = 0;
      // iterating over detectors starting from the target
      // compute the theoretical energy loss of the avatar
      // compare to the calibrated/calculated energy
      // remove this energy from the avatar energy
      PART->GetReconstructionTrajectory()->IterateBackFrom();
      while ((node = PART->GetReconstructionTrajectory()->GetNextNode())) {
         det = (KVFAZIADetector*)node->GetDetector();
         Double_t temp = det->GetELostByParticle(&avatar);
         PART->SetParameter(Form("FAZIA.avatar.E%s", det->GetLabel()), temp);
         avatar.SetKE(avatar.GetKE() - temp);
         ndet++;
      }
   }
}

void KVFAZIAGroupReconstructor::CalibrateCoherencyParticle(KVReconstructedNucleus* PART)
{
   // Calibration routine for particles added in AddCoherencyParticles() method.
   //
   // Only particles identified in Si1-Si2 or Si1-PSA are treated.
   //
   // We take into account, where possible, the calculated energy losses in SI1 and SI2
   // of the "parent" nucleus (i.e. the one which stopped in the CSI).

   PART->SetIsUncalibrated();
   PART->SetECode(KVFAZIA::ECodes::NO_CALIBRATION_ATTEMPTED);

   // particle identified in Si1 PSA, detector is calibrated
   if (PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI1_PSA) {
      if (si1->IsCalibrated()) {
         double e1 = si1->GetCalibratedEnergy();
         if (e1 <= 0) {
            Warning("CalibrateParticle",
                    "IDCODE=11 Z=%d A=%d calibrated SI1 E=%f",
                    PART->GetZ(), PART->GetA(), e1);
            return;
         }
         PART->SetParameter("FAZIA.ESI1", -e1); // energy loss is calculated in this case
         PART->SetEnergy(e1);
         SetCalibrationStatus(*PART, KVFAZIA::ECodes::SOME_ENERGY_LOSSES_CALCULATED); // energy loss is calculated in this case
      }
   }

   // particle identified in Si1-Si2
   if (PART->GetIDCode() == KVFAZIA::IDCodes::ID_SI1_SI2) {
      if (si1->IsCalibrated() && si2->IsCalibrated()) { //  with both detectors calibrated
         double e1(0), e2(0);
         if ((e2 = si2->GetCalibratedEnergy()) > 0) {
            if (!((e1 = si1->GetCalibratedEnergy()) > 0)) { // if SI1 not fired, we calculate from SI2 energy
               e1 = si1->GetDeltaEFromERes(PART->GetZ(), PART->GetA(), e2);
            }
         }
         else if ((e1 = si1->GetCalibratedEnergy()) > 0) {
            // if SI2 not fired, we calculate from SI1 energy
            e2 = si1->GetEResFromDeltaE(PART->GetZ(), PART->GetA(), e1);
         }

         PART->SetParameter("FAZIA.ESI1", -e1); // always calculated
         PART->SetParameter("FAZIA.ESI2", -e2); // always calculated
         if (e1 > 0 && e2 > 0) {
            PART->SetEnergy(e1 + e2);
            SetCalibrationStatus(*PART, KVFAZIA::ECodes::SOME_ENERGY_LOSSES_CALCULATED); // always calculated
         }
      }
   }

   if (PART->IsCalibrated()) {

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

      // check for energy loss coherency
      KVNucleus avatar;
      avatar.SetZAandE(PART->GetZ(), PART->GetA(), PART->GetKE());

      int ndet = 0;
      KVGeoDetectorNode* node = 0;
      // iterating over detectors starting from the target
      // compute the theoretical energy loss of the avatar
      // compare to the calibrated/calculated energy
      // remove this energy from the avatar energy
      PART->GetReconstructionTrajectory()->IterateBackFrom();
      while ((node = PART->GetReconstructionTrajectory()->GetNextNode())) {
         auto det = (KVFAZIADetector*)node->GetDetector();
         Double_t temp = det->GetELostByParticle(&avatar);
         PART->SetParameter(Form("FAZIA.avatar.E%s", det->GetLabel()), temp);
         avatar.SetKE(avatar.GetKE() - temp);
         ndet++;
      }
   }
}

void KVFAZIAGroupReconstructor::PostReconstructionProcessing()
{
   // Copy FPGA energy values to reconstructed particle parameter lists
   // Set values in detectors for identification/calibration procedures

   for (KVReconstructedEvent::Iterator it = GetEventFragment()->begin(); it != GetEventFragment()->end(); ++it) {
      KVReconstructedNucleus* rnuc = it.get_pointer<KVReconstructedNucleus>();

      rnuc->GetReconstructionTrajectory()->IterateFrom();

      KVGeoDetectorNode* node;
      while ((node = rnuc->GetReconstructionTrajectory()->GetNextNode())) {

         KVFAZIADetector* det = (KVFAZIADetector*)node->GetDetector();

         // now copy all detector signals to reconstructed particle parameter list...
         // they are stored with format "[detname].[signal_name]" except for
         // DetTag and GTTag which are the same for all detectors of the same telescope
         // and so are only stored once with name "DetTag" or "GTTag".
         TIter it(&det->GetListOfDetectorSignals());
         KVDetectorSignal* ds;
         while ((ds = (KVDetectorSignal*)it())) {
            if (ds->IsRaw() && !ds->IsExpression())
               // only store raw data, excluding any expressions based only on raw data
            {
               TString pname;
               // Only store non-zero parameters
               if (ds->GetValue() != 0) {
                  if (TString(ds->GetName()) != "DetTag" && TString(ds->GetName()) != "GTTag")
                     pname = Form("%s.%s", det->GetName(), ds->GetName());
                  else
                     pname = ds->GetName();
                  rnuc->GetParameters()->SetValue(pname, ds->GetValue());
               }
            }
         }
      }
   }
}

void KVFAZIAGroupReconstructor::IdentifyParticle(KVReconstructedNucleus& PART)
{
   // Coherency codes (CCode):
   // -1 no modification du to coherency checks
   // 1 CsI id = gamma + good in Si-Si -> Si-Si
   // 2 CsI id -> Si-CsI id
   // 3 Si-CsI id -> Si-Si id because Z(sicsi)<Z(sisi)
   // 4 stopped in CsI (no id) + good id Si-Si -> Si-Si
   // 5 stopped in SI2 (no id) + good id Si1PSA -> Si1PSA

   KVGroupReconstructor::IdentifyParticle(PART);

   // check for unvetoed punch-through particles in Si1-Si2 identification
   auto si1si2 = id_by_type.find("Si-Si");
   if (si1si2 != id_by_type.end()) HandleSI1SI2PunchThrough(si1si2->second, PART);

   bool si1_pileup(false), si2_pileup(false);

   // check for failed PSA identification for particles stopped in Si1
   // change status => KVReconstructedNucleus::kStatusStopFirstStage
   // estimation of minimum Z from energy loss (if Si1 calibrated)
   if (PART.GetStoppingDetector() == si1 && !PART.IsIdentified()) {
      PART.SetStatus(KVReconstructedNucleus::kStatusStopFirstStage);
      TreatStatusStopFirstStage(PART);
      return;
   }

   // Coherency checks for identifications
   if (PART.IsIdentified()) {
      /********** PARTICLES initially stopped/identified in the CSI *****************/
      if (partID.IsType("CsI")) {
         bool coherency_particle_added_in_si1si2 = false;
         if (partID.IDcode == KVFAZIA::IDCodes::ID_GAMMA) { // gammas
            // look at Si1-Si2 identification.
            // if a correct identification was obtained, we ignore the gamma in CsI and change to Si1-Si2
            if (si1si2 != id_by_type.end()) {
               if (si1si2->second->IDOK) {
                  //               Info("IdentifyParticle","Got GAMMA in CsI, changing to Si1Si2 identification [Z=%d A=%d]",
                  //                    si1si2->second->Z, si1si2->second->A);
                  ChangeReconstructedTrajectory(PART);
                  partID = *(si1si2->second);
                  PART.SetIsIdentified();
                  PART.SetIdentification(&partID, fSi1Si2IDTelescope);
                  PART.GetParameters()->SetValue("CCode", 1);
               }
            }
         }
         else if (partID.IDOK) {
            // good identification in CsI (light charged particle)
            // check for pile-up in Si2 (and beyond) - in this case, measured energy loss in Si2+Si1
            // should not be attributed to this particle
            auto si2csi = id_by_type.find("Si-CsI");
            if (si2csi != id_by_type.end()) {
               // detect a pile-up in Si2 in coincidence with particule detected in CsI
               // the following covers the following cases:
               //   - good ID Si-CsI (IDquality < kICODE4) with Z>Zcsi
               //   - ID "between the lines" in Si-CsI (IDquality=kICODE4,kICODE5) with Z>Zcsi
               //   - point above last Z line in Si-CsI grid (kICODE7) with Z>Zcsi
               // In these cases, there is a second particle stopped in Si2: and
               // hence the measured Si1 energy loss is not to be used either
               if ((si2csi->second->Z > partID.Z)) {
                  si2_pileup = true;
                  si1_pileup = true;
               }
            }
            if (si1si2 != id_by_type.end()) {
               // detect a pile-up in Si1/2 in coincidence with particule detected in CsI
               // the following covers the following cases:
               //   - good ID Si1-Si2 (IDquality < kICODE4) with Z>Zcsi
               //   - ID "between the lines" in Si1-Si2 (IDquality=kICODE4,kICODE5) with Z>Zcsi
               //   - point above last Z line in Si1-Si2 grid (kICODE7) with Z>Zcsi
               // In these cases, there is a second particle stopped in Si2: and
               // hence the measured Si1 energy loss is not to be used either
               if (si1si2->second->Z > partID.Z) {
                  si2_pileup = true;
                  si1_pileup = true;
                  if (si1si2->second->IDOK) {
                     // if in addition the si1si2 identification is OK, we need to add a new particle
                     // to the event corresponding to this fragment stopped in SI2 in coincidence with
                     // the LCP in the CsI
                     coherency_particles.push_back({
                        &PART, si2->GetNode(), theTrajectory,
                        fSi1Si2IDTelescope,
                        (Int_t)si1si2->second->GetNumber(), PART.GetNumberOfIdentificationResults()
                     }
                                                  );
                     coherency_particle_added_in_si1si2 = true;
                  }
               }
            }
            auto sipsa = id_by_type.find("SiPSA");
            if (sipsa != id_by_type.end()) {
               // detect a pile-up in Si1 in coincidence with particule detected in CsI
               if ((sipsa->second->Z > partID.Z)) {
                  si1_pileup = true;
                  if (sipsa->second->IDOK && !coherency_particle_added_in_si1si2) {
                     // if in addition the si1psa identification is OK, and if we have not already added
                     // a new particle stopping in SI2 after looking at Si1-Si2 identification,
                     // we need to add a new particle to the event corresponding to this fragment stopped in SI1
                     // in coincidence with the LCP in the CsI
                     coherency_particles.push_back({
                        &PART, si1->GetNode(), theTrajectory,
                        (KVIDTelescope*)PART.GetReconstructionTrajectory()->GetIDTelescopes()->Last(),
                        (Int_t)sipsa->second->GetNumber(), PART.GetNumberOfIdentificationResults()
                     }
                                                  );
                  }
               }
            }

         }
      }
      /********** PARTICLES initially identified in SI2-CSI *****************/
      else if (partID.IsType("Si-CsI")) {
         // ions correctly identified in Si2-CsI should have coherent identification in Si1-Si2: as the particle punches
         // through Si2 the Si1-Si2 identification should underestimate the A and/or Z of the ion, i.e. either the A
         // or the Z from Si1-Si2 identification should be smaller than from Si2-CsI identification.
         //
         // the following is to treat the case (quite typical for fazia) where a particle stops in Si2 (id in Si1Si2)
         // but a small noise in the CsI makes it look like a particle stopped in CsI and identified in Si2-CsI
         int zz = partID.Z;
         if (si1si2 != id_by_type.end()) {
            KVIdentificationResult* idr_si1si2 = si1si2->second;
            if (idr_si1si2->IDOK && idr_si1si2->IDcode == KVFAZIA::IDCodes::ID_SI1_SI2) { // only change if there is no suspicion of punch-through in si1-si2
               if (zz < idr_si1si2->Z) {
                  //               Info("IdentifyParticle","SiCsI identification [Z=%d A=%d] changed to SiSi identification [Z=%d A=%d]",
                  //                    PART.GetZ(),PART.GetA(),si1si2->second->Z,si1si2->second->A);
                  ChangeReconstructedTrajectory(PART);
                  partID = *(si1si2->second);
                  PART.SetIdentification(&partID, fSi1Si2IDTelescope);
                  PART.GetParameters()->SetValue("CCode", 3);
               }
            }
         }
      }
   }
   else {
      // particle not identified, apparently stopped in CsI, with Si1-Si2 identification?
      if (PART.GetStoppingDetector()->IsLabelled("CSI")) {
         if (si1si2 != id_by_type.end()) {
            if (si1si2->second->IDOK) {
               // stopping detector becomes Si2, identification Si1-Si2 accepted
               //               Info("IdentifyParticle","Unidentified, stopped in CsI, good Si1Si2 identification [Z=%d A=%d]",
               //                   si1si2->second->Z,si1si2->second->A);
               ChangeReconstructedTrajectory(PART);
               partID = *(si1si2->second);
               PART.SetIsIdentified();
               PART.SetIdentification(&partID, fSi1Si2IDTelescope);
               PART.GetParameters()->SetValue("CCode", 4);
            }
         }
      }
      // particle not identified, apparently stopped in SI2, with Si1-PSA identification?
      // Such particles were added to the event in 1.12/05, but they (mostly) correspond to
      // punch-through in SI1 and give bad "accumulations" of data in e.g. Z-V plots.
      // So they are best left unidentified! (we still give the CCode value 5)
      // They receive idcode KVFAZIA::ID_SI1_PUNCH_THROUGH, are identified, but not in Z (or A)
      if (PART.GetStoppingDetector()->IsLabelled("SI2")) {
         auto sipsa = id_by_type.find("SiPSA");
         if (sipsa != id_by_type.end()) {
            if (sipsa->second->IDOK) {
               // stopping detector becomes Si1
               ChangeReconstructedTrajectory(PART);
               partID = *(sipsa->second);
               PART.SetIsIdentified();
               auto sipsa_idtel = (KVIDTelescope*)PART.GetReconstructionTrajectory()->GetIDTelescopes()->Last();
               partID.Zident = false;
               partID.Aident = false;
               partID.SetComment("particle partially identified by pulse shape analysis in SI1, although it is punching through (no SI2 signal or SI1-SI2 id)");
               PART.SetIdentification(&partID, sipsa_idtel);
               PART.GetParameters()->SetValue("CCode", 5);
            }
         }
      }
   }
   PART.SetParameter("si1_pileup", si1_pileup);
   PART.SetParameter("si2_pileup", si2_pileup);
}

void KVFAZIAGroupReconstructor::ChangeReconstructedTrajectory(KVReconstructedNucleus& PART)
{
   // Change the reconstruction trajectory & the stopping detector for the particle.
   //
   // The stopping detector is moved 1 closer to the target, i.e.
   //
   //   - initial stopping detector=CSI with trajectory CSI/SI2/SI1: final stopping=SI2, trajectory=SI2/SI1
   //   - initial stopping detector=SI2 with trajectory SI2/SI1: final stopping=SI1, trajectory=SI1

   PART.GetReconstructionTrajectory()->IterateFrom();
   PART.GetReconstructionTrajectory()->GetNextNode();
   KVGeoDetectorNode* node = PART.GetReconstructionTrajectory()->GetNextNode();
   KVGeoDNTrajectory* traj = (KVGeoDNTrajectory*)node->GetTrajectories()->First();
   const KVReconNucTrajectory* newtraj = (const KVReconNucTrajectory*)GetGroup()->GetTrajectoryForReconstruction(traj, node);
   PART.ModifyReconstructionTrajectory(newtraj);
}

void KVFAZIAGroupReconstructor::HandleSI1SI2PunchThrough(KVIdentificationResult* idr, KVReconstructedNucleus& PART)
{
   // SI1-SI2 identification may suffer from unvetoed punch-through particles.
   //
   // In this case informational cuts/contours have been defined to indicate when a particle
   // is identified in a region which may be polluted by punch-through.
   //
   // In this case, 2 cases are treated:
   //
   //  a) for well-identified particles (IDquality<4), there is an ambiguity to their identification:
   //     it may well be truly a particle with the deduced Z & A which stopped in SI2, or it may
   //     in fact be a particle with larger Z &/or A which punched-through.
   //
   //     For these particles, we change the general IDCode to FAZIAIDCodes::ID_SI1_SI2_MAYBE_PUNCH_THROUGH
   //
   //  b) particles which are not well-identified because between the identification lines, too far to be
   //     considered identified, idquality=4,5, are in this case most likely to be punching through particles
   //     with a Z at least equal to 1 more than that given by the identification routine (Zmin).
   //
   //     For these particles, we change the general IDCode to FAZIAIDCodes::ID_SI1_SI2_PUNCH_THROUGH

   if (idr->IdentifyingGridHasFlagWhichBegins("punch_through")) {
      KVString pt_flag = idr->IdentifyingGridGetFlagWhichBegins("punch_through");
      bool treat_punch_through = true;
      if (pt_flag.GetNValues("|") == 2) {
         // Z range specified as : "punch_through|Z=1-13"
         pt_flag.Begin("|=");
         pt_flag.Next();
         pt_flag.Next();
         assert(!pt_flag.End());// this should never happen
         KVNumberList zrange(pt_flag.Next());
         // only particles with Z in given range are treated for punch-through
         treat_punch_through = zrange.Contains(idr->Z);
      }
      if (treat_punch_through) {
         bool pt_treated = false;
         if (idr->IDquality < KVIDZAGrid::kICODE4) {
            // change general IDcode
            idr->IDcode = KVFAZIA::IDCodes::ID_SI1_SI2_MAYBE_PUNCH_THROUGH;
            idr->SetComment("Apparently well-identified particle, but could be punching through to CsI (in which case Z is a minimum)");
            pt_treated = true;
         }
         else if (idr->IDquality < KVIDZAGrid::kICODE6) {
            // change general IDcode & identification status
            idr->IDcode = KVFAZIA::IDCodes::ID_SI1_SI2_PUNCH_THROUGH;
            idr->SetComment("Particle punching through SI2, identified Z is only a minimum estimation");
            idr->IDOK = kTRUE; // this will previously have been false
            idr->Zident = kFALSE;
            pt_treated = true;
         }
         if (pt_treated && (PART.GetStoppingDetector() == si2)) {
            // For any particles stopped (at least apparently) in SI2, we may (potentially) change the identification of particle,
            // or at least its IDCode
            PART.SetIsIdentified();
            PART.SetIdentification(idr, fSi1Si2IDTelescope);
         }
      }
   }
}

void KVFAZIAGroupReconstructor::SetGroup(const KVGroup* g)
{
   KVGroupReconstructor::SetGroup(g);
   csi = (KVFAZIADetector*)g->GetDetectorByType("CsI");
   assert(csi != nullptr);
   // set unique trajectory for group
   theTrajectory = (KVGeoDNTrajectory*)csi->GetNode()->GetForwardTrajectories()->First();
   theTrajectory->IterateBackFrom();
   si1 = (KVFAZIADetector*)theTrajectory->GetNextNode()->GetDetector();
   si2 = (KVFAZIADetector*)theTrajectory->GetNextNode()->GetDetector();
   assert(si1 != nullptr);
   assert(si2 != nullptr);
   fSi1Si2IDTelescope = (KVIDTelescope*)theTrajectory->GetIDTelescopes()->FindObjectByType("Si-Si");
   assert(fSi1Si2IDTelescope != nullptr);
}

void KVFAZIAGroupReconstructor::AddCoherencyParticles()
{
   // Called to add any nuclei not included in the initial reconstruction, but "revealed"
   // by consistency checks between identifications and calibrations of other nuclei
   //
   // These particles have a (string) parameter "COHERENCY"

   //std::cout << "===========================================================================================\n" << std::endl;
   //Info("AddCoherencyParticles", "There are %d particles to add to this event\n", (int)coherency_particles.size());
   for (auto& part : coherency_particles) {

      auto rnuc = GetEventFragment()->AddParticle();

//      Info("AddCoherencyParticle", "Adding particle in pile-up with Z=%d A=%d E=%f in %s",
//           part.original_particle->GetZ(), part.original_particle->GetA(), part.original_particle->GetE(), part.original_particle->GetStoppingDetector()->GetName());

      auto ESI1_parent = part.original_particle->GetParameters()->HasDoubleParameter("FAZIA.ESI1") ?
                         TMath::Abs(part.original_particle->GetParameters()->GetDoubleValue("FAZIA.ESI1"))
                         : 0;
      auto ESI2_parent = part.original_particle->GetParameters()->HasDoubleParameter("FAZIA.ESI2") ?
                         TMath::Abs(part.original_particle->GetParameters()->GetDoubleValue("FAZIA.ESI2"))
                         : 0;
//      std::cout << "ESI1 = " << ESI1_parent << " [" << si1->GetDetectorSignalValue("Energy") <<
//                   "] ESI2 = " << ESI2_parent << " [" << si2->GetDetectorSignalValue("Energy") << "]" << std::endl;

      // reconstruction
      auto Rtraj = (const KVReconNucTrajectory*)GetGroup()->GetTrajectoryForReconstruction(part.stopping_trajectory, part.stopping_detector_node);

//      std::cout << "SI1: " << (si1->IsCalibrated() ? "CALIB." : "NOT CALIB.")
//                << "   SI2: " << (si2->IsCalibrated() ? "CALIB." : "NOT CALIB.")
//                << "   CSI: " << (csi->IsCalibrated(
//                                     Form("Z=%d,A=%d", part.original_particle->GetZ(), part.original_particle->GetA())
//                                  ) ? "CALIB." : "NOT CALIB.")  << std::endl;

      rnuc->SetReconstructionTrajectory(Rtraj);
      rnuc->SetParameter("ARRAY", GetGroup()->GetParentStructure<KVMultiDetArray>()->GetName());
      rnuc->SetParameter("COHERENCY",
                         "Particle added to event after consistency checks between identifications and calibrations of other nuclei");
      // identification
      Int_t idnumber = 1;
      for (int i = part.first_id_result_to_copy; i <= part.max_id_result_index; ++i) {
         auto IDR = rnuc->GetIdentificationResult(idnumber++);
         // copy existing identification results from "parent" particle
         part.original_particle->GetIdentificationResult(i)->Copy(*IDR);
      }
      rnuc->SetIsIdentified();
      rnuc->SetIdentification(rnuc->GetIdentificationResult(1), part.identifying_telescope);
//      Info("AddCoherencyParticle", "Initial ident of particle to add: Z=%d A=%d identified in %s",
//           rnuc->GetZ(), rnuc->GetA(), rnuc->GetIdentifyingTelescope()->GetType());
//      rnuc->GetIdentificationResult(1)->Print();
      // with both silicons calibrated, we can try to subtract the contributions of the parent
      // particle (using inverse calibrations and calculated energy losses of parent),
      // then try a new identification (only for Si1-Si2: we don't know how to recalculate the PSA parameters for SI1)
      // as E789 Si1-Si2 identifications are implemented with 2 grids, first a low range
      // QL1.Amplitude vs. Q2.FPGAEnergy grid (upto Z=8), then full range QH1.FPGAEnergy vs.
      // Q2.FPGAEnergy, and calibrations for SI1 & SI2 use QH1.FPGAEnergy and Q2.FPGAEnergy
      // respectively, we set QL1.Amplitude=0 in order to force the use of the recalculated
      // QH1.FPGAEnergy and Q2.FPGAEnergy in the full range grid.
      // *actually, SI1 may have also an "Energy-QL1" calibration from QL1.Amplitude,
      // in which case we can modify this as well
      if (si1->IsCalibrated() && si2->IsCalibrated()) {
         // calculate new values of raw parameters
         double new_q2{0}, new_qh1{0}, new_ql1{0};
         auto ESI1_qh1 = si1->GetDetectorSignalValue("Energy");
         auto ESI1_ql1 = si1->GetDetectorSignalValue("Energy-QL1");
         auto ESI2 = si2->GetEnergy();
         new_qh1 = si1->GetInverseDetectorSignalValue("Energy", TMath::Max(0., ESI1_qh1 - ESI1_parent), "QH1.FPGAEnergy");
         if (si1->HasDetectorSignal("Energy-QL1")) {
            new_ql1 = si1->GetInverseDetectorSignalValue("Energy-QL1", TMath::Max(0., ESI1_ql1 - ESI1_parent), "QL1.Amplitude");
         }
//         Info("AddCoherencyParticle", "Changing raw data parameters:");
//         std::cout << "  QL1:  " << si1->GetDetectorSignalValue("QL1.Amplitude") << "  ==>  " << new_ql1 << std::endl;
//         std::cout << "  QH1:  " << si1->GetDetectorSignalValue("QH1.FPGAEnergy") << "  ==>  " << new_qh1 << std::endl;
         si1->SetDetectorSignalValue("QH1.FPGAEnergy", new_qh1);
         si1->SetDetectorSignalValue("QL1.Amplitude", new_ql1);

         if (rnuc->GetIDCode() == KVFAZIA::IDCodes::ID_SI1_SI2) {
            new_q2 = si2->GetInverseDetectorSignalValue("Energy", TMath::Max(0., ESI2 - ESI2_parent), "Q2.FPGAEnergy");
//            std::cout << "   Q2:  " << si2->GetDetectorSignalValue("Q2.FPGAEnergy") << "  ==>  " << new_q2 << std::endl;
            si2->SetDetectorSignalValue("Q2.FPGAEnergy", new_q2);

            // now retry the identification
            KVIdentificationResult IDR;
            IDR.SetNumber(1);
            part.identifying_telescope->Identify(&IDR);
//            rnuc->GetIdentificationResult(1)->Print();
            if (IDR.IDOK) {
//               Info("AddCoherencyParticle", "Achieved new identification for particle:");
               *(rnuc->GetIdentificationResult(1)) = IDR;
               rnuc->SetIdentification(rnuc->GetIdentificationResult(1), part.identifying_telescope);
               // check we are not in punch-through region
               HandleSI1SI2PunchThrough(rnuc->GetIdentificationResult(1), *rnuc);
            }
            else {
//               Info("AddCoherencyParticle", "Identification has failed for particle");
               if (new_q2 < 1) {
//                  Info("AddCoherencyParticle", "Try SI1-PSA identification?");
//                  part.original_particle->GetIdentificationResult(5)->Print();
                  // subtraction of original particle leaves nothing in SI2: try SI1-PSA ?
                  if (part.original_particle->GetIdentificationResult(5)->IDOK) {
                     // modify reconstruction trajectory: now starts on SI1 not SI2
                     Rtraj = (const KVReconNucTrajectory*)GetGroup()->GetTrajectoryForReconstruction(part.stopping_trajectory, si1->GetNode());
                     rnuc->ModifyReconstructionTrajectory(Rtraj);
                     part.original_particle->GetIdentificationResult(5)->Copy(*rnuc->GetIdentificationResult(1));
                     auto idt = (KVIDTelescope*)Rtraj->GetIDTelescopes()->First();
                     rnuc->SetIdentification(rnuc->GetIdentificationResult(1), idt);
                  }
                  else {
                     // leave original estimation of identification Si1-Si2
                     // but check we are not in punch-through region
                     HandleSI1SI2PunchThrough(rnuc->GetIdentificationResult(1), *rnuc);
                  }
               }
               else {
                  // leave original estimation of identification Si1-Si2
                  // but check we are not in punch-through region
                  HandleSI1SI2PunchThrough(rnuc->GetIdentificationResult(1), *rnuc);
               }
            }
         }
      }
      // calibration
      if (rnuc->IsIdentified()) {
         CalibrateCoherencyParticle(rnuc);
//         Info("AddCoherencyParticle", "Added particle with Z=%d A=%d E=%f identified in %s",
//              rnuc->GetZ(), rnuc->GetA(), rnuc->GetE(), rnuc->GetIdentifyingTelescope()->GetType());
//         std::cout << "ESI1 = " << rnuc->GetParameters()->GetDoubleValue("FAZIA.ESI1")
//                   << " [" << rnuc->GetParameters()->GetDoubleValue("FAZIA.avatar.ESI1") << "]"
//                   << " ESI2 = " << rnuc->GetParameters()->GetDoubleValue("FAZIA.ESI2")
//                   << " [" << rnuc->GetParameters()->GetDoubleValue("FAZIA.avatar.ESI2") << "]"
//                   << std::endl << std::endl;
      }
   }
//   std::cout << "===========================================================================================\n\n" << std::endl;
}
