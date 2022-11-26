//Created by KVClassFactory on Fri Apr 26 12:45:15 2013
//Author: John Frankland,,,

#include "KVGeoDetectorNode.h"
#include "KVGeoDNTrajectory.h"
#include "KVDetector.h"
#include "TList.h"

ClassImp(KVGeoDetectorNode)

void KVGeoDetectorNode::CalculateForwardsTrajectories()
{
   // Fill list with all trajectories going forwards from this node

   fNTrajForwards = 0;
   if (GetNTraj()) {
      TIter next(GetTrajectories());
      KVGeoDNTrajectory* t;
      while ((t = (KVGeoDNTrajectory*)next())) {
         if (!t->EndsAt(this)) {
            fNTrajForwards++;
            fTrajF.Add(t);
         }
      }
   }
}

void KVGeoDetectorNode::CalculateBackwardsTrajectories()
{
   // Fill list with all trajectories going backwards from this node

   fNTrajBackwards = 0;
   if (GetNTraj()) {
      TIter next(GetTrajectories());
      KVGeoDNTrajectory* t;
      while ((t = (KVGeoDNTrajectory*)next())) {
         if (!t->BeginsAt(this)) {
            fNTrajBackwards++;
            fTrajB.Add(t);
         }
      }
   }

}

void KVGeoDetectorNode::SetDetector(KVDetector* d)
{
   fDetector = d;
}

KVDetector* KVGeoDetectorNode::GetDetector() const
{
   return fDetector;
}

const Char_t* KVGeoDetectorNode::GetName() const
{
   // Name of node is same as name of associated detector
   return (fDetector ? fDetector->GetName() : KVBase::GetName());
}

void KVGeoDetectorNode::ls(Option_t*) const
{
   std::cout << "Detector Node " << GetName() << std::endl;
   if (GetNDetsInFront()) {
      std::cout << "In front:" << std::endl;
      fInFront.Print();
   }
   if (GetNDetsBehind()) {
      std::cout << "Behind:" << std::endl;
      fBehind.Print();
   }
   if (GetNTraj()) {
      std::cout << "Trajectories:" << std::endl;
      for (auto p : fTraj) dynamic_cast<KVGeoDNTrajectory*>(p)->ls();
   }
}

void KVGeoDetectorNode::AddTrajectory(KVGeoDNTrajectory* t)
{
   fTraj.Add(t);
}

void KVGeoDetectorNode::AddInFront(KVDetector* d)
{
   fInFront.Add(d);
}

void KVGeoDetectorNode::AddBehind(KVDetector* d)
{
   fBehind.Add(d);
}
Bool_t KVGeoDetectorNode::IsInFrontOf(KVDetector* d)
{
   // return true if this node is directly in front of the detector
   return fBehind.FindObject(d) != nullptr;
}
Bool_t KVGeoDetectorNode::IsBehind(KVDetector* d)
{
   // return true if this node is directly behind the detector
   return fInFront.FindObject(d) != nullptr;
}

void KVGeoDetectorNode::RehashLists()
{
   // Call this method if detector names change after lists are filled
   // (they are hash lists, if names of objects change, strange behaviour
   // will occur: you could put the same object in a list twice)

   if (GetNDetsInFront()) fInFront.Rehash();
   if (GetNDetsBehind())  fBehind.Rehash();
}

KVGeoDNTrajectory* KVGeoDetectorNode::FindTrajectory(const char* title) const
{
   // Return pointer to trajectory passing through this node with given title
   // The title is of the form "DET1/DET2/DET3/" made of the names of the
   // detectors/nodes on the trajectory

   return (KVGeoDNTrajectory*)fTraj.FindObjectByTitle(title);
}

KVGeoDNTrajectory* KVGeoDetectorNode::FindTrajectory(UInt_t number) const
{
   // Return pointer to trajectory passing through this node with given number

   return (KVGeoDNTrajectory*)fTraj.FindObjectByNumber(number);
}

void KVGeoDetectorNode::BuildTrajectoriesForwards(TSeqCollection* list)
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

   if (!list->GetEntries()) {
      // no trajectories in list
      // add new trajectory starting here
      list->Add(new KVGeoDNTrajectory(this));
   }
   else {
      // add this node to each trajectory in list
      list->R__FOR_EACH(KVGeoDNTrajectory, AddLast)(this);
   }
   // add each trajectory to list of trajectories through this node
   TIter nextT(list);
   KVGeoDNTrajectory* traj;

   // if no nodes in front of this one, stop
   if (!GetNDetsInFront()) return;

   nextT.Reset();
   TList newTrajectories;
   while ((traj = (KVGeoDNTrajectory*)nextT())) {
      KVGeoDNTrajectory baseTraj(*traj);
      // for each trajectory in list
      // for first node in front of this one, continue existing trajectory
      // for each subsequent node in front, create new copy of existing trajectory
      // and continue it
      TIter nextN(&fInFront);
      KVGeoDetectorNode* node;
      KVDetector* det;
      Int_t node_num = 1;
      while ((det = (KVDetector*)nextN())) {
         node = det->GetNode();
         if (node_num == 1) node->BuildTrajectoriesForwards(list);
         else {
            KVGeoDNTrajectory* newTraj = new KVGeoDNTrajectory(baseTraj);
            newTrajectories.Add(newTraj);
            node->BuildTrajectoriesForwards(&newTrajectories);
         }
         node_num++;
      }
   }
   if (newTrajectories.GetEntries()) {
      list->AddAll(&newTrajectories);
   }
}
