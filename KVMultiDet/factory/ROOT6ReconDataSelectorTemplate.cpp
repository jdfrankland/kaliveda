#include "KVDataAnalyser.h"
#include "KVMultiDetArray.h"

void ROOT6ReconDataSelectorTemplate::InitAnalysis(void)
{
   // Declaration of histograms, global variables, etc.
   // Called at the beginning of the analysis
   // The examples given are compatible with interactive, batch,
   // and PROOFLite analyses.

   /*** ADDING GLOBAL VARIABLES TO THE ANALYSIS ***/
   /* These will be automatically calculated for each event before
      your Analysis() method will be called                        */
   AddGV("KVZtot", "ztot");                             // total charge
   auto zvtot = AddGV("KVZVtot", "zvtot");              // total Z*vpar
   zvtot->SetMaxNumBranches(1);    // only write "Z" component in TTree

   AddGV("KVMult", "mtot"); // total multiplicity
   // total multiplicity in forward CM hemisphere
   auto gv = AddGV("KVMult", "mtot_av");
   gv->SetSelection({
      "Vcm>0", [](const KVNucleus * n)
      {
         return n->GetVpar() > 0;
      }}
                   );
   gv->SetFrame("CM");

   /*** DECLARING SOME HISTOGRAMS ***/
   AddHisto<TH1F>("zdist", "Charge distribution", 100, -.5, 99.5);
   AddHisto<TH2F>("zvpar", "Z vs V_{par} in CM", 100, -15., 15., 75, .5, 75.5);

   /*** USING A TREE ***/
   auto t = AddTree("myTree");

   GetGVList()->MakeBranches(t); // store global variable values in branches

   /*** DEFINE WHERE TO SAVE THE RESULTS ***/
   // This filename will be used for interactive and PROOFlite jobs.
   // When running in batch mode, this will automatically use the job name.
   SetJobOutputFileName("ROOT6ReconDataSelectorTemplate_results.root");
}

//_____________________________________
void ROOT6ReconDataSelectorTemplate::InitRun(void)
{
   // Initialisations for each run
   // Called at the beginning of each run

   // This is the place to define the correct identification/calibration codes for particles
   // which will be used in your analysis. These particles will be labelled 'OK'
   // (i.e. the method IsOK() for these particles returns kTRUE).
   //
   // If no explicit selection is indicated here, an automatic selection for each multidetector
   // array will be made using the default accepted values which are defined in variables
   // [array].ReconstructedNuclei.AcceptID/ECodes.
   //
   // You can change the selection (or deactivate it) here by doing any of the following:
   //
   // gMultiDetArray->AcceptIDCodes("12,33"); => accept only ID codes in list
   // gMultiDetArray->AcceptAllIDCodes();     => accept particles with any ID code
   //
   // If the experiment used a combination of arrays, codes have to be set for
   // each array individually:
   // gMultiDetArray->GetArray("[name]")->Accept.. => setting for array [name]

   // set title of TTree with name of analysed system
   GetTree("myTree")->SetTitle(GetCurrentRun()->GetSystemName());

   // reject reconstructed events which are not consistent with the DAQ trigger
   SetTriggerConditionsForRun(GetCurrentRun()->GetNumber());
}

//_____________________________________
Bool_t ROOT6ReconDataSelectorTemplate::Analysis(void)
{
   // Analysis method called event by event.
   // The current event can be accessed by a call to method GetEvent().
   // See KVReconstructedEvent documentation for the available methods.

   GetGVList()->FillBranches(); // update values of all global variable branches
   FillTree(); // write new results in TTree

   /*** LOOP OVER PARTICLES OF EVENT ***/
   for (auto& n : ReconEventOKIterator(GetEvent())) {
      // "OK" particles => using selection criteria of InitRun()
      // fill Z distribution
      FillHisto("zdist", n.GetZ());
      // fill Z-Vpar(CM)
      FillHisto("zvpar", n.GetFrame("CM")->GetVpar(), n.GetZ());
   }

   return kTRUE;
}

