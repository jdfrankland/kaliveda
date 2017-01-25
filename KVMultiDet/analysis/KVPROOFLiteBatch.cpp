//Created by KVClassFactory on Thu Dec 15 14:55:48 2016
//Author: John Frankland,,,

#include "KVPROOFLiteBatch.h"

#include <TProof.h>

ClassImp(KVPROOFLiteBatch)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVPROOFLiteBatch</h2>
<h4>Batch system interface to PROOFLite</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////


KVPROOFLiteBatch::KVPROOFLiteBatch(const Char_t* name)
   : KVBatchSystem(name)
{
}

//____________________________________________________________________________//

KVPROOFLiteBatch::~KVPROOFLiteBatch()
{
   // Destructor
}

void KVPROOFLiteBatch::SubmitTask(KVDataAnalyser* da)
{
   // Run analysis on PROOFLite facility

   // Open PROOFLite session and initialise KaliVeda package
   if (!gProof) {
      TProof* p = TProof::Open("");
      p->ClearCache();//to avoid problems with compilation of KVParticleCondition
      // enable KaliVeda on PROOF cluster
      if (p->EnablePackage("KaliVeda") != 0) {
         // first time, need to 'upload' package
         TString fullpath = KVBase::GetETCDIRFilePath("KaliVeda.par");
         p->UploadPackage(fullpath);
         p->EnablePackage("KaliVeda");
      }
   }
   da->SetProofMode(KVDataAnalyser::EProofMode::Lite);
   SetAnalyser(da);
   da->SubmitTask();
}

//____________________________________________________________________________//

