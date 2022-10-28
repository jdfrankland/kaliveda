#include "ExampleCorrelationAnalysis.h"

ClassImp(ExampleCorrelationAnalysis)

void ExampleCorrelationAnalysis::InitAnalysis(void)
{
   // Declaration of histograms, global variables, etc.
   // Called at the beginning of the analysis
   // The examples given are compatible with interactive, batch,
   // and PROOFLite analyses.

   AddGV("KVMult", "mtot");                             // total multiplicity

   mult_bin = GetGVList()->AddEventClassifier("mtot");
   // Multiplicity cuts corresponding to 5 equal statistics bins of Xe+Sn 50AMeV
   std::vector<Double_t> mult_slices = {8.4195047, 14.449584, 21.173604, 28.044737};
   for (auto cut : mult_slices) mult_bin->AddCut(cut);

   for (int i = 0; i <= mult_slices.size(); ++i) {
      AddHisto<TH1F>(get_cor_histo_name(i, "dphi"), Form("Correlated spectrum #Delta#phi bin %d", i), 21, -4.5, 184.5);
      AddHisto<TH1F>(get_uncor_histo_name(i, "dphi"), Form("Uncorrelated spectrum #Delta#phi bin %d", i), 21, -4.5, 184.5);
   }

   auto t = AddTree("check_tree", "check event classifier");
   GetGVList()->MakeBranches(t);

   /*** DEFINE WHERE TO SAVE THE RESULTS ***/
   SetJobOutputFileName("ExampleCorrelationAnalysis_results.root");
}

//_____________________________________
void ExampleCorrelationAnalysis::InitRun(void)
{
   // Reject events with less identified particles than the acquisition multiplicity trigger
   SetTriggerConditionsForRun(GetCurrentRun()->GetNumber());
}

//_____________________________________
Bool_t ExampleCorrelationAnalysis::Analysis(void)
{
   auto bin = mult_bin->GetEventClassification();

   GetGVList()->FillBranches();
   FillTree();

   event_mixer.ProcessEvent(bin,
   ReconEventIterator(GetEvent(), {
      "alpha",
      [](const KVReconstructedNucleus * n)
      {
         return n->IsOK() && n->IsAMeasured() && n->IsIsotope(2, 4);
      }
   }),

   ReconEventIterator(GetEvent(), {
      "alpha",
      [](const KVReconstructedNucleus * n)
      {
         return n->IsOK() && n->IsAMeasured() && n->IsIsotope(2, 4);
      }
   }),

   [ = ](int bin_num, KVReconstructedNucleus & alpha, KVReconstructedNucleus & other_alpha) {
      // treat pair from same event
      if (&alpha == &other_alpha) return; // avoid same alpha!!
      KVPosition pos;
      auto dphi = pos.GetAzimuthalWidth(alpha.GetPhi(), other_alpha.GetPhi());
      if (dphi > 180) dphi = 360 - dphi;
      GetHisto(get_cor_histo_name(bin_num, "dphi"))->Fill(dphi);
   },

   [ = ](int bin_num, KVReconstructedNucleus & alpha, particle & other_alpha) {
      // treat pair from different events
      if (other_alpha.det_index != alpha.GetStoppingDetector()->GetIndex()) { // avoid particles in same detector!
         KVPosition pos;
         auto dphi = pos.GetAzimuthalWidth(alpha.GetPhi(), other_alpha.phi);
         if (dphi > 180) dphi = 360 - dphi;
         GetHisto(get_uncor_histo_name(bin_num, "dphi"))->Fill(dphi);
      }
   }
                           );

   return kTRUE;
}
