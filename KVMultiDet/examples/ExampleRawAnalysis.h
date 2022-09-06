#ifndef __EXAMPLERAWANALYSIS_H
#define __EXAMPLERAWANALYSIS_H

/**
 \class ExampleRawAnalysis
 \brief Analysis of raw data

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Sat Jul 30 10:42:36 2022
*/

#include "KVRawDataAnalyser.h"

class ExampleRawAnalysis : public KVRawDataAnalyser {
   Int_t Mult;
   TString DetSigName;
   Double_t DetSigVal;

public:
   ExampleRawAnalysis() {}
   virtual ~ExampleRawAnalysis() {}

   void InitAnalysis();
   void InitRun();
   Bool_t Analysis();
   void EndRun() {}
   void EndAnalysis() {}

   ClassDef(ExampleRawAnalysis, 1) //Analysis of raw data
};

#endif
