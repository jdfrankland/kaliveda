//Created by KVClassFactory on Fri Apr 26 12:45:15 2013
//Author: John Frankland,,,

#include "KVGeoDetectorNode.h"
#include "KVGeoDNTrajectory.h"
#include "KVDetector.h"
#include "KVUniqueNameList.h"
#include "TList.h"

ClassImp(KVGeoDetectorNode)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVGeoDetectorNode</h2>
<h4>Stores lists of detectors in front and behind this node</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

void KVGeoDetectorNode::init()
{
    fInFront=0;
    fBehind=0;
     fDetector=0;
	 fTraj=0;
     fNTrajForwards=-1;
     fNTraj=-1;
     fTrajF=0;
}

void KVGeoDetectorNode::CalculateForwardsTrajectories()
{
    // Fill list with all trajectories going forwards from this node

    fNTrajForwards=0;
    if(GetNTraj()){
        TIter next(GetTrajectories());
        KVGeoDNTrajectory* t;
        while(( t = (KVGeoDNTrajectory*)next() )){
            if(!t->EndsAt(this)){
                fNTrajForwards++;
                if(!fTrajF) fTrajF=new KVUniqueNameList;
                fTrajF->Add(t);
            }
        }
    }
}

KVGeoDetectorNode::KVGeoDetectorNode()
{
   // Default constructor
    init();
}

//________________________________________________________________

KVGeoDetectorNode::KVGeoDetectorNode(const Char_t* name) : KVBase(name, "Detector node")
{
   // Write your code here
    init();
}

KVGeoDetectorNode::~KVGeoDetectorNode()
{
   // Destructor
    SafeDelete(fInFront);
    SafeDelete(fBehind);
    SafeDelete(fTraj);
    SafeDelete(fTrajF);
}

void KVGeoDetectorNode::SetDetector(KVDetector *d)
{
    fDetector=d;
}

KVDetector *KVGeoDetectorNode::GetDetector() const
{
    return fDetector;
}

const Char_t *KVGeoDetectorNode::GetName() const
{
    // Name of node is same as name of associated detector
    return (fDetector ? fDetector->GetName() : KVBase::GetName());
}

void KVGeoDetectorNode::ls(Option_t*) const
{
    std::cout << "Detector Node " << GetName() << std::endl;
    if(fInFront){
        std::cout << "In front:" << std::endl;
        fInFront->Print();
    }
    if(fBehind){
        std::cout << "Behind:" << std::endl;
        fBehind->Print();
    }
	 if(fTraj){
	     std::cout << "Trajectories:" << std::endl;
		  fTraj->R__FOR_EACH(KVGeoDNTrajectory,ls)();
	 }
}

void KVGeoDetectorNode::AddTrajectory(KVGeoDNTrajectory* t)
{
	if(!fTraj) {fTraj = new KVUniqueNameList; fTraj->SetCleanup();}
	fTraj->Add(t);
}

void KVGeoDetectorNode::AddInFront(KVDetector* d)
{
    if(!fInFront) fInFront = new KVUniqueNameList;
    fInFront->Add(d);
}

void KVGeoDetectorNode::AddBehind(KVDetector* d)
{
    if(!fBehind) fBehind = new KVUniqueNameList;
    fBehind->Add(d);
}
Bool_t KVGeoDetectorNode::IsInFrontOf(KVDetector* d)
{
    // return true if this node is directly in front of the detector
    return (fBehind && fBehind->FindObject(d)!=0);
}
Bool_t KVGeoDetectorNode::IsBehind(KVDetector* d)
{
    // return true if this node is directly behind the detector
    return (fInFront && fInFront->FindObject(d)!=0);
}

KVSeqCollection *KVGeoDetectorNode::GetForwardTrajectories() const
{
    // Return list of all trajectories going forwards from this node
    // Returns 0x0 if there are no forwards trajectories

    if(fNTrajForwards<0) const_cast<KVGeoDetectorNode*>(this)->CalculateForwardsTrajectories();
    return fTrajF;
}

Int_t KVGeoDetectorNode::GetNDetsInFront() const
{
    // Returns number of detectors directly in front of this one
    return (fInFront ? fInFront->GetEntries() : 0);
}

Int_t KVGeoDetectorNode::GetNDetsBehind() const
{
    // Returns number of detectors directly behind this one
    return (fBehind ? fBehind->GetEntries() : 0);
}

Int_t KVGeoDetectorNode::GetNTraj() const
{
    // Returns number of trajectories passing through this node
    if(fNTraj<0){
        const_cast<KVGeoDetectorNode*>(this)->fNTraj=(fTraj ? fTraj->GetEntries() : 0);
    }
    return fNTraj;
}

Int_t KVGeoDetectorNode::GetNTrajForwards() const
{
    // Returns number of trajectories which go forwards (towards the target)
    // from this node, i.e. the number of trajectories of which this is not the
    // end-point node
    // If not already done, this sets up the list of forwards trajectories

    if(fNTrajForwards<0) const_cast<KVGeoDetectorNode*>(this)->CalculateForwardsTrajectories();
    return fNTrajForwards;
}

void KVGeoDetectorNode::RehashLists()
{
    // Call this method if detector names change after lists are filled
    // (they are hash lists, if names of objects change, strange behaviour
    // will occur: you could put the same object in a list twice)

    if(fInFront) dynamic_cast<KVUniqueNameList*>(fInFront)->Rehash();
    if(fBehind) dynamic_cast<KVUniqueNameList*>(fBehind)->Rehash();
}

KVGeoDNTrajectory *KVGeoDetectorNode::GetForwardTrajectoryWithMostFiredDetectors() const
{
    // Return pointer to trajectory going forwards from this node
    // on which there are the largest number of fired detectors
    // In case of ambiguities (i.e. more than one trajectory with the same number
    // of fired detectors) this method returns 0x0

    Int_t most_fired=0;
    KVGeoDNTrajectory* mft=0;
    TIter next( GetForwardTrajectories() );
    KVGeoDNTrajectory* t;
    while( (t = (KVGeoDNTrajectory*)next()) ){

        Int_t nfired = t->GetNumberOfFiredDetectorsForwardsFrom(this);
        if(nfired>most_fired){
            most_fired=nfired;
            mft=t;
        }
        else if(most_fired && (nfired==most_fired)){
            return 0x0;
        }
    }
    return mft;
}

KVGeoDNTrajectory *KVGeoDetectorNode::GetForwardTrajectoryWithLeastUnfiredDetectors() const
{
    // Return pointer to trajectory going forwards from this node
    // on which there are the smallest number of unfired detectors.
    // In case of ambiguities (i.e. more than one trajectory with the same number
    // of unfired detectors) this method returns 0x0

    Int_t least_fired=99;
    KVGeoDNTrajectory* mft=0;
    TIter next( GetForwardTrajectories() );
    KVGeoDNTrajectory* t;
    while( (t = (KVGeoDNTrajectory*)next()) ){

        Int_t nunfired = t->GetNumberOfUnfiredDetectorsForwardsFrom(this);
        if(nunfired<least_fired){
            least_fired=nunfired;
            mft=t;
        }
        else if(nunfired==least_fired){
            return 0x0;
        }
    }
    return mft;
}

void KVGeoDetectorNode::BuildTrajectoriesForwards(TList*list)
{
	// Add this node to each trajectory in list
	// Then continue trajectories for each node in front of this one
	// If more than one node is in front, a new trajectory is created
	// and added to the list for each extra node
	//
	// N.B. we are building trajectories starting from nodes furthest from
	// target and moving towards it. Trajectories always go from the stopping
	// detector towards the target.
	// Therefore we add each new node to the end of each trajectory.
	
	if(!list->GetEntries()){
		// no trajectories in list
		// add new trajectory starting here
		list->Add(new KVGeoDNTrajectory(this));
	}
	else {
		// add this node to each trajectory in list
		list->R__FOR_EACH(KVGeoDNTrajectory,AddLast)(this);
	}
	// add each trajectory to list of trajectories through this node
	TIter nextT(list);
	KVGeoDNTrajectory* traj;
	
	
	// if no nodes in front of this one, stop
	if(!GetNDetsInFront()) return;

   nextT.Reset();	
	TList* newTrajectories = new TList;
	while( (traj = (KVGeoDNTrajectory*)nextT()) ){
		KVGeoDNTrajectory baseTraj(*traj);
		// for each trajectory in list
		// for first node in front of this one, continue existing trajectory
		// for each subsequent node in front, create new copy of existing trajectory
		// and continue it
		TIter nextN(fInFront);
		KVGeoDetectorNode* node;
		KVDetector* det;
		Int_t node_num=1;
		while( (det = (KVDetector*)nextN()) ){
			node = det->GetNode();
			if(node_num==1) node->BuildTrajectoriesForwards(list);
			else{
				KVGeoDNTrajectory* newTraj = new KVGeoDNTrajectory(baseTraj);
				newTrajectories->Add(newTraj);
				node->BuildTrajectoriesForwards(newTrajectories);
			}
			node_num++;
		}
	}
	if(newTrajectories->GetEntries()){
		list->AddAll(newTrajectories);
	}
	delete newTrajectories;
}
