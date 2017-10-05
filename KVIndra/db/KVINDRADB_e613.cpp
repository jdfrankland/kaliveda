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

TString KVINDRADB_e613::GetListOfOoOACQPar(int run)
{
   select_runs_in_dbtable("OoOACQPar", run, "OutOfOrder");
   TString ooo;
   while (GetDB().get_next_result())
      ooo = GetDB()["OoOACQPar"]["OutOfOrder"].get_data<TString>();
   return ooo;
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
                     GetDB().update("Gains", detname, nl.GetSQL("Run Number"));

                     printf("%s     -> Runs=%s Gain=%1.3lf\n", detname.Data(), nl.AsString(), gain);
                  } else {
                     nl.SetList(((TObjString*)toks->At(1))->GetString());
                     Double_t gain = ffile.GetDoubleReadPar(1);
                     GetDB()["Gains"][new_column_name].set_data(gain);
                     GetDB().update("Gains", new_column_name, nl.GetSQL("Run Number"));

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
   GetDB().update("Calibrations", "Pedestals_ChIoSi,Pedestals_CsI", default_run_list.GetSQL("Run Number"));

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
   // Read channel-volt calibrations for ChIo/Si detectors
   // The value of the "ElectronicCalibration" column in the "Calibrations" table for each run
   // gives the name of the table containing the calibrations for the run.
   // [As only one calibration is used for all runs, this is "ElectronicCalibration_1" for all]
   //
   // The "ElectronicCalibration_x" tables have the following structure:
   // detName | parName | type | npar | a0 | a1 | a2 | gainRef

   //need description of INDRA geometry
   if (!gIndra) {
      KVMultiDetArray::MakeMultiDetector(fDataSet.Data());
   }
   //gIndra exists, but has it been built ?
   if (!gIndra->IsBuilt())
      gIndra->Build();

   KVFileReader flist;
   TString fp;
   if (!KVBase::SearchKVFile(gDataSet->GetDataSetEnv("INDRADB.ElectronicCalibration", ""), fp, gDataSet->GetName())) {
      Error("ReadChannelVolt", "Fichier %s, inconnu au bataillon", gDataSet->GetDataSetEnv("INDRADB.ElectronicCalibration", ""));
      return;
   }

   if (!flist.OpenFileToRead(fp.Data())) {
      return;
   }

   GetDB().add_column("Calibrations", "ElectronicCalibration", "TEXT");
   GetDB()["Calibrations"]["ElectronicCalibration"] = "ElectronicCalibration_1";
   GetDB().update("Calibrations", "ElectronicCalibration");
   KVSQLite::table ec("ElectronicCalibration_1");
   ec.add_column("detName", "TEXT");
   ec.add_column("parName", "TEXT");
   ec.add_column("type", "TEXT");
   ec.add_column("npar", "INTEGER");
   ec.add_column("a0", "REAL");
   ec.add_column("a1", "REAL");
   ec.add_column("a2", "REAL");
   ec.add_column("gainRef", "REAL");
   GetDB().add_table(ec);
   KVSQLite::table& ecTab = GetDB()["ElectronicCalibration_1"];

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
            std::map<int, int> ring_run; // run to use as gain reference for each ring
            std::map<int, KVNameValueList> gains; // gains for each run given as reference

            while ((rec = (TEnvRec*)it.Next())) {

               TString srec(rec->GetName());
               //On recupere le run reference pour lequel a ete fait la calibration
               if (srec.BeginsWith("Ring.")) {
                  srec.ReplaceAll("Ring.", "");
                  KVNumberList nring(srec);
                  nring.Begin();
                  int run = TString(rec->GetValue()).Atoi();
                  while (!nring.End()) {
                     Int_t rr = nring.Next();
                     ring_run[rr] = run;
                     Info("ReadChannelVolt", "Couronne %d, run associee %d", rr, TString(rec->GetValue()).Atoi());
                  }
                  // store gains for runs
                  if (!gains.count(run)) gains[run] = GetGains(run);
               } else if (srec.BeginsWith("Pedestal")) {
                  srec.ReplaceAll("Pedestal.", "");
                  int pedrun = TString(rec->GetValue()).Atoi();
                  // Get ChIoSi pedestals for run
                  pedestals = GetPedestals_ChIoSi(pedrun);
               } else {

                  if (!GetDB().is_inserting()) GetDB().prepare_data_insertion("ElectronicCalibration_1");

                  KVString spar(rec->GetValue());
                  std::vector<KVString> toks = spar.Vectorize(",");
                  if (toks.size() >= 3) {
                     Double_t a0 = toks[1].Atof();
                     Double_t a1 = toks[2].Atof();
                     Double_t a2 = toks[3].Atof();
                     Double_t pied = pedestals.GetValue<double>(Form("%s_%s", srec.Data(), sgain.Data()));
                     //Fit Canal-Volt realise avec soustraction piedestal
                     //chgmt de variable pour passer de (Canal-piedestal) a Canal brut
                     a0 = a0 - pied * a1 + pied * pied * a2;
                     a1 = a1 - 2 * pied * a2;
                     //a2 inchange
                     KVINDRADetector* det = (KVINDRADetector*)gIndra->GetDetector(srec);
                     if (det) {

                        //On recupere le run de ref, pour avoir le gain associe a chaque detecteur
                        Double_t gain = gains[ring_run[det->GetRingNumber()]].GetValue<double>(srec);
                        //le gain est mis comme troisieme parametre

                        ecTab["detName"] = srec;
                        ecTab["parName"] = Form("%s_%s", rec->GetName(), sgain.Data());
                        ecTab["type"] = cal_type;
                        ecTab["npar"] = 3;
                        ecTab["a0"] = a0;
                        ecTab["a1"] = a1;
                        ecTab["a2"] = a2;
                        ecTab["gainRef"] = gain;

                        GetDB().insert_data_row();
                     }
                  } else {
                     Warning("ReadChannelVolt", "Pb de format %s", rec->GetValue());
                  }
               }
            }
            GetDB().end_data_insertion();
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
   //
   // For each run, the "ChIoSiVoltMeVCalib" column of "Calibrations" table holds the name
   // of the table to use ("ChIoSiVoltMeVCalib_1", "ChIoSiVoltMeVCalib_2", etc.)
   //
   // Each table has following structure:
   //
   //   detName | npar | a0 | a1

   KVFileReader flist;
   TString fp;
   if (!KVBase::SearchKVFile(gDataSet->GetDataSetEnv("INDRADB.ChIoSiVoltMeVCalib", ""), fp, gDataSet->GetName())) {
      Error("ReadVoltEnergyChIoSi", "Fichier %s, inconnu au bataillon", gDataSet->GetDataSetEnv("INDRADB.ChIoSiVoltMeVCalib", ""));
      return;
   }

   if (!flist.OpenFileToRead(fp.Data())) {
      return;
   }

   KVSQLite::table mev("ChIoSiVoltMeVCalib_1");
   GetDB().add_column("Calibrations", "ChIoSiVoltMeVCalib", "TEXT") = mev.name();
   GetDB().update("Calibrations", "ChIoSiVoltMeVCalib"); // one calibration for all runs
   mev.add_column("detName", "TEXT");
   mev.add_column("npar", "INTEGER");
   mev.add_column("a0", "REAL");
   mev.add_column("a1", "REAL");
   GetDB().add_table(mev);
   KVSQLite::table& mevTab = GetDB()[mev.name()];
   GetDB().prepare_data_insertion(mev.name());

   while (flist.IsOK()) {
      flist.ReadLine(NULL);
      KVString file = flist.GetCurrentLine();
      if (file != "" && !file.BeginsWith('#')) {
         if (KVBase::SearchKVFile(file.Data(), fp, gDataSet->GetName())) {
            Info("ReadVoltEnergyChIoSi", "Lecture de %s", fp.Data());
            TEnv env;
            env.ReadFile(fp.Data(), kEnvAll);
            TIter it(env.GetTable());
            TEnvRec* rec;
            while ((rec = (TEnvRec*)it.Next())) {
               Double_t a0(0), a1(1);
               KVString spar(rec->GetValue());
               vector<KVString> toks = spar.Vectorize(",");
               if (toks.size() >= 2) {
                  a0 = toks[1].Atof();
                  a1 = toks[2].Atof();
               }
               mevTab["detName"] = rec->GetName();
               mevTab["npar"] = 2;
               mevTab["a0"] = a0;
               mevTab["a1"] = a1;
               GetDB().insert_data_row();
            }
         }
      }
   }
   GetDB().end_data_insertion();
   Info("ReadVoltEnergyChIoSi", "End of reading");

}

void KVINDRADB_e613::ReadOoOACQParams()
{
   // So many out of order parameters (and with such a complicated history)
   // So we add a table called "OoOACQPar" with a column "Run Number"
   // and a column "OutOfOrder" which contains a comma-separated list of
   // out of order parameters for each run

   KVSQLite::table ooo("OoOACQPar");
   ooo.add_column("Run Number", "INTEGER");
   ooo.add_column("OutOfOrder", "TEXT");
   GetDB().add_table(ooo);

   GetDB().prepare_data_insertion(ooo.name());

   TString calling_method = "ReadOoOACQParams";
   TString informational = "Lecture des parametres d acq hors service ...";
   TString calibfilename = "OoOACQPar";
   TString fp;
   if (!KVBase::SearchKVFile(GetCalibFileName(calibfilename), fp, fDataSet.Data())) {
      Error(calling_method, "Fichier %s, inconnu au bataillon", GetCalibFileName(calibfilename));
      return;
   }
   Info(calling_method, "%s", informational.Data());

   TEnv env;
   TEnvRec* rec = 0;
   env.ReadFile(fp.Data(), kEnvAll);
   TIter it(env.GetTable());

   // list with one entry per runlist, each holds list of all parameters for runlist
   KVNameValueList runlists;

   while ((rec = (TEnvRec*)it.Next())) {

      KVNamedParameter* par = runlists.FindParameter(rec->GetValue());
      if (par) {
         TString old = par->GetTString();
         old += ",";
         old += rec->GetName();
         par->Set(old);
      } else
         runlists.SetValue(rec->GetValue(), rec->GetName());

   }

   int nrunlists = runlists.GetNpar();
   KVNumberList all_runs(kFirstRun, kLastRun, 1);
   all_runs.Begin();
   while (!all_runs.End()) {
      int run = all_runs.Next();
      // build complete list for this run
      TString all_pars;
      for (int i = 0; i < nrunlists; ++i) {
         KVNamedParameter* par = runlists.GetParameter(i);
         if (KVNumberList(par->GetName()).Contains(run)) {
            if (all_pars != "") all_pars += ",";
            all_pars += par->GetTString();
         }
      }
      //std::cout << run << " : " << all_pars << std::endl;
      if (all_pars != "") {
         GetDB()[ooo.name()]["Run Number"] = run;
         GetDB()[ooo.name()]["OutOfOrder"] = all_pars;
         GetDB().insert_data_row();
      }
   }
   GetDB().end_data_insertion();
}
