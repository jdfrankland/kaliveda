//Created by KVClassFactory on Mon Oct 19 09:33:44 2015
//Author: John Frankland,,,

#ifndef __KVEVENTRECONSTRUCTOR_H
#define __KVEVENTRECONSTRUCTOR_H

#include "KVBase.h"

class KVEventReconstructor : public KVBase {

public:
   KVEventReconstructor();
   virtual ~KVEventReconstructor();

   void Copy(TObject& obj) const;

   virtual void ReconstructEvent(KVReconstructedEvent*, KVDetectorEvent*);
   virtual void ReconstructParticle(KVReconstructedNucleus* part, const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node);
   virtual void AnalyseParticlesInGroup(KVGroup* grp);

   ClassDef(KVEventReconstructor, 1) //Base class for handling event reconstruction
};

#endif
