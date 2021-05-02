#include "KVMultiDetArray.h"

void FilteredEventAnalysisTemplate::InitAnalysis()
{
   // INITIALISATION PERFORMED AT BEGINNING OF ANALYSIS
   // Here you define:
   //   - global variables
   //   - histograms
   //   - trees
   //
   // NB: no access to multidetector array or reaction
   //     kinematics yet: see InitRun()

   // DEFINITION OF GLOBAL VARIABLES FOR ANALYSIS
   AddGV("KVMult", "mult");    // total multiplicity of each event
   AddGV("KVZVtot", "ZVTOT");  // total pseudo-momentum

   // DEFINITION OF HISTOGRAMS
   AddHisto(new TH2F("Z_Vpar", "Z vs V_{par} [cm/ns] in CM", 250, -10, 10, 75, .5, 75.5));

   // DEFINITION OF TREE USED TO STORE RESULTS
   CreateTreeFile();

   TTree* t = new TTree("data", GetOpt("SimulationInfos"));

   // add a branch to tree for each defined global variable
   GetGVList()->MakeBranches(t);

   AddTree(t);

   // check if we can access the original simulated events before filtering
   // (activated when selecting both filtered & simulated files in kaliveda-sim GUI)
   link_to_unfiltered_simulation = IsOptGiven("AuxFiles");
}

//____________________________________________________________________________________

void FilteredEventAnalysisTemplate::InitRun()
{
   // INITIALISATION PERFORMED JUST BEFORE ANALYSIS
   // In this method the multidetector array/setup used to filter
   // the simulation is available (gMultiDetArray)
   // The kinematics of the reaction is available (KV2Body*)
   // using gDataAnalyser->GetKinematics()

   // normalize ZVtot to projectile Z*v
   const KV2Body* kin = gDataAnalyser->GetKinematics();
   GetGV("ZVTOT")->SetNormalization(kin->GetNucleus(1)->GetVpar()*kin->GetNucleus(1)->GetZ());
}

//____________________________________________________________________________________

Bool_t FilteredEventAnalysisTemplate::Analysis()
{
   // EVENT BY EVENT ANALYSIS

   // Rejection of less-well measured events:
   //   here we require reconstruction of at least 80% of projectile quasi-momentum
   if (GetGV("ZVTOT")->GetValue() < 0.8) return kTRUE;

   // if we can access the events of the unfiltered simulation, read in the event corresponding
   // to the currently analysed reconstructed event
   if (link_to_unfiltered_simulation) GetFriendTreeEntry(GetEvent()->GetParameters()->GetIntValue("SIMEVENT_TREE_ENTRY"));

   for (KVReconstructedEvent::Iterator it = KVReconstructedEvent::OKEventIterator(GetEvent()).begin();
         it != KVReconstructedEvent::Iterator::End(); ++it) {
      // if we can access the events of the unfiltered simulation, and if Gemini++ was used
      // to decay events before filtering, this is how you can access the "parent" nucleus
      // of the current detected decay product
      // KVSimNucleus* papa = (KVSimNucleus*)GetFriendEvent()->GetParticle( part->GetParameters()->GetIntValue("GEMINI_PARENT_INDEX") );

      FillHisto("Z_Vpar", (*it).GetFrame("CM")->GetVpar(), (*it).GetZ());
   }

   GetGVList()->FillBranches();
   FillTree();

   return kTRUE;
}

