/***************************************************************************
                          kvINDRADB.cpp  -  description
                             -------------------
    begin                : 9/12 2003
    copyright            : (C) 2003 by J.D. Frankland
    email                : frankland@ganil.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "Riostream.h"
#include "KVINDRA.h"
#include "KVINDRADB.h"
#include "KVINDRADBRun.h"
#include "KVDBSystem.h"
#include "KVDBAlphaPeak.h"
#include "KVDBElasticPeak.h"
#include "KVDBChIoPressures.h"
#include "KVDBParameterSet.h"
#include "KVRunListLine.h"
#include "KVChIo.h"
#include "KVString.h"
#include "KVTarget.h"
#include "TObjArray.h"
#include "TEnv.h"
#include "KVDataSet.h"
#include "KVDataSetManager.h"
#include "KVCsI.h"
#include "TH1.h"
#include "KVNumberList.h"

KVINDRADB* gIndraDB;

////////////////////////////////////////////////////////////////////////////////
//Database containing information on runs, systems, calibrations etc. for an INDRA
//experiment or set of experiments (dataset).
//
//Each dataset is described by a KVDataSet object which is initialised by the KVDataSetManager.
//
//For each dataset, a directory exists under $KVROOT/KVFiles/name_of_dataset,
//where it is assumed the database for this dataset can be found.
//
//If DataBase.root does not exist, KVDataSet will try to rebuild it using the appropriate
//KVINDRADB class (see Plugins in $KVROOT/KVFiles/.kvrootrc).
//
//By default, KVINDRADB::Build() will read any or all of the standard format files Runlist.csv,
//Systems.dat and ChIoPressures.dat which may be found in the dataset's directory.

using namespace std;

ClassImp(KVINDRADB)

void KVINDRADB::init()
{

//   fChIoPressures = AddTable("ChIo Pressures", "Pressures of ChIo");
//   fTapes = AddTable("Tapes", "List of data storage tapes");
//   fCsILumCorr = AddTable("CsIGainCorr", "CsI gain corrections for total light output");
//   fPedestals = AddTable("Pedestals", "List of pedestal files");
//   fChanVolt =
//      AddTable("Channel-Volt",
//               "Calibration parameters for Channel-Volts conversion");
//   fVoltMeVChIoSi =
//      AddTable("Volt-Energy ChIo-Si",
//               "Calibration parameters for ChIo-Si Volts-Energy conversion");
//   fLitEnerCsIZ1 =
//      AddTable("Light-Energy CsI Z=1",
//               "Calibration parameters for CsI detectors");
//   fLitEnerCsI =
//      AddTable("Light-Energy CsI Z>1",
//               "Calibration parameters for CsI detectors");

//   fGains = 0;
//   fAbsentDet = 0;
//   fOoODet = 0;
//   fOoOACQPar = 0;

   fPulserData = 0;
}

KVINDRADB::KVINDRADB(const Char_t* name): KVExpDB(name,
         "INDRA experiment parameter database")
{
   init();
}


KVINDRADB::KVINDRADB(): KVExpDB("KVINDRADB",
                                   "INDRA experiment parameter database")
{
   init();
}

void KVINDRADB::cd()
{

   KVDataBase::cd();
   gIndraDB = this;
}

//___________________________________________________________________________
KVINDRADB::~KVINDRADB()
{
   //reset global pointer gIndraDB if it was pointing to this database

   if (gIndraDB == this)
      gIndraDB = nullptr;
   SafeDelete(fPulserData);
}

//____________________________________________________________________________

KVList* KVINDRADB::GetCalibrationPeaks(Int_t run, KVDetector* detector,
                                       Int_t peak_type, Int_t signal_type,
                                       Double_t peak_energy)
{
   //Use this method to access the 'peaks' (see class KVDBPeak and derivatives)
   //used for the calibration of detectors in a given run.
   //
   //Mandatory argument :
   //      run             :       number of the run for which calibration peaks are valid
   //Optional arguments :
   //      detector        :       detector for which peaks are valid
   //      peak_type       :       type of peak
   //                              peak_type = 1  Thoron alpha peak E=6.062 MeV
   //                              peak_type = 2  Thoron alpha peak E=8.785 MeV
   //                              peak_type = 3  Elastic scattering peak
   //                              peak_type = 4  Thoron alpha peak E=6.062 MeV (no gas in ChIo)
   //                              peak_type = 5  Thoron alpha peak E=8.785 MeV (no gas in ChIo)
   //      signal_type     :    one of the INDRA signal types, see KVINDRA::GetDetectorByType
   //      peak_energy :   nominal energy corresponding to peak (incident energy of projectile
   //                              in case of elastic scattering peak).
   //
   //The peaks are read as and when necessary in the peak database file.
   //
   //USER'S RESPONSIBILITY TO DELETE LIST AFTERWARDS.

   if (!gIndra) {
      Error("GetCalibrationPeaks",
            "No KVINDRA object found. You need to build INDRA before using this method.");
      return 0;
   }

   if (!OpenCalibrationPeakFile()) {
      //Calibration peak file not defined or not found
      Error("GetCalibrationPeaks",
            "Calibration peak file not defined or not found");
      return 0;
   }
   //types of peaks
   enum {
      kAlpha6MeV = 1,
      kAlpha8MeV = 2,
      kElastic = 3,
      kAlpha6MeVSG = 4,
      kAlpha8MeVSG = 5
   };

   TString sline;
   Int_t frun = 0, lrun = 0;
   KVList* peak_list = new KVList();

   //set to true after reading a run range corresponding to 'run'
   Bool_t ok_for_this_run = kFALSE;

   Int_t cour, modu, sign, typ2, first, last, entries;
   Float_t mean, error, sigma, constante;

   while (GetPeakFileStream().good()) {

      sline.ReadLine(GetPeakFileStream());

      if (sline.BeginsWith("Run Range :")) {    // Run Range found

         if (sscanf(sline.Data(), "Run Range : %d %d", &frun, &lrun) != 2) {
            Warning("ReadPeakList()",
                    "Bad format in line :\n%s\nUnable to read run range values",
                    sline.Data());
         } else {
            if (TMath::Range(frun, lrun, run) == run) { //frun <= run <= lrun
               ok_for_this_run = kTRUE;
            } else
               ok_for_this_run = kFALSE;
         }
      }                         //Run Range found
      else if (!sline.BeginsWith("#") && ok_for_this_run) {

         //only analyse (non-commented) lines which follow a valid run range

         if (sscanf(sline.Data(), "%d %d %d %d %f %f %f %d %f %d %d",
                    &cour, &modu, &sign, &typ2, &mean, &error, &sigma,
                    &entries, &constante, &first, &last) != 11) {
            Warning("GetCalibrationPeaks()",
                    "Bad format in line :\n%s\nUnable to read peak parameters",
                    sline.Data());
         } else {               // parameters correctly read

            //find corresponding detector

            KVDetector* pic_det =
               //the chio's in the file are written with the
               //ring,module of the Si/CsI in coinc
               (sign > ChIo_T) ? gIndra->GetDetectorByType(cour, modu, sign)
               : gIndra->GetDetectorByType(cour, modu, CsI_R)->GetChIo();

            //is it the right detector ?
            if (detector && detector != pic_det)
               continue;        //read next line

            //is it the right type of peak ?
            if (peak_type > 0 && peak_type != typ2)
               continue;

            //is it the right signal type ?
            if (signal_type && signal_type != sign)
               continue;

            KVDBPeak* peak = 0;
            KVDBSystem* sys = 0;

            switch (typ2) {     //what type of peak

               case kAlpha8MeVSG:

                  peak = new KVDBAlphaPeak(pic_det->GetName());
                  peak->SetGas(kFALSE);
                  peak->SetEnergy(PEAK_THORON_2);
                  break;

               case kAlpha6MeVSG:

                  peak = new KVDBAlphaPeak(pic_det->GetName());
                  peak->SetGas(kFALSE);
                  peak->SetEnergy(PEAK_THORON_1);
                  break;

               case kElastic:

                  peak = new KVDBElasticPeak(pic_det->GetName());
                  sys = (KVDBSystem*) GetRun(first)->GetSystem();
                  if (sys) {
                     peak->SetEnergy(sys->GetEbeam() * sys->GetAbeam());
                     ((KVDBElasticPeak*) peak)->SetZproj(sys->GetZbeam());
                     ((KVDBElasticPeak*) peak)->SetAproj(sys->GetAbeam());
                  }
                  break;

               case kAlpha8MeV:

                  peak = new KVDBAlphaPeak(pic_det->GetName());
                  peak->SetGas(kTRUE);
                  peak->SetEnergy(PEAK_THORON_2);
                  break;

               case kAlpha6MeV:

                  peak = new KVDBAlphaPeak(pic_det->GetName());
                  peak->SetGas(kTRUE);
                  peak->SetEnergy(PEAK_THORON_1);
                  break;

            }                   //what type of peak

            //test energy of peak if necessary
            if (peak && peak_energy > 0) {
               if (typ2 == kElastic) {
                  if (!
                        ((TMath::
                          Abs(peak->GetEnergy() /
                              (((KVDBElasticPeak*) peak)->GetAproj()) -
                              peak_energy))
                         <= 0.1 * (peak->GetEnergy() /
                                   ((KVDBElasticPeak*) peak)->GetAproj())))
                     continue;  //read next line
               } else {
                  if (!
                        (TMath::Abs(peak->GetEnergy() - peak_energy) <=
                         0.1 * peak->GetEnergy()))
                     continue;
               }
            }

            if (peak) {
               peak->SetSigType(KVINDRADetector::SignalTypes[sign]);
               peak->SetRing(cour);
               peak->SetModule(modu);
               peak->SetParameters(mean, error, sigma, (Double_t) entries,
                                   constante, (Double_t) first,
                                   (Double_t) last);
               peak->SetParamName(0, "Mean");
               peak->SetParamName(1, "Error");
               peak->SetParamName(2, "Sigma");
               peak->SetParamName(3, "Peak integral (entries)");
               peak->SetParamName(4, "Gaussian amplitude");
               peak->SetParamName(5, "First run");
               peak->SetParamName(6, "Last run");
               peak_list->Add(peak);

               //Set gain associated with peak.
               //This is the gain of the detector during the first run used to trace the peak.
               KVDBRun* kvrun = (KVDBRun*) GetRun(first);
               KVRList* param_list = nullptr;//kvrun->GetLinks("Gains");
               if (!param_list) {
                  //no gains defined - everybody has gain=1
                  peak->SetGain(1.00);
               } else {
                  KVDBParameterSet* kvdbps =
                     (KVDBParameterSet*) param_list->
                     FindObjectByName(pic_det->GetName());
                  if (!kvdbps) {
                     //no gain defined for this detector for this run - gain=1
                     peak->SetGain(1.00);
                  } else {
                     peak->SetGain(kvdbps->GetParameter(0));
                  }
               }
            }                   //if (peak)

         }                      //parameters correctly read

      }                         //Run range found

   }                            //while( .good() )

   CloseCalibrationPeakFile();
   return peak_list;
}

//_____________________________________________________________________________

Bool_t KVINDRADB::OpenCalibrationPeakFile()
{
   //Returns kTRUE if calibration peak file is open, connected to
   //ifstream __ifpeaks (access through GetPeakFileStream()), and ready
   //for reading.
   //
   //The file is opened if not already open.
   //The stream is repositioned at the beginning of the file if already open.

   if (GetPeakFileStream().is_open()) {
      Error("OpenCalibrationPeakFile", "File already open");
      return kTRUE;
   }
   return OpenCalibFile("CalibPeaks", __ifpeaks);
}

//____________________________________________________________________________

void KVINDRADB::CloseCalibrationPeakFile()
{
   //Close file containing database of calibration peaks
   __ifpeaks.close();
   __ifpeaks.clear();
}

//____________________________________________________________________________
void KVINDRADB::ReadGainList()
{

   // Read the file listing any detectors whose gain value changes during experiment
   // For each defined range of runs, the corresponding gain settings will be stored
   // in tables called "GainSettings_1", "GainSettings_2", etc.
   // In the "Calibrations" table the name of the table will appear in column "Gains"
   // for all concerned runs.
   // The "GainSettings_x" tables have the structure:
   //   | detName [TEXT] | gain [REAL] |

   gain_list_reader glr(this);
   glr.ReadCalib("Gains", "ReadGainList()", "Reading gains ...");
}

//____________________________________________________________________________

void KVINDRADB::ReadChIoPressures()
{
   //Read ChIo pressures for different run ranges and enter into database.
   //Format of file is:
   //
   //# some comments
   //#which start with '#'
   //Run Range : 6001 6018
   //ChIos 2_3    50.0
   //ChIos 4_5    50.0
   //ChIos 6_7    50.0
   //ChIos 8_12   30.0
   //ChIos 13_17  30.0
   //
   //Pressures (of C3F8) are given in mbar).

   ifstream fin;
   if (!OpenCalibFile("Pressures", fin)) {
      Error("ReadChIoPressures()", "Could not open file %s",
            GetCalibFileName("Pressures"));
      return;
   }
   Info("ReadChIoPressures()", "Reading ChIo pressures parameters...");

   KVString sline;

   KVNumberList runrange;
   Bool_t prev_rr = kFALSE;     //was the previous line a run range indication ?
   Bool_t read_pressure = kFALSE; // have we read any pressures recently ?
   Int_t frun, lrun;

   // set up tables in db
   KVSQLite::table pressures("ChIo Pressures");
   pressures.add_primary_key("id");
   int id = 1;
   pressures.add_column("2_3", "REAL");
   pressures.add_column("4_5", "REAL");
   pressures.add_column("6_7", "REAL");
   pressures.add_column("8_12", "REAL");
   pressures.add_column("13_17", "REAL");
   GetDB().add_table(pressures);

   GetDB().add_column("Calibrations", "ChIo Pressures", "INTEGER");

   KVNameValueList prlist;

   while (fin.good()) {         // parcours du fichier

      sline.ReadLine(fin);
      if (sline.BeginsWith("Run Range :")) {    // run range found
         if (!prev_rr) {        // New set of run ranges to read

            //have we just finished reading some pressures ?
            if (read_pressure) {
               GetDB().prepare_data_insertion("ChIo Pressures");
               GetDB()["ChIo Pressures"].prepare_data(prlist);
               GetDB().insert_data_row();
               GetDB().end_data_insertion();
               GetDB()["Calibrations"]["ChIo Pressures"].set_data(id);
               GetDB().update("Calibrations", runrange.GetSQL("Run Number"), "ChIo Pressures");
               ++id;
               read_pressure = kFALSE;
               prlist.Clear();
               runrange.Clear();
            }

         }
         if (sscanf(sline.Data(), "Run Range : %u %u", &frun, &lrun) != 2) {
            Warning("ReadChIoPressures()",
                    "Bad format in line :\n%s\nUnable to read run range values",
                    sline.Data());
         } else {
            runrange.Add(Form("%d-%d", frun, lrun));
            prev_rr = kTRUE;
         }
      }                         // Run Range found
      if (fin.eof()) {          //fin du fichier
         //have we just finished reading some pressures ?
         if (read_pressure) {
            GetDB().prepare_data_insertion("ChIo Pressures");
            GetDB()["ChIo Pressures"].prepare_data(prlist);
            GetDB().insert_data_row();
            GetDB().end_data_insertion();
            GetDB()["Calibrations"]["ChIo Pressures"].set_data(id);
            GetDB().update("Calibrations", runrange.GetSQL("Run Number"), "ChIo Pressures");
            ++id;
            read_pressure = kFALSE;
            prlist.Clear();
         }
      }
      if (sline.BeginsWith("ChIos")) {  //line with chio pressure data

         prev_rr = kFALSE;

         //take off 'ChIos' and any leading whitespace
         sline.Remove(0, 5);
         sline.Strip(TString::kLeading);
         //split up ChIo ring numbers and pressure
         sline.Begin(" ");
         TString chio = sline.Next(kTRUE);
         KVString press = sline.Next(kTRUE);
         read_pressure = kTRUE;
         prlist.SetValue(chio, press.Atof());
      }                         //line with ChIo pressure data
   }                            //parcours du fichier
   fin.close();
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetEventCrossSection(Int_t run, Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   //Returns calculated cross-section [mb] per event for the run in question.
   //See KVINDRADBRun::GetEventCrossSection()
   if (!GetRun(run))
      return 0;
   return GetRun(run)->GetEventCrossSection(Q_apres_cible, Coul_par_top);
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetTotalCrossSection(Int_t run, Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   //Returns calculated total measured cross-section [mb] for the run in question.
   //See KVINDRADBRun::GetTotalCrossSection()
   if (!GetRun(run))
      return 0;
   return GetRun(run)->GetTotalCrossSection(Q_apres_cible, Coul_par_top);
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetEventCrossSection(Int_t run1, Int_t run2,
      Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   // Returns calculated average cross-section [mb] per event for the runs in question.
   // It is assumed that all runs correspond to the same reaction,
   // with the same beam & target characteristics and multiplicity trigger.
   // The target thickness etc. are taken from the first run.

   KVTarget* targ = GetRun(run1)->GetTarget();
   if (!targ) {
      Error("GetEventCrossSection", "No target for run %d", run1);
      return 0;
   }
   Double_t sum_xsec = 0;
   for (int run = run1; run <= run2; run++) {

      if (!GetRun(run))
         continue;              //skip non-existent runs
      sum_xsec +=
         GetRun(run)->GetNIncidentIons(Q_apres_cible,
                                       Coul_par_top) * (1. - GetRun(run)->GetTempsMort());
   }
   //average X-section [mb] per event = 1e27 / (no. atoms in target * SUM(no. of projectile nuclei * (1 - TM)) )
   return (1.e27 / (targ->GetAtomsPerCM2() * sum_xsec));
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetTotalCrossSection(Int_t run1, Int_t run2,
      Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   //Returns calculated total measured cross-section [mb] for the runs in question.
   //This is SUM (GetEventCrossSection(run1,run2) * SUM( events )
   //where SUM(events) is the total number of events measured in all the runs
   Int_t sum = 0;
   for (int run = run1; run <= run2; run++) {

      if (!GetRun(run))
         continue;              //skip non-existent runs
      sum += GetRun(run)->GetEvents();

   }
   return sum * GetEventCrossSection(run1, run2, Q_apres_cible,
                                     Coul_par_top);
}

//__________________________________________________________________________________________________________________


Double_t KVINDRADB::GetEventCrossSection(const Char_t* system_name,
      Int_t mult_trig,
      Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   //Returns calculated average cross-section [mb] per event for all runs of the named system* with trigger multiplicity 'mul_trig'.
   //                                               *to see the list of all system names, use gIndraDB->GetSystems()->ls()
   //See KVINDRADBRun::GetNIncidentIons() for meaning of other arguments

   KVDBSystem* system = GetSystem(system_name);
   if (!system) {
      Error("GetEventCrossSection",
            "System %s unknown. Check list of systems (gIndraDB->GetSystems()->ls()).",
            system_name);
      return 0.;
   }
   KVTarget* targ = system->GetTarget();
   if (!targ) {
      Error("GetEventCrossSection", "No target defined for system %s",
            system_name);
      return 0;
   }
   //loop over all runs of system, only using those with correct trigger multiplicity
   Double_t sum_xsec = 0;
   system->GetRunList().Begin();
   while (!system->GetRunList().End()) {

      KVINDRADBRun* run = GetRun(system->GetRunList().Next());
      if (run->GetTrigger() != mult_trig)
         continue;              //skip runs with bad trigger
      sum_xsec +=
         run->GetNIncidentIons(Q_apres_cible,
                               Coul_par_top) * (1. - run->GetTempsMort());

   }
   //average X-section [mb] per event = 1e27 / (no. atoms in target * SUM(no. of projectile nuclei * (1 - temps mort)))
   return (1.e27 / (targ->GetAtomsPerCM2() * sum_xsec));
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetTotalCrossSection(const Char_t* system_name,
      Int_t mult_trig,
      Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   //Returns calculated total cross-section [mb] for all events in all runs of the named system* with trigger multiplicity 'mul_trig'.
   //                                               *to see the list of all system names, use gIndraDB->GetSystems()->ls()
   //See KVINDRADBRun::GetNIncidentIons() for meaning of other arguments

   KVDBSystem* system = GetSystem(system_name);
   if (!system) {
      Error("GetTotalCrossSection",
            "System %s unknown. Check list of systems (gIndraDB->GetSystems()->ls()).",
            system_name);
      return 0.;
   }
   Int_t sum = 0;
   //loop over all runs of system, only using those with correct trigger multiplicity
   system->GetRunList().Begin();
   while (!system->GetRunList().End()) {

      KVINDRADBRun* run = GetRun(system->GetRunList().Next());

      if (run->GetTrigger() != mult_trig)
         continue;              //skip runs with bad trigger
      sum += run->GetEvents();
   }

   return sum * GetEventCrossSection(system_name, mult_trig, Q_apres_cible,
                                     Coul_par_top);
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetTotalCrossSection(TH1* events_histo, Double_t Q_apres_cible, Double_t Coul_par_top)
{
   // Calculate the cross-section [mb] for a given selection of events in several runs,
   // given by the TH1, which is a distribution of run numbers (i.e. a histogram filled with
   // the number of selected events for each run, the run number is on the x-axis of the histogram).

   Info("GetTotalCrossSection", "Calculating cross-section for q=%f", Q_apres_cible);
   Double_t xsec = 0, ninc = 0;
   KVTarget* targ = 0;
   for (int i = 1; i <= events_histo->GetNbinsX(); i++) {
      Double_t events = events_histo->GetBinContent(i);
      if (events == 0) continue;
      int run_num = events_histo->GetBinCenter(i);
      KVINDRADBRun* run = GetRun(run_num);
      if (!targ) targ = run->GetTarget();
      ninc +=
         run->GetNIncidentIons(Q_apres_cible,
                               Coul_par_top);
      xsec += events / (1. - run->GetTempsMort());
      cout << "Run#" << run_num << "   Events : " << events
           << "   Dead time : " << run->GetTempsMort() << endl;
   }
   return (1.e27 / (ninc * targ->GetAtomsPerCM2())) * xsec;
}

//__________________________________________________________________________________________________________________

void KVINDRADB::Build()
{

   //Use KVINDRARunListReader utility subclass to read complete runlist

   //get full path to runlist file, using environment variables for the current dataset
   TString runlist_fullpath;
   KVBase::SearchKVFile(GetDBEnv("Runlist"), runlist_fullpath, fDataSet.Data());

   //set comment character for current dataset runlist
   SetRLCommentChar(GetDBEnv("Runlist.Comment")[0]);

   //set field separator character for current dataset runlist
   if (!strcmp(GetDBEnv("Runlist.Separator"), "<TAB>"))
      SetRLSeparatorChar('\t');
   else
      SetRLSeparatorChar(GetDBEnv("Runlist.Separator")[0]);

   //by default we set two keys for both recognising the 'header' lines and deciding
   //if we have a good run line: the "Run" and "Events" fields must be present
   GetLineReader()->SetFieldKeys(2, GetDBEnv("Runlist.Run"),
                                 GetDBEnv("Runlist.Events"));
   GetLineReader()->SetRunKeys(2, GetDBEnv("Runlist.Run"),
                               GetDBEnv("Runlist.Events"));

   kFirstRun = 999999;
   kLastRun = 0;
   ReadRunList(runlist_fullpath.Data());
   //new style runlist
   if (IsNewRunList())
      ReadNewRunList();

   ReadSystemList();
   ReadChIoPressures();
   ReadGainList();
   ReadChannelVolt();
   ReadVoltEnergyChIoSi();
   ReadCalibCsI();
   ReadPedestalList();
//   ReadAbsentDetectors();
//   ReadOoOACQParams();
//   ReadOoODetectors();

   // read all available mean pulser data and store in tree
//   if (!fPulserData) fPulserData = new KVINDRAPulserDataTree;
//   fPulserData->SetRunList(GetRuns());
//   fPulserData->Build();

//   ReadCsITotalLightGainCorrections();
}


//____________________________________________________________________________
void KVINDRADB::ReadNewRunList()
{
   //Read new-style runlist (written using KVDBRun v.10 or later)

   ifstream fin;
   if (!OpenCalibFile("Runlist", fin)) {
      Error("ReadNewRunList()", "Could not open file %s",
            GetCalibFileName("Runlist"));
      return;
   }

   Info("ReadNewRunList()", "Reading run parameters ...");

   KVString line;
   KVINDRADBRun* run;

   while (fin.good() && !fin.eof()) {
      line.ReadLine(fin);

      if (line.Length() > 1 && !line.BeginsWith("#") && !line.BeginsWith("Version")) {
         run = new KVINDRADBRun;
         run->ReadRunListLine(line);
         if (run->GetNumber() < 1) {
            delete run;
         } else {
            AddRun(run);
            Int_t run_n = run->GetNumber();
            kLastRun = TMath::Max(kLastRun, run_n);
            kFirstRun = TMath::Min(kFirstRun, run_n);
         }
      }
   }
   fin.close();
}

KVDBChIoPressures KVINDRADB::GetChIoPressures(int run)
{
   // Retrieve ChIo pressures for this run
   KVDBChIoPressures p;
   GetDB().select_data("Calibrations", Form("\"Run Number\"=%d", run));
   int id = 0;
   while (GetDB().get_next_result())
      id = GetDB()["Calibrations"]["ChIo Pressures"].data().GetInt();

   if (!id) {
      Info("GetChIoPressures", "No ChIo pressures defined for run %d", run);
      return p; // all pressures = 0
   }

   KVSQLite::table& pressures = GetDB()["ChIo Pressures"];
   GetDB().select_data("ChIo Pressures", Form("id=%d", id));
   while (GetDB().get_next_result()) {
      p.SetPressures(
         pressures["2_3"].data().GetDouble(),
         pressures["4_5"].data().GetDouble(),
         pressures["6_7"].data().GetDouble(),
         pressures["8_12"].data().GetDouble(),
         pressures["13_17"].data().GetDouble()
      );
   }
   return p;
}

KVNameValueList KVINDRADB::GetGains(int run)
{
   // Returns detector gains for run in the form DET_NAME=[gain]

   KVNameValueList gains;
   GetDB().select_data("Calibrations", Form("\"Run Number\"=%d", run));
   TString tablename;
   while (GetDB().get_next_result())
      tablename = GetDB()["Calibrations"]["Gains"].data().GetString();

   if (tablename != "") {
      GetDB().select_data(tablename);
      KVSQLite::table& t = GetDB()[tablename.Data()];
      while (GetDB().get_next_result()) {
         gains.SetValue(t["detName"].data().GetString(), t["gain"].data().GetDouble());
      }
   }
   return gains;
}
//____________________________________________________________________________

void KVINDRADB::GoodRunLine()
{
   //For each "good run line" in the run list file, we:
   //      add a KVINDRADBRun to the database if it doesn't already exist
   //      add a KVDBTape to the database if the "tape" field is active and if it doesn't already exist
   //      set properties of run and tape objects
   //kFirstRun & kLastRun are set

   KVRunListLine* csv_line = GetLineReader();

   //run number
   Int_t run_n = csv_line->GetIntField(GetDBEnv("Runlist.Run"));

   if (!run_n) {
      cout << "run_n = 0 ?????????  line number =" << GetRLLineNumber() <<
           endl;
      GetLineReader()->Print();
      return;
   }
   kLastRun = TMath::Max(kLastRun, run_n);
   kFirstRun = TMath::Min(kFirstRun, run_n);

   /*********************************************
   WE CREATE A NEW RUN AND ADD
    IT TO THE DATABASE. WE SET ALL
    AVAILABLE INFORMATIONS ON
    RUN FROM THE FILE. ERROR IF
    DBASE RUN ALREADY EXISTS =>
    SAME RUN APPEARS TWICE
   *********************************************/
   KVINDRADBRun* run = GetRun(run_n);
   if (!run) {

      run = new KVINDRADBRun(run_n);
      AddRun(run);

      TString key = GetDBEnv("Runlist.Buffers");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Buffers", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.Tape");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Tape", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.Events");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetEvents(csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.Far1");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Faraday 1", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.Far2");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Faraday 2", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.Time");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetTime(csv_line->GetFloatField(key.Data()));
      key = GetDBEnv("Runlist.Size");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetSize(csv_line->GetFloatField(key.Data()));
      key = GetDBEnv("Runlist.GDir");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Gene DIRECT", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.GLas");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Gene LASER", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.GElec");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Gene ELECT", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.GTest");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Gene TEST", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.GMarq");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Gene MARQ", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.GTM");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Gene TM", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.DEC");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("DEC", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.FC");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("FC", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.OK");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("OK", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.FT");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("FT", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.AVL");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("AVL", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.OCD");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("OCD", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.OA");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("OA", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.RAZ");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("RAZ", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.PlastAll");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Plast All", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.PlastG");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Plast G", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.PlastC");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Plast C", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.PlastD");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetScaler("Plast D", csv_line->GetIntField(key.Data()));
      key = GetDBEnv("Runlist.TMpc");
      if (csv_line->HasFieldValue(key.Data()))
         run->SetTMpercent(csv_line->GetFloatField(key.Data()));
      key = GetDBEnv("Runlist.Trigger");
      if (csv_line->HasField(key.Data()))
         run->SetTrigger(GetRunListTrigger(key.Data(), "M>=%d"));
      key = GetDBEnv("Runlist.Start");
      if (csv_line->HasField(key.Data()))
         run->SetStartDate(csv_line->GetField(key.Data()));
      key = GetDBEnv("Runlist.End");
      if (csv_line->HasField(key.Data()))
         run->SetEndDate(csv_line->GetField(key.Data()));
      key = GetDBEnv("Runlist.Comments");
      if (csv_line->HasField(key.Data()))
         run->SetComments(csv_line->GetField(key.Data()));
      key = GetDBEnv("Runlist.Log");
      if (csv_line->HasField(key.Data()))
         run->SetLogbook(csv_line->GetField(key.Data()));

   } else {
      Error("GoodRunLine", "Run %d already exists", run_n);
   }
}
//__________________________________________________________________________________________________________________

const Char_t* KVINDRADB::GetDBEnv(const Char_t* type) const
{
   //Will look for gEnv->GetValue name "name_of_dataset.INDRADB.type"
   //then "INDRADB.type" if no dataset-specific value is found.

   if (!gDataSetManager)
      return "";
   KVDataSet* ds = gDataSetManager->GetDataSet(fDataSet.Data());
   if (!ds)
      return "";
   return ds->GetDataSetEnv(Form("INDRADB.%s", type));
}

//____________________________________________________________________________

void KVINDRADB::WriteObjects(TFile* file)
{
   // Write associated objects (i.e. KVINDRAPulserDataTree's TTree) in file
   if (fPulserData) fPulserData->WriteTree(file);
}

//____________________________________________________________________________

void KVINDRADB::ReadObjects(TFile* file)
{
   // Read associated objects (i.e. KVINDRAPulserDataTree's TTree) from file
   if (!fPulserData) fPulserData = new KVINDRAPulserDataTree;
   fPulserData->ReadTree(file);
}

//____________________________________________________________________________

void KVINDRADB::ReadCsITotalLightGainCorrections()
{
   // Read in gain corrections for CsI total light output.
   // Looks for directory
   //    $KVROOT/KVFiles/[dataset name]/[lumcorrdir]
   // where [lumcorrdir] is defined in .kvrootrc by one of the two variables
   //    INDRADB.CsILumCorr:   [lumcorrdir]
   //    [dataset name].INDRADB.CsILumCorr:   [lumcorrdir]
   // the latter value takes precedence for a given dataset over the former, generic, value.
   //
   // If the directory is not found we look for a compressed archive file
   //    $KVROOT/KVFiles/[dataset name]/[lumcorrdir].tgz
   //
   // The files in the directory containing the corrections for each run have
   // names with the format given by
   //    INDRADB.CsILumCorr.FileName:   [format]
   //    [dataset name].INDRADB.CsILumCorr.FileName:   [format]
   // the latter value takes precedence for a given dataset over the former, generic, value.
   // The [format] should include a placeholder for the run number, e.g.
   //    INDRADB.CsILumCorr.FileName:   run%04d.cor
   //    INDRADB.CsILumCorr.FileName:   Run%d.corrLum
   // etc. etc.
   //
   // The contents of each file should be in the following format:
   //    CSI_0221_R    1.00669
   //    CSI_0321_R    1.01828
   //    CSI_0322_R    1.00977
   // i.e.
   //   name_of_detector   correction
   //Any other lines are ignored.

   Info("ReadCsITotalLightGainCorrections",
        "Reading corrections...");

   // get name of directory for this dataset from .kvrootrc
   TString search;
   search = GetDBEnv("CsILumCorr");
   if (search == "") {
      Error("ReadCsITotalLightGainCorrections", "INDRADB.CsILumCorr is not defined. Check .kvrootrc files.");
   }

   KVTarArchive gain_cor(search.Data(), GetDataSetDir());
   if (!gain_cor.IsOK()) {
      Info("ReadCsITotalLightGainCorrections", "No corrections found");
      return;
   }

   TString filefmt;
   filefmt = GetDBEnv("CsILumCorr.FileName");
   if (filefmt == "") {
      Error("ReadCsITotalLightGainCorrections", "INDRADB.CsILumCorr.FileName is not defined. Check .kvrootrc files.");
   }

   // boucle sur tous les runs
   TIter next_run(GetRuns());
   KVINDRADBRun* run = 0;
   while ((run = (KVINDRADBRun*)next_run())) {

      Int_t run_num = run->GetNumber();
      TString filepath;
      filepath.Form(filefmt.Data(), run_num);
      filepath.Prepend("/");
      filepath.Prepend(search.Data());
      ifstream filereader;
      if (KVBase::SearchAndOpenKVFile(filepath, filereader, fDataSet.Data())) {

         KVString line;
         line.ReadLine(filereader);
         while (filereader.good()) {

            line.Begin(" ");
            if (line.End()) {
               line.ReadLine(filereader);
               continue;
            }
            KVString det_name = line.Next(kTRUE);
            if (!det_name.BeginsWith("CSI_")) {
               line.ReadLine(filereader);
               continue;
            }
            if (line.End()) {
               line.ReadLine(filereader);
               continue;
            }
            Double_t correction = line.Next(kTRUE).Atof();

            KVDBParameterSet* cps = new KVDBParameterSet(det_name.Data(),
                  "CsI Total Light Gain Correction", 1);

            cps->SetParameters(correction);
            //fCsILumCorr->AddRecord(cps);
            //cps->AddLink("Runs", run);
            line.ReadLine(filereader);
         }
         filereader.close();
      } else {
         Warning("ReadCsITotalLightGainCorrections", "Run %d: no correction", run_num);
      }

   }

}

//________________________________________________________________________________
void KVINDRADB::ReadChannelVolt()
{
   //Read Channel-Volt calibrations for ChIo and Si detectors (including Etalons).
   //The parameter filenames are taken from the environment variables
   //        [dataset name].INDRADB.ElectronicCalibration:     [chio & si detectors]
   //        [dataset name].INDRADB.ElectronicCalibration.Etalons:   [etalons]

   channel_volt_reader cvr(this);
   cvr.ReadCalib("ElectronicCalibration", "ReadChannelVolt",
                 "Reading electronic calibration for ChIo and Si...");

   etalon_channel_volt_reader ecvr(this);
   ecvr.ReadCalib("ElectronicCalibration.Etalons",
                  "ReadChannelVolt()", "Reading electronic calibration for Si75 and SiLi...");
}

//__________________________________________________________________________________

void KVINDRADB::ReadVoltEnergyChIoSi()
{
   //Read Volt-Energy(MeV) calibrations for ChIo and Si detectors.
   //The parameter filename is taken from the environment variable
   //        [dataset name].INDRADB.ChIoSiVoltMeVCalib:

   volt_energy_chiosi_reader vecr(this);
   vecr.ReadCalib("ChIoSiVoltMeVCalib", "ReadVoltEnergyChIoSi",
                  "Reading ChIo/Si calibration parameters...");
}

//__________________________________________________________________________

void KVINDRADB::ReadPedestalList()
{
   //Read the names of pedestal files to use for each run range, found
   //in file with name defined by the environment variable:
   //   [dataset name].INDRADB.Pedestals:    ...
   //
   // Filenames are stored in the "Calibrations" table in columns
   // "Pedestals_ChIoSi" and "Pedestals_CsI"

   ifstream fin;
   if (!OpenCalibFile("Pedestals", fin)) {
      Error("ReadPedestalList()", "Could not open file %s",
            GetCalibFileName("Pedestals"));
      return;
   }
   Info("ReadPedestalList()", "Reading pedestal file list...");

   KVString line;
   Char_t filename_chio[80], filename_csi[80];
   KVNumberList runrange;
   UInt_t first, last;
   GetDB().add_column("Calibrations", "Pedestals_ChIoSi", "TEXT");
   GetDB().add_column("Calibrations", "Pedestals_CsI", "TEXT");
   int nped_chiosi(1), nped_csi(1);

   while (fin.good()) {         //lecture du fichier

      // lecture des informations
      line.ReadLine(fin);

      //recherche une balise '+'
      if (line.BeginsWith('+')) {       //balise trouvee

         line.Remove(0, 1);

         if (sscanf
               (line.Data(), "Run Range : %u %u", &first,
                &last) != 2) {
            Warning("ReadPedestalList()", "Format problem in line \n%s",
                    line.Data());
         } else
            runrange.Add(Form("%u-%u", first, last));

         line.ReadLine(fin);

         sscanf(line.Data(), "%s", filename_chio);

         line.ReadLine(fin);

         sscanf(line.Data(), "%s", filename_csi);

         TString pchiosi_table = Form("Pedestals_ChIoSi_%d", nped_chiosi);
         if (ReadPedestals(filename_chio, pchiosi_table)) {
            GetDB()["Calibrations"]["Pedestals_ChIoSi"].set_data(pchiosi_table);
            GetDB().update("Calibrations", runrange.GetSQL("Run Number"), "Pedestals_ChIoSi");
            ++nped_chiosi;
         }
         TString pcsi_table = Form("Pedestals_CsI_%d", nped_csi);
         if (ReadPedestals(filename_csi, pcsi_table)) {
            GetDB()["Calibrations"]["Pedestals_CsI"].set_data(pcsi_table);
            GetDB().update("Calibrations", runrange.GetSQL("Run Number"), "Pedestals_CsI");
            ++nped_csi;
         }
         runrange.Clear();
      }                         // balise trouvee
   }                            // lecture du fichier
   fin.close();
}

Bool_t KVINDRADB::ReadPedestals(const TString& filename, const TString& tablename)
{
   // read pedestals in "filename" and fill new table "tablename" with them

   ifstream file_pied;
   if (!KVBase::SearchAndOpenKVFile(filename, file_pied, fDataSet)) {
      Error("ReadPedestals", "Problem opening file %s",
            filename.Data());
      return kFALSE;
   }

   KVSQLite::table ped(tablename.Data());
   ped.add_column("cou", "INTEGER");
   ped.add_column("mod", "INTEGER");
   ped.add_column("typ", "INTEGER");
   ped.add_column("pedestal", "REAL");
   ped.add_column("filename", "TEXT");
   GetDB().add_table(ped);
   KVSQLite::table& pedTable = GetDB()[tablename.Data()];
   GetDB().prepare_data_insertion(tablename);

   //skip first 5 lines - header
   TString line;
   for (int i = 5; i; i--) {
      line.ReadLine(file_pied);
   }

   int cou, mod, type, n_phys, n_gene;
   float ave_phys, sig_phys, ave_gene, sig_gene;

   while (file_pied.good()) {

      file_pied >> cou >> mod >> type >> n_phys >> ave_phys >>
                sig_phys >> n_gene >> ave_gene >> sig_gene;

      pedTable["cou"].set_data(cou);
      pedTable["mod"].set_data(mod);
      pedTable["typ"].set_data(type);
      pedTable["pedestal"].set_data(ave_gene);
      pedTable["filename"].set_data(filename);
      GetDB().insert_data_row();
   }
   GetDB().end_data_insertion();

   file_pied.close();

   return kTRUE;
}

void KVINDRADB::ReadCalibCsI()
{
   //Read CsI Light-Energy calibrations for Z=1 and Z>1
   //The parameter filenames are taken from the environment variables
   //        [dataset name].INDRADB.CalibCsI.Z=1
   //        [dataset name].INDRADB.CalibCsI.Z>1
   //These calibrations are valid for all runs

   ReadLightEnergyCsI("Z=1");
   ReadLightEnergyCsI("Z>1");
}
//_____________________________________________________________________________

void KVINDRADB::ReadLightEnergyCsI(const Char_t* zrange)
{
   //Read CsI Light-Energy calibrations for Z=1 (zrange="Z=1") or Z>1 (zrange="Z>1")
   //
   //These calibrations are valid for all runs

   ifstream fin;
   TString filename;
   filename.Form("CalibCsI.%s", zrange);
   if (!OpenCalibFile(filename.Data(), fin)) {
      Error("ReadLightEnergyCsI()", "Could not open file %s",
            GetCalibFileName(filename.Data()));
      return;
   }
   Info("ReadLightEnergyCsI()",
        "Reading Light-Energy CsI calibration parameters (%s)...", zrange);

   filename.ReplaceAll("=", "_eq_");
   filename.ReplaceAll(">", "_gt_");
   GetDB().add_column("Calibrations", filename.Data(), "TEXT");
   TString tablename = filename + "_1";
   KVSQLite::table calib(tablename.Data());
   calib.add_column("detName", "TEXT");
   calib.add_column("type", "TEXT");
   calib.add_column("npar", "INTEGER");
   for (int i = 0; i < 4; ++i) calib.add_column(Form("a%d", i), "REAL");
   GetDB().add_table(calib);
   KVSQLite::table& calTab = GetDB()[tablename.Data()];

   TString sline;
   Float_t a[4];    // calibration parameters
   Int_t ring, mod;

   GetDB().prepare_data_insertion(tablename);

   while (fin.good()) {         //reading the file
      sline.ReadLine(fin);
      if (fin.good()) {
         if (!sline.BeginsWith("#")) {  //skip comments
            if (sscanf
                  (sline.Data(), "%d %d %f %f %f %f", &ring, &mod, &a[0],
                   &a[1], &a[2], &a[3]) != 6) {
               Warning("ReadLightEnergyCsI()",
                       "Bad format in line :\n%s\nUnable to read parameters",
                       sline.Data());
               return;
            } else {            //parameters correctly read
               calTab["detName"].set_data(Form("CSI_%02d%02d", ring, mod));
               calTab["type"].set_data(Form("Light-Energy CsI %s", zrange));
               calTab["npar"].set_data(4);
               for (int i = 0; i < 4; ++i) calTab[Form("a%d", i)].set_data(a[i]);
               GetDB().insert_data_row();
            }                   //parameters correctly read
         }                      //data line
      }                         //if(fin.good
   }                            //reading the file
   fin.close();
   GetDB().end_data_insertion();

   //these calibrators are valid for all runs
   KVNumberList runrange(Form("%d-%d", kFirstRun, kLastRun));
   GetDB()["Calibrations"][filename.Data()].set_data(tablename);
   GetDB().update("Calibrations", runrange.GetSQL("Run Number"), filename);
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetEventCrossSection(KVNumberList runs,
      Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   // Returns calculated average cross-section [mb] per event for the runs in question.
   // It is assumed that all runs correspond to the same reaction,
   // with the same beam & target characteristics and multiplicity trigger.
   // The target thickness etc. are taken from the first run.

   runs.Begin();
   Int_t run1 = runs.Next();
   KVTarget* targ = GetRun(run1)->GetTarget();
   if (!targ) {
      Error("GetEventCrossSection", "No target for run %d", run1);
      return 0;
   }
   Double_t sum_xsec = 0;
   runs.Begin();
   while (!runs.End()) {

      int run = runs.Next();
      if (!GetRun(run))
         continue;              //skip non-existent runs
      sum_xsec +=
         GetRun(run)->GetNIncidentIons(Q_apres_cible,
                                       Coul_par_top) * (1. - GetRun(run)->GetTempsMort());
   }

   //average X-section [mb] per event = 1e27 / (no. atoms in target * SUM(no. of projectile nuclei * (1 - TM)) )
   return (1.e27 / (targ->GetAtomsPerCM2() * sum_xsec));
}

//__________________________________________________________________________________________________________________

Double_t KVINDRADB::GetTotalCrossSection(KVNumberList runs,
      Double_t Q_apres_cible,
      Double_t Coul_par_top) const
{
   //Returns calculated total measured cross-section [mb] for the runs in question.
   //This is SUM (GetEventCrossSection(run1,run2) * SUM( events )
   //where SUM(events) is the total number of events measured in all the runs
   Int_t sum = 0;
   runs.Begin();
   while (!runs.End()) {
      int run = runs.Next();
      if (!GetRun(run))
         continue;              //skip non-existent runs
      sum += GetRun(run)->GetEvents();

   }
   return sum * GetEventCrossSection(runs, Q_apres_cible,
                                     Coul_par_top);
}


//____________________________________________________________________________
void KVINDRADB::ReadAbsentDetectors()
{
   //Lit le fichier ou sont list�s les d�tecteurs retir�s au cours
   //de la manip
   TString fp;
   if (!KVBase::SearchKVFile(GetCalibFileName("AbsentDet"), fp, fDataSet.Data())) {
      Error("ReadAbsentDetectors", "Fichier %s, inconnu au bataillon", GetCalibFileName("AbsentDet"));
      return;
   }
   Info("ReadAbsentDetectors()", "Lecture des detecteurs absents...");
   //fAbsentDet = AddTable("Absent Detectors", "Name of physically absent detectors");

   KVDBRecord* dbrec = 0;
   TEnv env;
   TEnvRec* rec = 0;
   env.ReadFile(fp.Data(), kEnvAll);
   TIter it(env.GetTable());

   while ((rec = (TEnvRec*)it.Next())) {
      KVString srec(rec->GetName());
      KVNumberList nl(rec->GetValue());
      cout << rec->GetValue() << endl;
      if (srec.Contains(",")) {
         srec.Begin(",");
         while (!srec.End()) {
            dbrec = new KVDBRecord(srec.Next(), "Absent Detector");
            dbrec->AddKey("Runs", "List of Runs");
            //fAbsentDet->AddRecord(dbrec);
            //LinkRecordToRunRange(dbrec, nl);
         }
      } else {
         dbrec = new KVDBRecord(rec->GetName(), "Absent Detector");
         dbrec->AddKey("Runs", "List of Runs");
         //fAbsentDet->AddRecord(dbrec);
         //LinkRecordToRunRange(dbrec, nl);
      }
   }

}

//____________________________________________________________________________
void KVINDRADB::ReadOoODetectors()
{

   //Lit le fichier ou sont list�s les d�tecteurs ne marchant plus au cours
   //de la manip
   TString fp;
   if (!KVBase::SearchKVFile(GetCalibFileName("OoODet"), fp, fDataSet.Data())) {
      Error("ReadOoODetectors", "Fichier %s, inconnu au bataillon", GetCalibFileName("OoODet"));
      return;
   }
   Info("ReadOoODetectors()", "Lecture des detecteurs hors service ...");
   //fOoODet = AddTable("OoO Detectors", "Name of out of order detectors");

   KVDBRecord* dbrec = 0;
   TEnv env;
   TEnvRec* rec = 0;
   env.ReadFile(fp.Data(), kEnvAll);
   TIter it(env.GetTable());

   while ((rec = (TEnvRec*)it.Next())) {
      KVString srec(rec->GetName());
      KVNumberList nl(rec->GetValue());
      cout << rec->GetValue() << endl;
      if (srec.Contains(",")) {
         srec.Begin(",");
         while (!srec.End()) {
            dbrec = new KVDBRecord(srec.Next(), "OoO Detector");
            dbrec->AddKey("Runs", "List of Runs");
            //fOoODet->AddRecord(dbrec);
            //LinkRecordToRunRange(dbrec, nl);
         }
      } else {
         dbrec = new KVDBRecord(rec->GetName(), "OoO Detector");
         dbrec->AddKey("Runs", "List of Runs");
         //fOoODet->AddRecord(dbrec);
         //LinkRecordToRunRange(dbrec, nl);
      }
   }

}

//____________________________________________________________________________
void KVINDRADB::ReadOoOACQParams()
{

   //Lit le fichier ou sont list�s les parametres d acquisition ne marchant plus au cours
   //de la manip
   TString fp;
   if (!KVBase::SearchKVFile(GetCalibFileName("OoOACQPar"), fp, fDataSet.Data())) {
      Error("ReadNotWorkingACQParams", "Fichier %s, inconnu au bataillon", GetCalibFileName("OoOACQPar"));
      return;
   }
   Info("ReadNotWorkingACQParams()", "Lecture des parametres d acq hors service ...");
   //fOoOACQPar = AddTable("OoO ACQPars", "Name of not working acq parameters");

   KVDBRecord* dbrec = 0;
   TEnv env;
   TEnvRec* rec = 0;
   env.ReadFile(fp.Data(), kEnvAll);
   TIter it(env.GetTable());

   while ((rec = (TEnvRec*)it.Next())) {
      KVString srec(rec->GetName());
      KVNumberList nl(rec->GetValue());
      cout << rec->GetValue() << endl;
      if (srec.Contains(",")) {
         srec.Begin(",");
         while (!srec.End()) {
            dbrec = new KVDBRecord(srec.Next(), "OoO ACQPar");
            dbrec->AddKey("Runs", "List of Runs");
            //fOoOACQPar->AddRecord(dbrec);
            //LinkRecordToRunRange(dbrec, nl);
         }
      } else {
         dbrec = new KVDBRecord(rec->GetName(), "OoO ACQPar");
         dbrec->AddKey("Runs", "List of Runs");
         //fOoOACQPar->AddRecord(dbrec);
         //LinkRecordToRunRange(dbrec, nl);
      }
   }

}


void KVINDRADB::calib_file_reader::ReadCalib(const TString& calib_type,
      const TString& caller_method_name,
      const TString& informational_message)
{
   // Generic method to read an INDRA calibration file

   ifstream fin;
   if (!mydb->OpenCalibFile(calib_type, fin)) {
      mydb->Error(caller_method_name, "Could not open file %s",
                  mydb->GetCalibFileName(calib_type));
      return;
   }
   mydb->Info(caller_method_name, informational_message);

   GetDB().add_column("Calibrations", calib_type.Data(), "TEXT");

   get_table().set_name(Form("%s_%d", calib_type.Data(), new_table_num));
   initial_setup_new_table();

   TString sline;

   Int_t frun = 0, lrun = 0;
   Bool_t prev_rr = kTRUE;
   Bool_t got_data = kFALSE;

   while (fin.good()) {         //reading the file
      sline.ReadLine(fin);
      if (fin.eof()) {          //fin du fichier
         if (got_data) {
            write_data_to_db(calib_type);
            got_data = kFALSE;
         }
         fin.close();
         return;
      }
      if (sline.BeginsWith("Run Range :")) {    // Run Range found
         if (!prev_rr) {        // new run ranges set
            if (got_data) {
               write_data_to_db(calib_type);
               got_data = kFALSE;
            }
         }
         if (sscanf(sline.Data(), "Run Range : %u %u", &frun, &lrun) != 2) {
            mydb->Warning(caller_method_name,
                          "Bad format in line :\n%s\nUnable to read run range values",
                          sline.Data());
            cout << "sscanf=" << sscanf(sline.Data(), "Run Range : %u %u",
                                        &frun, &lrun) << endl;
         } else {
            prev_rr = kTRUE;
            runrange.Add(Form("%d-%d", frun, lrun));
         }
      }                         //Run Range found
      else if (sline.Sizeof() > 1 && !sline.BeginsWith("#")) {  //non void nor comment line
         format_warning = false;
         if (!read_data_line(sline.Data())) {
            if (format_warning) mydb->Warning(caller_method_name,
                                                 "Bad format in line :\n%s\nUnable to read",
                                                 sline.Data());
         } else {               //parameters correctly read
            if (!got_data) {
               add_new_table(calib_type);
            }
            insert_data_into_table();
            prev_rr = kFALSE;
            got_data = kTRUE;
         }                      //parameters correctly read
      }                         //non void nor comment line
   }                            //reading the file
   fin.close();
}

void KVINDRADB::gain_list_reader::initial_setup_new_table()
{
   get_table().add_column("detName", "TEXT");
   get_table().add_column("gain", "REAL");
}

void KVINDRADB::calib_file_reader::write_data_to_db(const TString& calib_type)
{
   GetDB().end_data_insertion();
   GetDB()["Calibrations"][calib_type.Data()].set_data(get_table().name().c_str());
   GetDB().update("Calibrations", runrange.GetSQL("Run Number"), calib_type.Data());
   runrange.Clear();
}

bool KVINDRADB::gain_list_reader::read_data_line(const char* ss)
{
   return (sscanf(ss, "%7s %f", det_name, &gain) == 2);
}

void KVINDRADB::calib_file_reader::add_new_table(const TString& calib_type)
{
   ++new_table_num;
   get_table().set_name(Form("%s_%d", calib_type.Data(), new_table_num));
   GetDB().add_table(get_table());
   GetDB().prepare_data_insertion(get_table().name().c_str());
}

void KVINDRADB::gain_list_reader::insert_data_into_table()
{
   GetDB()[get_table().name()]["detName"].set_data(TString(det_name));
   GetDB()[get_table().name()]["gain"].set_data(gain);
   GetDB().insert_data_row();
}

void KVINDRADB::channel_volt_reader::initial_setup_new_table()
{
   get_table().add_column("detName", "TEXT");
   get_table().add_column("parName", "TEXT");
   get_table().add_column("type", "TEXT");
   get_table().add_column("npar", "INTEGER");
   get_table().add_column("a0", "REAL");
   get_table().add_column("a1", "REAL");
   get_table().add_column("a2", "REAL");
}

bool KVINDRADB::channel_volt_reader::read_data_line(const char* s)
{
   return (sscanf(s, "%u %u %u %f %f %f %f %f",
                  &cour, &modu, &sign, &a0, &a1, &a2, &dum1,
                  &dum2) == 8);
}

void KVINDRADB::channel_volt_reader::insert_data_into_table()
{
   TString det_name, par_name, cal_type;
   switch (sign) {
      case ChIo_GG:
         det_name.Form("CI_%02u%02u", cour, modu);
         par_name = det_name + "_GG";
         cal_type = "Channel-Volt GG";
         break;
      case ChIo_PG:
         det_name.Form("CI_%02u%02u", cour, modu);
         par_name = det_name + "_PG";
         cal_type = "Channel-Volt PG";
         break;
      case Si_GG:
         det_name.Form("SI_%02u%02u", cour, modu);
         par_name = det_name + "_GG";
         cal_type = "Channel-Volt GG";
         break;
      case Si_PG:
         det_name.Form("SI_%02u%02u", cour, modu);
         par_name = det_name + "_PG";
         cal_type = "Channel-Volt PG";
         break;
   }
   GetDB()[get_table().name()]["detName"].set_data(det_name);
   GetDB()[get_table().name()]["parName"].set_data(par_name);
   GetDB()[get_table().name()]["type"].set_data(cal_type);
   GetDB()[get_table().name()]["npar"].set_data(3);
   GetDB()[get_table().name()]["a0"].set_data(a0);
   GetDB()[get_table().name()]["a1"].set_data(a1);
   GetDB()[get_table().name()]["a2"].set_data(a2);
   GetDB().insert_data_row();
}

bool KVINDRADB::etalon_channel_volt_reader::read_data_line(const char* s)
{
   return (sscanf(s, "%s %f %f %f", det_name, &a0, &a1, &a2) == 4);
}

void KVINDRADB::etalon_channel_volt_reader::insert_data_into_table()
{
   TString cal_type, detName;
   KVString par_name = det_name;
   par_name.Begin("_");
   detName += par_name.Next();
   detName += "_";
   detName += par_name.Next();
   cal_type = "Channel-Volt " + par_name.Next();
   GetDB()[get_table().name()]["detName"].set_data(detName);
   GetDB()[get_table().name()]["parName"].set_data(par_name);
   GetDB()[get_table().name()]["type"].set_data(cal_type);
   GetDB()[get_table().name()]["npar"].set_data(3);
   GetDB()[get_table().name()]["a0"].set_data(a0);
   GetDB()[get_table().name()]["a1"].set_data(a1);
   GetDB()[get_table().name()]["a2"].set_data(a2);
   GetDB().insert_data_row();
}

void KVINDRADB::volt_energy_chiosi_reader::initial_setup_new_table()
{
   get_table().add_column("detName", "TEXT");
   get_table().add_column("npar", "INTEGER");
   get_table().add_column("a0", "REAL");
   get_table().add_column("a1", "REAL");
   get_table().add_column("chi", "REAL");
}

bool KVINDRADB::volt_energy_chiosi_reader::read_data_line(const char* s)
{
   TString sline(s);
   if (sline.BeginsWith("SI") || sline.BeginsWith("CI")) {   //data line
      if (sscanf(s, "%7s %f %f %f", det_name, &a0, &a1, &chi) != 4) {
         format_warning = true;
         return false;
      }
      return true;
   }
   return false;
}

void KVINDRADB::volt_energy_chiosi_reader::insert_data_into_table()
{
   GetDB()[get_table().name()]["detName"].set_data(det_name);
   GetDB()[get_table().name()]["npar"].set_data(2);
   GetDB()[get_table().name()]["a0"].set_data(a0);
   GetDB()[get_table().name()]["a1"].set_data(a1);
   GetDB()[get_table().name()]["chi"].set_data(chi);
   GetDB().insert_data_row();
}
