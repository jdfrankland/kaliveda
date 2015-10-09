//Created by John Frankland on Sun May 19 2002
//Author: John Frankland (frankland@ganil.fr)
#ifndef KVDETECTOREVENT_H
#define KVDETECTOREVENT_H

#include "KVBase.h"
#include "KVUniqueNameList.h"
#include "KVGroup.h"

class KVDetectorEvent: public KVBase {

private:

   KVUniqueNameList fHitGroups;        //list of groups hit by particles in the event

public:

   KVDetectorEvent() : fHitGroups(kFALSE) {}
   virtual ~ KVDetectorEvent()
   {
      Clear();
   }
   void AddGroup(KVGroup* grp)
   {
      fHitGroups.Add(grp);
   }
   virtual void Clear(Option_t* opt = "");
   virtual void Print(Option_t* t = "") const;
   KVUniqueNameList* GetGroups()
   {
      return &fHitGroups;
   }
   Bool_t ContainsGroup(KVGroup* grp)
   {
      return (fHitGroups.FindObject(grp->GetName()) != nullptr);
   }
   virtual UInt_t GetMult() const
   {
      return fHitGroups.GetEntries();
   }

   ClassDef(KVDetectorEvent, 2) // List of hit groups in a multidetector array
};
#endif
