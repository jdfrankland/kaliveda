//Created by KVClassFactory on Tue Jan 27 11:37:39 2015
//Author: ,,,

#ifndef __KVFAZIA_H
#define __KVFAZIA_H

#include "KVMultiDetArray.h"

#include <KVGeoImport.h>
#include <KVEnv.h>
#include <KVSignal.h>
#include "KVFAZIATrigger.h"

#if ROOT_VERSION_CODE <= ROOT_VERSION(5,32,0)
#include "TGeoMatrix.h"
#endif

class KVDetectorEvent;
#ifdef WITH_PROTOBUF
#ifndef __CINT__
namespace DAQ {
   class FzEvent;
}
#endif
#endif

/**
  \class KVFAZIA
  \ingroup FAZIAGeo
  \brief Description of a FAZIA detector geometry
 */
class KVFAZIA : public KVMultiDetArray {
protected:
   TString fFGeoType;  //type of FAZIA geometry (="compact",...)
   Double_t fFDist;    //distance of FAZIA detectors from target (in cm)
   Double_t fFThetaMin;//minimum polar angle for compact geometry (in degrees)
   Int_t fNblocks;   //number of blocks
   Int_t fStartingBlockNumber;   //starting number of block incrementation
   //Bool_t fBuildTarget; //kTRUE to include target frame in the geometry
   TString fCorrespondanceFile; //name of the file where are listed links between geometry and detector names
   KVString fDetectorLabels;
   KVString fSignalTypes;
   Double_t fImport_dTheta;//! for geometry import
   Double_t fImport_dPhi;//! for geometry import
   Double_t fImport_ThetaMin;//! for geometry import
   Double_t fImport_ThetaMax;//! for geometry import
   Double_t fImport_PhiMin;//! for geometry import
   Double_t fImport_PhiMax;//! for geometry import
   Double_t fImport_Xorg;//! for geometry import
   Double_t fImport_Yorg;//! for geometry import
   Double_t fImport_Zorg;//! for geometry import
   int fQuartet[8][2];//! quartet number from #FEE and #FPGA
   int fTelescope[8][2];//! telescope number from #FEE and #FPGA

   // values of trapezoidal filter rise time set in the fpgas defined in .kvrootrc
   Double_t fQH1risetime;
   Double_t fQ2risetime;
   Double_t fQ3slowrisetime;
   Double_t fQ3fastrisetime;

   // trigger pattern read from data for each event
   KVFAZIATrigger fTrigger;

   void SetTriggerPatternsForDataSet(const TString& dataset);
   void SetTriggerPattern(uint16_t fp)
   {
      fTrigger.SetTriggerPattern(fp);
   }

   //methods to be implemented in child classes
   virtual void BuildFAZIA();
   virtual void GetGeometryParameters();
   //

   virtual void BuildTarget();
   void GenerateCorrespondanceFile();
   virtual void SetNameOfDetectors(KVEnv& env);

   virtual void DefineStructureFormats(KVGeoImport&) {}

#ifdef WITH_MFM
   Bool_t handle_raw_data_event_mfmframe(const MFMCommonFrame&);
#endif
   void copy_fired_parameters_to_recon_param_list()
   {
      // override KVMultiDetArray method.
      // raw data signals are not copied to the general (event) parameter list,
      // they are stored in the lists of the individual particles and reset in the
      // detectors when the particles are read back.
      //
      // the formatting of the signal names is not compatible with the code in KVReconstructedEvent::Streamer,
      // hence the FAZIA-specific treatment which is handled by KVFAZIA::FillDetectorList.
   }

   void PerformClosedROOTGeometryOperations();

   void CreateCorrespondence();
#ifdef WITH_PROTOBUF
   Bool_t handle_raw_data_event_protobuf(KVProtobufDataReader&);
#ifndef __CINT__
   Bool_t treat_event(const DAQ::FzEvent&);
#endif
   Double_t TreatEnergy(Int_t sigid, Int_t eid, UInt_t val);
#endif
   TString GetSignalName(Int_t bb, Int_t qq, Int_t tt, Int_t idsig);

   void ReadTriggerPatterns(KVExpDB* db);

   void prepare_to_handle_new_raw_data()
   {
      // Override base method to reset trigger pattern before reading new raw event
      KVMultiDetArray::prepare_to_handle_new_raw_data();
      SetTriggerPattern(0);
   }
   void SetIDCodeForIDTelescope(KVIDTelescope*) const;

   // The following methods are used by the current implementation of the filter.
   // They should be removed in future implementations.
   virtual UChar_t GetNormalCalibrationCode()
   {
      // return a general calibration code for correctly calibrated particles
      return ECodes::NORMAL_CALIBRATION;
   }
public:
   enum IDCodes {
      /** \enum KVFAZIA::IDCodes
          \brief Identification quality codes attributed to particles reconstructed from data
       */
      NO_IDENTIFICATION = -1,    ///< no identification possible for particle
      ID_GAMMA = 0,    ///< 'gamma' particle identified by pulse shape analysis in CSI
      ID_SI1_PSA = 11, ///< particle identified by pulse shape analysis in SI1
      ID_SI1_SI2 = 12, ///< particle identified in SI1-SI2 telescope
      ID_SI1_CSI = 13, ///< particle identified in SI1-SI2 telescope
      ID_SI2_PSA = 22, ///< particle identified by pulse shape analysis in SI1
      ID_SI2_CSI = 23, ///< particle identified in SI2-CSI telescope
      ID_SI12_CSI = 123, ///< particle identified in (SI1+SI2)-CSI telescope
      ID_CSI_PSA = 33, ///< particle identified by pulse shape analysis in CSI
      ID_STOPPED_IN_FIRST_STAGE = 5, ///< particle stopped in SI1, no identification possible better than estimation of minimum Z
      ID_SI1_SI2_MAYBE_PUNCH_THROUGH = 120, ///< possible ambiguity of particle identification in SI1-SI2 due to unvetoed punch-through
      ID_SI1_SI2_PUNCH_THROUGH = 121, ///< particle punching through SI2, identified Z is a minimum value
      ID_SI1_PUNCH_THROUGH = 110, ///< particle partially identified by pulse shape analysis in SI1, although it is punching through (no SI2 signal or SI1-SI2 id)
   };
   TString GetIDCodeMeaning(Int_t idc) const
   {
      switch (idc) {
         case NO_IDENTIFICATION:
            return "NO_IDENTIFICATION";
         case ID_GAMMA:
            return "ID_GAMMA";
         case ID_SI1_PSA:
            return "ID_SI1_PSA";
         case ID_SI2_PSA:
            return "ID_SI2_PSA";
         case ID_SI1_SI2:
            return "ID_SI1_SI2";
         case ID_SI2_CSI:
            return "ID_SI2_CSI";
         case ID_SI1_CSI:
            return "ID_SI1_CSI";
         case ID_SI12_CSI:
            return "ID_SI12_CSI";
         case ID_CSI_PSA:
            return "ID_CSI_PSA";
         case ID_STOPPED_IN_FIRST_STAGE:
            return "ID_STOPPED_IN_FIRST_STAGE";
         case ID_SI1_SI2_MAYBE_PUNCH_THROUGH:
            return "ID_SI1_SI2_MAYBE_PUNCH_THROUGH";
         case ID_SI1_SI2_PUNCH_THROUGH:
            return "ID_SI1_SI2_PUNCH_THROUGH";
         case ID_SI1_PUNCH_THROUGH:
            return "ID_SI1_PUNCH_THROUGH";
         default:
            return Form("(unknown:%d)", idc);
      }
   }
   enum ECodes {
      /** \enum KVFAZIA::ECodes
          \brief Calibration quality codes attributed to particles reconstructed from data
       */
      NO_CALIBRATION_ATTEMPTED = 0, ///< particle stopped in detectors with no available calibration
      NORMAL_CALIBRATION = 1,       ///< normal well-calibrated particle with no problems
      SOME_ENERGY_LOSSES_CALCULATED = 2, ///< particle calibration OK, with some detector energies calculated
      WARNING_CSI_MAX_ENERGY = 3,    ///< particle calibration OK, although apparent energy would mean punching through the CsI
      ENERGY_LOSSES_TENTATIVELY_CALCULATED = 4    ///< particle calibration OK, with some detector energies tentatively calculated (Z<2)
   };
   TString GetECodeMeaning(Int_t ec) const
   {
      switch (ec) {
         case NO_CALIBRATION_ATTEMPTED:
            return "NO_CALIBRATION_ATTEMPTED";
         case NORMAL_CALIBRATION:
            return "NORMAL_CALIBRATION";
         case SOME_ENERGY_LOSSES_CALCULATED:
            return "SOME_ENERGY_LOSSES_CALCULATED";
         case WARNING_CSI_MAX_ENERGY:
            return "WARNING_CSI_MAX_ENERGY";
         case ENERGY_LOSSES_TENTATIVELY_CALCULATED:
            return "ENERGY_LOSSES_TENTATIVELY_CALCULATED (for Z=1,2 stopped in uncalibrated CsI)";
         default:
            return Form("(unknown:%d)", ec);
      }
   }
   virtual Int_t GetIDCodeForParticlesStoppingInFirstStageOfTelescopes() const
   {
      return IDCodes::ID_STOPPED_IN_FIRST_STAGE;
   }

   KVFAZIA(const Char_t* title = "");
   virtual ~KVFAZIA();
   void AddDetectorLabel(const Char_t* label);

   virtual void Build(Int_t = -1);

   void GetDetectorEvent(KVDetectorEvent* detev, const TSeqCollection* dets);
   Int_t GetNumberOfBlocks() const
   {
      return fNblocks;
   }
   void IncludeTargetInGeometry(Bool_t include = kTRUE)
   {
      fBuildTarget = include;
   }

   KVString GetDetectorLabels() const
   {
      return fDetectorLabels;
   }
   const Char_t* GetSignalTypes() const
   {
      return fSignalTypes.Data();
   }

   void SetGeometryImportParameters(Double_t dt = 0.25, Double_t dp = 1.0, Double_t tmin = 2., Double_t pmin = 0, Double_t tmax = 20., Double_t pmax = 360.,
                                    Double_t xorg = 0, Double_t yorg = 0, Double_t zorg = 0)
   {
      // Set angular arguments for call to KVGeoImport::ImportGeometry in KVFAZIA::Build
      // Also set origin for geometry import to (xorg,yorg,zorg) [default: (0,0,0)]
      fImport_dPhi = dp;
      fImport_dTheta = dt;
      fImport_PhiMax = pmax;
      fImport_PhiMin = pmin;
      fImport_ThetaMax = tmax;
      fImport_ThetaMin = tmin;
      fImport_Xorg = xorg;
      fImport_Yorg = yorg;
      fImport_Zorg = zorg;
   }
   void FillDetectorList(KVReconstructedNucleus* rnuc, KVHashList* DetList, const KVString& DetNames);

   KVGroupReconstructor* GetReconstructorForGroup(const KVGroup*) const;
   Double_t GetSetupParameter(const Char_t* parname);

   const KVFAZIATrigger& GetTrigger() const
   {
      return fTrigger;
   }
   virtual void SetRawDataFromReconEvent(KVNameValueList&);
   virtual void MakeCalibrationTables(KVExpDB*);

   std::string GetTriggerForCurrentRun() const;
   void DeduceIdentificationTelescopesFromGeometry();

   ClassDef(KVFAZIA, 1) //Base class for description of the FAZIA set up
};

//................  global variable
R__EXTERN KVFAZIA* gFazia;

#endif
