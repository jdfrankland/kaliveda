#include "ExampleReconAnalysis.h"
#include "KVReconstructedNucleus.h"
#include "KVBatchSystem.h"

ClassImp(ExampleReconAnalysis)

void ExampleReconAnalysis::InitAnalysis(void)
{
   // Declaration of histograms, global variables, etc.
   // Called at the beginning of the analysis
   // The examples given are compatible with interactive, batch,
   // and PROOFLite analyses.

   /*** ADDING GLOBAL VARIABLES TO THE ANALYSIS ***/
   // For E789, we recommend to not use the global variables for the moment,
   // as they are only calculated for particles whose "OK" status is set according
   // to a single selection of identification & calibration codes defined in InitRun().
   // In the present example, this selection has been deactivated (see comments in InitRun()).
   //
   // In a future version of KaliVeda, it will be possible to handle several global variable
   // lists in the analysis, each with its specific particle selection criteria (not limited to
   // ID codes and E codes).

   /*** DECLARING SOME HISTOGRAMS ***/
   AddHisto<TH1F>("all_idcodes_fazia", "ID codes in FAZIA", 10, 0, 10);
   AddHisto<TH1F>("all_idcodes_indra", "ID codes in INDRA", 10, 0, 10);
   AddHisto<TH1F>("all_ecodes_fazia", "E codes in FAZIA", 5, 0, 5);
   AddHisto<TH1F>("all_ecodes_indra", "E codes in INDRA", 5, 0, 5);
   AddHisto<TH1F>("mtot_ch", "Total charged multiplicity", 50, -.5, 49.5);
   add_idcode_histos("mtot_ch");
   AddHisto<TH1F>("zdist", "Charge distribution", 50, -.5, 49.5);
   add_idcode_histos("zdist");
   AddHisto<TH2F>("a_vs_z", "A vs. Z", 35, -.5, 34.5, 65, -.5, 64.5);
   add_idcode_histos("a_vs_z");
   // some kinematic properties - for calibrated particles only
   AddHisto<TH2F>("vper_proton", "Inv. velocity plot (protons) [cm/ns]", 500, 0, 15, 500, -10, 10);
   AddHisto<TH2F>("vper_alpha", "Inv. velocity plot (alphas) [cm/ns]", 500, 0, 15, 500, -10, 10);
   AddHisto<TH2F>("Z_vpar", "Z vs. parallel velocity [cm/ns]", 500, 0, 15, 35, .5, 35.5);
   AddHisto<TH1F>("calibrated_ecodes_fazia", "E codes for calibrated particles in FAZIA", 5, 0, 5);
   AddHisto<TH1F>("calibrated_ecodes_indra", "E codes for calibrated particles in INDRA", 5, 0, 5);

   /*** USING A TREE ***/
   // if you want to store data in a TTree, do as follows:
   // auto t = AddTree("myTree");
   // t->Branch("myVar", &myVar);
   //  etc.

   /*** DEFINE WHERE TO SAVE THE RESULTS ***/
   // This filename will be used for interactive and PROOFlite jobs.
   // When running in batch mode, this will automatically use the job name.
   SetJobOutputFileName("ExampleReconAnalysis_results.root");
}

void ExampleReconAnalysis::add_idcode_histos(const TString& histo_name)
{
   AddHisto<TH1F>(Form("%s_idcodes_fazia", histo_name.Data()),
                  Form("%s : ID codes in FAZIA", GetHisto(histo_name)->GetTitle()), 10, 0, 10);
   AddHisto<TH1F>(Form("%s_idcodes_indra", histo_name.Data()),
                  Form("%s : ID codes in INDRA", GetHisto(histo_name)->GetTitle()), 10, 0, 10);
}

//_____________________________________
void ExampleReconAnalysis::InitRun(void)
{
   // Initialisations for each run
   // Called at the beginning of each run

   // This is the place to define the correct identification/calibration codes for particles
   // which will be used in your analysis. These particles will be labelled 'OK'
   // (i.e. the method IsOK() for these particles returns kTRUE).
   //
   // For E789 it is recommended to deactivate this functionality and select particles
   // individually depending on the needs of the analysis (i.e. include uncalibrated &
   // partially (or un-)identfied particles when calculating multiplicities, but limit
   // to well-identified particles for e.g. isospin transport studies and/or
   // well-calibrated particles when dealing with kinematic properties).
   gMultiDetArray->AcceptAllIDCodes();
   gMultiDetArray->AcceptAllECodes();

   INDRA = gMultiDetArray->GetArray("INDRA");
   FAZIA = gMultiDetArray->GetArray("FAZIA");

   // set title of TTree with name of analysed system
   // GetTree("myTree")->SetTitle(GetCurrentRun()->GetSystemName());

   // The following will reject reconstructed events for which the FAZIA trigger
   // bit pattern is not consistent with the physics trigger i.e. M>=2.
   // This will reject events where only the downscaled M>=1 trigger fired.
   //
   // Instead of this, you can use
   //   if( FAZIA->GetTrigger().IsTrigger( [name of trigger] ) ) { ... }
   // to test or select events individually according to the fired trigger pattern,
   // in your Analysis() method.
   //
   // See KVFAZIATrigger for details.
   SetTriggerConditionsForRun(GetCurrentRun()->GetNumber());
}

void ExampleReconAnalysis::fill_idcode_histos(const TString& histo_name, const KVReconstructedNucleus& rn)
{
   if (rn.InArray("INDRA"))
      FillHisto(Form("%s_idcodes_indra", histo_name.Data()), INDRA->GetIDCodeMeaning(rn.GetIDCode()), 1);
   else if (rn.InArray("FAZIA"))
      FillHisto(Form("%s_idcodes_fazia", histo_name.Data()), FAZIA->GetIDCodeMeaning(rn.GetIDCode()), 1);
   else
      Fatal("fill_idcode_histos", "Particle not in INDRA and not in FAZIA!!! array=%s",
            rn.GetArrayName().Data());
}

void ExampleReconAnalysis::fill_ecode_histos(const TString& histo_name, const KVReconstructedNucleus& rn)
{
   if (rn.InArray("INDRA"))
      FillHisto(Form("%s_ecodes_indra", histo_name.Data()), INDRA->GetECodeMeaning(rn.GetECode()), 1);
   else if (rn.InArray("FAZIA"))
      FillHisto(Form("%s_ecodes_fazia", histo_name.Data()), FAZIA->GetECodeMeaning(rn.GetECode()), 1);
   else
      Fatal("fill_ecode_histos", "Particle not in INDRA and not in FAZIA!!! array=%s",
            rn.GetArrayName().Data());
}

//_____________________________________
Bool_t ExampleReconAnalysis::Analysis(void)
{
   // Analysis method called event by event.
   // The current event can be accessed by a call to method GetEvent().
   // See KVReconstructedEvent documentation for the available methods.


   int mtot_ch = 0;

   for (auto& n : EventIterator(*GetEvent())) {
      auto& rn = dynamic_cast<KVReconstructedNucleus&>(n);

      fill_idcode_histos("all", rn);
      fill_ecode_histos("all", rn);

      // 'Identified' particles may in fact not be identified at all, or only partially
      // They include particles stopping in the first stage of deltaE-E telescopes,
      // heavy (Z>5) particles stopped in CsI without a deltaE detector in front,
      // or even gamma particles (FAZIA) & "neutrons" (INDRA).
      if (rn.IsIdentified() && rn.GetZ() > 0) {
         // To calculate the best estimate of total charged particle multiplicity,
         // we include all 'Identified' particles with Z>0
         fill_idcode_histos("mtot_ch", rn);
         ++mtot_ch;
      }

      // Particles with 'IsZMeasured'=true are particles whose Z was measured...
      // this means excluding i.e. particles stopping in the first stage of deltaE-E telescopes
      // or heavy (Z>5) particles stopped in CsI without a deltaE detector in front, for which
      // we can only estimate a lower limit for the Z.
      // However, it should be noted that while this condition excludes gamma particles,
      // neutrons have a measured Z too (it is equal to 0!).
      if (rn.IsZMeasured() && rn.GetZ() > 0) { // GetZ()>0: exclude neutrons (INDRA)
         FillHisto("zdist", rn.GetZ());
         fill_idcode_histos("zdist", rn);

         // Particles with 'IsAMeasured'=true have isotopic mass resolution
         if (rn.IsAMeasured()) {
            FillHisto("a_vs_z", rn.GetZ(), rn.GetA());
            fill_idcode_histos("a_vs_z", rn);
         }

         // Only particles with 'IsCalibrated'=true have measured kinematical properties
         if (rn.IsCalibrated()) {
            if (rn.IsAMeasured()) { // limit to isotopically identified LCP
               if (rn.IsIsotope(1, 1)) FillHisto("vper_proton", rn.GetVpar(), rn.GetVperp(),
                                                    1. / (1.e-3 + TMath::Abs(rn.GetVperp())));
               else if (rn.IsIsotope(2, 4)) FillHisto("vper_alpha", rn.GetVpar(), rn.GetVperp(),
                                                         1. / (1.e-3 + TMath::Abs(rn.GetVperp())));
            }
            FillHisto("Z_vpar", rn.GetVpar(), rn.GetZ());
            fill_ecode_histos("calibrated", rn);
         }
      }

   }
   FillHisto("mtot_ch", mtot_ch);

   return kTRUE;
}

