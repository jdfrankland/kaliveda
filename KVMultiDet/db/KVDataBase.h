#ifndef KV_DATA_BASE_H
#define KV_DATA_BASE_H
#include "KVBase.h"
#include "TString.h"
#include "SQLiteDB.h"

class TFile;
class KVNumberList;

class KVDataBase: public KVBase {

protected:
   KVSQLite::database fSQLdb;//! interface to SQLite backend
   TString fDataSet;//the name of the dataset to which this database is associated
   TString fDataSetDir;//the directory containing the dataset files
   KVSQLite::database& GetDB() const
   {
      return const_cast<KVSQLite::database&>(fSQLdb);
   }

public:
   KVDataBase() {}
   KVDataBase(const Char_t* name);
   KVDataBase(const Char_t* name, const Char_t* title);
   virtual ~ KVDataBase();
   virtual void connect_to_database(const TString& path);

   virtual void Build();
   virtual void Print(Option_t* option = "") const;
   virtual void cd();

   static KVDataBase* MakeDataBase(const Char_t* name, const Char_t* datasetdir);

   virtual void Save(const Char_t*)
   {
      ;
   }

   virtual void WriteObjects(TFile*);
   virtual void ReadObjects(TFile*);

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
