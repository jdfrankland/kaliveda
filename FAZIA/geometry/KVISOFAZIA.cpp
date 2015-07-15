//Created by KVClassFactory on Thu Jan 22 16:02:02 2015
//Author: ,,,

#include "KVISOFAZIA.h"

#include "KVUnits.h"
#include "KVFAZIABlock.h"

ClassImp(KVISOFAZIA)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVISOFAZIA</h2>
<h4>FAZIA set-up for ISOFAZIA experiment June 2015 - 4 blocks</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVISOFAZIA::KVISOFAZIA()
{
   // Default constructor

}

void KVISOFAZIA::GetGeometryParameters()
{
	
   fNblocks = 4;
	fFGeoType = "";
   fFThetaMin = 2.3;
   fFDist = 80.0;

}


KVISOFAZIA::~KVISOFAZIA()
{
   // Destructor
}

void KVISOFAZIA::BuildFAZIA()
{
   Info("BuildFAZIA", "Compact geometry, %f cm from target, theta-min=%f deg.",
        fFDist, fFThetaMin);

   TGeoVolume* top = gGeoManager->GetTopVolume();

   Double_t distance_block_cible = fFDist * KVUnits::cm;
   Double_t thick_si1 = 300 * KVUnits::um;
   TGeoTranslation trans;
   trans.SetDz(distance_block_cible + thick_si1 / 2.);

   KVFAZIABlock* block = new KVFAZIABlock;
	
   Int_t block_starting_number=fStartingBlockNumber;
   
   TGeoRotation rot1, rot2;
   TGeoHMatrix h;
   TGeoHMatrix* ph = 0;
   Double_t theta = 0;
   Double_t phi = 0;

   Double_t theta_min = fFThetaMin;//smallest lab polar angle in degrees
   Double_t centre_hole = 2.*tan(theta_min * TMath::DegToRad()) * distance_block_cible;
   Double_t dx = (block->GetTotalSideWithBlindage()) / 2.;
	
   printf("centre_hole=%lf - dx=%lf\n",centre_hole,dx);
   
   Double_t arc = block->GetTotalSideWithBlindage()/(distance_block_cible + thick_si1 / 2.);
   arc/=2;
   arc*=TMath::RadToDeg();
   
   TVector3 centre;
   for (Int_t bb=0;bb<fNblocks;bb+=1)
   {
   	/*
      if (bb==0) 			centre.SetXYZ(dx - centre_hole / 2, 0, distance_block_cible);
   	else if (bb==1)	centre.SetXYZ(dx + centre_hole / 2, 0, distance_block_cible);
   	else {
      	Warning("BuildFAZIA","Block position definition is done only for %d blocks",fNblocks);
      }
      */
      /*
      theta = centre.Theta() * TMath::RadToDeg();
   	phi = centre.Phi() * TMath::RadToDeg();
   	*/
      //theta=theta_min+arc;
      /*
      if (bb==4) { phi=90;  theta = 21.4+arc; }
      else 
      */
      if (bb==3) { phi=90;theta = 11.8+arc; }
      else if (bb==2) { phi=90;theta = 2.3+arc; }
      else if (bb==1) { phi=360-90;theta = 2.3+arc; }
      else if (bb==0) { phi=360-90;theta = 11.8+arc; }
      
      rot2.SetAngles(phi + 90., theta, 0.);
   	rot1.SetAngles(-1.*phi, 0., 0.);
   	h = rot2 * trans * rot1;
   	ph = new TGeoHMatrix(h);
   	top->AddNode(block, block_starting_number++, ph);
   
   }
   
   gGeoManager->CloseGeometry();
   gGeoManager->DefaultColors();

}
