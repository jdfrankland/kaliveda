//Created by KVClassFactory on Sat Oct  3 14:18:09 2009
//Author: John Frankland,,,

#ifndef __KVINDRADETECTOR_H
#define __KVINDRADETECTOR_H

#include "KVDetector.h"
#include "KVINDRATelescope.h"
//old BaseIndra type definitions
enum EBaseIndra_type {
   ChIo_GG = 1,
   ChIo_PG,                     //=2
   ChIo_T,                      //=3
   Si_GG,                       //=4
   Si_PG,                       //=5
   Si_T,                        //=6
   CsI_R,                       //=7
   CsI_L,                       //=8
   CsI_T,                       //=9
   Si75_GG,                     //=10
   Si75_PG,                     //=11
   Si75_T,                      //=12
   SiLi_GG,                     //=13
   SiLi_PG,                     //=14
   SiLi_T                       //=15
};
enum EBaseIndra_typePhos {
   Phos_R = 1,
   Phos_L,                      //=2
   Phos_T,                      //=3
};
class KVINDRADetector : public KVDetector {
protected:
   // for silicon and ionisation chamber detectors
   // factors for converting between GG and PG coder values
   Double_t fGGtoPG_0;           //GG-PG conversion factor: offset
   Double_t fGGtoPG_1;           //GG-PGconversion factor: slope

   KVINDRADetector* fChIo;//!pointer to ionisation chamber in group associated to this detector
   KVINDRADetector* FindChIo();
   Int_t NumeroCodeur;     //Numero du codeur (QDC pour les ChIo/Si)

public:
public:
   static Char_t SignalTypes[16][3];    //Use this static array to translate EBaseIndra_type signal type to a string giving the signal type

   KVINDRADetector()
      : fGGtoPG_0(0), fGGtoPG_1(1. / 15.), fChIo(nullptr),
        NumeroCodeur(0)
   {
      SetKVDetectorFiredACQParameterListFormatString();
   }
   virtual ~KVINDRADetector() {}
   KVINDRADetector(const Char_t* type, const Float_t thick = 0.0)
      : KVDetector(type, thick), fGGtoPG_0(0), fGGtoPG_1(1. / 15.),
        fChIo(nullptr), NumeroCodeur(0)
   {
      SetKVDetectorFiredACQParameterListFormatString();
   }

   KVINDRATelescope* GetTelescope() const
   {
      // Return pointer to telescope containing this detector
      return (KVINDRATelescope*)GetParentStructure("TELESCOPE");
   }

   virtual void SetSegment(UShort_t)
   {
      // Overrides KVDetector method.
      // 'Segmentation' of INDRA detectors is defined in ctor of dedicated
      // detector classes
   }

   const Char_t* GetArrayName();
   virtual UInt_t GetRingNumber() const
   {
      return (GetTelescope() ? GetTelescope()->GetRingNumber() : 0);
   }
   virtual UInt_t GetModuleNumber() const
   {
      return (GetTelescope() ? GetTelescope()->GetNumber() : 0);
   }

   virtual void AddACQParamType(const Char_t* type);
   virtual KVACQParam* GetACQParam(const Char_t* /*type*/);
   KVACQParam* GetACQParamWithType(Int_t);

   Double_t GetPGfromGG(Double_t GG = -1);
   Double_t GetGGfromPG(Double_t PG = -1);
   void GetGGtoPGConversionFactors(Double_t* par)
   {
      // fill array par[2] with values of offset and slope parameters
      // of linear conversion from GG to PG coder values.
      //  par[0] = offset = fGGtoPG_0
      //  par[1] = slope = fGGtoPG_1
      par[0] = fGGtoPG_0;
      par[1] = fGGtoPG_1;
   };
   void SetGGtoPGConversionFactors(Double_t alpha, Double_t beta)
   {
      // set parameters of linear conversion from GG to PG coder values
      // (silicon and ionisation chamber detectors)
      // alpha = offset, beta = slope
      //   PG  = alpha + beta*(GG - GG_0) + PG_0
      // where GG_0 and PG_0 are respectively GG and PG pedestals
      fGGtoPG_0 = alpha;
      fGGtoPG_1 = beta;
   };
   KVINDRADetector* GetChIo() const
   {
      return (fChIo ? fChIo : const_cast<KVINDRADetector*>(this)->FindChIo());
   };
   virtual Float_t GetPG()
   {
      return GetACQData("PG");
   };
   virtual Float_t GetGG()
   {
      return GetACQData("GG");
   };
   virtual Float_t GetR()
   {
      return GetACQData("R");
   };
   virtual Float_t GetL()
   {
      return GetACQData("L");
   };
   UShort_t GetMT()
   {
      return GetACQParam("T")->GetCoderData();
   };

   void SetNumeroCodeur(Int_t numero);
   Int_t GetNumeroCodeur();

   void SetThickness(Double_t thick);

   ClassDef(KVINDRADetector, 2) //Detectors of INDRA array
};

#endif
