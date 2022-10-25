//Created by KVClassFactory on Thu Feb 25 14:59:59 2016
//Author: bonnet,,,

#ifndef __KVEXPSETUP_H
#define __KVEXPSETUP_H

#include "KVMultiDetArray.h"
#include <KVExpDB.h>
class KVDBRun;

/**
  \class KVExpSetUp
  \ingroup Geometry
  \brief Describe an experimental set-up coupling two or more different detector arrays

KVExpSetUp combines two or more existing KVMultiDetArray objects to describe the
geometry, detectors, etc. of an experiment coupling several different arrays.
Each KVMultiDetArray can have its own
dedicated calibration database (KVExpDB and KVUpdater class)
used to store and set calibration parameters.

\note All detector & structure names in individual arrays must be unique! The group numbering has to be modified (in Build()) to make them unique.
 */
class KVExpSetUp : public KVMultiDetArray {

   void for_each_array(const std::function<void(KVMultiDetArray*)>& f,
                       const std::function<bool(KVMultiDetArray*)>& break_cond = [](KVMultiDetArray*)
   {
      return false;
   })
   const
   {
      // perform the same function call f for each array in the setup
      //
      // if break_cond is given, it is tested after each call to f, and if it returns true,
      // we stop the loop
      TIter next_array(&fMDAList);
      KVMultiDetArray* mda;
      while ((mda = (KVMultiDetArray*)next_array())) {
         f(mda);
         if (break_cond(mda)) break;
      }
   }

protected:
   KVList fMDAList;  //list of multidetarrays
   KVString lmultidetarrayclasses;

   void init();

#ifdef WITH_MFM
   Bool_t handle_raw_data_event_mfmframe(const MFMCommonFrame& mfmframe)
   {
      // Handle single (not merged) MFM frames of raw data. It is assumed
      // that each frame type corresponds to a different detector array.
      // Therefore as soon as one of them treats the data in the frame,
      // we return kTRUE.

      Bool_t return_value = kFALSE;
      for_each_array(
      [](KVMultiDetArray*) {},
      [&](KVMultiDetArray * mda) {
         if (mda->handle_raw_data_event_mfmframe(mfmframe)) {
            mda->fHandledRawData = true;
            return_value = kTRUE;
         }
         return return_value;
      }
      );
      return return_value;
   }
#endif
   void copy_fired_parameters_to_recon_param_list()
   {
      for_each_array([](KVMultiDetArray * mda) {
         if (mda->HandledRawData()) mda->copy_fired_parameters_to_recon_param_list();
      }
                    );
   }

   virtual void SetExpectedDetectorSignalNames()
   {
      for_each_array([](KVMultiDetArray * mda) {
         mda->SetExpectedDetectorSignalNames();
      }
                    );
   }

public:

   KVExpSetUp();
   virtual ~KVExpSetUp();
   virtual void Build(Int_t run = -1);
   void Clear(Option_t* opt = "")
   {
      // call Clear(opt) for each multidetector in the setup

      for_each_array([ = ](KVMultiDetArray * mda) {
         mda->Clear(opt);
      });
   }

   void FillDetectorList(KVReconstructedNucleus* rnuc, KVHashList* DetList, const KVString& DetNames)
   {
      // Call FillDetectorList for each array in turn, until DetList gets filled
      // This is because some arrays may override the KVMultiDetArray::FillDetectorList method

      for_each_array(
      [ = ](KVMultiDetArray * mda) {
         mda->FillDetectorList(rnuc, DetList, DetNames);
      },
      [ = ](KVMultiDetArray*) {
         return !DetList->IsEmpty();
      }
      );
   }
   virtual KVMultiDetArray* GetArray(const Char_t* name) const
   {
      // Return pointer to array in set up with given name
      return (KVMultiDetArray*)fMDAList.FindObject(name);
   }
   virtual void AcceptParticleForAnalysis(KVReconstructedNucleus*) const;

   void GetDetectorEvent(KVDetectorEvent* detev, const TSeqCollection* fired_params = nullptr)
   {
      // Override KVMultiDetArray method
      // Call each array in turn and add fired groups to the KVDetectorEvent

      for_each_array(
      [ = ](KVMultiDetArray * mda) {
         mda->GetDetectorEvent(detev, fired_params);
      }
      );
   }
   KVGroupReconstructor* GetReconstructorForGroup(const KVGroup* g) const
   {
      // Override KVMultiDetArray method
      // Call each array in turn to get reconstructor for group

      KVGroupReconstructor* gr(nullptr);
      for_each_array(
      [ =, &gr](KVMultiDetArray * mda) {
         gr = mda->GetReconstructorForGroup(g);
      },
      [ = ](KVMultiDetArray*) {
         return gr != nullptr;
      }
      );
      return gr;
   }
   void SetRawDataFromReconEvent(KVNameValueList& l)
   {
      prepare_to_handle_new_raw_data();
      for_each_array([&](KVMultiDetArray * mda) {
         mda->SetRawDataFromReconEvent(l);
      });
   }

   void GetArrayMultiplicities(KVReconstructedEvent*, KVNameValueList&, Option_t* = "");

   void MakeCalibrationTables(KVExpDB* db)
   {
      TString orig_dbtype = db->GetDBType();
      for_each_array([ = ](KVMultiDetArray * mda) {
         db->SetDBType(Form("%sDB", mda->GetName()));
         mda->MakeCalibrationTables(db);
      }
                    );
      db->SetDBType(orig_dbtype);
   }
   void SetCalibratorParameters(KVDBRun* r, const TString& = "")
   {
      // Set calibrators for all detectors for the run
      for_each_array([ = ](KVMultiDetArray * mda) {
         mda->SetCalibratorParameters(r, mda->GetName());
      }
                    );
   }
   void CheckStatusOfDetectors(KVDBRun* r, const TString& = "")
   {
      // Check status (present, working) for all detectors for the run
      for_each_array([ = ](KVMultiDetArray * mda) {
         mda->SetCurrentRunNumber(r->GetNumber());
         mda->CheckStatusOfDetectors(r, mda->GetName());
      }
                    );
   }

   virtual void AcceptAllIDCodes()
   {
      // Calling this method disables any selection of "OK" reconstructed particles according
      // to their identification code status, for all arrays in the setup.

      for_each_array([](KVMultiDetArray * mda) {
         mda->AcceptAllIDCodes();
      }
                    );
   }
   virtual void AcceptAllECodes()
   {
      // Calling this method disables any selection of "OK" reconstructed particles according
      // to their calibration code status, for all arrays in the setup.

      for_each_array([](KVMultiDetArray * mda) {
         mda->AcceptAllECodes();
      }
                    );
   }

   virtual void InitializeIDTelescopes()
   {
      // Override base method in order to set general identification codes for telescopes in each array

      for_each_array([](KVMultiDetArray * mda) {
         mda->InitializeIDTelescopes();
      }
                    );
   }

   void InitialiseRawDataReading(KVRawDataReader* R)
   {
      // Calls InitialiseRawDataReading() for each array of the setup in turn
      for_each_array([ = ](KVMultiDetArray * mda) {
         mda->InitialiseRawDataReading(R);
      }
                    );
   }
   Bool_t HandleRawDataEvent(KVRawDataReader*);

   virtual void SetSimMode(Bool_t on = kTRUE)
   {
      // Set simulation mode of each array

      for_each_array([ = ](KVMultiDetArray * mda) {
         mda->SetSimMode(on);
      }
                    );
   }
   virtual void SetTarget(KVTarget* target)
   {
      // Set target both for the overall combined array and each individual array

      KVMultiDetArray::SetTarget(target);
      for_each_array([ = ](KVMultiDetArray * mda) {
         mda->SetTarget(target);
      }
                    );
   }

   ClassDef(KVExpSetUp, 1) //Describe an experimental set-up made of several KVMultiDetArray objects
};

#endif
