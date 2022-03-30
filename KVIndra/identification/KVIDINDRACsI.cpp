/***************************************************************************
                          KVIDINDRACsI.cpp  -  description
                             -------------------
    begin                : Fri Feb 20 2004
    copyright            : (C) 2004 by J.D. Frankland
    email                : frankland@ganil.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "KVIDINDRACsI.h"
#include "TMath.h"
#include "KVIdentificationResult.h"
#include <KVIDGCsI.h>
#include <KVINDRADetector.h>
#include <KVReconstructedNucleus.h>

ClassImp(KVIDINDRACsI)

KVIDINDRACsI::KVIDINDRACsI()
{
   CsIGrid = 0;

   // thresholds used by filter for Rapide-Lente identifications
   fThresMin[0][0] = 1;
   fThresMax[0][0] = 2;    // protons
   fThresMin[0][1] = 2;
   fThresMax[0][1] = 6;    // deutons
   fThresMin[0][2] = 5;
   fThresMax[0][2] = 11;   // tritons
   fThresMin[0][3] = -1;
   fThresMax[0][3] = -1;   // 4H - NO!
   fThresMin[1][0] = -1;
   fThresMax[1][0] = -1;   // 1He - NO!
   fThresMin[1][1] = -1;
   fThresMax[1][1] = -1;   // 2He - NO!
   fThresMin[1][2] = 20;
   fThresMax[1][2] = 40;   // 3He
   fThresMin[1][3] = 1;
   fThresMax[1][3] = 3;    // alphas

   /* in principle all CsI telescopes can identify mass & charge */
   SetHasMassID(kTRUE);
}

//________________________________________________________________________________________//

Bool_t KVIDINDRACsI::Identify(KVIdentificationResult* IDR, Double_t x, Double_t y)
{
   // Particle identification and code setting using identification grid
   //
   // Note that gamma identification code is no longer attributed here: in case a gamma is identified,
   // the IDR->IDcode will be the same general ID code as for all CsI particles,
   // but IDR->IDquality == KVIDGCsI::kICODE10
   //
   // The KVIdentificationResult is first Clear()ed; then it is filled with IDtype = GetType()
   // of this identification telescope, IDattempted = true, and the results of the identification
   // procedure.

   IDR->Clear();
   IDR->IDattempted = true;
   IDR->SetIDType(GetType());

   //perform identification
   Double_t X, Y;
   GetIDGridCoords(X, Y, CsIGrid, x, y);
   IDR->SetGridName(CsIGrid->GetName());
   if (CsIGrid->IsIdentifiable(X, Y, &IDR->Rejecting_Cut)) {
      CsIGrid->Identify(X, Y, IDR);
   }
   else {
      // gamma rejection
      IDR->IDquality = KVIDGCsI::kICODE10;
      IDR->Z = 0;
      IDR->A = 0;
      IDR->IDOK = kTRUE;
      IDR->SetComment("gamma");
   }

   // set general ID code
   IDR->IDcode = GetIDCode();

   return kTRUE;
}

void KVIDINDRACsI::SetIdentificationStatus(KVReconstructedNucleus* n)
{
   // For filtering simulations (only implemented for Rapide-Lente identification)
   //
   // FILTERING WITH RAPIDE-LENTE IDENTIFICATIONS
   // If n->GetEnergy() is above threshold for mass identification, we set
   // n->IsAMeasured(kTRUE) (and n->IsZMeasured(kTRUE)).
   // Otherwise, we just set n->IsZMeasured(kTRUE) and use the A given by
   // the mass formula for the particle
   //
   // individual thresholds defined for 1H, 2H, 3H, 3He, 4He
   // for A>5 identification if CsI energy > 40 MeV
   //
   // If A is not measured, we make sure the KE of the particle corresponds to the simulated one
   //
   // FILTERING WITH OTHER IDENTIFICATION
   // The nucleus is declared to be Z & A identified, whatever its identity or energy

   n->SetZMeasured();

   if (fRapideLente) {
      if (n->GetA() > 5) {
         if (GetDetector(1)->GetEnergy() > 40)
            n->SetAMeasured();
         else {
            double e = n->GetE();
            n->SetZ(n->GetZ());
            n->SetE(e);
         }
         return;
      }
      if (fThresMin[n->GetZ() - 1][n->GetA() - 1] > 0) {
         Bool_t okmass = gRandom->Uniform() < smootherstep(fThresMin[n->GetZ() - 1][n->GetA() - 1], fThresMax[n->GetZ() - 1][n->GetA() - 1], GetDetector(1)->GetEnergy());
         if (okmass) {
            n->SetAMeasured();
         }
      }
      else {
         double e = n->GetE();
         n->SetZ(n->GetZ());
         n->SetE(e);
      }
   }
   else //
      n->SetAMeasured();
}


float KVIDINDRACsI::clamp(float x, float lowerlimit, float upperlimit)
{
   if (x < lowerlimit)
      x = lowerlimit;
   if (x > upperlimit)
      x = upperlimit;
   return x;
}

float KVIDINDRACsI::smootherstep(float edge0, float edge1, float x)
{
   // Scale, and clamp x to 0..1 range
   x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
   // Evaluate polynomial
   return x * x * x * (x * (x * 6 - 15) + 10);
}
