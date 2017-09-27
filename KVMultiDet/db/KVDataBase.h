#ifndef KV_DATA_BASE_H
#define KV_DATA_BASE_H
#include "KVBase.h"
#include "TString.h"
#include "SQLiteDB.h"
#include "KVList.h"
class TFile;
class KVNumberList;

class KVDataBase: public KVBase {

protected:
   KVSQLite::database fSQLdb;//! interface to SQLite backend
   TString fDataSet;//the name of the dataset to which this database is associated
   TString fDataSetDir;//the directory containing the dataset files
   TString fDataBaseDir;//the directory containing the database and associated files
   KVList  fROOTFiles;//associated files containing ROOT objects

   TFile* access_root_file(const TString& name, const TString& mode = "READ");
   void create_root_object_table(const TString& name);
   void write_object_in_root_file(TObject*, Int_t id, const TString& root_file_name, const TString& object_table);
   TObject* read_object_from_root_file(Int_t id, const TString& object_table);

public:
   KVDataBase() {}
   KVDataBase(const Char_t* name);
   KVDataBase(const Char_t* name, const Char_t* title);
   virtual ~ KVDataBase();
   virtual void connect_to_database(const TString& path);
   const TString& GetDataBaseDir() const
   {
      return fDataBaseDir;
   }
   KVSQLite::database& GetDB() const
   {
      return const_cast<KVSQLite::database&>(fSQLdb);
   }

   virtual void Build();
   virtual void Print(Option_t* option = "") const;
   virtual void cd();

   static KVDataBase* MakeDataBase(const Char_t* name, const Char_t* datasetdir);

   virtual void Save(const Char_t*)
   {
      ;
   }

   const Char_t* GetDataSetDir() const
   {
      return fDataSetDir;
   }
   void SetDataSetDir(const Char_t* d)
   {
      fDataSetDir = d;
   }


   ClassDef(KVDataBase, 3)     // Base Class for a database of parameters
};

//........ global variable
R__EXTERN KVDataBase* gDataBase;

#endif
