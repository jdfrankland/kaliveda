#ifndef __KVMATERIALSTACK_H
#define __KVMATERIALSTACK_H

#include "KVBase.h"
#include "KVDetector.h"

/**
 \class KVMaterialStack
 \brief A stack of materials in which successive energy losses of charged particles can be calculated

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Wed Nov 30 16:04:14 2022
*/

class KVMaterialStack : public KVBase {
private:
   TF1 fELossF;
   TF1 fEResF;
   TF1 fmaxELossF;
   const KVDetector* fDetector = nullptr;
   std::vector<KVMaterial> fAbsorbers;//!
   std::vector<double> fLayThick;//! layer thicknesses in cm
   Int_t fActiveLayer = 0;//!
   Double_t   fIncAngle = 0.;//!
   Double_t   fIncAngleCosine = 1.;//!

   Double_t ELossActive(Double_t* x, Double_t* par);
   Double_t EResDet(Double_t* x, Double_t* par);
   Double_t MaxELossActive(Double_t* x, Double_t* par);

public:
   KVMaterialStack() = default;
   KVMaterialStack(const KVDetector*, double = 0.0);

   void SetIncidenceAngle(double psi)
   {
      fIncAngle = psi;
      fIncAngleCosine = TMath::Cos(TMath::DegToRad() * psi);
   }

   TF1& GetELossFunction(Int_t Z, Int_t A);
   TF1& GetEResFunction(Int_t Z, Int_t A);
   TF1& GetMaxELossFunction(Int_t Z, Int_t A);

   Double_t GetMaxDeltaE(Int_t Z, Int_t A)
   {
      return GetELossFunction(Z, A).GetMaximum();
   }
   Double_t GetEIncOfMaxDeltaE(Int_t Z, Int_t A)
   {
      // Returns incident energy corresponding to maximum energy loss in the
      // active layer of the detector, for a given nucleus.

      return GetELossFunction(Z, A).GetMaximumX();
   }

   Double_t GetMinimumIncidentAngleForDEMax(Int_t Z, Int_t A, Double_t Emax)
   {
      return 1.01 * GetMaxELossFunction(Z, A).GetX(Emax); // increase returned angle by 1% just to be sure
   }
   Double_t GetIncidentEnergyFromERes(Int_t Z, Int_t A, Double_t Eres);
   Double_t GetIncidentEnergy(Int_t Z, Int_t A, Double_t delta_e, enum KVMaterial::SolType type = KVMaterial::SolType::kEmax);
   Double_t GetERes(Int_t Z, Int_t A, Double_t Einc);

   ClassDef(KVMaterialStack, 1) //A stack of materials in which successive energy losses of charged particles can be calculated
};

#endif
