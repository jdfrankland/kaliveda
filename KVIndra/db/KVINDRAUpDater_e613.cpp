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

