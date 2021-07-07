//Created by KVClassFactory on Mon Feb 17 13:51:39 2014
//Author: John Frankland,,,

#ifndef __KVFAZIAIDTELESCOPE_H
#define __KVFAZIAIDTELESCOPE_H

#include "KVIDTelescope.h"
#include "KVReconstructedNucleus.h"
#include "TF1.h"

class KVFAZIAIDTelescope : public KVIDTelescope {

protected:
   static TF1* fMassIDProb;
   double fMaxZ, fSigmaZ;

public:
   KVFAZIAIDTelescope();
   virtual void AddDetector(KVDetector* d);
   virtual void SetIdentificationStatus(KVReconstructedNucleus* n);

   ClassDef(KVFAZIAIDTelescope, 1) //Identification for FAZIA array
};

#endif
