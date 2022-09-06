#ifndef __EXAMPLERECONRAWANALYSIS_H
#define __EXAMPLERECONRAWANALYSIS_H

/**
 \class ExampleReconRawAnalysis
 \brief Analysis of reconstructed raw data

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Sat Jul 30 10:42:45 2022
*/

#include "KVReconRawDataAnalyser.h"

class ExampleReconRawAnalysis : public KVReconRawDataAnalyser {
   Int_t Mult;
   TString ArrayName;
   Int_t Z;

public:
   ExampleReconRawAnalysis() {}
   virtual ~ExampleReconRawAnalysis() {}

   void InitAnalysis();
   void InitRun();
   Bool_t Analysis();
   void EndRun() {}
   void EndAnalysis() {}

   ClassDef(ExampleReconRawAnalysis, 1) //Analysis of reconstructed raw data
};

#endif
