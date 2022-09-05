//Created by KVClassFactory on Tue Sep  8 16:00:28 2015
//Author: ,,,

#include "KVFAZIAIDCsI.h"
#include "KVIDGCsI.h"
#include "KVIdentificationResult.h"
#include "KVDataSet.h"

ClassImp(KVFAZIAIDCsI)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVFAZIAIDCsI</h2>
<h4>id telescope to manage FAZIA CsI identification</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVFAZIAIDCsI::KVFAZIAIDCsI()
{
   // Default constructor
   SetType("CsI");
   SetLabel("FAZIA.CsI");
   fMaxZ = 22.5; // dummy hight values, hard threshold at Z=4 set by CanIdentify() Mmethod
   fSigmaZ = .5;
   /* in principle all CsI R-L telescopes can identify mass & charge */
   SetHasMassID(kTRUE);
}

Bool_t KVFAZIAIDCsI::Identify(KVIdentificationResult* IDR, Double_t x, Double_t y)
{
   // Particle identification and code setting using identification grid
   //
   // To enable use of either KVIDGCsI (fast-slow grid a la INDRA) or KVIDZAGrid
   // (a la Valdre style), we override KVIDTelescope::Identify in order to correctly
   // label particles identified as gammas.

   Bool_t ok = KVFAZIAIDTelescope::Identify(IDR, x, y);

   // did we just see a gamma ?
   if (IDR->IDquality == KVIDGCsI::kICODE10
         || (IDR->IDquality == KVIDZAGrid::kICODE8 && IDR->Rejecting_Cut == "gamma_line")) {
      IDR->IDOK = true;
      IDR->IDcode = 0; // kludge: should be KVFAZIA::IDCodes::ID_GAMMA, but would require moving class to 'geometry'
      ok = kTRUE;
      IDR->IDquality = KVIDGCsI::kICODE10;
      IDR->Z = 0;
      IDR->A = 1;
   }
   return ok;
}
