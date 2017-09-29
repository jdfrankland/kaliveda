/*
$Id: KVINDRAPulserDataTree.h,v 1.5 2009/03/27 16:42:58 franklan Exp $
$Revision: 1.5 $
$Date: 2009/03/27 16:42:58 $
*/

//Created by KVClassFactory on Wed Jan 21 11:56:26 2009
//Author: franklan

#ifndef __KVINDRAPULSERDATATREE_H
#define __KVINDRAPULSERDATATREE_H

#include "KVBase.h"
#include "TTree.h"
#include "THashTable.h"
#include "Riostream.h"
#include "KVString.h"
#include "KVTarArchive.h"

#include "KVConfig.h"
#ifdef WITH_CPP11
#include <unordered_map>
#else
#include <map>
#endif

class KVINDRAPulserDataTree : public KVBase {
#if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
   KVINDRAPulserDataTree(const KVINDRAPulserDataTree&) : KVBase() {} // avoid problems with ROOT5 dictionary/gcc6
   KVINDRAPulserDataTree& operator=(const KVINDRAPulserDataTree&)
   {
      return *this;   // avoid problems with ROOT5 dictionary/gcc6
   }
#endif
protected:
   unique_ptr<TFile> fArbFile;//!file containing tree
   TTree* fArb;//!tree containing pulser data
   unique_ptr<KVTarArchive> fGeneDir;//directory/archive containing gene data
   unique_ptr<KVTarArchive> fPinDir;//directory/archive containing pin data
   Int_t fRun;//!run number used to build tree
#ifdef WITH_CPP11
   std::unordered_map<std::string, Float_t> fParameters;
   using ParameterMap = std::unordered_map<std::string, Float_t>;
#else
   std::map<std::string, Float_t> fParameters;
   typedef std::map<std::string, Float_t> ParameterMap;
#endif
   const TSeqCollection* fRunlist;//!list of runs given by database
   KVString fROOTFileDirectory;//directory in which to write "PulserData.root" file
   Bool_t fNoData;//no ROOT file available

   TString GetDirectoryName(const Char_t*);
   void CreateTree();
   void ReadData();
   UChar_t ReadData(Int_t);
   void ReadFile(std::ifstream&);
   Bool_t OpenGeneData(Int_t, std::ifstream&);
   Bool_t OpenPinData(Int_t, std::ifstream&);
   void ReadTree();
   void WriteTree();

public:
   KVINDRAPulserDataTree(const KVString&, Bool_t reading = kTRUE);
   virtual ~KVINDRAPulserDataTree();

   Bool_t HasData() const
   {
      return !fNoData;
   }

   void Build();
   TTree* GetTree() const
   {
      return fArb;
   }

   void SetRunList(const TSeqCollection* runs)
   {
      fRunlist = runs;
   }

   Float_t GetMean(const Char_t*, Int_t);

   ClassDef(KVINDRAPulserDataTree, 0) //Handles TTree with mean pulser data for every run
};

#endif
