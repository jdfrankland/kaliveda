//Created by KVClassFactory on Tue Feb 25 16:31:36 2014
//Author: John Frankland,,,

#include "KVGeoDNTrajectory.h"

#include <TPluginManager.h>

ClassImp(KVGeoDNTrajectory)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVGeoDNTrajectory</h2>
<h4>Path taken by particles through multidetector geometry</h4>
<!-- */
// --> END_HTML
// A KVGeoDNTrajectory represents one possible trajectory that particles produced in
// an event may take to travel from the target through the detectors of the multidetector
// array. Each trajectory is made up of a series of KVGeoDetectorNode objects which
// are each associated with a detector in the geometry. As the trajectories are mostly
// used in particle reconstruction & identification, each trajectory starts from the last
// detector in a stack and moves towards the target.
//
// NAME, NUMBER & TITLE
// GetName()   - The name of each trajectory is "GDNTraj_x" where x is the unique number of the trajectory.
// GetNumber() - The unique number of the trajectory.
// GetTitle()  - The title of each trajectory is "DET1/DET2/DET3/..." made up from the names of the
//               detectors/nodes on the trajectory.
//
// ITERATE OVER TRAJECTORY
// In order to visit each detector/node of the trajectory in order, use the iterators:
//
//    KVGeoDNTrajectory* trajectory; // pointer to some valid trajectory
//    trajectory->IterateFrom();     // default: start from first node of trajectory
//                                   // [i.e. the detector furthest from the target]
//    KVGeoDetectorNode* node;
//    while( (node = trajectory->GetNextNode()) ){
//
//       KVDetector* detector = node->GetDetector();   // detector associated to node
//       /* your code */
//
//    }
//
// You can also start from any node on the trajectory:
//
//    KVGeoDetectorNode* node_on_traj = trajectory->GetNodeAt(1); // start from second node
//    trajectory->IterateFrom(node_on_traj);
//    while( (node = trajectory->GetNextNode()) ){
//
//       /* your code */
//
//    }
//
// To iterate in the other direction i.e. away from the target:
//
//    trajectory->IterateBackFrom(); // default: start from last node of trajectory
//                                   // [i.e. the detector closest to the target]
//    trajectory->IterateBackFrom(node_on_traj);
////////////////////////////////////////////////////////////////////////////////

Int_t KVGeoDNTrajectory::fGDNTrajNumber = 0;

KVGeoDNTrajectory::KVGeoDNTrajectory() : fNodes(3, 0), fIDTelescopes(kFALSE), fAddToNodes(kTRUE)
{
   // Default constructor
   init();
}
//______________________
KVGeoDNTrajectory::KVGeoDNTrajectory(KVGeoDetectorNode* node) : fNodes(3, 0), fIDTelescopes(kFALSE), fAddToNodes(kTRUE)

{
   // Create a new trajectory starting from node
   AddFirst(node);
   init();
}
//______________________
KVGeoDNTrajectory::KVGeoDNTrajectory(const KVGeoDNTrajectory& obj)
   : KVBase(), fNodes(3, 0), fIDTelescopes(kFALSE), fAddToNodes(kTRUE)
{
   //copy ctor
   obj.Copy(*this);
   init();
}

KVGeoDNTrajectory::~KVGeoDNTrajectory()
{
   // Destructor
}

KVGeoDNTrajectory* KVGeoDNTrajectory::Factory(const char* plugin, const KVGeoDNTrajectory* t, const KVGeoDetectorNode* n)
{
   // Instantiate & return object of class corresponding to plugin

   TPluginHandler* ph = LoadPlugin("KVGeoDNTrajectory", plugin);
   if (ph) {

      return (KVGeoDNTrajectory*)ph->ExecPlugin(2, t, n);
   }
   return (KVGeoDNTrajectory*)nullptr;
}

void KVGeoDNTrajectory::init()
{
   fIter_idx = fIter_idx_sav = -1;
   ++fGDNTrajNumber;
   SetName(Form("GDNTraj_%d", fGDNTrajNumber));
   SetNumber(fGDNTrajNumber);
}

void KVGeoDNTrajectory::rebuild_title()
{
   // called every time a new node is added to the trajectory
   // to update the title with the new node name
   // Dynamically constructed title: DET1/DET2/DET3/...

   TString t;
   TIter next(&fNodes);
   KVGeoDetectorNode* n;
   while ((n = (KVGeoDetectorNode*)next())) {
      t += n->GetName();
      t += "/";
   }
   SetTitle(t);
}

void KVGeoDNTrajectory::Copy(TObject& obj) const
{
   KVBase::Copy(obj);
   KVGeoDNTrajectory& CastedObj = (KVGeoDNTrajectory&)obj;
   TIter next(&fNodes);
   KVGeoDetectorNode* node;
   while ((node = (KVGeoDetectorNode*)next())) CastedObj.AddLast(node);
   CastedObj.fAddToNodes = fAddToNodes;
   fIDTelescopes.Copy(CastedObj.fIDTelescopes);
}

KVGeoDNTrajectory& KVGeoDNTrajectory::operator=(const KVGeoDNTrajectory& t)
{
   t.Copy(*this);
   return (*this);
}

