#include "KVSimReader_ELIE2023_asym.h"



void KVSimReader_ELIE2023_asym::define_output_filename()
{
   // ROOT file called either
   //
   //       ELIE_[PROJ]_[TARG]_[EBEAM]AMeV_ASYM.root
   //
   // or
   //
   //       ELIE_[PROJ]_[TARG]_[EBEAM]AMeV_Run[xx...]_ASYM.root
   //
   // if run_number is defined
   //
   // Call after reading file header

   if (run_number >= 0)
      SetROOTFileName(Form("ELIE_%s_%s_%.1fAMeV_Run%d_ASYM.root",
                           proj.GetSymbol(), targ.GetSymbol(), ebeam, run_number));
   else
      SetROOTFileName(Form("ELIE_%s_%s_%.1fAMeV_ASYM.root",
                           proj.GetSymbol(), targ.GetSymbol(), ebeam));
   tree_title.Form("ELIE secondary events %s + %s %.1f MeV/nuc.",
                   proj.GetSymbol(), targ.GetSymbol(), ebeam);
}

ClassImp(KVSimReader_ELIE2023_asym)


