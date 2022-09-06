//Created by KVClassFactory on Thu Jul 19 15:38:10 2018
//Author: eindra

#include "KVReconEventSelector.h"

#include <KVClassFactory.h>
#include <KVReconDataAnalyser.h>

ClassImp(KVReconEventSelector)


void KVReconEventSelector::Init(TTree* tree)
{
   // When using PROOF, need to set tree pointer in KVDataAnalyser
   KVEventSelector::Init(tree);
   if (tree && gDataAnalyser->GetProofMode() != KVDataAnalyser::EProofMode::None) {
      gDataAnalyser->SetAnalysedTree(tree);
   }
}

void KVReconEventSelector::Make(const Char_t* kvsname)
{
   // Generate a new recon data analysis selector class
   //
   // This will be based on a template file which may change as a function of the current
   // dataset, if a variable
   //~~~~
   //[dataset].ReconEventSelector.Template
   //~~~~
   // is defined with the name of the template

   KVClassFactory cf(kvsname, "Analysis of reconstructed events", "",
                     kTRUE,
                     gDataSet->GetDataSetEnv("ReconEventSelector.Template", "ROOT6ReconDataSelectorTemplate"));

   cf.AddImplIncludeFile("KVReconstructedNucleus.h");
   cf.AddImplIncludeFile("KVBatchSystem.h");

   cf.GenerateCode();
}


/** \example ExampleReconAnalysis.cpp
# Example of an analysis class for reconstructed data (i.e. events with nuclei)

This is the analysis class generated by default by KaliVedaGUI for reconstructed data analysis.

\include ExampleReconAnalysis.h
*/

/** \example ExampleE789ReconAnalysis.cpp
# Example of an analysis class for reconstructed data of INDRAFAZIA experiment E789

This is the analysis class generated by default by KaliVedaGUI for E789 reconstructed data analysis.

\include ExampleE789ReconAnalysis.h
*/
