//Created by KVClassFactory on Mon Feb 17 13:52:51 2014
//Author: John Frankland,,,

#ifndef __KVFAZIAIDSiPSA_H
#define __KVFAZIAIDSiPSA_H

#include "KVFAZIAIDTelescope.h"
#include "TF1.h"

class KVFAZIAIDSiPSA : public KVFAZIAIDTelescope {

   static TF1* fZThreshold;//! empirical threshold for Z identification
   static TF1* fAThreshold;//! empirical threshold for A identification

public:
   KVFAZIAIDSiPSA();

   virtual void Initialize();

   Bool_t CheckTheoreticalIdentificationThreshold(KVNucleus* /*ION*/, Double_t /*EINC*/ = 0.0);
   void SetIdentificationStatus(KVIdentificationResult* IDR, const KVNucleus*);

   ClassDef(KVFAZIAIDSiPSA, 1) //PSA identification in first silicon of FAZIA telescopes
};

#endif
