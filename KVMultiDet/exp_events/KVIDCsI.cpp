//Created by KVClassFactory on Wed Mar 23 14:04:55 2016
//Author: John Frankland,,,

#include "KVIDCsI.h"
#include "KVIDGraph.h"

ClassImp(KVIDCsI)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVIDCsI</h2>
<h4>A generic 1-member CsI fast-slow identification telescope</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVIDCsI::KVIDCsI()
   : KVIDTelescope()
{
   // Default constructor
}

//____________________________________________________________________________//

KVIDCsI::~KVIDCsI()
{
   // Destructor
}

//____________________________________________________________________________//

void KVIDCsI::Initialize()
{
   // Override default initialization method
   // If telescope has 1 CsI detector and at least 1 grid
   // then it is ready to identify particles, after we initialise
   // the grid

   if (GetDetectors()->GetEntries() == 1 && GetDetector(1)->IsType("CsI") && GetIDGrid()) {
      GetIDGrid()->Initialize();
      SetBit(kReadyForID);
   } else
      ResetBit(kReadyForID);
}
