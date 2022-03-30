/***************************************************************************
                          KVIDINDRACsI.h  -  description
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

#ifndef KVIDINDRACsI_H
#define KVIDINDRACsI_H

#include "KVINDRAIDTelescope.h"
#include "KVIDGraph.h"

/**
  \class KVIDCsI
  \ingroup INDRAIdent
  \brief Identification in CsI scintillators for INDRA
 */
class KVIDINDRACsI: public KVINDRAIDTelescope {

   KVIDGraph* CsIGrid;//! telescope's grid
   Bool_t fRapideLente;// set to true when using rapide-lente grid i.e. KVIDGCsI
   Int_t fThresMin[2][4];// min ID thresholds (smooth step)
   Int_t fThresMax[2][4];// max ID thresholds (smooth step)

protected:
   float smootherstep(float edge0, float edge1, float x);
   float clamp(float x, float lowerlimit, float upperlimit);

public:
   KVIDINDRACsI();
   void Initialize()
   {
      KVINDRAIDTelescope::Initialize();
      CsIGrid = GetIDGrid();
      fRapideLente = CsIGrid && CsIGrid->InheritsFrom("KVIDGCsI");
   }
   virtual Bool_t Identify(KVIdentificationResult*, Double_t x = -1., Double_t y = -1.);

   virtual Bool_t CanIdentify(Int_t Z, Int_t /*A*/)
   {
      // Used for filtering simulations
      // Returns kTRUE if this telescope is theoretically capable of identifying a given nucleus,
      // without considering thresholds etc.
      // For CsI detectors, identification is possible up to Z=5
      return (Z < 6);
   }
   void SetIdentificationStatus(KVReconstructedNucleus* n);

   ClassDef(KVIDINDRACsI, 3)        //INDRA identification using CsI R-L matrices
};

#endif
