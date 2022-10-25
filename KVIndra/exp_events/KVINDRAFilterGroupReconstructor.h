#ifndef __KVINDRAFILTERGROUPRECONSTRUCTOR_H
#define __KVINDRAFILTERGROUPRECONSTRUCTOR_H

#include "KVFilterGroupReconstructor.h"

/**
 \class KVINDRAFilterGroupReconstructor
 \brief Reconstruct simulated events after filtering with INDRA

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Fri Jul  2 12:06:08 2021
*/

class KVINDRAFilterGroupReconstructor : public KVFilterGroupReconstructor {
public:
   KVINDRAFilterGroupReconstructor()
      : KVFilterGroupReconstructor()
   {
   }

   ClassDef(KVINDRAFilterGroupReconstructor, 1) //Reconstruct simulated events after filtering with INDRA
};

#endif
