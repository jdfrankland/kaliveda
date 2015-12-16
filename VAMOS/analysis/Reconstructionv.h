#ifndef _RECONSTRUCTION_CLASS

/**
   WARNING: This class has been deprecated and will eventually be removed.

   Deprecated by: Peter Wigg (peter.wigg.314159@gmail.com)
   Date:          Thu  8 Oct 13:33:33 BST 2015
*/

#include "Defines.h" // __ENABLE_DEPRECATED_VAMOS__
#ifdef __ENABLE_DEPRECATED_VAMOS__

// This class is only compiled if __ENABLE_DEPRECATED_VAMOS__ is set in
// VAMOS/analysis/Defines.h. If you enable the deprecated code using the default
// build options then a LARGE number of warnings will be printed to the
// terminal. To disable these warnings (not advised) compile VAMOS with
// -Wno-deprecated-declarations. Despite the warnings the code should compile
// just fine.

#define _RECONSTRUCTION_CLASS

#include "Deprecation.h"
#include "Rtypes.h"
#include "LogFile.h"
#include "Random.h"
#include "DriftChamberv.h"
#include "TVector3.h"
#include "KVDataSet.h"

#include "KVINDRAe503.h"

#include <cmath>


class Reconstructionv {

   Bool_t Ready;

public:

   Reconstructionv(LogFile* Log, DriftChamberv* Drift);
   virtual ~Reconstructionv(void);

   LogFile* L;
   DriftChamberv* Dr;

   bool Present; //true if coordinates determined

   void Init(void); //Init for every event,  variables go to -500.
   void Calculate(); // Calulate  Initial coordinates
   void Show(void);
   void Treat(void);
   void outAttach(TTree* outT);
   void CreateHistograms();
   void FillHistograms();
   void PrintCounters(void);

   Random* Rnd;

   Float_t BrhoRef;
   Float_t AngleVamos; // in degrees
   Float_t DDC1;
   Float_t DSED1;
   Float_t DDC2;
   Float_t DCHIO;
   Float_t DSI;
   Float_t DCSI;

   Double_t Coef[4][330]; //D,T,P,Path seventh order reconst in x,y,t,p

   Float_t Delta1[600];
   Float_t Delta2[600];
   Float_t Deg1[600];
   Float_t Deg2[600];
   Float_t Facteur[600];
   Float_t Stat[600];
   Float_t Etendue[600];
   Double_t corr_pl;
   Double_t deltat;

   Int_t Brho_tag;
   Float_t Brho_min[600];
   Float_t Brho_max[600];

   Float_t Theta;
   Float_t Phi;
   Float_t Brho;
   Float_t Path;
   Float_t PathOffset;
   Float_t ThetaL;
   Float_t PhiL;
   Float_t ThetaLdeg;
   Float_t PhiLdeg;
   Float_t Thetadeg;
   Float_t Phideg;

   Float_t Brho_always;
   Float_t Theta_always;
   Float_t Phi_always;
   Float_t Path_always;

   //Counters
   Int_t Counter[6];

   void SetBrhoRef(Float_t brho)
   {
      BrhoRef = brho;
   }
   void SetAngleVamos(Float_t theta)
   {
      AngleVamos = theta;
   }

   ClassDef(Reconstructionv, 0)

};

#endif // __ENABLE_DEPRECATED_VAMOS__ is set
#endif // _RECONSTRUCTION_CLASS is not set

#ifdef _RECONSTRUCTION_CLASS
DEPRECATED_CLASS(Reconstructionv);
#endif

