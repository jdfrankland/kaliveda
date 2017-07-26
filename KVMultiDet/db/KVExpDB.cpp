//Created by KVClassFactory on Tue Jul 12 11:43:52 2016
//Author: bonnet,,,

#include "KVExpDB.h"

#include "KVDBSystem.h"
#include "KVNumberList.h"

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

}

void KVExpDB::fill_systemlist_from_database()
{

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
         next_char = fin.peek();
      }
      fin.close();
   } else {
      Error("ReadSystemList()", "Could not open file %s",
            GetCalibFileName("Systems"));
   }
   // if any runs are not associated with any system
   // we create an 'unknown' system and associate it to all runs
//   KVDBSystem* sys = 0;
//   TIter nextRun(GetRuns());
//   KVDBRun* run;
//   while ((run = (KVDBRun*)nextRun())) {
//      if (!run->GetSystem()) {
//         if (!sys) {
//            sys = new KVDBSystem("[unknown]");
//            AddSystem(sys);
//         }
//         sys->AddRun(run->GetNumber());
//      }
//   }

   // write systems data in dB table 'Systems'
   fSQLdb.prepare_data_insertion("Systems");
   TIter next(&fSystems);
   KVDBSystem* s;
   int sysid = 1;
   while ((s = (KVDBSystem*)next())) {
      s->SetSysid(sysid);
      fSQLdb["Systems"]["sysname"].set_data(s->GetName());
      fSQLdb["Systems"]["runlist"].set_data(s->GetRunList().AsString());
      fSQLdb.insert_data_row();
      ++sysid;
   }
   fSQLdb.end_data_insertion();
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

void KVExpDB::Build()
{
   // Read systems list
   // Set up 'Systems' table in sqlite-db
   // Columns are:
   //        sysid (INTEGER PRIMARY KEY)
   //        sysname (TEXT)
   //        runlist (TEXT) : runlist in KVNumberList format

   KVSQLite::table systems("Systems");
   systems.add_primary_key("sysid");
   systems.add_column("sysname", "TEXT");
   systems.add_column("runlist", "TEXT");
   fSQLdb.add_table(systems);

   ReadSystemList();
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
