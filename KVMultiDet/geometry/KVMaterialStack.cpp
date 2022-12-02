#include "KVMaterialStack.h"

ClassImp(KVMaterialStack)

KVMaterialStack::KVMaterialStack(const KVDetector* D, double incidence)
   : KVBase(),
     fDetector(D),
     fELossF("ELossActive", this, &KVMaterialStack::ELossActive, 0., 1.e+04, 2, "KVMaterialStack", "ELossActive", TF1::EAddToList::kNo),
     fEResF("ERes", this, &KVMaterialStack::EResDet, 0., 1.e+04, 2, "KVMaterialStack", "EResDet", TF1::EAddToList::kNo),
     fmaxELossF("maxELoss", this, &KVMaterialStack::MaxELossActive, 0., 90., 2, "KVMaterialStack", "MaxELossActive", TF1::EAddToList::kNo)
{
   // Copy all materials to stack, adjust thicknesses according to incidence angle

   fELossF.SetNpx(500);
   fEResF.SetNpx(500);
   fmaxELossF.SetNpx(500);

   TIter next(D->GetListOfAbsorbers());
   KVMaterial* mat;
   auto active = D->GetActiveLayer();
   int layer = 0;
   while ((mat = (KVMaterial*)next())) {
      if (mat == active) fActiveLayer = layer;
      fAbsorbers.push_back(*mat);
      fLayThick.push_back(mat->GetThickness());
      ++layer;
   }
}

TF1& KVMaterialStack::GetELossFunction(Int_t Z, Int_t A)
{
   fELossF.SetParameters((Double_t)Z, (Double_t)A);
   fELossF.SetRange(0., fDetector->GetSmallestEmaxValid(Z, A));
   fELossF.SetTitle(Form("Energy loss [MeV] in detector %s for Z=%d A=%d", fDetector->GetName(), Z, A));
   return fELossF;
}

TF1& KVMaterialStack::GetEResFunction(Int_t Z, Int_t A)
{
   fEResF.SetParameters((Double_t)Z, (Double_t)A);
   fEResF.SetRange(0., fDetector->GetSmallestEmaxValid(Z, A));
   fEResF.SetTitle(Form("Residual energy [MeV] after detector %s for Z=%d A=%d", fDetector->GetName(), Z, A));
   return fEResF;
}

TF1& KVMaterialStack::GetMaxELossFunction(Int_t Z, Int_t A)
{
   fmaxELossF.SetParameters((Double_t)Z, (Double_t)A);
   return fmaxELossF;
}

Double_t KVMaterialStack::ELossActive(Double_t* x, Double_t* par)
{
   // Calculates energy loss (in MeV) in active layer of stack,
   // taking into account preceding layers
   //
   // Arguments are:
   //    x[0] is incident energy in MeV
   // Parameters are:
   //   par[0]   Z of ion
   //   par[1]   A of ion

   Double_t e = x[0];
   int layer = 0;
   if (fActiveLayer > 0) {
      for (auto& mat : fAbsorbers) {
         e = mat.GetERes(par[0], par[1], e, fLayThick[layer] / fIncAngleCosine);
         if (e <= 0.)
            return 0.; // return 0 if particle stops in layers before active layer
         ++layer;
         if (layer == fActiveLayer) break;
      }
   }
   //calculate energy loss in active layer
   return fAbsorbers[fActiveLayer].GetDeltaE(par[0], par[1], e, fLayThick[layer] / fIncAngleCosine);
}

Double_t KVMaterialStack::MaxELossActive(Double_t* x, Double_t* par)
{
   // Calculates maximum energy loss (in MeV) in active layer of stack,
   // taking into account preceding layers
   //
   // Arguments are:
   //    x[0] is incident angle of particle measured wrt normal to entrance window
   // Parameters are:
   //   par[0]   Z of ion
   //   par[1]   A of ion

   Double_t psi = x[0];
   auto psi_sav = fIncAngle;
   SetIncidenceAngle(psi);
   auto demax = GetELossFunction(par[0], par[1]).GetMaximum();
   SetIncidenceAngle(psi_sav);
   return demax;
}

Double_t KVMaterialStack::EResDet(Double_t* x, Double_t* par)
{
   // Calculates residual energy (in MeV) of particle after traversing all layers of detector.
   // Returned value is -1000 if particle stops in one of the layers of the detector.
   //
   // Arguments are:
   //    x[0] is incident energy in MeV
   // Parameters are:
   //   par[0]   Z of ion
   //   par[1]   A of ion

   Double_t e = x[0];
   int layer = 0;
   for (auto& mat : fAbsorbers) {
      Double_t eres = mat.GetERes(par[0], par[1], e, fLayThick[layer] / fIncAngleCosine);   //residual energy after layer
      if (eres <= 0.)
         return -1000.;          // return -1000 if particle stops in layers before active layer
      e = eres;
      ++layer;
   }
   return e;
}

Double_t KVMaterialStack::GetIncidentEnergyFromERes(Int_t Z, Int_t A, Double_t Eres)
{
   // Overrides KVMaterial::GetIncidentEnergyFromERes
   //
   // Calculate incident energy of nucleus from residual energy.
   //
   // Returns -1 if Eres is out of defined range of values

   if (Z < 1 || Eres <= 0.) return 0.;
   Int_t status;
   Double_t einc = KVBase::ProtectedGetX(&GetEResFunction(Z, A), Eres, status);
   if (status != 0) {
      // problem with inversion - value out of defined range of function
      return -1;
   }
   return einc;
}

Double_t KVMaterialStack::GetIncidentEnergy(Int_t Z, Int_t A, Double_t delta_e, KVMaterial::SolType type)
{
   // Overrides KVMaterial::GetIncidentEnergy
   // Returns incident energy corresponding to energy loss delta_e in active layer for a given nucleus.
   //
   // By default the solution corresponding to the highest incident energy is returned
   // This is the solution found for Einc greater than the maximum of the dE(Einc) curve.
   // If you want the low energy solution set SolType = KVIonRangeTable::kEmin.
   //
   // WARNING: calculating the incident energy of a particle using only the dE in a detector
   // is ambiguous, as in general (and especially for very heavy ions) the maximum of the dE
   // curve occurs for Einc greater than the punch-through energy, therefore it is not always
   // true to assume that if the particle does not stop in the detector the required solution
   // is that for type=KVIonRangeTable::kEmax. For a range of energies between punch-through
   // and dE_max, the required solution is still that for type=KVIonRangeTable::kEmin.
   // If the residual energy of the particle is unknown, there is no way to know which is the
   // correct solution.
   //
   // WARNING 2
   // If the given energy loss in the active layer is greater than the maximum theoretical dE
   // for given Z & A, (dE > GetMaxDeltaE(Z,A)) then we return a NEGATIVE incident energy
   // corresponding to the maximum, GetEIncOfMaxDeltaE(Z,A)

   if (Z < 1) return 0.;

   Double_t DE = delta_e;

   // If the given energy loss in the active layer is greater than the maximum theoretical dE
   // for given Z & A, (dE > GetMaxDeltaE(Z,A)) then we return a NEGATIVE incident energy
   // corresponding to the maximum, GetEIncOfMaxDeltaE(Z,A)
   if (DE > GetMaxDeltaE(Z, A)) {
      return -GetEIncOfMaxDeltaE(Z, A);
   }

   TF1* dE = &GetELossFunction(Z, A);
   Double_t e1, e2;
   dE->GetRange(e1, e2);
   switch (type) {
      case KVMaterial::SolType::kEmin:
         e2 = GetEIncOfMaxDeltaE(Z, A);
         break;
      case KVMaterial::SolType::kEmax:
         e1 = GetEIncOfMaxDeltaE(Z, A);
         break;
   }
   int status;
   Double_t EINC = ProtectedGetX(dE, DE, status, e1, e2);
   if (status != 0) {
      return -EINC;
   }
   return EINC;
}

Double_t KVMaterialStack::GetERes(Int_t Z, Int_t A, Double_t Einc)
{
   // Overrides KVMaterial::GetERes
   // Returns residual energy of given nucleus after the detector.
   // Returns 0 if Einc<=0

   if (Einc <= 0.) return 0.;
   Double_t eres = GetEResFunction(Z, A).Eval(Einc);
   // Eres function returns -1000 when particle stops in detector,
   // in order for function inversion (GetEIncFromEres) to work
   if (eres < 0.) eres = 0.;
   return eres;
}

