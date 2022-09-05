#ifndef __KVFAZIAIDSISICSI_H
#define __KVFAZIAIDSISICSI_H

#include "KVFAZIAIDTelescope.h"

/**
 \class KVFAZIAIDSiSiCsI
 \brief Telescope for FAZIA identification using SI1 and/or SI2 with CSI

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Mon Sep  5 09:37:10 2022
*/

class KVFAZIAIDSiSiCsI : public KVFAZIAIDTelescope {
public:
   KVFAZIAIDSiSiCsI()
      : KVFAZIAIDTelescope()
   {
      SetType("Si-CsI");
      SetLabel("FAZIA.Si-CsI");
      SetHasMassID(kTRUE);
      // the following copied from Si-Si identification
      fMaxZ = 22.5;
      fSigmaZ = .5;
   }

   ClassDef(KVFAZIAIDSiSiCsI, 1) //Telescope for FAZIA identification using SI1 and/or SI2 with CSI
};

#endif
