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
   Int_t fIter_idx;//! index for iteration
   Int_t fIter_max;//! max index for iteration

   public:
   KVGeoDNTrajectory();
   KVGeoDNTrajectory(KVGeoDetectorNode*);
   KVGeoDNTrajectory(const KVGeoDNTrajectory&);
   virtual ~KVGeoDNTrajectory();
   void Copy(TObject& obj) const;

   const Char_t* GetTitle() const;

   Bool_t Contains(const Char_t* node_name) const
   {
      return (fNodes.FindObject(node_name)!=NULL);
   }

   Int_t GetN() const { return fNodes.GetEntries(); }
   Int_t Index(const TObject*node) const { return fNodes.IndexOf(node); }

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
           cout << GetName() << " : " << GetTitle() << endl;
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

   KVGeoDetectorNode* GetNodeAt(Int_t i)
   {
           // Return i-th node in trajectory
           //  i=0 -> first node, furthest from target
           //  i=GetN()-1 -> last node, closest to target

       return (KVGeoDetectorNode*)fNodes[i];
   }

   void IterateFrom(const KVGeoDetectorNode *node0)
   {
       // Start an iteration over the trajectory nodes, starting from node0.
       // After calling this method, calling method GetNextNode()
       // will return each node of the trajectory starting with node0
       // to the last one, after which it returns 0x0

       fIter_idx = Index(node0);
       fIter_max = GetN()-1;
   }

   KVGeoDetectorNode *GetNextNode()
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
       return 0x0;
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

   ClassDef(KVGeoDNTrajectory,1)//Path taken by particles through multidetector geometry
};

#endif
