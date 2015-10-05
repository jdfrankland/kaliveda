//Created by KVClassFactory on Tue Feb 25 16:31:36 2014
//Author: John Frankland,,,

#ifndef __KVGEODNTRAJECTORY_H
#define __KVGEODNTRAJECTORY_H

#include "KVBase.h"
#include "TObjArray.h"
#include "KVGeoDetectorNode.h"
#include "KVDetector.h"

class KVGeoDNTrajectory : public KVBase
{
   void init();

   TObjArray fNodes;
   static Int_t fGDNTrajNumber;
   mutable Int_t fIter_idx;//! index for iteration
   mutable Int_t fIter_max;//! max index for iteration
   TList fIDTelescopes;//list of id telescopes on this trajectory

   public:
   KVGeoDNTrajectory();
   KVGeoDNTrajectory(KVGeoDetectorNode*);
   KVGeoDNTrajectory(const KVGeoDNTrajectory&);
   virtual ~KVGeoDNTrajectory();
   void Copy(TObject& obj) const;

   const Char_t* GetTitle() const;

   KVGeoDetectorNode* GetNode(const Char_t* name) const
   {
      // return pointer to node with given name in this trajectory
      return (KVGeoDetectorNode*)fNodes.FindObject(name);
   }
   Bool_t Contains(const Char_t* name) const
   {
      // returns kTRUE if trajectory contains the given node/detector
      // (nodes have the same name as the detector they are associated to)
      return GetNode(name)!=nullptr;
   }

   Int_t GetN() const {
      // return number of nodes on this trajectory
      return fNodes.GetEntries();
   }
   Int_t Index(const TObject*node) const {
      // return index of node (=0,1,...)
      return fNodes.IndexOf(node);
   }

   void AddLast(KVGeoDetectorNode* n)
   {
           // add node to end of trajectory
           fNodes.AddLast(n);
           n->AddTrajectory(this);
   }

   void AddFirst(KVGeoDetectorNode* n)
   {
           // add node to start of trajectory
           fNodes.AddFirst(n);
           n->AddTrajectory(this);
   }

   void ls(Option_t* = "") const
   {
           std::cout << GetName() << " : " << GetTitle() << std::endl;
   }

   Bool_t EndsAt(const Char_t* node_name) const
   {
       // Return kTRUE if node_name is last node of trajectory
       // (i.e. closest to target)

       return !strcmp(node_name,fNodes.Last()->GetName());
   }

   Bool_t BeginsAt(const Char_t* node_name) const
   {
       // Return kTRUE if node_name is first node of trajectory
       // (i.e. furthest from target)

       return !strcmp(node_name,fNodes.First()->GetName());
   }

   Bool_t EndsAt(const KVGeoDetectorNode* d) const
   {
       // Return kTRUE if n is last node of trajectory
       // (i.e. closest to target)

       return ( d == (KVGeoDetectorNode*)fNodes.Last() );
   }

   Bool_t BeginsAt(const KVGeoDetectorNode *d) const
   {
       // Return kTRUE if node_name is first node of trajectory
       // (i.e. furthest from target)

       return ( d == (KVGeoDetectorNode*)fNodes.First() );
   }

   Bool_t Contains(const KVGeoDetectorNode *n) const
   {
       return (fNodes.FindObject(n)!=NULL);
   }
   Bool_t ContainsAll(const TCollection* l) const
   {
      // Returns kTRUE if trajectory contains all detectors/nodes in the list
      // N.B. we only check the names of the (TObject-derived) objects in the list
      TIter next(l);
      TObject* o;
      while((o=next())) {
         if(!Contains(o->GetName())) return kFALSE;
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

      TIter next(l,direction);
      TObject* o = next();
      KVGeoDetectorNode* n = GetNode(o->GetName());
      if(!n) return kFALSE;
      IterateFrom(n);
      GetNextNode();
      while((o=next())){
         n = GetNextNode();
         if(!n) return kFALSE;// trajectory ended before list of detectors
         if(!n->IsCalled(o->GetName())) return kFALSE;
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

   void IterateFrom(const KVGeoDetectorNode *node0 = nullptr) const
   {
       // Start an iteration over the trajectory nodes, starting from node0.
       // If node0=nullptr (default value), start from the first node.
       // After calling this method, calling method GetNextNode()
       // will return each node of the trajectory starting with node0
       // to the last one, after which it returns 0x0

       fIter_idx = (node0==nullptr ? 0 : Index(node0));
       fIter_max = GetN()-1;
   }

   KVGeoDetectorNode *GetNextNode() const
   {
       // Get next node in iteration over trajectory.
       // See IterateFrom(KVGeoDetectorNode*)

       if(fIter_idx>-1){
           if(fIter_idx<fIter_max) return GetNodeAt(fIter_idx++);
           else {
               // last node
               fIter_idx=-1;
               return GetNodeAt(fIter_max);
           }
       }
       return nullptr;
   }

   Int_t GetNumberOfFiredDetectorsForwardsFrom(const KVGeoDetectorNode *node, Option_t * opt = "any")
   {
       // Starting from 'node', calculate and return the number of fired detectors
       // going forwards along this trajectory
       // The option string will be passed to KVDetector::Fired(Option_t*)

       Int_t f=0;
       IterateFrom(node);
       KVGeoDetectorNode* n;
       while( (n = GetNextNode()) ){
           f += n->GetDetector()->Fired(opt);
       }
       return f;
   }

   Int_t GetNumberOfUnfiredDetectorsForwardsFrom(const KVGeoDetectorNode *node, Option_t * opt = "any")
   {
       // Starting from 'node', calculate and return the number of unfired detectors
       // going forwards along this trajectory
      // The option string will be passed to KVDetector::Fired(Option_t*)

       Int_t f=0;
       Int_t tot=0;
       IterateFrom(node);
       KVGeoDetectorNode* n;
       while( (n = GetNextNode()) ){
           f += n->GetDetector()->Fired(opt);
           tot++;
       }
       return tot-f;
   }

   void FillListOfIDTelescopes();
   const TList& GetIDTelescopes() const
   {
      // List of identification telescopes on trajectory
      return fIDTelescopes;
   }

   ClassDef(KVGeoDNTrajectory,1)//Path taken by particles through multidetector geometry
};

#endif
