//Created by KVClassFactory on Tue Jul 12 11:43:52 2016
//Author: bonnet,,,

#include "KVExpDB.h"

#include "KVDBSystem.h"
#include "KVNumberList.h"

#include <TClass.h>
#include <iostream>
using namespace std;

ClassImp(KVExpDB)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVExpDB</h2>
<h4>base class to describe database of an experiment</h4>
<!-- */
// --> END_HTML
// An experiment database contains at least two tables:
//    Runs      -> infos on all runs
//    Systems   -> infos on all systems
////////////////////////////////////////////////////////////////////////////////
void KVExpDB::init()
{
   //default initialisations
   kFirstRun = 0;
   kLastRun = 0;
   fRuns.SetOwner();
}

void KVExpDB::fill_runlist_from_database()
{
   // Reconstruct list of KVDBRun objects from sqlitedb informations

   Info("OpenDataBase", "rebuilding run table from SQLiteDB");
   fSQLdb.select_data("Runs");
   KVSQLite::table& runs = fSQLdb["Runs"];
   while (fSQLdb.get_next_result()) {
      KVDBRun* r = (KVDBRun*)TClass::GetClass(runs["class_name"].get_data<TString>())->New();
      KVNameValueList default_params;
      r->GetDefaultDBColumns(default_params);
      r->ReadDefaultDBColumns(runs);
      r->SetRunid(runs["runid"].get_data<int>());
      int ncols = runs.number_of_columns();
      for (int i = 0; i < ncols; ++i) {
         KVSQLite::column& col = runs[i];
         if (!default_params.HasParameter(col.name())) {
            r->GetParameters().SetValue(col.data());
         }
      }

      AddRun(r);
   }
}

void KVExpDB::fill_systemlist_from_database()
{
   // Reconstruct list of KVDBSystem objects from sqlitedb informations

   Info("OpenDataBase", "rebuilding systems table from SQLiteDB");
   fSQLdb.select_data("Systems");
   KVSQLite::table& systems = fSQLdb["Systems"];
   std::vector<int> target_id;
   while (fSQLdb.get_next_result()) {
      KVDBSystem* sys = new KVDBSystem(systems["sysname"].get_data<TString>());
      sys->SetSysid(systems["sysid"].get_data<int>());
      sys->SetZbeam(systems["Zbeam"].get_data<int>());
      sys->SetAbeam(systems["Abeam"].get_data<int>());
      sys->SetEbeam(systems["Ebeam"].get_data<double>());
      sys->SetZtarget(systems["Ztarget"].get_data<int>());
      sys->SetAtarget(systems["Atarget"].get_data<int>());
      AddSystem(sys);
      target_id.push_back(systems["target"].get_data<int>());
   }
   // rebuild runlist for each system, and link run<->system
   TIter next(&fSystems);
   KVDBSystem* sys;
   KVNumberList runlist;
   KVSQLite::table& runs = fSQLdb["Runs"];
   std::vector<int>::iterator target_id_it = target_id.begin();
   while ((sys = (KVDBSystem*)next())) {
      fSQLdb.select_data("Runs", "Run Number", Form("sysid=%d", sys->GetSysid()));
      while (fSQLdb.get_next_result()) {
         int run = runs["Run Number"].get_data<int>();
         runlist.Add(run);
         GetRun(run)->SetSystem(sys);
      }
      sys->SetRuns(runlist);
      runlist.Clear();
      // retrieve target from file
      sys->SetTarget(dynamic_cast<KVTarget*>(read_object_from_root_file(*target_id_it, "Targets")));
      ++target_id_it;
   }
}

void KVExpDB::fill_database_from_runlist()
{
   // Called when database is built.
   // Extract all informations from runs in fRuns to create and fill the 'Runs' table
   // We also create 'Calibrations' and 'DetectorStatus' tables with a row for each run

   // first loop over all runs in order to list all individual parameters
   KVUniqueNameList full_param_list;
   KVDBRun* r;
   TIter next(&fRuns);
   while ((r = (KVDBRun*)next())) {
      int n = r->GetParameters().GetNpar();
      for (int i = 0; i < n; ++i) {
         KVNamedParameter* param = r->GetParameters().GetParameter(i);
         full_param_list.Add(param);
      }
   }
   KVSQLite::table t("Runs");
   t.add_primary_key("runid");
   t.add_foreign_key("sysid", "Systems", "sysid");
   t.add_column("class_name", "TEXT");

   // get default list of parameters from first run
   KVNameValueList defaultlist;
   r = (KVDBRun*)fRuns.First();
   r->GetDefaultDBColumns(defaultlist);
   int ndefpar = defaultlist.GetNpar();
   KVNamedParameter* param;
   for (int i = 0; i < ndefpar; ++i) {
      param = defaultlist.GetParameter(i);
      t.add_column(param->GetName(), param->GetSQLType());
   }
   TIter it(&full_param_list);
   while ((param = (KVNamedParameter*)it())) {
      t.add_column(param->GetName(), param->GetSQLType());
   }
   //t.print();
   fSQLdb.add_table(t);

   KVSQLite::table calib("Calibrations");
   calib.add_foreign_key("Run Number", "Runs", "Run Number");
   fSQLdb.add_table(calib);
   KVSQLite::table status("DetectorStatus");
   status.add_foreign_key("Run Number", "Runs", "Run Number");
   fSQLdb.add_table(status);

   // now fill the tables
   fSQLdb.prepare_data_insertion("Runs");
   next.Reset();
   int runid = 1;
   KVSQLite::table& runtable = fSQLdb["Runs"];
   while ((r = (KVDBRun*)next())) {
      r->SetRunid(runid);
      r->FillDefaultDBColumns(defaultlist);
      for (int i = 0; i < ndefpar; ++i) {
         param = defaultlist.GetParameter(i);
         runtable[param->GetName()].set_data(*param);
      }
      it.Reset();
      while ((param = (KVNamedParameter*)it())) {
         if (r->Has(param->GetName())) runtable[param->GetName()].set_data(*(r->GetParameters().FindParameter(param->GetName())));
         else runtable[param->GetName()].set_null();
      }
      runtable["sysid"].set_null();// no links to systems for the moment
      runtable["class_name"].set_data(r->ClassName());
      fSQLdb.insert_data_row();
   }
   fSQLdb.end_data_insertion();

//   next.Reset();
//   fSQLdb.prepare_data_insertion("Calibrations");
//   KVSQLite::table& calibs = fSQLdb["Calibrations"];
//   while ((r = (KVDBRun*)next())) {
//      calibs["Run Number"].set_data((int)r->GetNumber());
//      fSQLdb.insert_data_row();
//   }
//   fSQLdb.end_data_insertion();
//   next.Reset();
//   fSQLdb.prepare_data_insertion("DetectorStatus");
//   KVSQLite::table& statuss = fSQLdb["DetectorStatus"];
//   while ((r = (KVDBRun*)next())) {
//      statuss["Run Number"].set_data((int)r->GetNumber());
//      fSQLdb.insert_data_row();
//   }
//   fSQLdb.end_data_insertion();
   // copy run numbers to Calibrations and DetectorStatus tables
   fSQLdb.copy_table_data("Runs", "Calibrations", "Run Number");
   fSQLdb.copy_table_data("Runs", "DetectorStatus", "Run Number");
}

KVExpDB::KVExpDB()
   : KVDataBase()
{
   // Default constructor
}

//____________________________________________________________________________//

KVExpDB::KVExpDB(const Char_t* name)
   : KVDataBase(name)
{
   // Constructor inherited from KVDataBase
   init();
}

//____________________________________________________________________________//

KVExpDB::KVExpDB(const Char_t* name, const Char_t* title)
   : KVDataBase(name, title)
{
   // Constructor inherited from KVDataBase
   init();
}

void KVExpDB::connect_to_database(const TString& path)
{
   // Open the SQLite database file DataBase.sqlite at 'path'
   // If the file does not exist, this will trigger Build()
   // If needed, fill runlist & systemlist from database

   KVDataBase::connect_to_database(path);
   if (!fRuns.GetEntries()) fill_runlist_from_database();
   if (!fSystems.GetEntries()) fill_systemlist_from_database();
}

//____________________________________________________________________________//

KVExpDB::~KVExpDB()
{
   // Destructor
}

//____________________________________________________________________________
void KVExpDB::ReadSystemList()
{
   //Reads list of systems with associated run ranges
   //
   //There are 2 formats for the description of systems:
   //
   //+129Xe + natSn 25 MeV/A           '+' indicates beginning of system description
   //129 54 119 50 0.1 25.0            A,Z of projectile and target, target thickness (mg/cm2), beam energy (MeV/A)
   //Run Range : 614 636               runs associated with system
   //Run Range : 638 647               runs associated with system
   //
   //This is sufficient in the simple case where the experiment has a single
   //layer target oriented perpendicular to the beam. However, for more
   //complicated targets we can specify as follows :
   //
   //+155Gd + 238U 36 MeV/A
   //155 64 238 92 0.1 36.0
   //Target : 3 0.0                    target with 3 layers, angle 0 degrees
   //C 0.02                            1st layer : carbon, 20 g/cm2
   //238U 0.1                          2nd layer : uranium-238, 100 g/cm2
   //C 0.023                           3rd layer : carbon, 23 g/cm2
   //Run Range : 770 804
   //
   //Lines beginning '#' are comments.

   // create object table for targets
   create_root_object_table("Targets");
   int target_id = 1;

   std::ifstream fin;
   if (OpenCalibFile("Systems", fin)) {
      Info("ReadSystemList()", "Reading Systems parameters ...");

      TString line;

      char next_char = fin.peek();
      while (next_char != '+' && fin.good()) {
         line.ReadLine(fin, kFALSE);
         next_char = fin.peek();
      }

      while (fin.good() && !fin.eof() && next_char == '+') {
         KVDBSystem* sys = new KVDBSystem("NEW SYSTEM");
         AddSystem(sys);
         sys->Load(fin);
         if (sys->GetTarget()) {
            // store targets in ROOT file
            write_object_in_root_file(sys->GetTarget(), target_id, "Targets.root", "Targets");
            ++target_id;
         }
         next_char = fin.peek();
      }
      fin.close();
   } else {
      Error("ReadSystemList()", "Could not open file %s",
            GetCalibFileName("Systems"));
   }

   // Create & fill 'Runs' table in sqlite-db
   fill_database_from_runlist();

   // Set up 'Systems' table in sqlite-db
   // Columns are:
   //        sysid (INTEGER PRIMARY KEY)
   //        sysname (TEXT)
   //        target (INTEGER) : target id in Targets table

   KVSQLite::table systems("Systems");
   systems.add_primary_key("sysid");
   systems.add_column("sysname", "TEXT");
   systems.add_column("target", "INTEGER");
   systems.add_column("Zbeam", "INTEGER");
   systems.add_column("Abeam", "INTEGER");
   systems.add_column("Ztarget", "INTEGER");
   systems.add_column("Atarget", "INTEGER");
   systems.add_column("Ebeam", "REAL");
   fSQLdb.add_table(systems);

   // write systems data in dB table 'Systems'
   fSQLdb.prepare_data_insertion("Systems");
   TIter next(&fSystems);
   KVDBSystem* s;
   int sysid = 1;
   target_id = 1;
   KVSQLite::table& systab = fSQLdb["Systems"];
   while ((s = (KVDBSystem*)next())) {
      s->SetSysid(sysid);
      systab["sysname"].set_data(s->GetName());
      systab["Zbeam"].set_data((int)s->GetZbeam());
      systab["Abeam"].set_data((int)s->GetAbeam());
      systab["Ebeam"].set_data(s->GetEbeam());
      systab["Ztarget"].set_data((int)s->GetZtarget());
      systab["Atarget"].set_data((int)s->GetAtarget());
      if (s->GetTarget()) {
         systab["target"].set_data(target_id);
         ++target_id;
      } else {
         systab["target"].set_null();
      }
      fSQLdb.insert_data_row();
      ++sysid;
   }
   fSQLdb.end_data_insertion();

   // now link systems & runs
   next.Reset();
   while ((s = (KVDBSystem*)next())) {
      fSQLdb["Runs"]["sysid"].set_data(s->GetSysid());
      fSQLdb.update("Runs", s->GetRunList().GetSQL("Run Number"), "sysid");
      s->GetRunList().Begin();
      KVDBRun* r;
      while (!s->GetRunList().End()) if ((r = GetRun(s->GetRunList().Next()))) r->SetSystem(s);
   }
}
//__________________________________________________________________________________________________________________

void KVExpDB::WriteSystemsFile() const
{
   //Write the 'Systems.dat' file for this database.
   //The actual name of the file is given by the value of the environment variable
   //[dataset_name].INDRADB.Systems (if it exists), otherwise the value of
   //INDRADB.Systems is used. The file is written in the
   //$KVROOT/[dataset_directory] directory.

   ofstream sysfile;
   KVBase::SearchAndOpenKVFile(GetDBEnv("Systems"), sysfile, fDataSet.Data());
   TIter next(GetSystems());
   KVDBSystem* sys;
   TDatime now;
   sysfile << "# " << GetDBEnv("Systems") << " file written by "
           << ClassName() << "::WriteSystemsFile on " << now.AsString() << endl;
   cout << GetDBEnv("Systems") << " file written by "
        << ClassName() << "::WriteSystemsFile on " << now.AsString() << endl;
   while ((sys = (KVDBSystem*)next())) {
      if (strcmp(sys->GetName(), "[unknown]")) { //do not write dummy 'unknown' system
         sys->Save(sysfile);
         sysfile << endl;
      }
   }
   sysfile.close();
}
//__________________________________________________________________________________________________________________

void KVExpDB::Save(const Char_t* what)
{
   //Save (in the appropriate text file) the informations on:
   // what = "Systems" : write Systems.dat file
   // what = "Runlist" : write Runlist.csv
   if (!strcmp(what, "Systems")) WriteSystemsFile();
   else if (!strcmp(what, "Runlist")) WriteRunListFile();
}

//__________________________________________________________________________________________________________________

void KVExpDB::WriteRunListFile() const
{
   //Write a file containing a line describing each run in the database.
   //The delimiter symbol used in each line is '|' by default.
   //The first line of the file will be a header description, given by calling
   //KVDBRun::WriteRunListHeader() for the first run in the database.
   //Then we call KVDBRun::WriteRunListLine() for each run.
   //These are virtual methods redefined by child classes of KVDBRun.

   ofstream rlistf;
   KVBase::SearchAndOpenKVFile(GetDBEnv("Runlist"), rlistf, fDataSet.Data());
   TDatime now;
   rlistf << "# " << GetDBEnv("Runlist") << " file written by "
          << ClassName() << "::WriteRunListFile on " << now.AsString() << endl;
   cout << GetDBEnv("Runlist") << " file written by "
        << ClassName() << "::WriteRunListFile on " << now.AsString() << endl;
   if (GetRuns() && GetRuns()->GetEntries() > 0) {
      TIter next_run(GetRuns());
      //write header in file
      ((KVDBRun*) GetRuns()->At(0))->WriteRunListHeader(rlistf, GetDBEnv("Runlist.Separator")[0]);
      KVDBRun* run;
      while ((run = (KVDBRun*) next_run())) {

         run->WriteRunListLine(rlistf, GetDBEnv("Runlist.Separator")[0]);

      }
   } else {
      Warning("WriteRunListFile()", "run list is empty !!!");
   }
   rlistf.close();
}
//__________________________________________________________________________________________________________________

Bool_t KVExpDB::OpenCalibFile(const Char_t* type, ifstream& fs) const
{
   //Find and open calibration parameter file of given type. Return kTRUE if all OK.
   //types are defined in $KVROOT/KVFiles/.kvrootrc by lines such as (use INDRA as example)
   //
   //# Default name for file describing systems for each dataset.
   //INDRADB.Systems:     Systems.dat
   //
   //A file with the given name will be looked for in $KVROOT/KVFiles/name_of_dataset
   //where name_of_dataset is given by KVDataBase::fDataSet.Data()
   //i.e. the name of the associated dataset.
   //
   //Filenames specific to a given dataset may also be defined:
   //
   //INDRA_camp5.INDRADB.Pedestals:      Pedestals5.dat
   //
   //where 'INDRA_camp5' is the name of the dataset in question.
   //GetDBEnv() always returns the correct filename for the currently active dataset.

   return KVBase::SearchAndOpenKVFile(GetDBEnv(type), fs, fDataSet.Data());
}

//__________________________________________________________________________________________________________________

const Char_t* KVExpDB::GetDBEnv(const Char_t*) const
{
   Info("GetDBEnv", "To be defined in child class");
   return 0;
}

//__________________________________________________________________________________________________________________

void KVExpDB::PrintRuns(KVNumberList& nl) const
{
   // Print compact listing of runs in the number list like this:
   //
   // root [9] gIndraDB->PrintRuns("8100-8120")
   // RUN     SYSTEM                          TRIGGER         EVENTS          COMMENTS
   // ------------------------------------------------------------------------------------------------------------------
   // 8100    129Xe + 58Ni 8 MeV/A            M>=2            968673
   // 8101    129Xe + 58Ni 8 MeV/A            M>=2            969166
   // 8102    129Xe + 58Ni 8 MeV/A            M>=2            960772
   // 8103    129Xe + 58Ni 8 MeV/A            M>=2            970029
   // 8104    129Xe + 58Ni 8 MeV/A            M>=2            502992          disjonction ht chassis 1
   // 8105    129Xe + 58Ni 8 MeV/A            M>=2            957015          intensite augmentee a 200 pA

   printf("RUN\tSYSTEM\t\t\t\tTRIGGER\t\tEVENTS\t\tCOMMENTS\n");
   printf("------------------------------------------------------------------------------------------------------------------\n");
   nl.Begin();
   while (!nl.End()) {
      KVDBRun* run = GetRun(nl.Next());
      if (!run) continue;
      printf("%4d\t%-30s\t%s\t\t%d\t\t%s\n",
             run->GetNumber(), (run->GetSystem() ? run->GetSystem()->GetName() : "            "), run->GetTriggerString(),
             run->GetEvents(), run->GetComments());
   }
}

void KVExpDB::select_runs_in_dbtable(const TString& table, const KVNumberList& runs, const KVString& columns)
{
   // Select rows in sqlite DB table for given runs (we assume table has column "Run Number")
   GetDB().select_data(table, columns, runs.GetSQL("Run Number"));
}
