//Created by KVClassFactory on Tue Jul 12 11:43:52 2016
//Author: bonnet,,,

#ifndef __KVEXPDB_H
#define __KVEXPDB_H

#include "KVDataBase.h"
#include "KVDBRun.h"

class KVDBSystem;

class KVExpDB : public KVDataBase {

protected:

   Int_t kFirstRun;
   Int_t kLastRun;
   KVHashList fRuns;//! list of KVDBRun objects for all runs
   KVList fSystems;//! list of KVDBSystem objects for all systems

   Bool_t OpenCalibFile(const Char_t* type, std::ifstream& fs) const;
   void ReadSystemList();
   void init();

   virtual void fill_runlist_from_database();
   virtual void fill_systemlist_from_database();
   virtual void fill_database_from_runlist();

public:

   KVExpDB();
   KVExpDB(const Char_t* name);
   KVExpDB(const Char_t* name, const Char_t* title);

   void connect_to_database(const TString& path);
   virtual ~KVExpDB();

   void AddRun(KVDBRun* r)
   {
      fRuns.Add(r);
   }
   const TSeqCollection* GetRuns() const
   {
      return &fRuns;
   }
   KVDBRun* GetRun(Int_t number) const
   {
      return (KVDBRun*)fRuns.FindObject(Form("run%06d", number));
   }
   Int_t GetFirstRunNumber() const
   {
      return kFirstRun;
   }
   Int_t GetLastRunNumber() const
   {
      return kLastRun;
   }

   KVDBSystem* GetSystem(const Char_t* system) const
   {
      return (KVDBSystem*) fSystems.FindObject(system);
   }
   const KVSeqCollection* GetSystems() const
   {
      return &fSystems;
   }

   void AddSystem(KVDBSystem* r)
   {
      fSystems.Add(r);
   }
   void WriteSystemsFile() const;
   void WriteRunListFile() const;

   virtual void Save(const Char_t*);

   virtual const Char_t* GetDBEnv(const Char_t*) const;
   const Char_t* GetCalibFileName(const Char_t* type) const
   {
      return GetDBEnv(type);
   }

   virtual void WriteObjects(TFile*) {}
   virtual void ReadObjects(TFile*) {}
   virtual void PrintRuns(KVNumberList&) const;

   ClassDef(KVExpDB, 1) //base class to describe database of an experiment
};

#endif
