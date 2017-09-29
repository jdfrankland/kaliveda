//Created by KVClassFactory on Wed Nov 16 14:10:43 2011
//Author: bonnet

#include "KVINDRAUpDater_e613.h"
#include "KVDBParameterSet.h"
#include "KVINDRA.h"
#include "KVINDRADB.h"
#include "KVDetector.h"
#include "KVACQParam.h"
#include "KVCalibrator.h"
#include "KVChannelVolt.h"

ClassImp(KVINDRAUpDater_e613)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVINDRAUpDater_e613</h2>
<h4>Sets run parameters for INDRA_e613 dataset</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVINDRAUpDater_e613::KVINDRAUpDater_e613()
{
   // Default constructor
}

KVINDRAUpDater_e613::~KVINDRAUpDater_e613()
{
   // Destructor
}

//_______________________________________________________________//

void KVINDRAUpDater_e613::SetGains(KVDBRun* kvrun)
{
   //Set gains used during this run
   //Read gains in database just print detectors for which gains have changed

   KVNameValueList gain_list = gIndraDB->GetGains(kvrun->GetNumber());
   if (gain_list.IsEmpty()) {
      return;
      Warning("SetGains", "No gains defined for this run in database");
   }
   Int_t ndets = gain_list.GetNpar();
   Info("SetGains", "Loop on %d detectors : ...", ndets);

   Int_t nchange = 0;
   KVDetector* kvd = 0;
   Double_t oldgain;
   TString list;
   for (Int_t i = 0; i < ndets; i++) {
      KVNamedParameter* dbps = gain_list.GetParameter(i);
      kvd = gIndra->GetDetector(dbps->GetName());
      if (!kvd) {
         //Error("SetGains",
         //      "Detector %s is unknown or does not exist at the current time...???",
         //   dbps->GetName());
         continue;
      } else {
         oldgain = kvd->GetGain();
         if (oldgain != dbps->GetDouble()) {
            kvd->SetGain(dbps->GetDouble());
            //cout << "            " << kvd->GetName() << " set gain from " << oldgain << " to G=" << kvd->GetGain() << endl;
            list += kvd->GetName();
            list += ",";
            nchange += 1;
         }
      }
   }
   if (nchange == 0)
      Info("SetGains", "Gains of the %d detectors are the same as the run before ", ndets);
   else
      Info("SetGains", "Gains have been changed for %d detectors (total = %d)", nchange, ndets);

}

////_______________________________________________________________//

void KVINDRAUpDater_e613::SetPedestals(KVDBRun* kvrun)
{
   //Set pedestals for this run

   gIndraDB->select_runs_in_dbtable("Calibrations", kvrun->GetNumber(), "Pedestals_ChIoSi,Pedestals_CsI");
   TString ped_chio_tab, ped_csi_tab;
   while (gIndraDB->GetDB().get_next_result()) {
      ped_chio_tab = gIndraDB->GetDB()["Calibrations"]["Pedestals_ChIoSi"].data().GetString();
      ped_csi_tab = gIndraDB->GetDB()["Calibrations"]["Pedestals_CsI"].data().GetString();
   }
   if (ped_chio_tab == "" && ped_csi_tab == "") {
      Warning("SetPedestals", "No pedestals defined for this run in database");
      return;
   }

   Int_t nchange(0), ndets(0);
   TString list;

   gIndraDB->GetDB().select_data(ped_chio_tab);
   KVSQLite::table& ped_chio = gIndraDB->GetDB()[ped_chio_tab];
   while (gIndraDB->GetDB().get_next_result()) {
      KVACQParam* acq = gIndra->GetACQParam(ped_chio["detName"].data().GetString());
      if (!acq) {
         Error("SetPedestals", "ACQ Parameter not defined %s", ped_chio["detName"].data().GetString());
      } else {
         ++ndets;
         Float_t oldped = acq->GetPedestal();
         Float_t newped = ped_chio["pedestal"].data().GetDouble();
         if (oldped != newped) {
            acq->SetPedestal(newped);

            list += acq->GetName();
            list += ",";
            ++nchange;
         }
      }
   }
   gIndraDB->GetDB().select_data(ped_csi_tab);
   KVSQLite::table& ped_csi = gIndraDB->GetDB()[ped_csi_tab];
   while (gIndraDB->GetDB().get_next_result()) {
      KVACQParam* acq = gIndra->GetACQParam(ped_csi["detName"].data().GetString());
      if (!acq) {
         Error("SetPedestals", "ACQ Parameter not defined %s", ped_csi["detName"].data().GetString());
      } else {
         ++ndets;
         Float_t oldped = acq->GetPedestal();
         Float_t newped = ped_csi["pedestal"].data().GetDouble();
         if (oldped != newped) {
            acq->SetPedestal(newped);

            list += acq->GetName();
            list += ",";
            ++nchange;
         }
      }
   }
   if (nchange == 0)
      Info("SetGains", "Pedestals of the %d acquisition parameters are the same than the run before ", ndets);
   else
      Info("SetGains", "Pedestals have been changed for %d acquisition parameters (total = %d)", nchange, ndets);

}
//______________________________________________________________________________

//void KVINDRAUpDater_e613::SetChVoltParameters(KVDBRun* kvrun)
//{


//   KVRList* param_list = kvrun->GetLinks("Channel-Volt");
//   if (!param_list)
//      return;
//   if (!param_list->GetSize())
//      return;

//   KVDetector* kvd;
//   KVDBParameterSet* kvps;
//   KVCalibrator* kvc;
//   TIter next_ps(param_list);


//   TString str;

//   // Setting Channel-Volts calibration parameters
//   while ((kvps = (KVDBParameterSet*) next_ps())) {     // boucle sur les parametres
//      str = kvps->GetName();
//      str.Remove(str.Sizeof() - 4, 3);  //Removing 3 last letters (ex : "_PG")
//      kvd = gIndra->GetDetector(str.Data());
//      if (!kvd) {
//         //    Warning("SetChVoltParameters(UInt_t)", "Dectector %s not found !",
//         //            str.Data());
//      } else {                  // detector found
//         kvc = kvd->GetCalibrator(kvps->GetName(), kvps->GetTitle());
//         if (!kvc)
//            Warning("SetChVoltParameters(UInt_t)",
//                    "Calibrator %s %s not found !", kvps->GetName(),
//                    kvps->GetTitle());
//         else {                 //calibrator found
//            //Prise en compte du gain du detecteur quand la rampe gene a ete faite
//            //pour ponderation des coef dans KVChannelVolt
//            Double_t gain_ref = kvps->GetParameter(kvc->GetNumberParams());
//            ((KVChannelVolt*)kvc)->SetGainRef(gain_ref);
//            for (Int_t i = 0; i < kvc->GetNumberParams(); i++) {
//               kvc->SetParameter(i, kvps->GetParameter(i));
//            }
//            kvc->SetStatus((gain_ref != 0)); // calibrator ready
//         }                      //calibrator found
//      }                         //detector found
//   }                            //boucle sur les parameters
//}
////_______________________________________________________________//

void KVINDRAUpDater_e613::SetParameters(UInt_t run)
{
   // Do some e613 run-dependent fixing and bodging
   //
   // - for runs < 559, the cables of CsI detectors 2.9 and 3.10 were interchanged
   //     for these runs, we change the names of the corresponding acquisition
   //     parameters so that correct data are associated to each detector

   // inversion cables CsI 2.9 & 3.10
   if (run < 559) {
      KVDetector* d = gIndra->GetDetector("CSI_0209");
      d->GetACQParam("R")->SetName("CSI_0310_R");
      d->GetACQParam("L")->SetName("CSI_0310_L");
      d->GetACQParam("T")->SetName("CSI_0310_T");
      d = gIndra->GetDetector("CSI_0310");
      d->GetACQParam("R")->SetName("CSI_0209_R");
      d->GetACQParam("L")->SetName("CSI_0209_L");
      d->GetACQParam("T")->SetName("CSI_0209_T");
      ((KVHashList*)gIndra->GetACQParams())->Rehash();
   } else {
      KVDetector* d = gIndra->GetDetector("CSI_0209");
      d->GetACQParam("R")->SetName("CSI_0209_R");
      d->GetACQParam("L")->SetName("CSI_0209_L");
      d->GetACQParam("T")->SetName("CSI_0209_T");
      d = gIndra->GetDetector("CSI_0310");
      d->GetACQParam("R")->SetName("CSI_0310_R");
      d->GetACQParam("L")->SetName("CSI_0310_L");
      d->GetACQParam("T")->SetName("CSI_0310_T");
      ((KVHashList*)gIndra->GetACQParams())->Rehash();
   }
   KVINDRAUpDater::SetParameters(run);
}

