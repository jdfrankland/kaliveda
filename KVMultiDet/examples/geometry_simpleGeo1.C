//# KVGeoImport example: A simple multidetector geometry with 3 detectors
//
// To build, import into a KVMultiDetArray and view:
// kaliveda[0]: .x simpleGeo1.C+

#include "TGeoManager.h"
#include "TGeoMedium.h"
#include "TGeoVolume.h"
#include "KVMaterial.h"
#include "KVUnits.h"
#include "KVGeoImport.h"
#include "KVMultiDetArray.h"

TGeoManager* Geo1()
{
   /* must create geomanager before anything else */
   TGeoManager* myGeo = new TGeoManager("simpleGeo1", "A simple detector array");

   /* materials required for creation of geometry */
   KVMaterial silicon("Si");

   /* top volume must be big enough to contain all detectors - if in doubt, make it too big!! */
   TGeoVolume* top = myGeo->MakeBox("WORLD", silicon.GetGeoMedium("Vacuum"),
                                    50 * KVUnits::cm, 50 * KVUnits::cm, 50 * KVUnits::cm);
   myGeo->SetTopVolume(top);

   double si_dim  = 5 * KVUnits::cm; // square silicon detector 5cm x 5cm
   double si_thick = 300 * KVUnits::um; // silicon thickness 300um

   // build the detector
   TGeoVolume* si_det = myGeo->MakeBox("DET_SILICON", silicon.GetGeoMedium(), si_dim / 2., si_dim / 2., si_thick / 2.);
   si_det->SetLineColor(si_det->GetMaterial()->GetDefaultColor());

   double si_dist = 10 * KVUnits::cm; // distance from target (origin): 10cm
   // place the detectors one behind the other with a spacing of 0.25cm
   top->AddNode(si_det, 1, new TGeoTranslation(0, 0, si_dist + 0.5 * si_thick));
   top->AddNode(si_det, 2, new TGeoTranslation(0, 0, si_dist + 0.5 * si_thick + 0.25 * KVUnits::cm));
   top->AddNode(si_det, 3, new TGeoTranslation(0, 0, si_dist + 0.5 * si_thick + 0.50 * KVUnits::cm));

   myGeo->CloseGeometry();
   myGeo->DefaultColors();

   return myGeo;
}

void simpleGeo1()
{
   KVGeoImport gimp(Geo1(), KVMaterial::GetRangeTable(), new KVMultiDetArray);
   gimp.ImportGeometry();

   gMultiDetArray->Print();

   gMultiDetArray->GetGeometry()->GetTopVolume()->Draw("ogl");
}
