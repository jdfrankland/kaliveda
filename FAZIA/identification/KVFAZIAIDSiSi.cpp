//Created by KVClassFactory on Mon Mar 16 11:45:14 2015
//Author: ,,,

#include "KVFAZIAIDSiSi.h"

ClassImp(KVFAZIAIDSiSi)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVFAZIAIDSiSi</h2>
<h4>identification telescope for FAZIA Si-Si idcards</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVFAZIAIDSiSi::KVFAZIAIDSiSi()
{
   // Default constructor
   SetType("Si-Si");
   SetLabel("FAZIA.Si-Si");
   SetHasMassID(kTRUE);
   fMaxZ = 22.5;
   fSigmaZ = .5;
}
