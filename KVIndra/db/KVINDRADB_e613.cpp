//Created by KVClassFactory on Mon Oct 24 14:38:16 2011
//Author: bonnet

#include "KVINDRADB_e613.h"
#include "KVINDRA.h"
#include "KVFileReader.h"
#include "TObjArray.h"
#include "KVDBParameterSet.h"
#include "KVDBChIoPressures.h"

using namespace std;

ClassImp(KVINDRADB_e613)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVINDRADB_e613</h2>
<h4>Child class for e613 experiment</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVINDRADB_e613::KVINDRADB_e613()
{
   // Default constructor
}

//___________________________________________________________________________
KVINDRADB_e613::KVINDRADB_e613(const Char_t* name): KVINDRADB(name)
{
   Info("KVINDRADB_e613", "Hi coquine, tu es sur la manip e613 ...");
}

//___________________________________________________________________________
KVINDRADB_e613::~KVINDRADB_e613()
{
   // Destructor
}

KVNameValueList KVINDRADB_e613::GetGains(int run)
{
   // Override KVINDRADB method due to different database structure (see KVINDRADB_e613::ReadGainList)
   // INDRA array object must have been built before using this method

   KVNameValueList gainlist;

   if (!gIndra || !gIndra->IsBuilt()) {
      Warning("GetGains(int)", "Build multidetector for dataset before calling this method");
      return gainlist;
   }
   KVHashList* chio = gIndra->GetListOfChIo();
   KVHashList* sili = gIndra->GetListOfSi();

   KVNameValueList gain_individual;//for individual detectors
   KVSQLite::table& gain_table = GetDB()["Gains"];
   int ncols = gain_table.number_of_columns();
   select_runs_in_dbtable("Gains", run);
   while (GetDB().get_next_result()) {
      for (int col = 1; col < ncols; ++col) {
         if (!gain_table[col].is_null()) {
            // are we dealing with a set of detectors or an individual detector?
            TString det(gain_table[col].name());
            Double_t Gain = gain_table[col].get_data<double>();
            if (gIndra->GetDetector(det)) {
               // put in individual list; apply after treating all sets
               gain_individual.SetValue(det, Gain);
            } else {
               // range of detectors i.e. "CI_02", "SI_06", "SILI" etc.
               KVHashList* dets(nullptr);
               switch (det[0]) {
                  case 'S':
                     dets = sili;
                     break;
                  case 'C':
                     dets = chio;
                     break;
                  default:
                     Error("GetGains(int)",
                           "Unknown detector specification %s", det.Data());
                     continue;
               }
               unique_ptr<KVSeqCollection> detset(dets->GetSubListWithName(Form("%s*", det.Data())));
               TIter it(detset.get());
               TObject* d;
               while ((d = it())) {
                  gainlist.SetValue(d->GetName(), Gain);
               }
            }
         }
      }
   }
   // any individual detector gains?
   if (gain_individual.GetNpar()) {
      for (int i = 0; i < gain_individual.GetNpar(); ++i) {
         KVNamedParameter* p = gain_individual.GetParameter(i);
         gainlist.SetValue(p->GetName(), p->GetDouble());
      }
   }
   return gainlist;
}

//____________________________________________________________________________
void KVINDRADB_e613::ReadGainList()
{
   // Read the file listing any detectors whose gain value changes during experiment
   // information are in  [dataset name].INDRADB.Gains:    ...
   //
   // We fill a table "Gains" in the database with the following columns:
   // Run Number | SI_01 | CI_02 | SI75 | etc.

   KVFileReader flist;
   TString fp;
   if (!KVBase::SearchKVFile(GetCalibFileName("Gains"), fp, fDataSet.Data())) {
      Error("ReadGainList", "Fichier %s, inconnu au bataillon", GetCalibFileName("Gains"));
      return;
   }

   if (!flist.OpenFileToRead(fp.Data())) {
      //Error("ReadGainList","Error opening file named %s",fp.Data());
      return;
   }
   Info("ReadGainList()", "Reading gains ...");

   KVSQLite::table gt("Gains");
   gt.add_column("Run Number", "INTEGER");
   GetDB().add_table(gt);
   // fill run number column
   GetDB().copy_table_data("Runs", "Gains", "Run Number");
   TString new_column_name;

   while (flist.IsOK()) {

      flist.ReadLine(".");
      if (! flist.GetCurrentLine().IsNull()) {
         unique_ptr<TObjArray> toks(flist.GetReadPar(0).Tokenize("_"));
         Int_t nt = toks->GetEntries();
         Int_t ring = -1;
         unique_ptr<KVSeqCollection> sl;
         TString det_type = "";
         if (nt <= 1) {
            Warning("ReadGainList", "format non gere");
         } else {
            //format : Gain_[det_type]_R[RingNumber].dat
            //exemple  : Gain_SI_R07.dat
            det_type = ((TObjString*)toks->At(1))->GetString();
            if (nt == 2) {
               ring = 0;
            } else if (nt == 3) {
               sscanf(((TObjString*)toks->At(2))->GetString().Data(), "R%d", &ring);
            } else {
               Warning("ReadGainList", "format non gere");
            }
            if (ring != 0)
               new_column_name = GetDB().add_column("Gains", Form("%s_%02d", det_type.Data(), ring), "REAL").name();
            else
               new_column_name = GetDB().add_column("Gains", det_type.Data(), "REAL").name();
         }

         KVNumberList nl;
         KVFileReader ffile;
         if (KVBase::SearchKVFile(flist.GetCurrentLine().Data(), fp, fDataSet.Data())) {
            ffile.OpenFileToRead(fp.Data());

            while (ffile.IsOK()) {
               ffile.ReadLine(":");
               if (! ffile.GetCurrentLine().IsNull()) {

                  toks.reset(ffile.GetReadPar(0).Tokenize("."));
                  if (toks->GetEntries() > 2) {
                     // line of type: RunRange.[detector].[runlist]
                     // specifies gain of specific detector for runs
                     // we add a column for this detector (if not already existing)
                     // and update it accordingly
                     TString detname = ((TObjString*)toks->At(1))->GetString();
                     nl.SetList(((TObjString*)toks->At(2))->GetString());
                     Double_t gain = ffile.GetDoubleReadPar(1);
                     if (!GetDB()["Gains"].has_column(detname)) GetDB().add_column("Gains", detname, "REAL");
                     GetDB()["Gains"][detname].set_data(gain);
                     GetDB().update("Gains", nl.GetSQL("Run Number"), detname);

                     printf("%s     -> Runs=%s Gain=%1.3lf\n", detname.Data(), nl.AsString(), gain);
                  } else {
                     nl.SetList(((TObjString*)toks->At(1))->GetString());
                     Double_t gain = ffile.GetDoubleReadPar(1);
                     GetDB()["Gains"][new_column_name].set_data(gain);
                     GetDB().update("Gains", nl.GetSQL("Run Number"), new_column_name);

                     if (ring) printf("%s Ring %d -> Runs=%s Gain=%1.3lf\n", det_type.Data(), ring, nl.AsString(), gain);
                     else printf("%s     -> Runs=%s Gain=%1.3lf\n", det_type.Data(), nl.AsString(), gain);
                  }
               }
            }
         }
         ffile.CloseFile();

      }
   }

   flist.CloseFile();

}
//____________________________________________________________________________
void KVINDRADB_e613::ReadPedestalList()
{
   //Read the names of pedestal files to use for each run range, found
   //in file with name defined by the environment variable:
   //   [dataset name].INDRADB.Pedestals:    ...
   //Actuellement lecture d un seul run de piedestal
   //et donc valeur unique pour l ensemble des runs
   //
   // Each set of pedestals are stored in tables with names
   // "Pedestals_ChIoSi_1", "Pedestals_ChIoSi_2", etc. etc.;
   // the name of the table to use for each run is stored in the "Calibrations" table
   // columns "Pedestals_ChIoSi" (CI, SI, SILI, SI75) and "Pedestals_CsI"


   KVFileReader flist;
   TString fp;
   if (!KVBase::SearchKVFile(gDataSet->GetDataSetEnv("INDRADB.Pedestals", ""), fp, gDataSet->GetName())) {
      Error("ReadPedestalList", "Fichier %s, inconnu au bataillon", gDataSet->GetDataSetEnv("INDRADB.Pedestals", ""));
      return;
   }

   if (!flist.OpenFileToRead(fp.Data())) {
      return;
   }

   GetDB().add_column("Calibrations", "Pedestals_ChIoSi", "TEXT");
   GetDB().add_column("Calibrations", "Pedestals_CsI", "TEXT");

   // set up the 2 unique tables used for all runs
   TString pchiosi_table = "Pedestals_ChIoSi_1";
   TString pcsi_table = "Pedestals_CsI_1";
   KVSQLite::table pchio(pchiosi_table);
   pchio.add_column("detName", "TEXT");
   pchio.add_column("pedestal", "REAL");
   GetDB().add_table(pchio);
   KVSQLite::table pcsi(pcsi_table);
   pcsi.add_column("detName", "TEXT");
   pcsi.add_column("pedestal", "REAL");
   GetDB().add_table(pcsi);

   KVNumberList default_run_list;
   default_run_list.SetMinMax(kFirstRun, kLastRun);
   Info("ReadPedestalList", "liste des runs par defaut %s", default_run_list.AsString());

   // set the table names for all runs
   GetDB()["Calibrations"]["Pedestals_ChIoSi"].set_data(pchiosi_table);
   GetDB()["Calibrations"]["Pedestals_CsI"].set_data(pcsi_table);
   GetDB().update("Calibrations", default_run_list.GetSQL("Run Number"), "Pedestals_ChIoSi,Pedestals_CsI");

   while (flist.IsOK()) {
      flist.ReadLine(NULL);
      KVString file = flist.GetCurrentLine();
      KVNumberList nl;
      if (file != "" && !file.BeginsWith('#')) {
         if (KVBase::SearchKVFile(file.Data(), fp, gDataSet->GetName())) {
            Info("ReadPedestalList", "Lecture de %s", fp.Data());
            TString tablename;
            if (file.BeginsWith("pied_CSI")) tablename = pcsi_table;
            else tablename = pchiosi_table;
            KVSQLite::table& ped_tab = GetDB()[tablename];
            GetDB().prepare_data_insertion(tablename);
            TEnv env;
            env.ReadFile(fp.Data(), kEnvAll);
            TIter it(env.GetTable());
            TEnvRec* rec;
            while ((rec = (TEnvRec*)it.Next())) {
               if (!strcmp(rec->GetName(), "RunRange")) {
                  nl.SetList(rec->GetValue());// run range read but not used
               } else {
                  ped_tab["detName"].set_data(rec->GetName());
                  ped_tab["pedestal"].set_data(env.GetValue(rec->GetName(), 0.0));
                  GetDB().insert_data_row();
               }
            }
            GetDB().end_data_insertion();
         }
      }
   }
   Info("ReadPedestalList", "End of reading");
}

//____________________________________________________________________________
void KVINDRADB_e613::ReadChannelVolt()
{

   //need description of INDRA geometry
   if (!gIndra) {
      KVMultiDetArray::MakeMultiDetector(fDataSet.Data());
   }
   //gIndra exists, but has it been built ?
   if (!gIndra->IsBuilt())
      gIndra->Build();

   KVNumberList default_run_list;
   default_run_list.SetMinMax(kFirstRun, kLastRun);
   Info("ReadChannelVolt", "liste des runs par defaut %s", default_run_list.AsString());

   KVFileReader flist;
   TString fp;
   if (!KVBase::SearchKVFile(gDataSet->GetDataSetEnv("INDRADB.ElectronicCalibration", ""), fp, gDataSet->GetName())) {
      Error("ReadChannelVolt", "Fichier %s, inconnu au bataillon", gDataSet->GetDataSetEnv("INDRADB.ElectronicCalibration", ""));
      return;
   }

   if (!flist.OpenFileToRead(fp.Data())) {
      return;
   }

   Double_t a0, a1, a2; //parametre du polynome d ordre 2
   Double_t gain = 0; //valeur du gain de reference
   Double_t dum2 = -2;
   Double_t pied = 0; //valeur du piedestal

   while (flist.IsOK()) {
      flist.ReadLine(NULL);
      KVString file = flist.GetCurrentLine();
      if (file != "" && !file.BeginsWith('#')) {
         if (KVBase::SearchKVFile(file.Data(), fp, gDataSet->GetName())) {
            Info("ReadChannelVolt", "Lecture de %s", fp.Data());
            TString cal_type;
            TString sgain = "GG";
            if (fp.Contains("PGtoVolt")) sgain = "PG";
            cal_type = "Channel-Volt " + sgain;

            TEnv env;
            env.ReadFile(fp.Data(), kEnvAll);
            TIter it(env.GetTable());
            TEnvRec* rec;

            KVNameValueList pedestals; // pedestals from run given on line "Pedestal: xxx"
            while ((rec = (TEnvRec*)it.Next())) {
               KVNumberList nring;
               TString srec(rec->GetName());
               //On recupere le run reference pour lequel a ete fait la calibration
               if (srec.BeginsWith("Ring.")) {
                  srec.ReplaceAll("Ring.", "");
                  nring.SetList(srec);
                  nring.Begin();
                  while (!nring.End()) {
                     Int_t rr = nring.Next();
                     ring_run.SetValue(Form("%d", rr), TString(rec->GetValue()).Atoi());
                     Info("ReadChannelVolt", "Couronne %d, run associee %d", rr, TString(rec->GetValue()).Atoi());
                  }
               } else if (srec.BeginsWith("Pedestal")) {
                  srec.ReplaceAll("Pedestal.", "");
                  dbpied = GetRun(TString(rec->GetValue()).Atoi());
               } else {

                  TString spar(rec->GetValue());
                  toks = spar.Tokenize(",");
                  if (toks->GetEntries() >= 3) {
                     a0 = ((TObjString*)toks->At(1))->GetString().Atof();
                     a1 = ((TObjString*)toks->At(2))->GetString().Atof();
                     a2 = ((TObjString*)toks->At(3))->GetString().Atof();
                     //par_pied = ((KVDBParameterSet*)dbpied->GetLink("Pedestals", Form("%s_%s", rec->GetName(), sgain.Data())));
                     if (par_pied)
                        pied = par_pied->GetParameter();
                     //Fit Canal-Volt realise avec soustraction piedestal
                     //chgmt de variable pour passer de (Canal-piedestal) a Canal brut
                     a0 = a0 - pied * a1 + pied * pied * a2;
                     a1 = a1 - 2 * pied * a2;
                     //a2 inchange
                     //On recupere le run de ref, pour avoir le gain associe a chaque detecteur
                     KVINDRADetector* det = (KVINDRADetector*)gIndra->GetDetector(rec->GetName());
                     if (det) {

                        Int_t runref = ring_run.GetIntValue(Form("%d", det->GetRingNumber()));
                        if (!dbrun) {
                           dbrun = GetRun(runref);
                        } else if (dbrun->GetNumber() != runref) {
                           dbrun = GetRun(runref);
                        }
                        if (!dbrun) {
                           Warning("ReadChannelVolt", "Pas de run reference numero %d", runref);
                        }
                        //le gain est mis comme troisieme parametre
                        KVDBParameterSet* pargain;// = ((KVDBParameterSet*) dbrun->GetLink("Gains", rec->GetName()));
                        if (pargain) {
                           gain = pargain->GetParameter(0);
                        } else {
                           Info("ReadChannelVolt", "pas de gain defini pour le run %d et le detecteur %s", runref, rec->GetName());
                        }

                        //Si tout est dispo on enregistre la calibration pour ce detecteur
                        //
                        par = new KVDBParameterSet(Form("%s_%s", rec->GetName(), sgain.Data()), cal_type, 5);
                        par->SetParameters(a0, a1, a2, gain, dum2);

                        //fChanVolt->AddRecord(par);
                        //LinkRecordToRunRange(par, default_run_list);
                     }
                  } else {
                     a0 = a1 = a2 = gain = 0;
                     Warning("ReadChannelVolt", "Pb de format %s", rec->GetValue());
                  }
                  delete toks;
               }
            }
            delete env;

         }
      }
   }
   Info("ReadChannelVolt", "End of reading");

}

//____________________________________________________________________________
void KVINDRADB_e613::ReadVoltEnergyChIoSi()
{
   //Read Volt-Energy(MeV) calibrations for ChIo and Si detectors.
   //The parameter filename is taken from the environment variable
   //        [dataset name].INDRADB.ChIoSiVoltMeVCalib:

   KVFileReader flist;
   TString fp;
   if (!KVBase::SearchKVFile(gDataSet->GetDataSetEnv("INDRADB.ChIoSiVoltMeVCalib", ""), fp, gDataSet->GetName())) {
      Error("ReadVoltEnergyChIoSi", "Fichier %s, inconnu au bataillon", gDataSet->GetDataSetEnv("INDRADB.ChIoSiVoltMeVCalib", ""));
      return;
   }

   if (!flist.OpenFileToRead(fp.Data())) {
      return;
   }
   TEnv* env = 0;
   TEnvRec* rec = 0;
   KVDBParameterSet* par = 0;
   TObjArray* toks = 0;

   KVNumberList default_run_list;
   default_run_list.SetMinMax(kFirstRun, kLastRun);
   Info("ReadVoltEnergyChIoSi", "liste des runs par defaut %s", default_run_list.AsString());

   while (flist.IsOK()) {
      flist.ReadLine(NULL);
      KVString file = flist.GetCurrentLine();
      KVNumberList nl;
      if (file != "" && !file.BeginsWith('#')) {
         if (KVBase::SearchKVFile(file.Data(), fp, gDataSet->GetName())) {
            Info("ReadPedestalList", "Lecture de %s", fp.Data());
            env = new TEnv();
            env->ReadFile(fp.Data(), kEnvAll);
            TIter it(env->GetTable());
            while ((rec = (TEnvRec*)it.Next())) {

               Double_t a0 = 0, a1 = 1, chi = 1;
               TString spar(rec->GetValue());
               toks = spar.Tokenize(",");
               if (toks->GetEntries() >= 2) {
                  a0 = ((TObjString*)toks->At(1))->GetString().Atof();
                  a1 = ((TObjString*)toks->At(2))->GetString().Atof();
               }
               delete toks;

               par = new KVDBParameterSet(rec->GetName(), "Volt-Energy", 3);
               par->SetParameters(a0, a1, chi);
               //fVoltMeVChIoSi->AddRecord(par);
               //LinkRecordToRunRange(par, default_run_list);

            }
            delete env;
         }
      }
   }
   Info("ReadVoltEnergyChIoSi", "End of reading");

}

void KVINDRADB_e613::Build()
{
   // Build and fill the database for an INDRA experiment

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
   ReadPedestalList();
   ReadChannelVolt();
//   ReadVoltEnergyChIoSi();
//   ReadCalibCsI();
//   ReadAbsentDetectors();
//   ReadOoOACQParams();
//   ReadOoODetectors();

//   // read all available mean pulser data and store in tree
//   fPulserData.reset(new KVINDRAPulserDataTree(GetDataBaseDir(), kFALSE));
//   fPulserData->SetRunList(GetRuns());
//   fPulserData->Build();

//   ReadCsITotalLightGainCorrections();
}

