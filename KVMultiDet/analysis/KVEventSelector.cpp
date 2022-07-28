#define KVEventSelector_cxx
#include "KVEventSelector.h"
#include <KVClassMonitor.h>
#include <TStyle.h>
#include "TPluginManager.h"
#include "TSystem.h"
#include "KVDataRepositoryManager.h"
#include "KVDataRepository.h"
#include "KVDataSetManager.h"
#include "TProof.h"
#include "KVDataSetAnalyser.h"

using namespace std;

ClassImp(KVEventSelector)



void KVEventSelector::Begin(TTree* /*tree*/)
{
   // Need to parse options here for use in Terminate
   // Also, on PROOF, any KVDataAnalyser instance has to be passed to the workers
   // via the TSelector input list.

   ParseOptions();

   if (IsOptGiven("CombinedOutputFile")) {
      fCombinedOutputFile = GetOpt("CombinedOutputFile");
   }
   else if (gProof) {
      // when running with PROOF, if the user calls SetCombinedOutputFile()
      // in InitAnalysis(), it will only be executed on the workers (in SlaveBegin()).
      // therefore we call InitAnalysis() here, but deactivate CreateTreeFile(),
      // AddTree() and AddHisto() in order to avoid interference with workers
      fDisableCreateTreeFile = kTRUE;
      if (gDataAnalyser) {
         gDataAnalyser->RegisterUserClass(this);
         gDataAnalyser->preInitAnalysis();
      }
      InitAnalysis();              //user initialisations for analysis
      if (gDataAnalyser) gDataAnalyser->postInitAnalysis();
      fDisableCreateTreeFile = kFALSE;
   }

   if (gDataAnalyser) {
      if (GetInputList()) {
         gDataAnalyser->AddJobDescriptionList(GetInputList());
         //GetInputList()->ls();
      }
   }
   if (IsOptGiven("AuxFiles")) {
      SetUpAuxEventChain();
      if (GetInputList()) GetInputList()->Add(fAuxChain);
   }
}

void KVEventSelector::SlaveBegin(TTree* /*tree*/)
{
   // The SlaveBegin() function is called after the Begin() function.
   // When running with PROOF SlaveBegin() is called on each slave server.
   // The tree argument is deprecated (on PROOF 0 is passed).
   //
   // ParseOptions : Manage options passed as arguments
   //
   // Called user method InitAnalysis where users can create trees or histos
   // using the appropiate methods :
   // CreateTrees and CreateMethods
   //
   // Test the presence or not of such histo or tree
   // to manage it properly

   if (GetInputList() && GetInputList()->FindObject("JobDescriptionList")) {
      KVNameValueList* jdl = dynamic_cast<KVNameValueList*>(GetInputList()->FindObject("JobDescriptionList"));
      if (jdl) {
         KVDataAnalysisTask* task = nullptr;
         if (jdl->HasParameter("DataRepository")) {
            if (!gDataRepositoryManager) {
               new KVDataRepositoryManager;
               gDataRepositoryManager->Init();
            }
            gDataRepositoryManager->GetRepository(jdl->GetStringValue("DataRepository"))->cd();
            gDataSetManager->GetDataSet(jdl->GetStringValue("DataSet"))->cd();
            task = gDataSet->GetAnalysisTask(jdl->GetStringValue("AnalysisTask"));
         }
         else {
            if (!gDataSetManager) {
               gDataSetManager = new KVDataSetManager;
               gDataSetManager->Init();
            }
            task = gDataSetManager->GetAnalysisTaskAny(jdl->GetStringValue("AnalysisTask"));
         }
         gDataAnalyser = KVDataAnalyser::GetAnalyser(task->GetDataAnalyser());
         if (gDataSet && gDataAnalyser->InheritsFrom("KVDataSetAnalyser"))
            dynamic_cast<KVDataSetAnalyser*>(gDataAnalyser)->SetDataSet(gDataSet);
         gDataAnalyser->SetAnalysisTask(task);
         gDataAnalyser->RegisterUserClass(this);
         gDataAnalyser->SetProofMode((KVDataAnalyser::EProofMode)jdl->GetIntValue("PROOFMode"));
      }
   }

   ParseOptions();

   if (IsOptGiven("CombinedOutputFile")) {
      fCombinedOutputFile = GetOpt("CombinedOutputFile");
      Info("SlaveBegin", "Output file name = %s", fCombinedOutputFile.Data());
   }

   // tell the data analyser who we are
   if (gDataAnalyser) {
      gDataAnalyser->RegisterUserClass(this);
      gDataAnalyser->preInitAnalysis();
   }
   // make sure histo/tree creation are enabled
   fDisableCreateTreeFile = kFALSE;
   InitAnalysis();              //user initialisations for analysis
   if (gDataAnalyser) gDataAnalyser->postInitAnalysis();

   if (IsOptGiven("AuxFiles")) {
      // auxiliary files with PROOF
      if (GetInputList()) {
         fAuxChain = (TTree*)GetInputList()->FindObject(GetOpt("AuxTreeName"));
         InitFriendTree(fAuxChain, GetOpt("AuxBranchName"));
      }
   }
   Info("SlaveBegin", "fOutput->ls()");
   GetOutputList()->ls();
}

Bool_t KVEventSelector::Process(Long64_t entry)
{
   // The Process() function is called for each entry in the tree (or possibly
   // keyed object in the case of PROOF) to be processed. The entry argument
   // specifies which entry in the currently loaded tree is to be processed.
   // It can be passed to either KVEventSelector::GetEntry() or TBranch::GetEntry()
   // to read either all or the required parts of the data. When processing
   // keyed objects with PROOF, the object is already loaded and is available
   // via the fObject pointer.
   //
   // This function should contain the "body" of the analysis. It can contain
   // simple or elaborate selection criteria, run algorithms on the data
   // of the event and typically fill histograms.
   //
   // Processing will abort cleanly if static flag fCleanAbort has been set
   // by some external controlling process.
   //
   // Use fStatus to set the return value of TTree::Process().
   //
   // The return value is currently not used.

   if (gDataAnalyser && gDataAnalyser->AbortProcessingLoop()) {
      // abort requested by external process
      Abort(Form("Job received KILL signal from batch system after %lld events - batch job probably needs more CPU time (see end of job statistics)", fEventsRead), kAbortFile);
      return kFALSE;
   }

   fTreeEntry = entry;

   if (gDataAnalyser && gDataAnalyser->CheckStatusUpdateInterval(fEventsRead))
      gDataAnalyser->DoStatusUpdate(fEventsRead);

   GetEntry(entry);
   if (gDataAnalyser) gDataAnalyser->preAnalysis();
   fEventsRead++;
   if (GetEvent()) {
      //apply particle selection criteria
      if (fPartCond.IsSet()) {
         for (auto& part : EventOKIterator(GetEvent())) {
            part.SetIsOK(fPartCond.Test(part));
         }
      }
      SetAnalysisFrame();//let user define any necessary reference frames for OK particles

      // initialise global variables at first event
      if (fFirstEvent && !gvlist.IsEmpty()) {
         gvlist.Init();
         fFirstEvent = kFALSE;
      }
      RecalculateGlobalVariables();
   }

   Bool_t ok_anal = kTRUE;
   if (gvlist.IsEmpty() || !gvlist.AbortEventAnalysis()) {
      // if global variables are defined, events are only analysed if no global variables
      // in the list have failed an event selection condition
      ok_anal = Analysis();     //user analysis
      if (gDataAnalyser) gDataAnalyser->postAnalysis();
   }
   CheckEndOfRun();

   return ok_anal;
}

void KVEventSelector::CheckEndOfRun()
{
   // Testing whether EndRun() should be called
   if (AtEndOfRun()) {
      Info("Process", "End of file reached after %lld events", fEventsRead);
      if (gDataAnalyser) gDataAnalyser->preEndRun();
      EndRun();
      if (gDataAnalyser) gDataAnalyser->postEndRun();
      fNotifyCalled = kFALSE;//Notify will be called when next file is opened (in TChain)
   }

}

void KVEventSelector::SlaveTerminate()
{
   // The SlaveTerminate() function is called after all entries or objects
   // have been processed. When running with PROOF SlaveTerminate() is called
   // on each slave server.

   if (writeFile) {
      auto sav_dir = gDirectory;//save current working directory

      writeFile->cd();
      // loop over all histos and trees, writing them in the output file
      if (lhisto.GetEntries()) {
         lhisto.R__FOR_EACH(TH1, Write)();
      }
      if (ltree.GetEntries()) {
         ltree.R__FOR_EACH(TTree, Write)(0, TObject::kOverwrite);
      }
      writeFile->Close();
      if (gDataAnalyser->GetProofMode() != KVDataAnalyser::EProofMode::None) {
         fOutput->Add(mergeFile);
      }

      sav_dir->cd();
   }
}

void KVEventSelector::Terminate()
{
   // The Terminate() function is the last function to be called during
   // a query. It always runs on the client, it can be used to present
   // the results graphically or save the results to file.
   //
   // This method call the user defined EndAnalysis
   // where user can do what she wants
   //

   TDatime now;
   Info("Terminate", "Analysis ends at %s", now.AsString());

   if (gDataAnalyser) gDataAnalyser->preEndAnalysis();
   EndAnalysis();               //user end of analysis routine
   if (gDataAnalyser) gDataAnalyser->postEndAnalysis();

   if (GetInputList() && fAuxChain) GetInputList()->Remove(fAuxChain);
   SafeDelete(fAuxChain);
}

KVVarGlob* KVEventSelector::AddGV(const Char_t* class_name,
                                  const Char_t* name)
{
   //Add a global variable to the list of variables for the analysis.
   //
   //`"class_name"` must be the name of a valid class inheriting from KVVarGlob, e.g. any of the default global
   //variable classes defined as part of the standard KaliVeda package (see the [global variables module](group__Globvars.html)),
   //or the name of a user-defined class (see below).
   //
   //`"name"` is a unique name for the new global variable object which will be created and added to the internal
   //list of global variables. This name can be used to retrieve the object (see GetGV()) in the user's analysis.
   //
   //Returns pointer to new global variable object in case more than the usual default initialisation is necessary.
   //
   //### User-defined global variables
   //The user may use her own global variables in an analysis class, without having to add them to the main libraries.
   //If the given class name is not known, it is assumed to be a user-defined class and we attempt to compile and load
   //the class from the user's source code. For this to work, the user must:
   //
   //  1. add to the ROOT macro path the directory where her class's source code is kept, e.g. in `$HOME/.rootrc` add the following line:
   //
   //~~~~~~~~~~~~~~~
   //              +Unix.*.Root.MacroPath:      $(HOME)/myVarGlobs
   //~~~~~~~~~~~~~~~
   //
   //  2. for each user-defined class, add a line to $HOME/.kvrootrc to define a "plugin". E.g. for a class called MyNewVarGlob,
   //
   //~~~~~~~~~~~~~~~
   //              +Plugin.KVVarGlob:    MyNewVarGlob    MyNewVarGlob     MyNewVarGlob.cpp+   "MyNewVarGlob()"
   //~~~~~~~~~~~~~~~
   //
   //  It is assumed that `MyNewVarGlob.h` and `MyNewVarGlob.cpp` will be found in `$HOME/myVarGlobs` (in this example).

   KVVarGlob* vg = GetGVList()->AddGV(class_name, name);
   return vg;
}

//____________________________________________________________________________

void KVEventSelector::RecalculateGlobalVariables()
{
   //Use this method if you change e.g. the particle selection criteria in your
   //Analysis() method and want to recalculate the values of all global variables
   //for your new selection.
   //
   //WARNING: the global variables are calculated automatically for you for each event
   //before method Analysis() is called. In order for the correct particles to be included in
   //this calculation, make sure that at the END of Analysis() you reset the selection
   //criteria.

   if (!gvlist.IsEmpty()) gvlist.CalculateGlobalVariables(GetEvent());
}

//____________________________________________________________________________

void KVEventSelector::SetParticleConditions(const KVParticleCondition& cond, const KVString& upcast_class)
{
   //Use this method to set criteria for selecting particles to include in analysis.
   //The criteria defined in the KVParticleCondition object will be applied to every
   //particle and if they are not satisfied the particle's
   //"OK" flag will be set to false, i.e. the particle's IsOK() method will return kFALSE,
   //and the particle will not be included in iterations such as GetEvent()->GetNextParticle("OK").
   //Neither will the particle be included in the evaluation of any global variables.
   //
   //This method must be called in the user's InitAnalysis() or InitRun() method.
   //
   //If the methods used in the condition are not defined for KVNucleus, you can give the
   //name of the class to which the methods refer (upcast_class), or you can set it before
   //hand (SetParticleConditionsParticleClassName)

   fPartCond = cond;
   if (upcast_class != "") fPartCond.SetParticleClassName(upcast_class);
   else if (fPartName != "") fPartCond.SetParticleClassName(fPartName);
}

//____________________________________________________________________________

const KVHashList* KVEventSelector::GetHistoList() const
{

   //return the list of created trees
   return &lhisto;

}

//____________________________________________________________________________

TH1* KVEventSelector::GetHisto(const Char_t* histo_name) const
{

   return lhisto.get_object<TH1>(histo_name);

}

//____________________________________________________________________________

void KVEventSelector::add_histo(TH1* histo)
{
   // Declare a histogram to be used in analysis.
   // This method must be called when using PROOF.

   if (fDisableCreateTreeFile) return;

   lhisto.Add(histo);

   if (writeFile) histo->SetDirectory(writeFile); // if output file is initialized, associate histo with it
   else create_output_file();// try to initialize output file
}

void KVEventSelector::add_tree(TTree* tree)
{
   // Declare a TTree to be used in analysis.
   // This method must be called when using PROOF.

   if (fDisableCreateTreeFile) return;

   tree->SetAutoSave();
   ltree.Add(tree);

   if (writeFile) tree->SetDirectory(writeFile); // if output file is initialized, associate tree with it
   else create_output_file();// try to initialize output file
}

void KVEventSelector::create_output_file()
{
   // Create the file for saving histos and/or trees created during analysis.
   //
   // The name of the file must first be set using SetJobOutputFileName()

   if (fCombinedOutputFile == "") return;

   auto sav_dir = gDirectory;//save current working directory
   if (gDataAnalyser->GetProofMode() == KVDataAnalyser::EProofMode::None) {
      // Analysis running in 'normal' (non-PROOF) mode
      writeFile = TFile::Open(fCombinedOutputFile, "RECREATE");
   }
   else {
      // Analysis running with PROOF(Lite)
      mergeFile = new TProofOutputFile(fCombinedOutputFile, "M");
      mergeFile->SetOutputFileName(fCombinedOutputFile);
      writeFile = mergeFile->OpenFile("RECREATE");
      if (writeFile && writeFile->IsZombie()) SafeDelete(writeFile);
   }
   sav_dir->cd();//return to previous working directory

   if (!writeFile) {
      TString amsg = TString::Format("%s::create_output_file: could not create output ROOT file '%s'!",
                                     ClassName(), fCombinedOutputFile.Data());
      Abort(amsg, kAbortProcess);
   }

   // any previously declared histograms or trees must now be associated with the output file
   if (lhisto.GetEntries()) {
      lhisto.R__FOR_EACH(TH1, SetDirectory)(writeFile);
   }
   if (ltree.GetEntries()) {
      ltree.R__FOR_EACH(TTree, SetDirectory)(writeFile);
   }
}

Bool_t KVEventSelector::CreateTreeFile(const Char_t*)
{
   Deprecate("Calling this method is no longer required, and any filename given will be ignored."
             " Call SetJobOutputFileName() to define the output filename.");
   return kTRUE;
}

TTree* KVEventSelector::AddTree(const TString& name, const TString& title)
{
   // Add TTree with given name and title to list of TTree to be filled by user's analysis

   auto t = new TTree(name, title);
   add_tree(t);
   return t;
}

//____________________________________________________________________________

void KVEventSelector::FillHisto(const Char_t* histo_name, Double_t one, Double_t two, Double_t three, Double_t four)
{

   //Find in the list, if there is an histogram named "sname"
   //If not print an error message
   //If yes redirect to the right method according to its closest mother class
   //to fill it
   TH1* h1 = 0;
   if ((h1 = GetHisto(histo_name))) {
      if (h1->InheritsFrom("TH3"))
         FillTH3((TH3*)h1, one, two, three, four);
      else if (h1->InheritsFrom("TProfile2D"))
         FillTProfile2D((TProfile2D*)h1, one, two, three, four);
      else if (h1->InheritsFrom("TH2"))
         FillTH2((TH2*)h1, one, two, three);
      else if (h1->InheritsFrom("TProfile"))
         FillTProfile((TProfile*)h1, one, two, three);
      else if (h1->InheritsFrom("TH1"))
         FillTH1(h1, one, two);
      else
         Warning("FillHisto", "%s -> Classe non prevue ...", lhisto.FindObject(histo_name)->ClassName());
   }
   else {
      Warning("FillHisto", "%s introuvable", histo_name);
   }

}

void KVEventSelector::FillHisto(const Char_t* histo_name, const Char_t* label, Double_t weight)
{
   // Fill 1D histogram with named bins
   TH1* h1 = 0;
   if ((h1 = GetHisto(histo_name))) {
      h1->Fill(label, weight);
   }
   else {
      Warning("FillHisto", "%s introuvable", histo_name);
   }

}

//____________________________________________________________________________

void KVEventSelector::FillTH1(TH1* h1, Double_t one, Double_t two)
{

   h1->Fill(one, two);

}

//____________________________________________________________________________

void KVEventSelector::FillTProfile(TProfile* h1, Double_t one, Double_t two, Double_t three)
{

   h1->Fill(one, two, three);

}

//____________________________________________________________________________

void KVEventSelector::FillTH2(TH2* h2, Double_t one, Double_t two, Double_t three)
{

   h2->Fill(one, two, three);

}

//____________________________________________________________________________

void KVEventSelector::FillTProfile2D(TProfile2D* h2, Double_t one, Double_t two, Double_t three, Double_t four)
{

   h2->Fill(one, two, three, four);
}

//____________________________________________________________________________

void KVEventSelector::FillTH3(TH3* h3, Double_t one, Double_t two, Double_t three, Double_t four)
{

   h3->Fill(one, two, three, four);
}

void KVEventSelector::SetUpAuxEventChain()
{
   // Called by SlaveBegin() when user gives the following options:
   //
   //~~~~~~~~~~~~~~~~
   //    AuxFiles:               list of files containing "friend" TTrees to be made available during analysis
   //    AuxDir:                 directory in which to find AuxFiles
   //    AuxTreeName:            name of tree in AuxFiles containing KVEvent objects
   //    AuxBranchName:          name of branch in AuxFiles containing KVEvent objects
   //~~~~~~~~~~~~~~~~

   if (!IsOptGiven("AuxDir") || !IsOptGiven("AuxTreeName") || !IsOptGiven("AuxBranchName")) {
      Error("SetUpAuxEventChain", "if AuxFiles option given, you must define AuxDir, AuxTreeName and AuxBranchName");
      return;
   }
   KVString filelist = GetOpt("AuxFiles");
   KVString filedir = GetOpt("AuxDir");
   if (!filedir.EndsWith("/")) filedir += "/";
   TChain* auxchain = new TChain(GetOpt("AuxTreeName"));
   filelist.Begin("|");
   while (!filelist.End()) {
      KVString path = filedir + filelist.Next();
      auxchain->Add(path);
   }
   InitFriendTree(auxchain, GetOpt("AuxBranchName"));
}

//____________________________________________________________________________

const KVHashList* KVEventSelector::GetTreeList() const
{
   //return the list of created trees
   return &ltree;

}

//____________________________________________________________________________

TTree* KVEventSelector::GetTree(const Char_t* tree_name) const
{
   //return the tree named tree_name
   TTree* t = ltree.get_object<TTree>(tree_name);
   if (!t) Fatal("GetTree", "Tree %s not found: is this the right name?", tree_name);
   return t;
}

//____________________________________________________________________________

void KVEventSelector::FillTree(const Char_t* tree_name)
{
   //Filltree method, the tree named tree_name
   //has to be declared with AddTTree(TTree*) method
   //
   //if no sname="", all trees in the list is filled
   //
   if (!strcmp(tree_name, "")) {
      ltree.R__FOR_EACH(TTree, Fill)();
   }
   else {
      TTree* tt = 0;
      if ((tt = GetTree(tree_name))) {
         tt->Fill();
      }
      else {
         Warning("FillTree", "%s introuvable", tree_name);
      }
   }

}

void KVEventSelector::SetTriggerConditionsForRun(int run)
{
   // Call this method in your InitRun() method with the current run number in order to
   // automatically reject events which are not consistent with the acquisition trigger.
   gDataAnalyser->SetTriggerConditionsForRun(run);
}

void KVEventSelector::ParseOptions()
{
   // Analyse comma-separated list of options given to TTree::Process
   // and store all `"option=value"` pairs in fOptionList.
   // Options can then be accessed using IsOptGiven(), GetOptString(), etc.
   //
   //~~~~~~~~~~~~~~~
   //     BranchName=xxxx  :  change name of branch in TTree containing data
   //     EventsReadInterval=N: print "+++ 12345 events processed +++" every N events
   //~~~~~~~~~~~~~~~
   //
   // This method is called by SlaveBegin
   //

   fOptionList.ParseOptions(GetOption());

   // check for branch name
   if (IsOptGiven("BranchName")) SetBranchName(GetOpt("BranchName"));
   // check for events read interval
   if (IsOptGiven("EventsReadInterval")) SetEventsReadInterval(GetOpt("EventsReadInterval").Atoi());
}

void KVEventSelector::Init(TTree* tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
   Event = nullptr;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fChain->SetMakeClass(1);
   // When using PROOF, need to set tree pointer in KVDataAnalyser
   if (gDataAnalyser->GetProofMode() != KVDataAnalyser::EProofMode::None) {
      gDataAnalyser->SetTree(tree);
   }

   if (strcmp(GetBranchName(), "")  && fChain->GetBranch(GetBranchName())) {
      Info("Init", "Analysing data in branch : %s", GetBranchName());
      fChain->SetBranchAddress(GetBranchName(), &Event, &b_Event);
   }
   else {
      Error("Init", "Failed to link KVEvent object with a branch. Expected branch name=%s",
            GetBranchName());
   }
   //user additional branches addressing
   SetAdditionalBranchAddress();
   fEventsRead = 0;

}

void KVEventSelector::InitFriendTree(TTree* tree, const TString& branchname)
{
   // Set up a "friend" TTree/TChain containing KVEvent-derived objects in branch 'branchname'
   // N.B. this is not a "friend" in the sense of TTree::AddFriend, the main TTree and the
   // "friend" TTree can have different numbers of entries
   //
   // After calling this method at the beginning of the analysis, you can
   // access any of the events stored in the "friend" by doing:
   //
   //~~~~~~~~~~~~
   //   GetFriendTreeEntry(entry_number);
   //   KVEvent* friend_event = GetFriendEvent();
   //~~~~~~~~~~~~

   AuxEvent = 0;
   fAuxChain = tree;
   fAuxChain->SetBranchAddress(branchname, &AuxEvent);
   fAuxChain->Print();
   fAuxChain->GetEntry(0);
   fAuxChain->GetTree()->GetEntry(0);
}

Bool_t KVEventSelector::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   if (fNotifyCalled) return kTRUE; // avoid multiple calls at beginning of analysis
   fNotifyCalled = kTRUE;

   Info("Notify", "Beginning analysis of file %s (%lld events)", fChain->GetCurrentFile()->GetName(), fChain->GetTree()->GetEntries());

   if (gDataAnalyser) gDataAnalyser->preInitRun();
   InitRun();                   //user initialisations for run
   if (gDataAnalyser) gDataAnalyser->postInitRun();

   KVClassMonitor::GetInstance()->SetInitStatistics();

   return kTRUE;
}

/** \example ExampleSimDataAnalysis.cpp
# Example of an analysis class for simulated data

A simple example of analysis of simulated data, for use with kaliveda-sim

\include ExampleSimDataAnalysis.h
*/


/** \example ExampleFilteredSimDataAnalysis.cpp
# Example of an analysis class for filtered simulated data

A simple example of analysis of filtered simulated data, for use with kaliveda-sim.

\include ExampleFilteredSimDataAnalysis.h
*/

