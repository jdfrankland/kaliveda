//Created by KVClassFactory on Thu Aug 20 16:03:37 2009
//Author: marini

#ifndef __CSICALIB_H
#define __CSICALIB_H

#include "TTree.h"
#include "TFile.h"
#include "TEventList.h"
#include "KVLightEnergyCsI.h"
#include "KVLightEnergyCsIVamos.h"
#include "KVTelescope.h"
#include "KVDetector.h"
#include "KVTelescope.h"
#include "KVIDTelescope.h"
#include "KVList.h"
#include "TCut.h"
#include "Sive503.h"
#include "CsIv.h"
#include "LogFile.h"

#include "KVIDGraph.h"
#include "KVIDZAGrid.h"

#include "KVClassMonitor.h"
#include <string>
#include <string.h>

#include "KVReconstructedNucleus.h"

class CsICalib {

public:

   Int_t eN;
   Int_t status;

   UShort_t eLightSi;
   UShort_t eLightCsI;
   Double_t LightCsI;
   Double_t eEnergySi;
   Double_t eEnergyCsI;
   Double_t eEnergyGap;

   Double_t Einc;
   Double_t EEsi;
   Double_t Echio;

   Double_t sEnergySi;
   Double_t sEnergyCsI;
   Double_t sRefECsI;

   Double_t esi1;
   Double_t esi2;
   Double_t ecsi1;
   Double_t ecsi2;
   Double_t diff1;
   Double_t diff2;

   Double_t diffsi;
   Double_t diffcsi;
   Double_t diffetot;
   Double_t difflum;

   Double_t CanalCsI;

   Int_t right;
   Int_t left;

   Int_t eZ;
   Int_t eA;
   Int_t sA;
   Double_t iA;

   Double_t ePied;
   Double_t a;
   Double_t b;
   Double_t c;
   Double_t alpha;

   Double_t a1;
   Double_t a2;
   Double_t a3;
   Double_t a4;

   Double_t thick;

   KVReconstructedNucleus* frag;
   KVLightEnergyCsIVamos* lum;

   KVTelescope* ttel;

   KVTelescope* kvt_icsi;
   KVTelescope* kvt_sicsi;
   KVDetector* kvd_si;
   KVDetector* kvd_csi;

   KVDetector* kvd_gap;

   KVList* list;
   KVIDGraph* kvid;
   KVList* list_chiosi;
   KVIDGraph* kvid_chiosi;
   KVList* list_sitof;
   KVIDGraph* kvid_sitof;

   KVDetector* gap;
   KVDetector* si;
   KVDetector* csi;


   KVDetector* ssi;
   KVDetector* ggap;
   KVDetector* ccsi;

   KVNucleus part;
   KVNucleus part2;

   Sive503* Si;
   CsIv* CsI;

   CsICalib(LogFile* Log, Sive503* Si);
   LogFile* L;
   virtual ~CsICalib();

   //Int_t ClearEvent(Int_t);
   string name;

   void SetTel1(KVDetector* si);
   KVDetector* GetTel1(void);
   void SetTel2(KVDetector* gap);
   KVDetector* GetTel2(void);
   void SetTel3(KVDetector* csi);
   KVDetector* GetTel3(void);

   void InitTelescope(Int_t, Int_t);
   void InitTelescopeChioSi(Int_t, Int_t);
   void InitTelescopeSiTof(Int_t);

   void InitSiCsI(Int_t);
   //void SetCalSi(Float_t,Float_t,Float_t);
   //void SetCsIPed(Float_t);
   //void SetCalCsI(Float_t,Float_t,Float_t);

   //void CalculateCanalCsI();

   void SetCalibration(Sive503*, CsIv*, Int_t, Int_t);

   void SetFragmentZ(Int_t);
   void SetFragmentA(Int_t);

   //necessary methods to GetResidualEnergyCsI: best estimation of ECsI and A
   Double_t GetResidualEnergyCsI(Double_t, Double_t);  //UShort_t,UShort_t

   void CalculateESi(Double_t);   //UShort_t
   void Bisection(Int_t, Double_t);  //UShort_t
   //void ECsIch(Double_t);
   Double_t BisectionLight(Double_t, Double_t, Double_t);
   void CompleteSimulation(Double_t);   //UShort_t
   //void CalculateECsI();
   //void SimulateEvent();

   void Interpolate();
   Double_t GetInterpolationD(Double_t, Double_t, Double_t, Double_t, Double_t);

   Double_t RetrieveEnergySi();
   Double_t RetrieveA();
   Double_t RetrieveLight();
   Double_t RetrieveEnergyCsI();

   ClassDef(CsICalib, 1) //CsICalib
};

#endif
