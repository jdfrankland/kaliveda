#include "KVDataBase.h"
#include "TROOT.h"
#include "TPluginManager.h"
#include "KVNumberList.h"
#include "KVBase.h"

KVDataBase* gDataBase = nullptr;

ClassImp(KVDataBase)

KVDataBase::KVDataBase(const Char_t* name, const Char_t* title)
   : KVBase(name, title)
{
   fDataSet = name;
}

//_______________________________________________________________________
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
      gDataBase = 0;
}

void KVDataBase::connect_to_database(const TString& path)
{
   // Open the SQLite database file DataBase.sqlite at 'path'
   // If the file does not exist, this will trigger Build()

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

//______________________________________________________________________________

void KVDataBase::WriteObjects(TFile*)
{
   // Abstract method. Can be overridden in child classes.
   // When the database is written to disk (by the currently active dataset, see
   // KVDataSet::WriteDBFile) any associated objects (histograms, trees, etc.)
   // can be written using this method.
   // The pointer to the file being written is passed as argument.

   AbstractMethod("WriteObjects");
}

//______________________________________________________________________________

void KVDataBase::ReadObjects(TFile*)
{
   // Abstract method. Can be overridden in child classes.
   // When the database is read from disk (by the currently active dataset, see
   // KVDataSet::OpenDBFile) any associated objects (histograms, trees, etc.)
   // stored in the file can be read using this method.
   // The pointer to the file being read is passed as argument.

   AbstractMethod("ReadObjects");
}

