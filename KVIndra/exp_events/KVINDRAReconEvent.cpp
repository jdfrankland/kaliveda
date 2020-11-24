/***************************************************************************
                          kvindrareconevent.cpp  -  description
                             -------------------
    begin                : Thu Oct 10 2002
    copyright            : (C) 2002 by J.D. Frankland
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

#include "Riostream.h"
#include "TROOT.h"
#include "KVINDRAReconEvent.h"
#include "KVList.h"
#include "KVGroup.h"
#include "KVTelescope.h"
#include "KVDetector.h"
#include "KVCsI.h"
#include "KVSilicon.h"
#include "KVDetectorEvent.h"
#include "KVINDRAReconNuc.h"
#include "KVINDRA.h"
#include "TStreamerInfo.h"
#include "KVIDCsI.h"
#include "KVIDGCsI.h"
#include "KVDataSet.h"
#include "KVChIo.h"

using namespace std;

ClassImp(KVINDRAReconEvent);

void KVINDRAReconEvent::init()
{
   //default initialisations
   fCodeMask = 0;
   fHitGroups = 0;
}

KVINDRAReconEvent::KVINDRAReconEvent(Int_t mult)
   : KVReconstructedEvent(mult)
{
   init();
   // KLUDGE: we have to destroy the TClonesArray in order to change
   // the class used for particles from KVReconstructedNucleus to
   // KVINDRAReconNuc
   delete fParticles;
   fParticles =  new TClonesArray(KVINDRAReconNuc::Class(), mult);
   CustomStreamer();
}

KVINDRAReconEvent::~KVINDRAReconEvent()
{
   if (fCodeMask) {
      delete fCodeMask;
      fCodeMask = 0;
   }
   SafeDelete(fHitGroups);
};


/////////////////////////////////////////////////////////////////////////////
void KVINDRAReconEvent::Streamer(TBuffer& R__b)
{
   // Stream an object of class KVINDRAReconEvent.
   // If acceptable ID/E codes have been set with methods AcceptIDCodes()/
   // AcceptECodes(), we loop over the newly-read particles in order to set
   // their IsOK() status according to these choices.
   // If not, it will have already been set according to the default code
   // selection (defined by [DataSet].INDRA.ReconstructedNuclei.AcceptID/ECodes)
   // in KVReconstructedNucleus::Streamer

   if (R__b.IsReading()) {
      R__b.ReadClassBuffer(KVINDRAReconEvent::Class(), this);
      if (fCodeMask && !fCodeMask->IsNull()) {
         KVINDRAReconNuc* par;
         while ((par = (KVINDRAReconNuc*)GetNextParticle())) {
            par->SetIsOK(CheckCodes(par->GetCodes()));
         }
      }
   }
   else {
      R__b.WriteClassBuffer(KVINDRAReconEvent::Class(), this);
   }
}

//______________________________________________________________________________
void KVINDRAReconEvent::Print(Option_t* option) const
{
   //Print out list of particles in the event.
   //If option="ok" only particles with IsOK=kTRUE are included.

   cout << GetTitle() << endl;  //system
   cout << GetName() << endl;   //run
   cout << "Event number: " << GetNumber() << endl << endl;
   cout << "MULTIPLICITY = " << ((KVINDRAReconEvent*) this)->
        GetMult(option) << endl << endl;

   KVINDRAReconNuc* frag = 0;
   while ((frag = (KVINDRAReconNuc*)GetNextParticle(option))) {
      frag->Print();
   }
}

//______________________________________________________________________________________________

void KVINDRAReconEvent::ChangeFragmentMasses(UChar_t mass_formula)
{
   //Changes the mass formula used to calculate A from Z for all nuclei in event
   //For the values of mass_formula, see KVNucleus::GetAFromZ
   //
   //The fragment energy is modified in proportion to its mass, this is due to the
   //contribution from the CsI light-energy calibration:
   //
   //    E -> E + E_CsI*( newA/oldA - 1 )
   //
   //From an original lunch by Remi Bougault.
   //
   //Only particles with 'acceptable' ID & E codes stopping in (or passing through)
   //a CsI detector are affected; particles whose mass was measured
   //(i.e. having KVReconstructedNucleus::IsAMeasured()==kTRUE)
   //are not affected by the change of mass formula.

   ResetGetNextParticle();
   KVINDRAReconNuc* frag;
   while ((frag = (KVINDRAReconNuc*)GetNextParticle("ok"))) {

      if (!frag->IsAMeasured()) {

         Float_t oldA = (Float_t)frag->GetA();
         frag->SetMassFormula(mass_formula);

         if (frag->GetCsI()) {
            Float_t oldECsI = frag->GetEnergyCsI();
            Float_t oldE = frag->GetEnergy();
            Float_t newECsI = oldECsI * ((Float_t)frag->GetA() / oldA);
            frag->GetCsI()->SetEnergy(newECsI);
            frag->SetEnergy(oldE - oldECsI + newECsI);
         }
      }
   }

}

//____________________________________________________________________________












