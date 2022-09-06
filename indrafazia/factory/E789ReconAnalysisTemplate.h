#include "KVReconEventSelector.h"
#include "KVMultiDetArray.h"

class E789ReconAnalysisTemplate : public KVReconEventSelector {

   KVMultiDetArray* INDRA, *FAZIA;

   void add_idcode_histos(const TString&);
   void fill_idcode_histos(const TString&, const KVReconstructedNucleus&);
   void fill_ecode_histos(const TString&, const KVReconstructedNucleus&);

public:
   E789ReconAnalysisTemplate() {}
   virtual ~E789ReconAnalysisTemplate() {}

   virtual void InitRun();
   virtual void EndRun() {}
   virtual void InitAnalysis();
   virtual Bool_t Analysis();
   virtual void EndAnalysis() {}

   ClassDef(E789ReconAnalysisTemplate, 0)
};
