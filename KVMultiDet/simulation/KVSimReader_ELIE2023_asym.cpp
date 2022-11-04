#include "KVSimReader_ELIE2023_asym.h"



void KVSimReader_ELIE2023_asym::define_output_filename()
{
   // ROOT file called: ELIE_[PROJ]_[TARG]_[EBEAM]AMeV_ASYM.root
   // Call after reading file header
   SetROOTFileName(Form("ELIE_%s_%s_%.1fAMeV_ASYM.root",
                        proj.GetSymbol(), targ.GetSymbol(), ebeam));
   tree_title.Form("ELIE secondary events %s + %s %.1f MeV/nuc.",
                   proj.GetSymbol(), targ.GetSymbol(), ebeam);
}

ClassImp(KVSimReader_ELIE2023_asym)


