//Created by KVClassFactory on Tue Feb 25 16:31:36 2014
//Author: John Frankland,,,

#ifndef __KVGEODNTRAJECTORY_H
#define __KVGEODNTRAJECTORY_H

#include "KVBase.h"
#include "TObjArray.h"
#include "KVGeoDetectorNode.h"
#include "KVDetector.h"

class KVGeoDNTrajectory : public KVBase {
   void init();

   TObjArray fNodes;
   static Int_t fGDNTrajNumber;
   mutable Int_t fIter_idx;//! index for iteration
   mutable Int_t fIter_limit;//! last index for iteration
   mutable Int_t fIter_delta;//! increment/decrement for each iteration
   mutable Int_t fIter_idx_sav;//! index for iteration
   mutable Int_t fIter_limit_sav;//! last index for iteration
   mutable Int_t fIter_delta_sav;//! increment/decrement for each iteration
   KVUniqueNameList fIDTelescopes;//list of id telescopes on this trajectory

   void rebuild_title();
   void SetTitle(const char* t = "")
   {
      KVBase::SetTitle(t);
   }

public:
   KVGeoDNTrajectory();
   KVGeoDNTrajectory(KVGeoDetectorNode*);
   KVGeoDNTrajectory(const KVGeoDNTrajectory&);
   virtual ~KVGeoDNTrajectory();
   void Copy(TObject& obj) const;
   KVGeoDNTrajectory& operator=(const KVGeoDNTrajectory&);

   KVGeoDetectorNode* GetNode(const Char_t* name) const
   {
      // return pointer to node with given name in this trajectory
      return (KVGeoDetectorNode*)fNodes.FindObject(name);
   }
   KVGeoDetectorNode* GetNodeInFront(const KVGeoDetectorNode* n) const
   {
      // return pointer to node immediately in front of 'n' on trajectory
      if (Contains(n) && !EndsAt(n)) {
         return GetNodeAt(Index(n) + 1);
      }
      return nullptr;
   }
   Bool_t Contains(const Char_t* name) const
   {
      // returns kTRUE if trajectory contains the given node/detector
      // (nodes have the same name as the detector they are associated to)
      return GetNode(name) != nullptr;
   }

   Int_t GetN() const
   {
      // return number of nodes on this trajectory
      return fNodes.GetEntries();
   }
   Int_t Index(const TObject* node) const
   {
      // return index of node (=0,1,...)
      return fNodes.IndexOf(node);
   }

   void AddLast(KVGeoDetectorNode* n)
   {
      // add node to end of trajectory
      fNodes.AddLast(n);
      n->AddTrajectory(this);
      rebuild_title();
   }

   void AddFirst(KVGeoDetectorNode* n)
   {
      // add node to start of trajectory
      fNodes.AddFirst(n);
      n->AddTrajectory(this);
      rebuild_title();
   }

   void ls(Option_t* = "") const
   {
      std::cout << GetName() << " : " << GetTitle() << std::endl;
   }

   Bool_t EndsAt(const Char_t* node_name) const
   {
      // Return kTRUE if node_name is last node of trajectory
      // (i.e. closest to target)

      return !strcmp(node_name, fNodes.Last()->GetName());
   }

   Bool_t BeginsAt(const Char_t* node_name) const
   {
      // Return kTRUE if node_name is first node of trajectory
      // (i.e. furthest from target)

      return !strcmp(node_name, fNodes.First()->GetName());
   }

   Bool_t EndsAt(const KVGeoDetectorNode* d) const
   {
      // Return kTRUE if n is last node of trajectory
      // (i.e. closest to target)

      return (d == (KVGeoDetectorNode*)fNodes.Last());
   }

   Bool_t BeginsAt(const KVGeoDetectorNode* d) const
   {
      // Return kTRUE if node_name is first node of trajectory
      // (i.e. furthest from target)

      return (d == (KVGeoDetectorNode*)fNodes.First());
   }

   Bool_t Contains(const KVGeoDetectorNode* n) const
   {
      return (fNodes.FindObject(n) != NULL);
   }
   Bool_t ContainsAll(const TCollection* l) const
   {
      // Returns kTRUE if trajectory contains all detectors/nodes in the list
      // N.B. we only check the names of the (TObject-derived) objects in the list
      TIter next(l);
      TObject* o;
      while ((o = next())) {
         if (!Contains(o->GetName())) return kFALSE;
      }
      return kTRUE;
   }
   Bool_t ContainsAllConsecutive(const TCollection* l, Bool_t direction = kIterForward) const
   {
      // Returns kTRUE if trajectory contains all detectors/nodes in the list
      // in the same consecutive order
      // N.B. we only check the names of the (TObject-derived) objects in the list
      //
      // if optional argument direction=kIterBackward then we look at the objects
      // in the list in reverse order

      TIter next(l, direction);
      TObject* o = next();
      KVGeoDetectorNode* n = GetNode(o->GetName());
      if (!n) return kFALSE;
      IterateFrom(n);
      GetNextNode();
      while ((o = next())) {
         n = GetNextNode();
         if (!n) return kFALSE; // trajectory ended before list of detectors
         if (!n->IsCalled(o->GetName())) return kFALSE;
      }
      return kTRUE;
   }

   KVGeoDetectorNode* GetNodeAt(Int_t i) const
   {
      // Return i-th node in trajectory
      //  i=0 -> first node, furthest from target
      //  i=GetN()-1 -> last node, closest to target

      return (KVGeoDetectorNode*)fNodes[i];
   }

   void SaveIterationState() const
   {
      // In order to perform an iteration while another is already in
      // progress, we store the current iteration.
      // Call RestoreIterationState() in order to continue.
      fIter_idx_sav = fIter_idx;
      fIter_delta_sav = fIter_delta;
      fIter_limit_sav = fIter_limit;
   }
   void RestoreIterationState() const
   {
      // See SaveIterationState().

      fIter_idx = fIter_idx_sav;
      fIter_delta = fIter_delta_sav;
      fIter_limit = fIter_limit_sav;
   }
   void IterateFrom(const KVGeoDetectorNode* node0 = nullptr) const
   {
      // FORWARD ITERATION: moving towards the target
      // Start an iteration over the trajectory nodes, starting from node0.
      // If node0=nullptr (default value), start from the first node.
      // After calling this method, calling method GetNextNode()
      // will return each node of the trajectory starting with node0
      // to the last one, after which it returns 0x0

      fIter_idx = (node0 == nullptr ? 0 : Index(node0));
      fIter_delta = 1;
      fIter_limit = GetN() - 1;
   }
   void IterateBackFrom(const KVGeoDetectorNode* node0 = nullptr) const
   {
      // BACKWARD ITERATION: moving away from the target
      // Start an iteration over the trajectory nodes, starting from node0.
      // If node0=nullptr (default value), start from the last node.
      // After calling this method, calling method GetNextNode()
      // will return each node of the trajectory starting with node0
      // to the last one, after which it returns 0x0

      fIter_idx = (node0 == nullptr ? GetN() - 1 : Index(node0));
      fIter_delta = -1;
      fIter_limit = 0;
   }

   KVGeoDetectorNode* GetNextNode() const
   {
      // Get next node in iteration over trajectory.
      // See IterateFrom(KVGeoDetectorNode*) and IterateBackFrom(KVGeoDetectorNode*)

      if (fIter_idx > -1) {
         if (fIter_delta * (fIter_limit - fIter_idx) > 0) {
            Int_t idx = fIter_idx;
            fIter_idx += fIter_delta;
            return GetNodeAt(idx);
         } else {
            // last node
            fIter_idx = -1;
            return GetNodeAt(fIter_limit);
         }
      }
      return nullptr;
   }

   Int_t GetNumberOfFiredDetectorsForwardsFrom(const KVGeoDetectorNode* node, Option_t* opt = "any")
   {
      // Starting from 'node', calculate and return the number of fired detectors
      // going forwards (move toward the target) along this trajectory
      // The option string will be passed to KVDetector::Fired(Option_t*)

      Int_t f = 0;
      IterateFrom(node);
      KVGeoDetectorNode* n;
      while ((n = GetNextNode())) {
         f += n->GetDetector()->Fired(opt);
      }
      return f;
   }

   Int_t GetNumberOfUnfiredDetectorsForwardsFrom(const KVGeoDetectorNode* node, Option_t* opt = "any")
   {
      // Starting from 'node', calculate and return the number of unfired detectors
      // going forwards (move toward the target) along this trajectory
      // The option string will be passed to KVDetector::Fired(Option_t*)

      Int_t f = 0;
      Int_t tot = 0;
      IterateFrom(node);
      KVGeoDetectorNode* n;
      while ((n = GetNextNode())) {
         f += n->GetDetector()->Fired(opt);
         tot++;
      }
      return tot - f;
   }

   const KVSeqCollection* GetIDTelescopes() const
   {
      // List of identification telescopes on trajectory
      return &fIDTelescopes;
   }
   KVSeqCollection* AccessIDTelescopeList()
   {
      // Modifiable list of identification telescopes on trajectory
      return &fIDTelescopes;
   }

   ClassDef(KVGeoDNTrajectory, 1) //Path taken by particles through multidetector geometry
};

#endif
