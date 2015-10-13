//Created by KVClassFactory on Mon Jul  8 15:40:35 2013
//Author: Guilain ADEMARD

#include "KVVAMOSReconGeoNavigator.h"
#include "KVVAMOSReconNuc.h"
#include "KVVAMOSDetector.h"
#include "KVIonRangeTableMaterial.h"

ClassImp(KVVAMOSReconGeoNavigator)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVVAMOSReconGeoNavigator</h2>
<h4>Propagate nuclei through the geometry of VAMOS for their reconstruction/calibrate/identification</h4>
<!-- */
// --> END_HTML
// Given a valid ROOT geometry, we propagate the nuclei in the VAMOS focal-plane
// detection area and, every time a nucleus traverses a volume made of a TGeoMaterial
// with a name corresponding to a material known by this range table,
// we calculate the step through the material (STEP) of the nucleus,
// and store it in the nucleus's list KVParticle::fParameters in the form
//
//   "STEP:[detector name]" = [step in material in cm]
//
// We also store the distance (DPATH in cm) between the intersection point at the focal plane
// and the point at the entrance of the volume if it is the first active volume of
// a detector
//
//   "DPATH:[detector name]" = [distance FP point to active volume entrance point]
//
// DPATH has the sign + if the volume is behind the focal plane or - is it
// is at the front of it.
//
// In the case of multilayer detectors, "[detector name]" in the previous
// examples is replaced by "[detector name]/[layer name]"
//
// This class is used for the reconstruction in KVVAMOSReconNuc.
// The propagation of the nucleus is first done from the focal-plane point
// toward the trajectory (KVVAMOSReconNuc::fRT.dirFP direction) and then
// from the same point but in the inverse direction.
//
// DPATH is used to calculate the correct flight distance of the nucleus.
// For example if the time TSED1_HF is used for a time-of-flight
// measurement then the real flight distance is the reconstructed path, from
// the target point to the focal plane, plus the value of DPATH:SED1/ACTIVE_SED2_Myl
//
//
// EXAMPLE
// =======
//
// In this case the VAMOS geometry is made up of 2 SeDs, 1 HARPEE ionization chamber and 1 silicon detector.
// The ionization chamber has 4 layers (window + gas + gas + gas), with an "ACTIVE" gas layer
//
// For a well reconstructed nucleus in VAMOS, this navigator is used and then
// you can access to the result of the propagation with:
//
// nuc->GetParameters()->Print()
//
// KVNameValueList::ParticleParameters : Parameters associated with a particle in an event (0xa163800)
//  <STEP:SED1/ACTIVE_SED1_Myl=0.000126196>
//  <DPATH:SED1/ACTIVE_SED1_Myl=-71.6663>
//  <STEP:SED2/ACTIVE_SED2_Myl=0.000126196>
//  <DPATH:SED2/ACTIVE_SED2_Myl=16.0801>
//  <STEP:CHI/CHI_Myl=0.000150153>
//  <STEP:CHI/CHI_C4H10=2.07011>
//  <STEP:CHI/ACTIVE_CHI_C4H10=10.4677>
//  <DPATH:CHI/ACTIVE_CHI_C4H10=56.8245>
//  <STEP:CHI/CHI_C4H10_1=1.27931>
//  <STEP:SI_17=0.0500511>
//  <DPATH:SI_17=68.575>
//
// here 'nuc' is a KVVAMOSReconNuc object.
////////////////////////////////////////////////////////////////////////////////

//________________________________________________________________

KVVAMOSReconGeoNavigator::KVVAMOSReconGeoNavigator(TGeoManager* g, KVIonRangeTable* r) : KVGeoNavigator(g)
{
   fRangeTable = r;
   fForward    = kTRUE;
}
//________________________________________________________________

KVVAMOSReconGeoNavigator::~KVVAMOSReconGeoNavigator()
{
   // Destructor
}
//________________________________________________________________

void KVVAMOSReconGeoNavigator::ParticleEntersNewVolume(KVNucleus* nuc)
{
   // Overrides method in KVGeoNavigator base class.
   // Every time a particle enters a new volume, we check the material to see
   // if it is known (i.e. contained in the range table fRangeTable).
   // If so, then we calculate the step through the material (STEP) of the nucleus
   // and the distance (DPATH in cm) between the intersection point at the focal plane
   // and the point at the entrance of the volume if it is the first active volume of a detector.
   // DPATH has the sign + if the volume is behind the focal plane or - if it
   // is at the front of it.
   //

   KVVAMOSReconNuc* rnuc = (KVVAMOSReconNuc*)nuc;

   TGeoVolume* stopVol = (TGeoVolume*)((KVVAMOSDetector*)rnuc->GetStoppingDetector())->GetActiveVolumes()->Last();

   // stop the propagation if the current volume is the stopping detector
   // of the nucleus but after the process of this volume
   if (GetCurrentVolume() == stopVol) SetStopPropagation();

   TGeoMaterial* material = GetCurrentVolume()->GetMaterial();
   KVIonRangeTableMaterial* irmat = 0;

   // skip the process if the current material is unkown
   if ((irmat = fRangeTable->GetMaterial(material))) {

      KVString dname;
      Bool_t multi;
      TString absorber_name;
      Bool_t is_active = kFALSE;
      if (GetCurrentDetectorNameAndVolume(dname, multi)) {
         is_active = kTRUE;
         if (multi) {
            absorber_name.Form("%s/%s", dname.Data(), GetCurrentNode()->GetName());
            is_active = absorber_name.Contains("ACTIVE_");
         } else absorber_name = dname;
      } else
         absorber_name = irmat->GetName();

      // Coordinates of the vector between the intersection point at the
      // focal plane and the point at the entrance of the current detector
      Double_t X = GetEntryPoint().X() - fOrigine.X();
      Double_t Y = GetEntryPoint().Y() - fOrigine.Y();
      Double_t Z = GetEntryPoint().Z() - fOrigine.Z();

      // Norm of this vector. The signe gives an infomation about the detectir position
      // (1: behind; -1: in front of) with respect to the focal plane.
      Double_t Delta = (fForward ? 1. : -1.);
      Delta *= TMath::Sqrt(X * X + Y * Y + Z * Z);

      if (fForward) {
         nuc->SetParameter(Form("STEP:%s", absorber_name.Data()), GetStepSize());
         if (is_active)   nuc->SetParameter(Form("DPATH:%s", dname.Data()), Delta);
      } else {
         if (is_active) nuc->GetParameters()->SetFirstValue(Form("DPATH:%s", dname.Data()), Delta);
         nuc->GetParameters()->SetFirstValue(Form("STEP:%s", absorber_name.Data()), GetStepSize());
      }
   }
}
//________________________________________________________________

void KVVAMOSReconGeoNavigator::PropagateNucleus(KVVAMOSReconNuc* nuc)
{
   // Propagates the VAMOS reconstructed nucleus along its focal plane
   // direction in the focal-plane detection area. It is possible only if
   // this direction is well reconstructed i.e. the FPCode of the nucleus
   // is different from kFPCode0.

   if (nuc->GetCodes().TestFPCode(kFPCode0)) return;

   // reminder of the momentum
   TVector3 p(nuc->GetMomentum());

   // For gGeoManager the origin is the target point.
   // Starting point has to be set from this origin.

   // intersection point between reconstructed trajectory and the Focal
   // Plane in the focal plane frame of reference
   Double_t XYZ[3] = { nuc->GetXf(), nuc->GetYf(), 0. };
   // same intersection point in the frame of reference
   // centered at the target point
   gVamos->FocalToTarget(XYZ, XYZ);
   fOrigine.SetXYZ(XYZ[0], XYZ[1], XYZ[2]);

   //  propagation direction = direction of the trajectory at the focal plane
   nuc->GetFocalPlaneDirection().GetXYZ(XYZ);
   gVamos->FocalToTargetVect(XYZ, XYZ);
   TVector3 direction(XYZ);

   // Forward propagation
   fForward  = kTRUE;
   nuc->SetMomentum(direction);
   PropagateParticle(nuc, &fOrigine);

   // Backward propagation
   fForward  = kFALSE;
   direction = -direction;
   nuc->SetMomentum(direction);
   PropagateParticle(nuc, &fOrigine);

   //set the initial momentum to the nucleus
   nuc->SetMomentum(p);
}
