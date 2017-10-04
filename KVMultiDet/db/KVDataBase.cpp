#include "KVDataBase.h"
#include "TROOT.h"
#include "TPluginManager.h"
#include "KVNumberList.h"
#include "KVBase.h"
#include "TSystem.h"

KVDataBase* gDataBase = nullptr;

ClassImp(KVDataBase)

KVDataBase::KVDataBase(const Char_t* name, const Char_t* title)
   : KVBase(name, title)
{
   fDataSet = name;
}

//_______________________________________________________________________
TFile* KVDataBase::access_root_file(const TString& name, const TString& mode)
{
   // Return handle to ROOT file with given name (no path) opened with given mode
   // (="CREATE", "RECREATE", "UPDATE", "READ": default is "READ")
   // In case file is not already open, the fDataBaseDir path will be used to locate it
   // If file does not exist:
   //     if mode="READ": print error and return nullptr
   //     if mode="UPDATE": change mode to "CREATE", print warning
   // If file exists and is already open:
   //     if mode!="READ" and existing file mode="READ":
   //          close file & reopen with new mode, print warning
   //
   // If a valid TFile handle is returned, it will be made the current working directory
   // (i.e. TFile::cd() will be called).

   TString MODE(mode);
   MODE.ToUpper();
   TString full_path;
   AssignAndDelete(full_path, gSystem->ConcatFileName(GetDataBaseDir(), name));
   // is file already open ?
   TFile* handle = fROOTFiles.get_object<TFile>(full_path);
   if (handle) {
      TString file_mode(handle->GetOption());
      // file already open
      if (MODE == "READ") {
         // "read" mode requested: doesn't matter if already open in (re)create/update mode (?)
         handle->cd();
         return handle;
      } else if (file_mode == "READ") {
         // file opened in "read" mode, writing mode requested
         Warning("access_root_file", "File %s previously opened in READ mode. Reopening with requested %s mode.",
                 full_path.Data(), MODE.Data());
         fROOTFiles.Remove(handle);
         delete handle;
         handle = TFile::Open(full_path, MODE);
         fROOTFiles.Add(handle);
         return handle;
      }
   } else {
      // file is not open
      // does file exist on disk ?
      bool file_exists = !gSystem->AccessPathName(full_path);
      if (!file_exists) {
         if (MODE == "READ") {
            // request to open non-existent file in read-only mode
            Error("access_root_file", "Request to open file %s in READ mode. File does not exist.",
                  full_path.Data());
            return nullptr;
         } else if (MODE == "UPDATE") {
            // can't update a file which doesn't exist; print warning and "CREATE" file
            MODE = "CREATE";
            Warning("access_root_file", "Request to UPDATE file %s which doesn't exist. File will be created.",
                    full_path.Data());
         }
      } else {
         if (MODE == "CREATE") {
            // file exists already
            Error("access_root_file", "Request to CREATE file %s, which already exists. Nullptr returned.",
                  full_path.Data());
            return nullptr;
         } else if (MODE == "RECREATE") {
            // file exists already
            Warning("access_root_file", "Request to RECREATE file %s, which already exists. Existing file will be overwritten.",
                    full_path.Data());
         }
      }
      handle = TFile::Open(full_path, MODE);
      fROOTFiles.Add(handle);
   }
   handle->cd();
   return handle;
}

void KVDataBase::create_root_object_table(const TString& name)
{
   // Add table to database to keep track of objects stored in a ROOT file
   // These tables all have the same structure:
   //
   //  | id | objectClass | fileName | uuid |

   KVSQLite::table t(name.Data());
   t.add_column("id", "INTEGER");
   t.add_column("objectClass", "TEXT");
   t.add_column("fileName", "TEXT");
   t.add_column("uuid", "TEXT");
   fSQLdb.add_table(t);
}

void KVDataBase::write_object_in_root_file(TObject* o, Int_t id, const TString& root_file_name, const TString& object_table)
{
   // Write the object in the ROOT file "root_file_name" (do not give path: we use fDataBaseDir)
   // The given "object_table" (previously created by a call to create_root_object_table()) will be
   // updated with the references needed to retrieve the object using its (unique) id number.

   TDirectory* cwd_save = gDirectory;
   TFile* f = access_root_file(root_file_name, "update");//will create file if necessary (with warning)
   if (!f) {
      Error("write_object_in_root_file",
            "Problem writing object %s of class %s in file %s",
            o->GetName(), o->ClassName(), root_file_name.Data());
      return;
   }
   TUUID uuid; // unique key for writing object
   // update object table
   fSQLdb.prepare_data_insertion(object_table);
   fSQLdb[object_table.Data()]["id"].set_data(id);
   fSQLdb[object_table.Data()]["objectClass"].set_data(o->ClassName());
   fSQLdb[object_table.Data()]["fileName"].set_data(root_file_name);
   fSQLdb[object_table.Data()]["uuid"].set_data(uuid.AsString());
   fSQLdb.insert_data_row();
   fSQLdb.end_data_insertion();
   // write object in file using unique key
   o->Write(uuid.AsString());
   cwd_save->cd();
}

TObject* KVDataBase::read_object_from_root_file(Int_t id, const TString& object_table)
{
   // Read in the object with given id in the object_table

   TDirectory* cwd_save = gDirectory;
   fSQLdb.select_data(object_table, "*", Form("id=%d", id));
   TObject* o = nullptr;
   while (fSQLdb.get_next_result()) {
      TString root_file = fSQLdb[object_table.Data()]["fileName"].get_data<TString>();
      TFile* f = access_root_file(root_file);
      if (!f) {
         Error("read_object_from_root_file", "Problem accessing file %s while looking for object with id=%d in table %s.",
               root_file.Data(), id, object_table.Data());
      } else {
         o = f->Get(fSQLdb[object_table.Data()]["uuid"].get_data<TString>());
      }
   }
   cwd_save->cd();
   return o;
}

KVDataBase::KVDataBase(const Char_t* name)
   : KVBase(name, "database")
{
   fDataSet = name;
}

//_______________________________________________________________________
KVDataBase::~KVDataBase()
{
   //reset global database pointer if it was pointing to this DB
   if (gDataBase == this)
      gDataBase = nullptr;
}

void KVDataBase::connect_to_database(const TString& path)
{
   // Open the SQLite database file DataBase.sqlite at 'path'
   // The path will be stored in fDataBaseDir
   // If the file does not exist, this will trigger Build()

   fDataBaseDir = path;
   TString dbfile = path + "/DataBase.sqlite";
   fSQLdb.open(dbfile);
   if (!fSQLdb.get_number_of_tables()) Build();
}

//__________________________________________________________________________
void KVDataBase::Build()
{
//  Methode that builds the DataBase from the parameter files
//  It does nothing here. Must be derived for each Database.
//
//
   AbstractMethod("Build");

}

void KVDataBase::Print(Option_t*) const
{
}

void KVDataBase::cd()
{
   // Set global gDataBase pointer to this database
   gDataBase = this;
}


//_________________________________________________________________________________

KVDataBase* KVDataBase::MakeDataBase(const Char_t* name, const Char_t* datasetdir)
{
   //Static function which will create and 'Build' the database object corresponding to 'name'
   //These are defined as 'Plugin' objects in the file $KVROOT/KVFiles/.kvrootrc :
   //
   //      Plugin.KVDataBase:    INDRA_camp1    KVDataBase1     KVIndra    "KVDataBase1()"
   //      +Plugin.KVDataBase:    INDRA_camp2    KVDataBase2     KVIndra    "KVDataBase2()"
   //      +Plugin.KVDataBase:    INDRA_camp4    KVDataBase4     KVIndra    "KVDataBase4()"
   //      +Plugin.KVDataBase:    INDRA_camp5    KVDataBase5     KVIndra5    "KVDataBase5()"
   //
   //The 'name' ("INDRA_camp1" etc.) corresponds to the name of a dataset in $KVROOT/KVFiles/manip.list
   //This name is stored in member variable fDataSet.
   //The constructors/macros used have arguments (const Char_t* name)

   //does plugin exist for given name ?
   TPluginHandler* ph;
   if (!(ph = KVBase::LoadPlugin("KVDataBase", name))) {
      return 0;
   }
   //execute constructor/macro for database
   KVDataBase* mda = (KVDataBase*) ph->ExecPlugin(1, name);
   mda->SetDataSetDir(datasetdir);
   return mda;
}
