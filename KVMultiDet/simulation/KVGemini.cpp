//Created by KVClassFactory on Fri Jul 25 10:05:03 2014
//Author: John Frankland,,,
#include "TTree.h"

#include "KVGemini.h"
#include "KVSimEvent.h"
#include "KVSimNucleus.h"

#include "CNucleus.h"

ClassImp(KVGemini)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVGemini</h2>
<h4>Interface to gemini++</h4>
<!-- */
// --> END_HTML
// You can use this class as an interface to Gemini++ to perform statistical
// decay of excited nuclei. In case of problems with the decay (CHucleus::abortEvent==1)
// we throw a standard exception of type gemini_bad_decay. This should be caught
// in your code when using DecaySingleNucleus or DecayEvent:
//
//     KVGemini gem;
//     KVSimEvent *hot, *cold;
//     try {
//        gem.DecayEvent(hot,cold);
//     }
//     catch (gemini_bad_decay& e) {
//        // bad event, could not calculate all decays
//     }
//
////////////////////////////////////////////////////////////////////////////////

KVGemini::KVGemini() : KVBase("gemini++", "Calculate statistical decay of excited nuclei")
{
   // Default constructor
   // enhanceIMF=true: call CWeight::setWeightIMF() before any decay
   //       (enhance the probabilty of IMF emission)
   //    in this case the event weights returned by
}

KVGemini::~KVGemini()
{
   // Destructor
}

void KVGemini::DecaySingleNucleus(KVSimNucleus& toDecay, KVSimEvent* decayProducts)
{
   // Calculate decay products of excited nucleus toDecay
   // Takes into account Z, A, E*, v and angular momentum of nucleus.
   // Adds all decay products to decayProducts event:
   //     - call decayProducts->Clear() before calling this method
   //       if you just want to keep the products of a single nucleus
   //
   // If there is a problem with the decay of the nucleus,
   // we throw an exception of type gemini_bad_decay

   CNucleus CN(toDecay.GetZ(), toDecay.GetA());
   CN.setCompoundNucleus(toDecay.GetExcitEnergy(), toDecay.GetAngMom().Mag());
   // set velocity
   CN.setVelocityCartesian(toDecay.GetVelocity().X(), toDecay.GetVelocity().Y(), toDecay.GetVelocity().Z());

   // set angle of spin axis
   CAngle ang(toDecay.GetAngMom().Theta(), toDecay.GetAngMom().Phi());
   CN.setSpinAxis(ang);

   CN.decay();

   if (CN.abortEvent) { //problem with decay?
      // throw exception on gemini++ error
      CN.reset();
      throw gemini_bad_decay();
   }

   CNucleus* nuc = CN.getProducts(0);

   while (nuc) {

      KVNucleus* N = decayProducts->AddParticle();
      N->SetZ(nuc->iZ);
      N->SetA(nuc->iA);
      N->SetExcitEnergy(nuc->fEx);
      float* v = nuc->getVelocityVector();
      TVector3 vv(v);
      N->SetVelocity(vv);
      nuc = CN.getProducts();

   }

   CN.reset();//<-- essential!
}

void KVGemini::DecayEvent(const KVSimEvent* hot, KVSimEvent* cold)
{
   // Perform statistical decay of all nuclei in 'hot' event
   // Takes into account Z, A, E*, v and spin of nuclei.
   // Adds all decay products to 'cold' output event
   //
   // If there is a problem with the decay of any of the nuclei in the event,
   // we throw an exception of type gemini_bad_decay

   cold->Clear();
   KVSimNucleus* hotnuc;
   while ((hotnuc = (KVSimNucleus*)const_cast<KVSimEvent*>(hot)->GetNextParticle())) {
      try {
         DecaySingleNucleus(*hotnuc, cold);
      } catch (exception& e) {
         // rethrow any exceptions
         throw;
      }
   }
}

void KVGemini::FillTreeWithEvents(KVSimNucleus& toDecay, Int_t nDecays, TTree* theTree, TString branchname)
{
   // Perform nDecays decays of nucleus toDecay and write the events
   // containing all decay products of each decay in theTree
   // If no branchname is given, branch is called "gemini"

   if (branchname == "") branchname = "gemini";
   KVSimEvent* decayProducts = new KVSimEvent;
   KVEvent::MakeEventBranch(theTree, branchname, "KVSimEvent", &decayProducts);

   while (nDecays--) {
      decayProducts->Clear();
      try {
         DecaySingleNucleus(toDecay, decayProducts);
      } catch (exception& e) {
         continue;
      }
      theTree->Fill();
      std::cout << "\xd" << "Gemini++ processing, " << nDecays << " decays left ..." << std::flush;
   }
   std::cout << std::endl;

}

void KVGemini::FillTreeWithArrays(KVSimNucleus& toDecay, Int_t nDecays, TTree* theTree, TString mode)
{
   // Perform nDecays decays of nucleus toDecay and write the events
   // containing all decay products of each decay in theTree
   // in the form of simple arrays.
   // mode = "EThetaPhi" (default) or "V" and decides what informations are
   // in the arrays (see KVEvent::FillArraysV and KVEvent::FillArraysEThetaPhi)

   Bool_t ethetaphi = (mode == "EThetaPhi");
#define MAX_GEMINI_ARRAY_SIZE 200
   Int_t mult, Z[MAX_GEMINI_ARRAY_SIZE], A[MAX_GEMINI_ARRAY_SIZE];
   Double_t x[MAX_GEMINI_ARRAY_SIZE], y[MAX_GEMINI_ARRAY_SIZE], z[MAX_GEMINI_ARRAY_SIZE];
   KVSimEvent* decayProducts = new KVSimEvent;

   theTree->Branch("Multiplicity", &mult, "mult/I");
   theTree->Branch("Z", Z, "Z[mult]/I");
   theTree->Branch("A", A, "A[mult]/I");
   if (ethetaphi) {
      theTree->Branch("Energy", x, "Energy[mult]/D");
      theTree->Branch("Theta", y, "Theta[mult]/D");
      theTree->Branch("Phi", z, "Phi[mult]/D");
   } else {
      theTree->Branch("Vx", x, "Vx[mult]/D");
      theTree->Branch("Vy", y, "Vy[mult]/D");
      theTree->Branch("Vz", z, "Vz[mult]/D");
   }
   while (nDecays--) {
      decayProducts->Clear();
      try {
         DecaySingleNucleus(toDecay, decayProducts);
      } catch (exception& e) {
         continue;
      }
      if (ethetaphi) decayProducts->FillArraysEThetaPhi(mult, Z, A, x, y, z);
      else decayProducts->FillArraysV(mult, Z, A, x, y, z);
      theTree->Fill();
      std::cout << "\xd" << "Gemini++ processing, " << nDecays << " decays left ..." << std::flush;
   }
   std::cout << std::endl;
}

Float_t KVGemini::GetMaxSpinWithFissionBarrier(int z, int a)
{
   // Maximum angular momentum with non-zero fission barrier (Sierk model)
   // Only for 19<=Z<=111
   if (z < 19 || z > 111) {
      Error("GetMaxSpinWithFissionBarrier", "Only use for 19<=Z<=111");
      return -1;
   }
   float amin = 1.2 * z + 0.01 * pow(z, 2);
   float amax = 5.8 * z - 0.024 * pow(z, 2);

   if (a < amin || a > amax) {
      Error("GetMaxSpinWithFissionBarrier", "Only valid for Z=%d with %d<=A<=%d",
            z, (int)amin, (int)amax);
      return -1;
   }
   return CYrast::instance()->getJmaxSierk(z, a);
}

Float_t KVGemini::GetFissionBarrierRLDM(int z, int a, float J)
{
   // Return Rotating Liquid Drop Model fission barrier for given spin in hbar units
   return CYrast::instance()->getBarrierFissionRLDM(z, a, J);
}
Float_t KVGemini::GetFissionBarrierSierk(int z, int a)
{
   // Return Sierk fission barrier for zero angular momentum
   if (GetMaxSpinWithFissionBarrier(z, a) < 0) return -1;
   return CYrast::instance()->getBarrierFissionSierk(0.);
}
