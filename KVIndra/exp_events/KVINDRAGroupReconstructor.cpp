//Created by KVClassFactory on Wed Feb 21 13:42:47 2018
//Author: John Frankland,,,

#include "KVINDRAGroupReconstructor.h"
#include <KVIDGCsI.h>
#include <KVIDINDRACsI.h>
#include <KVINDRADetector.h>
#include "KVINDRACodeMask.h"

ClassImp(KVINDRAGroupReconstructor)

TString KVINDRAGroupReconstructor::CSI_ID_TYPE = "CSI";

KVReconstructedNucleus* KVINDRAGroupReconstructor::ReconstructTrajectory(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   // Specialised event reconstruction for INDRA data
   // Triggered CsI detectors are checked: if it is a gamma, we count it (parameter "INDRA_GAMMA_MULT")
   // and add the name of the detector to the parameter "INDRA_GAMMA_DETS"
   // but do not reconstruct a particle.

   if (node->GetDetector()->IsType("CSI")) {
      if (node->GetDetector()->Fired(GetPartSeedCond())) {
         ++nfireddets;
         KVIDINDRACsI* idt = (KVIDINDRACsI*)traj->GetIDTelescopes()->FindObjectByType(CSI_ID_TYPE);
         if (idt) {
            KVIdentificationResult idr;
            if (idt->IsReadyForID()) {
               idt->Identify(&idr);
               if (idr.IDOK && idr.IDcode == kIDCode0) {
                  GetEventFragment()->GetParameters()->IncrementValue("INDRA_GAMMA_MULT", 1);
                  GetEventFragment()->GetParameters()->IncrementValue("INDRA_GAMMA_DETS", node->GetName());
                  node->GetDetector()->SetAnalysed();
                  return nullptr;
               }
            }
         }
         return GetEventFragment()->AddParticle();
      }
      return nullptr;
   }
   return KVGroupReconstructor::ReconstructTrajectory(traj, node);
}

void KVINDRAGroupReconstructor::Identify()
{
   KVGroupReconstructor::Identify();
   for (KVReconstructedEvent::Iterator it = GetEventFragment()->begin(); it != GetEventFragment()->end(); ++it) {
      KVReconstructedNucleus* d = it.get_pointer();
      if (d->IsIdentified() && d->GetStatus() == KVReconstructedNucleus::kStatusStopFirstStage) {
         if (d->GetIdentifyingTelescope()) {
            d->GetIdentifyingTelescope()->SetIDCode(d, kIDCode5);
         }
         else {
            SETINDRAIDCODE(d, 5);
         }
      }
      else if (!d->IsIdentified()) {
         SETINDRAIDCODE(d, 14); // unidentifiable particle
      }
   }
}

void KVINDRAGroupReconstructor::IdentifyParticle(KVReconstructedNucleus& PART)
{
   // INDRA-specific particle identification.
   // Here we attribute the Veda6-style general identification codes depending on the
   // result of KVReconstructedNucleus::Identify and the subcodes from the different
   // identification algorithms:
   // If the particle's mass A was NOT measured, we make sure that it is calculated
   // from the measured Z using the mass formula defined by default
   //
   //IDENTIFIED PARTICLES
   //Identified particles with ID code = 2 with subcodes 4 & 5
   //(masse hors limite superieure/inferieure) are relabelled
   //with kIDCode10 (identification entre les lignes CsI)
   //
   //UNIDENTIFIED PARTICLES
   //Unidentified particles receive the general ID code for non-identified particles (kIDCode14)
   //EXCEPT if their identification in CsI gave subcodes 6 or 7
   //(Zmin) then they are relabelled "Identified" with IDcode = 9 (ident. incomplete dans CsI ou Phoswich (Z.min))
   //Their "identifying" telescope is set to the CsI ID telescope

   KVGroupReconstructor::IdentifyParticle(PART);
   // if a successful identification has occured, identifying_telescope & partID are now set
   // and the particle has IsIdentified()=true

   // INDRA coherency treatment
   PART.SetParameter("Coherent", kTRUE);
   PART.SetParameter("Pileup", kFALSE);
   PART.SetParameter("UseFullChIoEnergyForCalib", kTRUE);
   Bool_t ok = DoCoherencyAnalysis(PART);

   if (ok) { // identification may change here due to coherency analysis
      PART.SetIsIdentified();
      PART.SetIdentifyingTelescope(identifying_telescope);
      PART.SetIdentification(&partID, identifying_telescope);
   } // if not ok, do we need to unset any previously identified particle?

   if (PART.IsIdentified()) {

      /******* IDENTIFIED PARTICLES *******/
      if (partID.IsType(CSI_ID_TYPE)) {     /**** CSI IDENTIFICATION ****/

         //Identified particles with ID code = 2 with subcodes 4 & 5
         //(masse hors limite superieure/inferieure) are relabelled
         //with kIDCode10 (identification entre les lignes CsI)

         Int_t grid_code = partID.IDquality;
         if (grid_code == KVIDGCsI::kICODE4 || grid_code == KVIDGCsI::kICODE5) {
            partID.IDcode = kIDCode10;
            PART.SetIdentification(&partID, identifying_telescope);
         }

      }

   }
   else {

      /******* UNIDENTIFIED PARTICLES *******/

      /*** general ID code for non-identified particles ***/
      SETINDRAIDCODE((&PART), 14);
#ifndef WITH_CPP11
      std::map<std::string, KVIdentificationResult*>::iterator csirl = id_by_type.find("CSI_R_L");
#else
      auto csirl = id_by_type.find("CSI_R_L");
#endif
      if (csirl != id_by_type.end()) {
         //Particles remaining unidentified are checked: if their identification in CsI R-L gave subcodes 6 or 7
         //(Zmin) then they are relabelled "Identified" with IDcode = 9 (ident. incomplete dans CsI ou Phoswich (Z.min))
         //Their "identifying" telescope is set to the CsI ID telescope
         if (csirl->second->IDattempted) {
            if (csirl->second->IDquality == KVIDGCsI::kICODE6 || csirl->second->IDquality == KVIDGCsI::kICODE7) {
               PART.SetIsIdentified();
               csirl->second->IDcode = kIDCode9;
               partID = *(csirl->second);
               identifying_telescope = (KVIDTelescope*)PART.GetReconstructionTrajectory()->GetIDTelescopes()->FindObjectByType("CSI_R_L");
               PART.SetIdentifyingTelescope(identifying_telescope);
               PART.SetIdentification(&partID, identifying_telescope);
            }
         }
      }

   }
}

void KVINDRAGroupReconstructor::CalculateChIoDEFromResidualEnergy(KVReconstructedNucleus* n, Double_t ERES)
{
   // calculate fEChIo from residual energy
   Double_t e0 = theChio->GetDeltaEFromERes(n->GetZ(), n->GetA(), ERES);
   theChio->SetEResAfterDetector(ERES);
   fEChIo = theChio->GetCorrectedEnergy(n, e0);
   fEChIo = -TMath::Abs(fEChIo);
   SETINDRAECODE(n, 2);
}

void KVINDRAGroupReconstructor::CalibrateParticle(KVReconstructedNucleus* PART)
{
   // Calculate and set the energy of a (previously identified) reconstructed particle
   //
   // This is only possible for correctly identified particles.
   // We exclude IDCODE9 particles (Zmin in CsI-RL)

   fEChIo = fESi = fECsI = 0;

   print_part = false;

   if (PART->GetIDCode() != 9) DoCalibration(PART);

   PART->SetIsCalibrated();
   PART->SetParameter("INDRA.ECHIO", fEChIo);
   PART->SetParameter("INDRA.ESI", fESi);
   PART->SetParameter("INDRA.ECSI", fECsI);
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
   CheckCsIEnergy(PART);

   //if(print_part) PART->Print();
}

double KVINDRAGroupReconstructor::DoBeryllium8Calibration(KVReconstructedNucleus* n)
{
   // Beryllium-8 = 2 alpha particles of same energy
   // We halve the total light output of the CsI to calculate the energy of 1 alpha
   // Then multiply resulting energy by 2
   // Note: fECsI is -ve, because energy is calculated not measured

   KVCsI* csi = GetCsI(n);
   Double_t half_light = csi->GetDetectorSignalValue("TotLight") * 0.5;
   KVNucleus tmp(2, 4);
   double ecsi = -2.*csi->GetCorrectedEnergy(&tmp, half_light, kFALSE);
   SETINDRAECODE(n, 2);
   return ecsi;
}

void KVINDRAGroupReconstructor::CheckCsIEnergy(KVReconstructedNucleus* n)
{
   // Check calculated CsI energy loss of particle.
   // If it is greater than the maximum theoretical energy loss
   // (depending on the length of CsI, the Z & A of the particle)
   // we set the energy calibration code to kECode3 (historical VEDA code
   // for particles with E_csi > E_max_csi)

   KVDetector* csi = GetCsI(n);
   if (csi && (n->GetZ() > 0) && (n->GetZ() < 3) && (csi->GetEnergy() > csi->GetMaxDeltaE(n->GetZ(), n->GetA()))) {
      SETINDRAECODE(n, 3);
   }
}

/*
   The following is a copy of the old
void KVINDRAReconEvent::SecondaryAnalyseGroup(KVGroup* grp)
   method from branch 1.10

void KVINDRAGroupReconstructor:PerformSecondaryAnalysis()
{
   // Perform identifications and calibrations of particles not included
   // in first round (methods IdentifyEvent() and CalibrateEvent()).
   //
   // Here we treat particles with GetStatus()==KVReconstructedNucleus::kStatusOKafterSub
   // after subtracting the energy losses of all previously calibrated particles in group from the
   // measured energy losses in the detectors they crossed.

   // loop over al identified & calibrated particles in group and subtract calculated
   // energy losses from all detectors
   KVINDRAReconNuc* nuc;
   TList sixparts;
   TIter parts(grp->GetParticles());
   while ((nuc = (KVINDRAReconNuc*)parts())) {
      if (nuc->IsIdentified() && nuc->IsCalibrated()) {
         nuc->SubtractEnergyFromAllDetectors();
         // reconstruct particles from pile-up in silicon detectors revealed by coherency CsIR/L - SiCsI
         if (nuc->IsSiPileup() && nuc->GetSi()->GetEnergy() > 0.1) {
            KVINDRAReconNuc* SIX = AddParticle();
            SIX->Reconstruct(nuc->GetSi());
            sixparts.Add(SIX);
         }
         // reconstruct particles from pile-up in si75 detectors revealed by coherency
         if (nuc->IsSi75Pileup()) {
            KVINDRAReconNuc* SIX = AddParticle();
            SIX->Reconstruct(nuc->GetSi75());
            sixparts.Add(SIX);
         }
         // reconstruct particles from pile-up in sili detectors revealed by coherency
         if (nuc->IsSiLiPileup()) {
            KVINDRAReconNuc* SIX = AddParticle();
            SIX->Reconstruct(nuc->GetSiLi());
            sixparts.Add(SIX);
         }

         // reconstruct particles from pile-up in ChIo detectors revealed by coherency CsIR/L - ChIoCsI
         if (nuc->IsChIoPileup() && nuc->GetChIo()->GetEnergy() > 1.0) {
            KVINDRAReconNuc* SIX = AddParticle();
            SIX->Reconstruct(nuc->GetChIo());
            sixparts.Add(SIX);
         }
      }
   }
   // reanalyse group
   KVReconstructedNucleus::AnalyseParticlesInGroup(grp);

   Int_t nident = 0; //number of particles identified in each step
   if (sixparts.GetEntries()) { // identify any particles added by coherency CsIR/L - SiCsI
      KVINDRAReconNuc* SIX;
      TIter nextsix(&sixparts);
      while ((SIX = (KVINDRAReconNuc*)nextsix())) {
         if (SIX->GetStatus() == KVReconstructedNucleus::kStatusOK) {
            SIX->Identify();
            if (SIX->IsIdentified()) {
               nident++;
               if (SIX->GetCodes().TestIDCode(kIDCode5)) SIX->SetIDCode(kIDCode7);
               else SIX->SetIDCode(kIDCode6);
               SIX->Calibrate();
               if (SIX->IsCalibrated()) SIX->SubtractEnergyFromAllDetectors();
            } else {
               // failure of ChIo-Si identification: particle stopped in ChIo ?
               // estimation of Z (minimum) from energy loss (if detector is calibrated)
               UInt_t zmin = ((KVDetector*)SIX->GetDetectorList()->Last())->FindZmin(-1., SIX->GetMassFormula());
               if (zmin) {
                  SIX->SetZ(zmin);
                  SIX->SetIsIdentified();
                  SIX->SetIDCode(kIDCode7);
                  // "Identifying" telescope is taken from list of ID telescopes
                  // to which stopping detector belongs
                  SIX->SetIdentifyingTelescope((KVIDTelescope*)SIX->GetStoppingDetector()->GetIDTelescopes()->Last());
                  SIX->Calibrate();
               }
            }
         }
      }
   }
   if (nident) { // newly-identified particles may change status of others in group
      // reanalyse group
      KVReconstructedNucleus::AnalyseParticlesInGroup(grp);
      nident = 0;
   }

   TIter parts2(grp->GetParticles()); // list may have changed if we have added particles
   // identify & calibrate any remaining particles with status=KVReconstructedNucleus::kStatusOK
   while ((nuc = (KVINDRAReconNuc*)parts2())) {
      if (!nuc->IsIdentified() && nuc->GetStatus() == KVReconstructedNucleus::kStatusOK) {
         nuc->ResetNSegDet();
         nuc->Identify();
         if (nuc->IsIdentified()) {
            nident++;
            nuc->Calibrate();
            if (nuc->IsCalibrated()) nuc->SubtractEnergyFromAllDetectors();
         }
      }
   }
   if (nident) { // newly-identified particles may change status of others in group
      // reanalyse group
      KVReconstructedNucleus::AnalyseParticlesInGroup(grp);
      nident = 0;
   }

   // any kStatusOKafterShare particles ?
   TList shareChIo;
   parts2.Reset();
   while ((nuc = (KVINDRAReconNuc*)parts2())) {
      if (!nuc->IsIdentified() && nuc->GetStatus() == KVReconstructedNucleus::kStatusOKafterShare) {
         shareChIo.Add(nuc);
      }
   }
   Int_t nshares = shareChIo.GetEntries();
   if (nshares) {
      KVChIo* theChIo = ((KVINDRAReconNuc*)shareChIo.At(0))->GetChIo();
      if (theChIo && nshares > 1) {
         // divide chio energy equally
         Double_t Eshare = theChIo->GetEnergyLoss() / nshares;
         theChIo->SetEnergyLoss(Eshare);
         // modify PG and GG of ChIo according to new energy loss
         Double_t volts = theChIo->GetVoltsFromEnergy(Eshare);
         Double_t GG = theChIo->GetCanalGGFromVolts(volts);
         Double_t PG = theChIo->GetCanalPGFromVolts(volts);
         theChIo->GetACQParam("PG")->SetData(TMath::Min(4095, (Int_t)PG));
         theChIo->GetACQParam("GG")->SetData(TMath::Min(4095, (Int_t)GG));
      }
      // now try to identify
      TIter nextSh(&shareChIo);
      while ((nuc = (KVINDRAReconNuc*)nextSh())) {
         nuc->SetNSegDet(10);
         nuc->Identify();
         if (nuc->IsIdentified()) {
            nuc->SetIDCode(kIDCode8);
            nuc->Calibrate();
         }
      }
   }

   // any remaining stopped in first stage particles ?
   parts2.Reset();
   while ((nuc = (KVINDRAReconNuc*)parts2())) {
      if (!nuc->IsIdentified() && nuc->GetStatus() == KVReconstructedNucleus::kStatusStopFirstStage) {
         // estimation of Z (minimum) from energy loss (if detector is calibrated)
         UInt_t zmin = ((KVDetector*)nuc->GetDetectorList()->Last())->FindZmin(-1., nuc->GetMassFormula());
         if (zmin) {
            nuc->SetZ(zmin);
            nuc->SetIsIdentified();
            nuc->SetIDCode(kIDCode5);
            // "Identifying" telescope is taken from list of ID telescopes
            // to which stopping detector belongs
            nuc->SetIdentifyingTelescope((KVIDTelescope*)nuc->GetStoppingDetector()->GetIDTelescopes()->Last());
            nuc->Calibrate();
         }
      }
   }
}
*/
