//Created by KVClassFactory on Thu Dec 15 16:01:29 2016
//Author: John Frankland,,,

#include "KVINDRAEventSelector.h"
#include "KVINDRAReconDataAnalyser.h"
#include "KVClassFactory.h"

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
   : KVEventSelector(arg1), fCurrentRun(nullptr)
{
   SetBranchName("INDRAReconEvent");
   SetEventsReadInterval(20000);
   SetParticleConditionsParticleClassName("KVINDRAReconNuc");
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
#ifdef WITH_CPP11
   if (tree && gDataAnalyser->GetProofMode() != KVDataAnalyser::EProofMode::None) {
#else
   if (tree && gDataAnalyser->GetProofMode() != KVDataAnalyser::None) {
#endif
      dynamic_cast<KVINDRAReconDataAnalyser*>(gDataAnalyser)->SetTree(tree);
   }
}

void KVINDRAEventSelector::SetAnalysisFrame()
{
   // Define the CM frame for the current event
   //calculate momenta of particles in reaction cm frame
   if (fCurrentRun->GetSystem() && fCurrentRun->GetSystem()->GetKinematics()) {
      GetEvent()->SetFrame("CM", fCurrentRun->GetSystem()->GetKinematics()->GetCMVelocity());
   }
}

void KVINDRAEventSelector::Make(const Char_t* kvsname)
{
   // Automatic generation of KVINDRAEventSelector-derived class for KaliVeda analysis

   KVClassFactory cf(kvsname, "User analysis class", "KVINDRAEventSelector", kTRUE);
   cf.AddImplIncludeFile("KVINDRAReconNuc.h");
   cf.AddImplIncludeFile("KVBatchSystem.h");
   cf.AddImplIncludeFile("KVINDRA.h");

   cf.GenerateCode();
}

