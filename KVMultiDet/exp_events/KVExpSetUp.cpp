//Created by KVClassFactory on Thu Feb 25 14:59:59 2016
//Author: bonnet,,,

#include "KVExpSetUp.h"
#include "TClass.h"
#include "KVReconstructedNucleus.h"

#include <KVGroup.h>
#include <KVRangeTableGeoNavigator.h>

ClassImp(KVExpSetUp)

void KVExpSetUp::init()
{
   fBuildTarget = kFALSE;
   fCloseGeometryNow = kFALSE;
   //modification of the owner mode compare to Mother classes
   SetOwnsDaughters(kFALSE);
   SetOwnsDetectors(kFALSE);
   fIDTelescopes->SetOwner(kFALSE);
}


KVExpSetUp::KVExpSetUp()
{
   // Default constructor
   init();
}

KVExpSetUp::~KVExpSetUp()
{
   // Destructor
   fCloseGeometryNow = kTRUE;
}


void KVExpSetUp::Build(Int_t run)
{
   // Build the combined arrays
   //
   // The name of the setup will be "[det1]-[det2]-..."

   // default ROOT geometry for all arrays
   gEnv->SetValue("KVMultiDetArray.ROOTGeometry", "yes");

   CreateGeoManager();

   Info("Build", "navigator=%p", GetNavigator());

   KVRangeTableGeoNavigator* gnl = new KVRangeTableGeoNavigator(gGeoManager, KVMaterial::GetRangeTable());
   SetNavigator(gnl);

   TString myname;

   KVMultiDetArray* tmp(nullptr);
   Int_t group_offset = 0;// for renumbering groups
   lmultidetarrayclasses = GetDataSetEnv(fDataSet, "DataSet.ExpSetUp.ClassList", IsA()->GetName());
   lmultidetarrayclasses.Begin(" ");

   // VERY IMPORTANT: deactivate 'SetParameters(run)' being called by MakeMultiDetector for sub-arrays!
   fMakeMultiDetectorSetParameters = kFALSE;

   while (!lmultidetarrayclasses.End()) {
      KVString sname = lmultidetarrayclasses.Next();
      Info("Build", "Build %s %s\n", fDataSet.Data(), sname.Data());
      gMultiDetArray = nullptr; //otherwise MakeMultiDetector will delete any previously built array
      tmp = MakeMultiDetector(fDataSet, run, sname.Data());
      if (tmp) {
         fMDAList.Add(tmp);
         if (myname != "") myname += "-";
         myname += tmp->GetName();
      }
      else {
         Error("Build", "NULL pointer returned by MakeMultiDetector");
      }
   }

   // VERY IMPORTANT: reactivate 'SetParameters(run)' so it is called for us when we return
   // to the MakeMultiDetector method which called us
   fMakeMultiDetectorSetParameters = kTRUE;

   gGeoManager->CloseGeometry();

   TIter nxt_mda(&fMDAList);
   while ((tmp = (KVMultiDetArray*)nxt_mda())) {
      tmp->PerformClosedROOTGeometryOperations();
      std::unique_ptr<KVSeqCollection> groups(tmp->GetStructureTypeList("GROUP"));
      if (group_offset) {
         // renumber all groups to keep unique names/numbers
         TIter next(groups.get());
         KVGroup* group;
         while ((group = (KVGroup*)next())) {
            Int_t group_number = group->GetNumber();
            group->SetNumber(group_number + group_offset);
         }
      }
      else
         group_offset += groups->GetEntries();

      fDetectors.AddAll((KVUniqueNameList*)tmp->GetDetectors());
      fStructures.AddAll((KVUniqueNameList*)tmp->GetStructures());
      fIDTelescopes->AddAll(tmp->GetListOfIDTelescopes());

      // retrieve correspondance list node path<->detector
      gnl->AbsorbDetectorPaths(tmp->GetNavigator());
   }

   SetGeometry(gGeoManager);

   gMultiDetArray = this;
   SetBit(kIsBuilt);
   SetName(myname);
}

void KVExpSetUp::AcceptParticleForAnalysis(KVReconstructedNucleus* NUC) const
{
   // Overrides KVMultiDetArray method
   // We use the "ARRAY" parameter of the reconstructed particle (if set)
   // to know which array of the setup detected it.

   if (NUC->GetParameters()->HasStringParameter("ARRAY")) {
      KVMultiDetArray* whichArray = GetArray(NUC->GetParameters()->GetStringValue("ARRAY"));
      if (whichArray) {
         whichArray->AcceptParticleForAnalysis(NUC);
      }
      else
         Warning("AcceptParticleForAnalysis", "ReconstructedNucleus has ARRAY=%s but no KVMultiDetArray with this name in set-up",
                 NUC->GetParameters()->GetStringValue("ARRAY"));
   }
   else {
      Warning("AcceptParticleForAnalysis", "ReconstructedNucleus has no ARRAY parameter: cannot determine correct KVMultiDetArray");
   }
}

void KVExpSetUp::GetArrayMultiplicities(KVReconstructedEvent* e, KVNameValueList& m, Option_t* opt)
{
   // Calculate multiplicities of particles in each array of the setup.
   // They will appear in the KVNameValueList as parameters with the name of the array.
   // e.g.
   //
   //    "INDRA" = 21
   //    "FAZIA" = 3
   //
   // Any option given will be used to determine the type of event iterator used -
   // see KVEvent::GetNextParticleIterator(Option_t*).

   m.Clear();
   for (KVReconstructedEvent::Iterator it = e->GetNextParticleIterator(opt); it != e->end(); ++it) {
      m.IncrementValue((*it).GetParameters()->GetStringValue("ARRAY"), 1);
   }
}

Bool_t KVExpSetUp::HandleRawDataEvent(KVRawDataReader* rawdata)
{
   // Set fRawDataReader pointer in each sub-array and call prepare_to_handle_new_raw_data()
   // for each sub-array, before treating raw data.
   //
   // copy fired signals & detectors of sub-arrays to main lists
   //
   // copy any reconstruction parameters from sub-arrays to main list

   TIter next_array(&fMDAList);
   KVMultiDetArray* mda;
   std::vector<std::thread> threads;
   while ((mda = (KVMultiDetArray*)next_array())) {
      threads.push_back(std::thread([ = ]() {
         mda->fRawDataReader = rawdata;
         mda->prepare_to_handle_new_raw_data();
      }));
   }
   for (auto& th : threads) {
      if (th.joinable()) th.join();
   }
   if (KVMultiDetArray::HandleRawDataEvent(rawdata)) {
      // copy fired signals & detectors of sub-arrays to main lists
      next_array.Reset();
      while ((mda = (KVMultiDetArray*)next_array())) {
         fFiredDetectors.AddAll(&mda->fFiredDetectors);
         fFiredSignals.AddAll(&mda->fFiredSignals);
         fReconParameters += mda->GetReconParameters();
      }
      return true;
   }
   return false;
}
