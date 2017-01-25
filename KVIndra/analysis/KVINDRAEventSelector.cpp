//Created by KVClassFactory on Thu Dec 15 16:01:29 2016
//Author: John Frankland,,,

#include "KVINDRAEventSelector.h"
#include "KVINDRAReconDataAnalyser.h"

ClassImp(KVINDRAEventSelector)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVINDRAEventSelector</h2>
<h4>Base class for analysis of reconstructed INDRA events</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVINDRAEventSelector::KVINDRAEventSelector(TTree* arg1)
   : KVEventSelector(arg1), fKinematics(nullptr)
{
   SetBranchName("INDRAReconEvent");
   SetEventsReadInterval(20000);
}

//____________________________________________________________________________//

KVINDRAEventSelector::~KVINDRAEventSelector()
{
   // Destructor
}

void KVINDRAEventSelector::Init(TTree* tree)
{
   // When using PROOF, need to set tree pointer in KVDataAnalyser
   KVEventSelector::Init(tree);
   if (tree && gDataAnalyser->GetProofMode() != KVDataAnalyser::EProofMode::None) {
      dynamic_cast<KVINDRAReconDataAnalyser*>(gDataAnalyser)->SetTree(tree);
   }
}

void KVINDRAEventSelector::SetAnalysisFrame()
{
   // Define the CM frame for the current event
   //calculate momenta of particles in reaction cm frame
   if (fKinematics) {
      GetEvent()->SetFrame("CM", fKinematics->GetCMVelocity());
   }
}

//____________________________________________________________________________//

