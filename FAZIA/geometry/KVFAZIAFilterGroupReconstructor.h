#ifndef __KVFAZIAFILTERGROUPRECONSTRUCTOR_H
#define __KVFAZIAFILTERGROUPRECONSTRUCTOR_H

#include "KVFilterGroupReconstructor.h"

/**
 \class KVFAZIAFilterGroupReconstructor
 \brief Reconstruct particles after simulated detection in FAZIA telescope

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Fri Sep 23 13:30:07 2022
*/

class KVFAZIAFilterGroupReconstructor : public KVFilterGroupReconstructor {
public:
   KVFAZIAFilterGroupReconstructor()
      : KVFilterGroupReconstructor()
   {
   }

   ClassDef(KVFAZIAFilterGroupReconstructor, 1) //Reconstruct particles after simulated detection in FAZIA telescope
};

#endif
