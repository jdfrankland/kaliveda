#include "KVSQLROOTFile.h"
#include "TSystem.h"

ClassImp(KVSQLROOTFile)

KVString KVSQLROOTFile::UUID_for_object(const KVString& name) const
{
   // Return unique identifier used to store object with given name in ROOT file

   fObjDB->select_data("objTable", "unique_id", Form("name = \"%s\"", name.Data()));
   KVString uuid;
   while (fObjDB->get_next_result())
      uuid = get_objTable()["unique_id"].get_data<KVString>();
   return uuid;
}

TObject* KVSQLROOTFile::get_object_with_UUID(const KVString& uuid) const
{
   // Retrieve object from file using its name

   auto it = fObjList.find(uuid.Data());// object already retrieved from file ?
   if (it != fObjList.end()) return it->second;
   auto t = fObjStore->Get(uuid);
   fObjList[uuid.Data()] = t;
   return t;
}

KVSQLROOTFile::KVSQLROOTFile(const KVString& filepath, Option_t* option)
{
   // Open or (re)create a combined ROOT object store & SQL database with info on the stored objects.
   //
   // \param[in] filepath path to file. any shell variables will be expanded.
   // \param[in] option see TFile constructors for possible values. default="READ".

   TString FP = filepath;
   TString OPT = option;
   OPT.ToUpper();

   gSystem->ExpandPathName(FP);
   if (OPT.Contains("CREATE")) {
      // create directory using path
      gSystem->mkdir(filepath, true); // recursive: create any missing directories in path
   }
   FP.Append("/");
   TString rpath = FP + "objStore.root";
   if (OPT == "RECREATE") gSystem->Unlink(rpath);
   save_working_directory();
   fObjStore.reset(new TFile(rpath, OPT));
   restore_working_directory();
   TString sqlpath = FP + "objInfos.sqlite";
   if (OPT == "RECREATE") gSystem->Unlink(sqlpath);
   fObjDB.reset(new KVSQLite::database(sqlpath));
   if (OPT.Contains("CREATE")) {
      // set up object table
      KVSQLite::table tmp("objTable");
      tmp.add_primary_key("obj_idx");
      tmp.add_column("name", "TEXT");
      tmp.add_column("class", "TEXT");
      tmp.add_column("unique_id", "TEXT");
      fObjDB->add_table(tmp);
   }
}

void KVSQLROOTFile::WriteObject(const TObject* obj, const KVNameValueList& infos)
{
   // Write object in ROOT file, store infos given in list

   if (!fObjDB->has_table("objInfos")) {
      // set up object info table if not already done
      KVSQLite::table tmp("objInfos");
      tmp.add_primary_key("info_idx");
      tmp.add_foreign_key("objTable", "obj_idx");
      fObjDB->add_table(tmp);
   }
   // add any missing columns to info table
   fObjDB->add_missing_columns("objInfos", infos);
   // get unique ID for object
   TUUID unid;
   TString unique_id = unid.AsString();
   // write object in ROOT file
   save_working_directory();
   fObjStore->cd();
   obj->Write(unique_id);
   restore_working_directory();
   // store infos on object
   // 1 - write name, class & unique id in object table
   fObjDB->prepare_data_insertion("objTable");
   get_objTable()["name"] = obj->GetName();
   get_objTable()["class"] = obj->ClassName();
   get_objTable()["unique_id"] = unique_id;
   fObjDB->insert_data_row();
   fObjDB->end_data_insertion();
   // 2 - get index of object from object table
   fObjDB->select_data("objTable", "obj_idx", Form("unique_id=\"%s\"", unique_id.Data()));
   int obj_idx;
   while (fObjDB->get_next_result()) obj_idx = get_objTable()["obj_idx"].get_data<int>();
   // 3 - add infos on object in info table
   fObjDB->prepare_data_insertion("objInfos");
   KVNameValueList info_copy(infos);
   info_copy.SetValue("obj_idx", obj_idx);
   //get_objInfos()["obj_idx"]=obj_idx;
   //for(auto& info : infos) get_objInfos()[info.GetName()]=info;
   get_objInfos().prepare_data(info_copy);
   fObjDB->insert_data_row();
   fObjDB->end_data_insertion();
}

TObject* KVSQLROOTFile::Get(const KVString& name) const
{
   // Return pointer to object with given name

   return get_object_with_UUID(UUID_for_object(name));
}

void KVSQLROOTFile::ls(Option_t*) const
{
   // List the contents of the file with associated infos
   KVString colnames = "name,class,";
   colnames += get_objInfos().get_column_names("obj_idx info_idx");
   colnames.Begin(",");
   TString tabs("\t\t\t\t");
   while (!colnames.End()) {
      std::cout << colnames.Next() << tabs;
   }
   std::cout << "\n==========================================================================================\n";
   if (fObjDB->select_data("objTable,objInfos", colnames)) {
      while (fObjDB->get_next_result()) {
         colnames.Begin(",");
         while (!colnames.End()) {
            auto colname = colnames.Next();
            if (colname == "name" || colname == "class")
               std::cout << get_objTable()[colname].get_data<TString>() << tabs;
            else
               std::cout << get_objInfos()[colname].get_data<TString>() << tabs;
         }
         std::cout << std::endl;
      }
   }
   else {
      Error("ls", "Problem with KVSQLite::database::select_data");
   }
}

void KVSQLROOTFile::FillListOfObjectsWithSelection(KVSeqCollection& list, const KVString& where)
{
   // Fill the list given as argument with pointers to all objects which obey the given selection.
   //
   // The 'where' string will be used as the `WHERE` clause of an SQLite selection.
   //
   // Examples:
   //
   //~~~~
   //WHERE column_1 = 100;
   //
   //WHERE column_2 IN (1,2,3);
   //
   //WHERE column_3 LIKE 'An%';
   //
   //WHERE column_4 BETWEEN 10 AND 20;
   //~~~~
   //
   // Note that string arguments should be enclosed in single quotes.
   //
   // The columns of both the `objTable` (`name`, `class`) and `objInfos` tables can be used in the selection.

   fObjDB->select_data("objTable,objInfos", "unique_id", where);
   while (fObjDB->get_next_result()) {
      auto uuid = get_objTable()["unique_id"].get_data<KVString>();
      list.Add(get_object_with_UUID(uuid));
   }
}
void KVSQLROOTFile::FillListOfObjectsWithSelection(KVSeqCollection& list, const KVString& where, const KVString& numberlist_column, int value)
{
   // Fill the list given as argument with pointers to all objects which obey the given selection.
   // 'numberlist_column' is the name of a column in the `objInfos` table containing strings which may be
   // interpreted as KVNumberList objects. Only the entries with a number list containing 'value'
   // will be selected.
   //
   // The 'where' string will be used as the `WHERE` clause of an SQLite selection.
   //
   // Examples:
   //
   //~~~~
   //WHERE column_1 = 100;
   //
   //WHERE column_2 IN (1,2,3);
   //
   //WHERE column_3 LIKE 'An%';
   //
   //WHERE column_4 BETWEEN 10 AND 20;
   //~~~~
   //
   // Note that string arguments should be enclosed in single quotes.
   //
   // The columns of both the `objTable` (`name`, `class`) and `objInfos` tables can be used in the selection.

   fObjDB->select_data("objTable,objInfos", Form("unique_id,%s", numberlist_column.Data()), where);
   while (fObjDB->get_next_result()) {
      KVNumberList nl(get_objInfos()[numberlist_column].get_data<KVString>());
      if (nl.Contains(value)) {
         auto uuid = get_objTable()["unique_id"].get_data<KVString>();
         list.Add(get_object_with_UUID(uuid));
      }
   }
}
