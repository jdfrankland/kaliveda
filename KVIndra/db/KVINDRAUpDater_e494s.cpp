//Created by KVClassFactory on Thu Oct 11 18:23:43 2012
//Author: Dijon Aurore

#include "KVINDRAUpDater_e494s.h"
#include "KVINDRA.h"
#include "KVMultiDetArray.h"
#include "KVIDGridManager.h"
#include "KVRTGIDManager.h"
#include "KVDBParameterSet.h"
#include "KVINDRADetector.h"
#include "KVINDRADB.h"
#include "KVSilicon.h"
using namespace std;

ClassImp(KVINDRAUpDater_e494s)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVINDRAUpDater_e494s</h2>
<h4>Class for setting INDRA-VAMOS parameter for each run (e494s/e503 experiment)</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVINDRAUpDater_e494s::KVINDRAUpDater_e494s()
{
   // Default constructor
}
//________________________________________________________________

KVINDRAUpDater_e494s::~KVINDRAUpDater_e494s()
{
   // Destructor
}
//________________________________________________________________

void KVINDRAUpDater_e494s::SetIDGrids(UInt_t run){
	// Set identification grid or identification function (KVTGID) 
	// for all ID telescopes for this run. 
    // The global ID grid manager gIDGridManager is used to set the
    // grids. The static function KVRTGIDManager::SetIDFuncInTelescopes
    //  is used to set the ID functions. First, any previously set grids/functions are removed.
    // Then all grids/functions for current run are set in the associated ID telescopes.

	Info("KVINDRAUpDater_e494s::SetIDGrids","Setting Identification Grids/Functions");
    TIter next_idt(gMultiDetArray->GetListOfIDTelescopes());

    KVIDTelescope  *idt    = NULL;
	KVRTGIDManager *rtgidm = NULL;

    while ((idt = (KVIDTelescope *) next_idt()))
    {
        idt->RemoveGrids();
		if(idt->InheritsFrom("KVRTGIDManager")){
 			rtgidm = (KVRTGIDManager *)idt->IsA()->DynamicCast(KVRTGIDManager::Class(),idt);
 			rtgidm->RemoveAllTGID();
		}

    }
    gIDGridManager->SetGridsInTelescopes(run);
	KVRTGIDManager::SetIDFuncInTelescopes(run);
}
//________________________________________________________________

void KVINDRAUpDater_e494s::SetParameters(UInt_t run){

	KVINDRAUpDater::SetParameters(run);
   	SetChVoltRefGains();
	KVDBRun *kvrun = gIndraDB->GetRun(run);
	if(!kvrun) return;
   	SetPedestalCorrections(kvrun);
}
//________________________________________________________________

void KVINDRAUpDater_e494s::SetPedestalCorrections(KVDBRun *run){
	// For any detectors, having a pedestal correction  defined
	// for their associated QDC and  one of their signals, will have 
	// their correction corresponding to the signal, set to the value 
	// for the run.

	Info("KVINDRAUpDater_e494s::SetPedestalCorrections","Setting pedestal corrections");

	KVRList *dp_list = run->GetLinks("DeltaPedestal");
	if(!dp_list) return;

	KVDBParameterSet *dp = NULL;
	TIter next_dp(dp_list);
	TString QDCnum;

	// Loop over INDRA detectors
	KVINDRADetector *det = NULL;
	TIter next_det(gMultiDetArray->GetListOfDetectors());
	while( (det = (KVINDRADetector *)next_det()) ){

		// Initializing each ACQ parameter pedestal correction for
		// all detectors
		KVACQParam *acqpar = NULL;
		TIter next_acqp(det->GetACQParamList());
		while( (acqpar = (KVACQParam *)next_acqp()) )
			acqpar->SetDeltaPedestal(0.);

		// Set the pedestal correction if a value exists for
		// the QDC associated to this detector
		next_dp.Reset();
		while( (dp = (KVDBParameterSet *)next_dp()) ){
			QDCnum = dp->GetName();
			QDCnum.Remove(0,3);
			if(det->GetNumeroCodeur() == QDCnum.Atoi()){
				acqpar = det->GetACQParam(dp->GetTitle());
				acqpar->SetDeltaPedestal(dp->GetParameter());
			}
		}
	}
}
//________________________________________________________________

void KVINDRAUpDater_e494s::SetPedestals(KVDBRun * kvrun)
{
//Set pedestals for this run   
// modifie MFR nov 2012 : add specific reading for etalons

	Info("KVINDRAUpDater_e494s::SetPedestals","Setting Pedestals");

    SetChIoSiPedestals(kvrun);
    SetSi75SiLiPedestals(kvrun);
    SetCsIPedestals(kvrun);
}
//________________________________________________________________

void KVINDRAUpDater_e494s::SetChIoSiPedestals(KVDBRun * kvrun)
{
//read Chio-Si pedestals
// modified MFR november 2012 : suppress etalons from this reading list

    if (!kvrun->GetKey("Pedestals"))
        return;
    if (!kvrun->GetKey("Pedestals")->GetLinks())
        return;
    if (!kvrun->GetKey("Pedestals")->GetLinks()->At(0))
        return;

    ifstream file_pied_chiosi;
    if (!KVBase::
            SearchAndOpenKVFile(kvrun->GetKey("Pedestals")->GetLinks()->At(0)->
                                GetName(), file_pied_chiosi, fDataSet.Data()))
    {
        Error("KVINDRAUpDater_e494s::SetChIoSiPedestals", "Problem opening file %s",
              kvrun->GetKey("Pedestals")->GetLinks()->At(0)->GetName());
        return;
    }
	Info("KVINDRAUpDater_e494s::SetChIoSiPedestals","Setting ChIo/Si pedestals from file: %s",
			kvrun->GetKey("Pedestals")->GetLinks()->At(0)->GetName());

    TString line;

    int cou, mod, type, n_phys, n_gene;
    float ave_phys, sig_phys, ave_gene, sig_gene;

    while (file_pied_chiosi.good())  {
      line.ReadLine(file_pied_chiosi);

      if( (line.Sizeof() >1) && !(line.BeginsWith("#") ) )  {
	if( sscanf(line.Data(),"%d %d %d %d %f %f %d %f %f", &cou, &mod, &type,
	    &n_phys, &ave_phys, &sig_phys, &n_gene, &ave_gene, &sig_gene) !=9){
	  Warning("KVINDRAUpDater_e494s::SetChIoSiPedestals"
			  ,"Bad Format in line :\n%s\nUnable to read",
		  line.Data());   
	} else  {	
       
	  KVDetector *det = gIndra->GetDetectorByType(cou, mod, type);
	  if (det) {
            switch (type)  {

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

	     default:

                break;
            } // end switch
	  }   //end if det
	}     // end if (line.Data) else
      }       // end line.SizeOf
    }         // end while
  
      
    file_pied_chiosi.close();
}
//________________________________________________________________

void KVINDRAUpDater_e494s::SetSi75SiLiPedestals(KVDBRun * kvrun)
{
//read Etalons pedestals
// new method MFR november 2012 : reading etalons pedestals

    if (!kvrun->GetKey("Pedestals"))
        return;
    if (!kvrun->GetKey("Pedestals")->GetLinks())
        return;
    if (!kvrun->GetKey("Pedestals")->GetLinks()->At(2))
        return;

    ifstream file_pied_etalons;
    if (!KVBase::
            SearchAndOpenKVFile(kvrun->GetKey("Pedestals")->GetLinks()->At(2)->
                                GetName(), file_pied_etalons, fDataSet.Data()))
    {
        Error("KVINDRAUpDater_e494s::SetSi75SiLiPedestals", "Problem opening file %s",
              kvrun->GetKey("Pedestals")->GetLinks()->At(2)->GetName());
        return;
    }

	Info("KVINDRAUpDater_e494s::SetSi75SiLiPedestals","Setting Si75/SiLi pedestals from file: %s",
			kvrun->GetKey("Pedestals")->GetLinks()->At(2)->GetName());

    TString line;

    int cou, mod, type, n_phys, n_gene;
    float ave_phys, sig_phys, ave_gene, sig_gene;

    while (file_pied_etalons.good())  {
      line.ReadLine(file_pied_etalons);
//      cout<<"ligne lue :"<<line.Data()<<endl;
      
      if( (line.Sizeof() >1) && !(line.BeginsWith("#") ) )  {
	if( sscanf(line.Data(),"%d %d %d %d %f %f %d %f %f", &cou, &mod, &type,
	    &n_phys, &ave_phys, &sig_phys, &n_gene, &ave_gene, &sig_gene) !=9){
	  Warning("KVINDRAUpDater_e494s::SetSi75SiLiPedestals"
			  ,"Bad Format in line :\n%s\nUnable to read",
		  line.Data());   
	} else  {	
	  KVDetector *det = gIndra->GetDetectorByType(cou, mod, type);

	  if (det) {
            switch (type) {

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
            }  //end switch
	  } // end if det
	} // end if line.Data else
      } // end line.Begins
    } // end while
	 
    file_pied_etalons.close();
}
 //________________________________________________________________

void KVINDRAUpDater_e494s::SetChVoltRefGains(){
	// For any detector, the gain is already taken into account
	// in the Channel->Volt calibration, then the reference gain of the
	// calibrator (KVChannelVolt) must be equal to the gain. (see
	// KVChannelVolt::Compute()).

	Info("KVINDRAUpDater_e494s::SetChVoltRefGains","Setting channel->Volt calibrator reference gain = detector gain");

	KVDetector *det = NULL;
	TIter next_det(gMultiDetArray->GetListOfDetectors());
	KVSeqCollection *sublist = NULL;
	while(( det = (KVDetector *)next_det() )){
		if( !det->GetListOfCalibrators() ) continue;
		sublist = det->GetListOfCalibrators()->GetSubListWithClass("KVChannelVolt");
		sublist->R__FOR_EACH(KVChannelVolt,SetGainRef)(det->GetGain());
		SafeDelete(sublist);
	}
}
