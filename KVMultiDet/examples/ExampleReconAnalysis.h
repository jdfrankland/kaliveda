#ifndef __EXAMPLERECONANALYSIS_H
#define __EXAMPLERECONANALYSIS_H

/**
 \class ExampleReconAnalysis
 \brief Analysis of reconstructed events

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Sat Jul 30 10:42:54 2022
*/

#include "KVReconEventSelector.h"

class ExampleReconAnalysis : public KVReconEventSelector {

   void add_idcode_histos(const TString&);
   void fill_idcode_histos(const TString&, const KVReconstructedNucleus&);
   void fill_ecode_histos(const TString&, const KVReconstructedNucleus&);

public:
   ExampleReconAnalysis() {}
   virtual ~ExampleReconAnalysis() {}

   virtual void InitRun();
   virtual void EndRun() {}
   virtual void InitAnalysis();
   virtual Bool_t Analysis();
   virtual void EndAnalysis() {}

   ClassDef(ExampleReconAnalysis, 1) //Analysis of reconstructed events
};

#endif
