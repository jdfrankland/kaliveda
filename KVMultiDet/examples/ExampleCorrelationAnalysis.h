#ifndef __CORRELATOR_H
#define __CORRELATOR_H

/**
 \class correlator
 \brief User analysis class

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Mon Oct 17 17:28:16 2022
*/

#include "KVReconEventSelector.h"
#include "KVEventMixer.h"
#include "KVEventClassifier.h"
#include <vector>
#include <deque>

class ExampleCorrelationAnalysis : public KVReconEventSelector {

   struct particle {
      double phi;//!
      int det_index;//!

      particle() = default;
      particle(particle&&) = default;
      particle(const particle&) = default;
      particle(const KVReconstructedNucleus& p) : phi{p.GetPhi()},
         det_index{p.GetStoppingDetector()->GetIndex()}
      {}
   };

   KVEventMixer<particle, 5> event_mixer;

   KVEventClassifier* mult_bin;

   TString get_cor_histo_name(int bin, const TString& quantity)
   {
      return Form("h_cor_%s_bin_%d", quantity.Data(), bin);
   }
   TString get_uncor_histo_name(int bin, const TString& quantity)
   {
      return Form("h_uncor_%s_bin_%d", quantity.Data(), bin);
   }

public:
   ExampleCorrelationAnalysis() {};
   virtual ~ExampleCorrelationAnalysis() {};

   virtual void InitRun();
   virtual void EndRun() {}
   virtual void InitAnalysis();
   virtual Bool_t Analysis();
   virtual void EndAnalysis() {}

   ClassDef(ExampleCorrelationAnalysis, 0) //User analysis class
};

#endif
