//Created by KVClassFactory on Fri Apr 26 12:45:15 2013
//Author: John Frankland,,,

#ifndef __KVDETECTORNODE_H
#define __KVDETECTORNODE_H

#include "KVBase.h"
class KVSeqCollection;
class KVDetector;
class KVGeoDNTrajectory;

class KVGeoDetectorNode : public KVBase {
    KVDetector*      fDetector;//!associated detector
   KVSeqCollection* fInFront;//list of detectors in front
   KVSeqCollection* fBehind;//list of detectors behind
    KVSeqCollection* fTraj;//list of trajectories passing through this node
    KVSeqCollection* fTrajF;//list of trajectories passing through this node going forwards
    Int_t fNTraj;//number of trajectories passing through this node
    Int_t fNTrajForwards;//number of trajectories going forwards from this node

   void init();

    void CalculateForwardsTrajectories();

public:
   KVGeoDetectorNode();
   KVGeoDetectorNode(const Char_t* name);
   virtual ~KVGeoDetectorNode();

   void SetDetector(KVDetector*);
   KVDetector* GetDetector() const;
   const Char_t* GetName() const;

   void AddInFront(KVDetector*);
   void AddBehind(KVDetector*);
   Bool_t IsInFrontOf(KVDetector*);
   Bool_t IsBehind(KVDetector*);
   KVSeqCollection* GetDetectorsInFront() const { return fInFront; }
   KVSeqCollection* GetDetectorsBehind() const { return fBehind; }
   KVSeqCollection* GetTrajectories() const { return fTraj; }
   KVSeqCollection* GetForwardTrajectories() const;
   Int_t GetNDetsInFront() const;
   Int_t GetNDetsBehind() const;
   Int_t GetNTraj() const;
   Int_t GetNTrajForwards() const;

	void BuildTrajectoriesForwards(TList*);
	void AddTrajectory(KVGeoDNTrajectory*);

   void RehashLists();

   KVGeoDNTrajectory* GetForwardTrajectoryWithMostFiredDetectors() const;
   KVGeoDNTrajectory* GetForwardTrajectoryWithLeastUnfiredDetectors() const;

   void ls(Option_t* option = "") const;

   ClassDef(KVGeoDetectorNode,2)//Stores lists of detectors in front and behind this node
};

#endif
