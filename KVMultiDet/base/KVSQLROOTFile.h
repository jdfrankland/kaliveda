#ifndef __KVSQLROOTFILE_H
#define __KVSQLROOTFILE_H

#include "KVBase.h"
#include "TFile.h"
#include "SQLiteDB.h"
#include <unordered_map>

/**
 \class KVSQLROOTFile
 \brief Combine ROOT file containing objects with SQLite database with info on the objects

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Tue Sep  6 16:49:41 2022
*/

class KVSQLROOTFile : public KVBase {
   std::unique_ptr<TFile>              fObjStore;//!
   std::unique_ptr<KVSQLite::database> fObjDB;//!

   TDirectory* savDir;//!
   void save_working_directory()
   {
      savDir = gDirectory;
   }
   void restore_working_directory()
   {
      savDir->cd();
   }

   KVSQLite::table& get_objTable() const
   {
      return fObjDB->get_table("objTable");
   }
   KVSQLite::table& get_objInfos() const
   {
      return fObjDB->get_table("objInfos");
   }

   mutable std::unordered_map<std::string, TObject*> fObjList; //! for quick look-up of objects using unique id

   KVString UUID_for_object(const KVString&) const;
   TObject* get_object_with_UUID(const KVString& name) const;

   TString fCurrentROOTFilePath;// full path to current ROOT file
public:
   KVSQLROOTFile(const KVString& filepath, Option_t* option = "READ");

   ~KVSQLROOTFile();

   void WriteObject(const TObject*, const KVNameValueList&);

   TObject* Get(const KVString& name) const;

   void ls(Option_t* = "") const;

   void FillListOfObjectsWithSelection(KVSeqCollection* list, const KVString& where);
   void FillListOfObjectsWithSelection(KVSeqCollection* list, const KVString& where, const KVString& numberlist_column,
                                       int value);

   ClassDef(KVSQLROOTFile, 1) //Combine ROOT file containing objects with SQLite database with info on the objects
};

#endif
