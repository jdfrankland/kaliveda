//Created by KVClassFactory on Thu Oct  8 10:52:39 2015
//Author: John Frankland,,,

#include "KVReconNucTrajectory.h"
#include "KVIDTelescope.h"

ClassImp(KVReconNucTrajectory)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVReconNucTrajectory</h2>
<h4>Reconstructed trajectory of nucleus detected by array</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVReconNucTrajectory::KVReconNucTrajectory(const KVReconNucTrajectory& o) :
   KVGeoDNTrajectory(), fIndependentIdentifications(0)
{
   // Copy constructor
   o.Copy(*this);
}

KVReconNucTrajectory::KVReconNucTrajectory(const KVGeoDNTrajectory* tr, const KVGeoDetectorNode* n) :
   KVGeoDNTrajectory(), fIndependentIdentifications(0)
{
   // Build a reconstructed trajectory on tr starting from node n

   fAddToNodes = kFALSE;
   KVUniqueNameList* idtlist = dynamic_cast<KVUniqueNameList*>(AccessIDTelescopeList());

   tr->SaveIterationState();// in case an iteration was already underway
   // add all nodes starting at n
   tr->IterateFrom(n);
   KVGeoDetectorNode* _n;
   while ((_n = tr->GetNextNode())) {
      AddLast(_n);
   }
   // add all ID telescopes from parent trajectory which contain only
   // detectors on this trajectory
   TIter next(tr->GetIDTelescopes());
   KVIDTelescope* idt;
   while ((idt = (KVIDTelescope*)next())) {
      if (ContainsAll(idt->GetDetectors())) {
         idtlist->Add(idt);
         if (idtlist->ObjectAdded()) fIndependentIdentifications += (Int_t)idt->IsIndependent();
      }
   }

   // unique name for fast look-up in hash table
   SetName(Form("%s_%s", tr->GetTrajectoryName(), n->GetName()));
   tr->RestoreIterationState();
}

KVReconNucTrajectory& KVReconNucTrajectory::operator=(const KVReconNucTrajectory& r)
{
   r.Copy(*this);
   return (*this);
}

void KVReconNucTrajectory::Copy(TObject& obj) const
{
   // This method copies the current state of 'this' object into 'obj'
   // You should add here any member variables, for example:
   //    (supposing a member variable KVReconNucTrajectory::fToto)
   //    CastedObj.fToto = fToto;
   // or
   //    CastedObj.SetToto( GetToto() );

   KVGeoDNTrajectory::Copy(obj);
   KVReconNucTrajectory& CastedObj = (KVReconNucTrajectory&)obj;
   CastedObj.fIndependentIdentifications = fIndependentIdentifications;
}

void KVReconNucTrajectory::ls(Option_t*) const
{
   KVGeoDNTrajectory::ls();
   std::cout << "Identifications [" << GetIDTelescopes()->GetEntries() << "/"
             << fIndependentIdentifications << "] : " << std::endl;
   TIter next(GetIDTelescopes());
   KVIDTelescope* idt;
   while ((idt = (KVIDTelescope*)next())) {
      std::cout << "\t" << idt->GetName() << " (" << idt->IsIndependent() << ")" << std::endl;
   }
}


