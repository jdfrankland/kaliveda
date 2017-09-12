/*
$Id: KVINDRAUpDater.cpp,v 1.12 2008/01/10 14:17:34 franklan Exp $
$Revision: 1.12 $
$Date: 2008/01/10 14:17:34 $
$Author: franklan $
*/

#include "KVINDRAUpDater.h"
#include "KVINDRA.h"
#include "KVCalibrator.h"
#include "KVNucleus.h"
#include "KVDBParameterSet.h"
#include "KVINDRADBRun.h"
#include "KVIDGridManager.h"
#include "KVDBChIoPressures.h"
#include "KVChIo.h"
#include "KVBase.h"
#include "KVSilicon.h"
#include "KVCsI.h"
#include "KVINDRADB.h"
using namespace std;


ClassImp(KVINDRAUpDater)
//////////////////////////////////////////////////////////////////////////////////
//
//KVINDRAUpDater
//
//Abstract class implementing necessary methods for setting INDRA parameters
//for each run, using information stored in the KVINDRADB database.
KVINDRAUpDater::KVINDRAUpDater()
{
   //Default ctor for KVINDRAUpDater object.
}

//_______________________________________________________________//

void KVINDRAUpDater::SetParameters(UInt_t run)
{
   //Set the parameters of INDRA for this run
   //This will:
   //      set the multiplicity trigger of gIndra using the database value for the run
   //      set special detector gains for run (if any)
   //      set the target corresponding to the run
   //      set the ChIo pressures for the run
   //      set calibration parameters for the run
   //      set identification parameters for the run
   //

   cout << "Setting parameters of INDRA array for run " << run << ":" <<
        endl;
   KVDBRun* kvrun = gIndraDB->GetRun(run);
   if (!kvrun) {
      Error("SetParameters(UInt_t)", "Run %u not found in database!", run);
      return;
   }
   SetTrigger(kvrun);
   SetTarget(kvrun);

   CheckStatusOfDetectors(kvrun);

   SetGains(kvrun);
   SetChIoPressures(kvrun);
   SetCalibrationParameters(run);
   SetIdentificationParameters(run);

}

//_______________________________________________________________//

void KVINDRAUpDater::SetCalibrationParameters(UInt_t run)
{
   //Set calibration parameters for this run.
   //This will:
   //      reset all the calibrators of all the detectors ready to receive the calibrators for the run (handled by child classes),
   //      set calibration parameters for the run
   //      set pedestals for the run
   //      set PHD parameters of silicon detectors for run

   cout << "Setting calibration parameters of INDRA array for run " << run << ":" <<
        endl;
   KVDBRun* kvrun = gIndraDB->GetRun(run);
   if (!kvrun) {
      Error("SetParameters(UInt_t)", "Run %u not found in database!", run);
      return;
   }
   //Reset all calibrators of all detectors first
   TIter next(gIndra->GetListOfDetectors());
   KVDetector* kvd;
   KVCalibrator* kvc;
   while ((kvd = (KVDetector*) next())) {
      if (kvd->GetListOfCalibrators()) {
         TIter next_cal(kvd->GetListOfCalibrators());
         while ((kvc = (KVCalibrator*) next_cal())) {
            kvc->Reset();
         }
      }
   }
   SetCalibParameters(kvrun);
   SetPedestals(kvrun);
   SetPHDs(kvrun);
}

//_______________________________________________________________//
void KVINDRAUpDater::SetTrigger(KVDBRun* kvrun)
{
   //Set trigger used during this run

   cout << "--> Setting Trigger:" << endl;
   gIndra->SetTrigger(kvrun->GetTrigger());
   cout << "      M>=" << (Int_t)gIndra->GetTrigger() << endl;
}


//_______________________________________________________________//
void KVINDRAUpDater::CheckStatusOfDetectors(KVDBRun* kvrun)
{

   KVRList* absdet;// = kvrun->GetLinks("Absent Detectors");
   KVRList* oooacq;// = kvrun->GetLinks("OoO ACQPars");
   KVRList* ooodet;// = kvrun->GetLinks("OoO Detectors");

   TIter next(gIndra->GetListOfDetectors());
   KVDetector* det;
   KVACQParam* acq;

   Int_t ndet_absent = 0;
   Int_t ndet_ooo = 0;
   Int_t nacq_ooo = 0;

   while ((det = (KVDetector*)next())) {
      //Test de la presence ou non du detecteur
      if (!absdet) {
         det->SetPresent();
      } else {
         if (absdet->FindObject(det->GetName(), "Absent Detector")) {
            det->SetPresent(kFALSE);
            ndet_absent += 1;
         } else {
            det->SetPresent();
         }
      }
      if (det->IsPresent()) {
         //Test du bon fonctionnement ou non du detecteur
         if (!ooodet) {
            det->SetDetecting();
         } else {
            if (ooodet->FindObject(det->GetName(), "OoO Detector")) {
               det->SetDetecting(kFALSE);
               ndet_ooo += 1;
            } else {
               det->SetDetecting();
            }
         }
         //Test du bon fonctionnement ou non des parametres d acquisition
         if (det->IsDetecting()) {
            TIter next_acq(det->GetACQParamList());
            if (!oooacq) {
               while ((acq = (KVACQParam*)next_acq())) {
                  acq->SetWorking();
               }
            } else {
               Int_t noff = 0;
               while ((acq = (KVACQParam*)next_acq())) {
                  if (oooacq->FindObject(acq->GetName(), "OoO ACQPar")) {
                     acq->SetWorking(kFALSE);
                     noff += 1;
                     nacq_ooo += 1;
                  } else {
                     acq->SetWorking();
                  }
               }
               if (noff == 3) {
                  det->SetDetecting(kFALSE);
                  ndet_ooo += 1;
                  nacq_ooo -= 3;
               }
            }
         }
      }
   }

   Info("KVINDRAUpDater", "%d detecteurs absents", ndet_absent);
   Info("KVINDRAUpDater", "%d detecteurs ne fonctionnent pas", ndet_ooo);
   Info("KVINDRAUpDater", "%d parametres d acquisition ne fonctionnent pas", nacq_ooo);



}
//_______________________________________________________________//

void KVINDRAUpDater::SetGains(KVDBRun* kvrun)
{
   //Set gains used during this run
   //First all detector gains are set to 1.
   //Then, any detectors for which a different gain has been defined
   //will have its gain set to the value for the run
   TIter next(gIndra->GetListOfDetectors());
   KVDetector* kvd;
   while ((kvd = (KVDetector*) next()))
      kvd->SetGain(1.00);

   KVNameValueList gains = gIndraDB->GetGains(kvrun->GetNumber());
   if (!gains.GetNpar()) return;

   cout << "--> Setting Gains:" << endl;
   Int_t ndets = gains.GetNpar();
   cout << "      Setting gains for " << ndets << " detectors : " << endl;
   for (int i = 0; i < ndets; i++) {
      kvd = gIndra->GetDetector(gains.GetNameAt(i));
      if (kvd) {
         kvd->SetGain(gains.GetDoubleValue(i));
         cout << "             " << kvd->GetName() << " : G=" << kvd->
              GetGain() << endl;
      } else {
         Error("SetGains", "Detector %s is not present", gains.GetNameAt(i));
      }
   }
}

//________________________________________________________________________________________________
//
// void KVINDRAUpDater::SetIDGrids(UInt_t run)
// {
//     // Use global ID grid manager gIDGridManager to set identification grids for all
//     // ID telescopes for this run. First, any previously set grids are removed.
//     // Then all grids for current run are set in the associated ID telescopes.
//
//     cout << "--> Setting Identification Grids" << endl;
//     TIter next_idt(gMultiDetArray->GetListOfIDTelescopes());
//     KVIDTelescope *idt;
//     while ((idt = (KVIDTelescope *) next_idt()))
//     {
//         idt->RemoveGrids();
//     }
//     gIDGridManager->SetGridsInTelescopes(run);
// }

//_____________________________________________________________________________

void KVINDRAUpDater::SetChIoPressures(KVDBRun* kvrun)
{
   //Update ChIo pressures for this run with values in database (if any)
   //
   //if pressure is equal to 0 (no gas)
   //mark the corresponding ChIo's as non detecting detector (see KVDetector::SetDetecting())
   //

   KVChIo* kvd;
   KVDBChIoPressures kvps = gIndraDB->GetChIoPressures(kvrun->GetNumber());

   KVSeqCollection* chios = gIndra->GetListOfChIo();
   cout << "--> Setting ChIo pressures" << endl;
   TIter next_chio(chios);
   cout << "       Ring 2/3: " << kvps.GetPressure(CHIO_2_3) << "mbar" << endl;
   cout << "       Ring 4/5: " << kvps.GetPressure(CHIO_4_5) << "mbar" << endl;
   cout << "       Ring 6/7: " << kvps.GetPressure(CHIO_6_7) << "mbar" << endl;
   cout << "      Ring 8/12: " << kvps.GetPressure(CHIO_8_12) << "mbar" << endl;
   cout << "     Ring 13/17: " << kvps.GetPressure(CHIO_13_17) << "mbar" << endl;
   while ((kvd = (KVChIo*) next_chio())) {
      if (!strcmp(kvd->GetType(), "CI")) {
         //check detector type: ="CI" for standard INDRA chio
         if (kvd->GetRingNumber() == 2)
            kvd->SetPressure(kvps.GetPressure(CHIO_2_3));
         if (kvd->GetRingNumber() == 4)
            kvd->SetPressure(kvps.GetPressure(CHIO_4_5));
         if (kvd->GetRingNumber() == 6)
            kvd->SetPressure(kvps.GetPressure(CHIO_6_7));
         if (kvd->GetRingNumber() >= 8 && kvd->GetRingNumber() <= 12)
            kvd->SetPressure(kvps.GetPressure(CHIO_8_12));
         if (kvd->GetRingNumber() >= 13 && kvd->GetRingNumber() <= 17)
            kvd->SetPressure(kvps.GetPressure(CHIO_13_17));
         if (kvd->GetPressure() == 0.0) {
            kvd->SetDetecting(kFALSE);
         } else {
            kvd->SetDetecting(kTRUE);
         }
      }
   }
}

//______________________________________________________________________________

void KVINDRAUpDater::SetChVoltParameters(KVDBRun* kvrun)
{
   // Read and set channel<->volt conversion parameters for ChIo, Si and Etalon detectors for run
   // For ChIo & Si detectors, the name of the table to use is in column "ElectronicCalibration" of
   // table "Calibrations". For etalon detectors, it is in column "ElectronicCalibration.Etalons".

   gIndraDB->GetDB().select_data("Calibrations", Form("\"Run Number\"=%d", kvrun->GetNumber()));
   TString calib_table[2];
   while (gIndraDB->GetDB().get_next_result()) {
      calib_table[0] = gIndraDB->GetDB()["Calibrations"]["ElectronicCalibration"].data().GetString();
      calib_table[1] = gIndraDB->GetDB()["Calibrations"]["ElectronicCalibration.Etalons"].data().GetString();
   }

   for (int calib = 0; calib < 2; ++calib) {
      gIndraDB->GetDB().select_data(calib_table[calib]);
      KVSQLite::table& caltab = gIndraDB->GetDB()[calib_table[calib].Data()];

      while (gIndraDB->GetDB().get_next_result()) {

         KVDetector* kvd = gIndra->GetDetector(caltab["detName"].data().GetString());
         if (!kvd)
            Warning("SetChVoltParameters(UInt_t)", "Dectector %s not found !",
                    caltab["detName"].data().GetString());
         else {                    // detector found
            KVCalibrator* kvc = kvd->GetCalibrator(caltab["parName"].data().GetString(), caltab["type"].data().GetString());
            if (!kvc)
               Warning("SetChVoltParameters(UInt_t)",
                       "Calibrator %s %s not found !", caltab["parName"].data().GetString(), caltab["type"].data().GetString());
            else {                 //calibrator found
               Int_t npars = caltab["npar"].data().GetInt();
               if (npars != kvc->GetNumberParams())
                  Warning("SetChVoltParameters(UInt_t)",
                          "Mismatch for calibrator with %d parameters, database provides %d",
                          kvc->GetNumberParams(), npars);
               Int_t imax = TMath::Min(npars, kvc->GetNumberParams());
               for (Int_t i = 0; i < imax; i++) {
                  kvc->SetParameter(i, caltab[Form("a%d", i)].data().GetDouble());
               }
               kvc->SetStatus(kTRUE);   // calibrator ready
            }                      //calibrator found
         }                         //detector found
      }                            //boucle sur les parameters
   }
}

//______________________________________________________________________________

void KVINDRAUpDater::SetVoltEnergyChIoSiParameters(KVDBRun* kvrun)
{
   // Read and set volt<-->mev conversion parameters for ChIo, Si and Etalon detectors for run
   // The name of the table to use is in column "ChIoSiVoltMeVCalib" of table "Calibrations".

   gIndraDB->GetDB().select_data("Calibrations", Form("\"Run Number\"=%d", kvrun->GetNumber()));
   TString calib_table;
   while (gIndraDB->GetDB().get_next_result())
      calib_table = gIndraDB->GetDB()["Calibrations"]["ChIoSiVoltMeVCalib"].data().GetString();

   // Setting Channel-Volts calibration parameters
   KVSQLite::table& caltab = gIndraDB->GetDB()[calib_table.Data()];
   gIndraDB->GetDB().select_data(calib_table.Data());

   while (gIndraDB->GetDB().get_next_result()) {

      KVDetector* kvd = gIndra->GetDetector(caltab["detName"].data().GetString());
      if (!kvd) {
         /*
         Warning("SetVoltEnergyParameters(UInt_t)",
                 "Dectector %s not found !", kvps->GetName());
         */
      } else {                  // detector found
         KVCalibrator* kvc = kvd->GetCalibrator(kvd->GetName(), "Volt-Energy");
         if (!kvc)
            Warning("SetVoltEnergyParameters(UInt_t)",
                    "Detector %s: Volt-Energy calibrator not found !", kvd->GetName());
         else {                 //calibrator found
            Int_t npars = caltab["npar"].data().GetInt();
            if (npars != kvc->GetNumberParams())
               Warning("SetVoltEnergyChIoSiParameters(UInt_t)",
                       "Mismatch for calibrator with %d parameters, database provides %d",
                       kvc->GetNumberParams(), npars);
            Int_t imax = TMath::Min(npars, kvc->GetNumberParams());
            for (Int_t i = 0; i < imax; i++) {
               kvc->SetParameter(i, caltab[Form("a%d", i)].data().GetDouble());
            }
            kvc->SetStatus(kTRUE);      // calibrator ready
         }                      //calibrator found
      }                         //detector found
   }                            //boucle sur les parameters
}

//______________________________________________________________________________

void KVINDRAUpDater::SetCalibParameters(KVDBRun* run)
{
   SetChVoltParameters(run);
   SetVoltEnergyChIoSiParameters(run);
   SetLitEnergyCsIParameters(run);
   SetCsIGainCorrectionParameters(run);
}

//____________________________________________________________________________________

void KVINDRAUpDater::SetPedestals(KVDBRun* kvrun)
{
   //Set pedestals for this run

   SetChIoSiPedestals(kvrun);
   SetCsIPedestals(kvrun);

}

//______________________________________________________________________________

void KVINDRAUpDater::SetCsIGainCorrectionParameters(KVDBRun* kvrun)
{
   // Sets KVCsI::fGainCorrection data member, used by KVCsI::GetCorrectedLumiereTotale
   // to return the total light output corrected by a run-dependent factor.
   // We set all detectors' correction to 1, then set the corrections defined for this
   // run, if any.

   TIter next_csi(gIndra->GetListOfCsI());
   KVCsI* csi;
   while ((csi = (KVCsI*)next_csi())) {
      csi->SetTotalLightGainCorrection(1.0);
   }

   KVRList* param_list;// = kvrun->GetLinks("CsIGainCorr");
   if (!param_list) {
      return;
   }
   if (!param_list->GetSize()) {
      return;
   }

   TIter next_ps(param_list);
   KVDBParameterSet* dbps;
   while ((dbps = (KVDBParameterSet*)next_ps())) {

      csi = (KVCsI*)gIndra->GetDetector(dbps->GetName());
      if (!csi) {
         // the name of the parameter set should be the name of the detector;
         // however, it may be the name of an acquisition parameter associated with
         // the detector!
         KVACQParam* a = gIndra->GetACQParam(dbps->GetName());
         if (a) csi = (KVCsI*)a->GetDetector();
         // still no good ?
         if (!csi) {
            Warning("SetCsIGainCorrectionParameters",
                    "Cannot find detector associated with %s", dbps->GetName());
            continue;
         }
      }
      csi->SetTotalLightGainCorrection(dbps->GetParameter());
      Info("SetCsIGainCorrectionParameters", "%s gain correction = %f", csi->GetName(), csi->GetTotalLightGainCorrection());
   }
}

//______________________________________________________________________________

void KVINDRAUpDater::SetLitEnergyCsIParameters(KVDBRun* kvrun)
{

   Info("SetLitEnergyCsIParameters", "Setting CsI calibrations...");

   gIndraDB->GetDB().select_data("Calibrations", Form("\"Run Number\"=%d", kvrun->GetNumber()));
   TString calib_table[2];
   while (gIndraDB->GetDB().get_next_result()) {
      calib_table[0] = gIndraDB->GetDB()["Calibrations"]["CalibCsI.Z_eq_1"].data().GetString();
      calib_table[1] = gIndraDB->GetDB()["Calibrations"]["CalibCsI.Z_gt_1"].data().GetString();
   }

   for (int calib = 0; calib < 2; ++calib) {
      gIndraDB->GetDB().select_data(calib_table[calib]);
      KVSQLite::table& caltab = gIndraDB->GetDB()[calib_table[calib].Data()];

      while (gIndraDB->GetDB().get_next_result()) {

         KVDetector* kvd = gIndra->GetDetector(caltab["detName"].data().GetString());
         if (!kvd)
            Warning("SetLitEnergyCsIParameters(UInt_t)",
                    "Dectector %s not found !", caltab["detName"].data().GetString());
         else {                    // detector found
            KVCalibrator* kvc = kvd->GetCalibrator(caltab["type"].data().GetString());
            if (!kvc) {
               Warning("SetLitEnergyCsIParameters(UInt_t)",
                       "Calibrator %s %s not found ! - it will be created",
                       kvd->GetName(), caltab["type"].data().GetString());
               kvd->SetCalibrators();
               kvc = kvd->GetCalibrator(caltab["type"].data().GetString());
            }
            Int_t npars = caltab["npar"].data().GetInt();
            if (npars != kvc->GetNumberParams())
               Warning("SetLitEnergyCsIParameters(UInt_t)",
                       "Mismatch for calibrator with %d parameters, database provides %d",
                       kvc->GetNumberParams(), npars);
            Int_t imax = TMath::Min(npars, kvc->GetNumberParams());
            for (Int_t i = 0; i < imax; i++) {
               kvc->SetParameter(i, caltab[Form("a%d", i)].data().GetDouble());
            }
            kvc->SetStatus(kTRUE);      // calibrator ready
         }                         //detector found
      }                            //boucle sur les parameters
   }
}

//______________________________________________________________________________

void KVINDRAUpDater::SetChIoSiPedestals(KVDBRun* kvrun)
{
   //read Chio-Si-Etalons pedestals

//   if (!kvrun->GetKey("Pedestals"))
//      return;
//   if (!kvrun->GetKey("Pedestals")->GetLinks())
//      return;
//   if (!kvrun->GetKey("Pedestals")->GetLinks()->At(0))
//      return;

   ifstream file_pied_chiosi;
//   if (!KVBase::
//         SearchAndOpenKVFile(kvrun->GetKey("Pedestals")->GetLinks()->At(0)->
//                             GetName(), file_pied_chiosi, fDataSet.Data())) {
//      Error("SetPedestals", "Problem opening file %s",
//            kvrun->GetKey("Pedestals")->GetLinks()->At(0)->GetName());
//      return;
//   }
//   cout << "--> Setting Pedestals" << endl;
//   cout << "    ChIo/Si/Etalons: " << kvrun->GetKey("Pedestals")->
//        GetLinks()->At(0)->GetName() << endl;

   //skip first 5 lines - header
   TString line;
   for (int i = 5; i; i--) {
      line.ReadLine(file_pied_chiosi);
   }

   int cou, mod, type, n_phys, n_gene;
   float ave_phys, sig_phys, ave_gene, sig_gene;

   while (file_pied_chiosi.good()) {

      file_pied_chiosi >> cou >> mod >> type >> n_phys >> ave_phys >>
                       sig_phys >> n_gene >> ave_gene >> sig_gene;

      KVDetector* det = gIndra->GetDetectorByType(cou, mod, type);
      if (det) {
         switch (type) {

            case ChIo_GG:

               det->SetPedestal("GG", ave_gene);
               break;

            case ChIo_PG:

               det->SetPedestal("PG", ave_gene);
               break;

            case Si_GG:

               det->SetPedestal("GG", ave_gene);
               break;

            case Si_PG:

               det->SetPedestal("PG", ave_gene);
               break;

            case SiLi_GG:

               det->SetPedestal("GG", ave_gene);
               break;

            case SiLi_PG:

               det->SetPedestal("PG", ave_gene);
               break;

            case Si75_GG:

               det->SetPedestal("GG", ave_gene);
               break;

            case Si75_PG:

               det->SetPedestal("PG", ave_gene);
               break;

            default:

               break;
         }
      }
   }
   file_pied_chiosi.close();
}

//______________________________________________________________________________

void KVINDRAUpDater::SetCsIPedestals(KVDBRun* kvrun)
{
//   if (!kvrun->GetKey("Pedestals"))
//      return;
//   if (!kvrun->GetKey("Pedestals")->GetLinks())
//      return;
//   if (!kvrun->GetKey("Pedestals")->GetLinks()->At(1))
//      return;

   //read CsI pedestals
   ifstream file_pied_csi;
//   if (!KVBase::
//         SearchAndOpenKVFile(kvrun->GetKey("Pedestals")->GetLinks()->At(1)->
//                             GetName(), file_pied_csi, fDataSet.Data())) {
//      Error("SetPedestals", "Problem opening file %s",
//            kvrun->GetKey("Pedestals")->GetLinks()->At(1)->GetName());
//      return;
//   }
//   cout << "--> Setting Pedestals" << endl;
//   cout << "    CsI            : " << kvrun->GetKey("Pedestals")->
//        GetLinks()->At(1)->GetName() << endl;

   int cou, mod, type, n_phys, n_gene;
   float ave_phys, sig_phys, ave_gene, sig_gene;
   TString line;

   //skip first 5 lines - header
   for (int i = 5; i; i--) {
      line.ReadLine(file_pied_csi);
   }

   while (file_pied_csi.good()) {

      file_pied_csi >> cou >> mod >> type >> n_phys >> ave_phys >> sig_phys
                    >> n_gene >> ave_gene >> sig_gene;

      KVDetector* det = gIndra->GetDetectorByType(cou, mod, type);
      if (det) {
         switch (type) {

            case CsI_R:

               det->SetPedestal("R", ave_gene);
               break;

            case CsI_L:

               det->SetPedestal("L", ave_gene);
               break;

            default:

               break;
         }
      }
   }
   file_pied_csi.close();
}

//_______________________________________________________________________________________

void KVINDRAUpDater::SetPHDs(KVDBRun*)
{
   //If the environment variable
   //   name_of_dataset.INDRADB.PHD:      name_of_file
   //is set, then the corresponding file (which must be in $KVROOT/KVFiles/name_of_dataset)
   //is read and used to set the (Moulton) pulse-height defect parameters of all silicon
   //detectors.

   TString phdfile = gIndraDB->GetDBEnv("PHD");

   if (phdfile != "") {
      cout << "--> Setting Si pulse height defect parameters (Moulton)" << endl;

      //get full path to file
      TString path;
      if (KVBase::SearchKVFile(phdfile.Data(), path, gDataSet->GetName())) {
         //read file with a TEnv
         TEnv phds;
         if (phds.ReadFile(path.Data(), kEnvLocal) != 0) {
            Error("SetPHDs", "TEnv::ReadFile != 0, cannot read PHD file");
         }
         //loop over all silicons
         TIter next_si(gIndra->GetListOfSi());
         KVSilicon* si;
         while ((si = (KVSilicon*)next_si())) {
            Int_t group = phds.GetValue(si->GetName(), 0);
            if (group) {
               Double_t p1 = phds.GetValue(Form("Group%d.p1", group), 0.0);
               Double_t p2 = phds.GetValue(Form("Group%d.p2", group), 0.0);
               //set parameters for this detector
               //using si->SetMoultonPHDParameters(Double_t a_1, Double_t a_2, Double_t b_1, Double_t b_2)
               //in our case,
               //   a_1 = 0.0223     a_2 = 0.5682   b_1 = p2  b_2 = p1
               si->SetMoultonPHDParameters(0.0223, 0.5682, p2, p1);
            }
         }
         //set flag in INDRA to say this has been done
         gIndra->PHDSet();
      } else {
         Error("SetPHDs", "File %s not found", phdfile.Data());
      }
   }
}
