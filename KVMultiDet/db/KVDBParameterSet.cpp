#include "KVDBParameterSet.h"
#include <cstdarg>
#include <iostream>
#include "TString.h"
#include "TBuffer.h"
using std::cout;
using std::endl;

ClassImp(KVDBParameterSet);

//___________________________________________________________________________
//      Jeu de parametres pour la calibration
//      Cette classe est juste un "wraper" pour un tableau de paramètre
//
//

//___________________________________________________________________________
KVDBParameterSet::KVDBParameterSet()
{
   fParamNumber = 0;
}

//___________________________________________________________________________
KVDBParameterSet::KVDBParameterSet(const Char_t* name,
                                   const Char_t* title,
                                   UShort_t pnum): KVBase(name, title)
{
   //Initialise a KVDBRecord for a set of "pnum" parameters.
   //The names of the parameters are "par_1", "par_2", etc.
   fParamNumber = pnum;
   for (UInt_t i = 0; i < pnum; i++) {
      fParameters.SetValue(Form("par_%u", i + 1), 0.0);
   }
}

//___________________________________________________________________________
KVDBParameterSet::~KVDBParameterSet()
{
}

//____________________________________________________________________________
void KVDBParameterSet::SetParameters(Double_t val, ...)
{
   // Be cautious when using this method, if the number of arguments is lower than
   // the expected number of parameters a segmantation fault will occur !!!!!
   // exceeding argument won't be considered at all

   va_list ap;
   va_start(ap, val);

   int arg_n = 0;
   SetParameter(arg_n++, val);
   while (arg_n < fParamNumber) {
      SetParameter(arg_n, va_arg(ap, Double_t));
      arg_n++;
   }

   va_end(ap);
}

//____________________________________________________________________________
void KVDBParameterSet::SetParameters(const std::vector<Double_t>* const pars)
{
   assert(pars->size() <= static_cast<UInt_t>(fParamNumber));

   UInt_t arg_n(0);
   std::vector<Double_t>::const_iterator it;
   for (it = pars->begin(); it != pars->end(); ++it) {
      SetParameter(arg_n, *it);
      ++arg_n;
   }
}

//____________________________________________________________________________
void KVDBParameterSet::SetParamName(UShort_t i, const Char_t* name)
{
   fParameters.GetParameter(i)->SetName(name);
}

//____________________________________________________________________________
Double_t KVDBParameterSet::GetParameter(TString name)  const
{
   return fParameters.GetDoubleValue(name);
}

//____________________________________________________________________________
void KVDBParameterSet::SetParameter(TString name, Double_t val)
{
   fParameters.SetValue(name, val);
}

//____________________________________________________________________________
void KVDBParameterSet::SetParamNames(const Char_t* name, ...)
{
   // Be cautious when using this method, if the number of arguments is lower than
   // the expected number of parameters a segmantation fault will occur !!!!!
   // exceeding argument won't be considered at all

#ifdef KV_DEBUG
   Info("SetParamNames(const Char_t* name,...)", "Start");
#endif
   va_list ap;
   va_start(ap, name);

   int arg_n = 0;
   SetParamName(arg_n, name);
   while (arg_n < fParamNumber) {
      SetParamName(arg_n, va_arg(ap, Char_t*));
      arg_n++;
   }

#ifdef KV_DEBUG
   Info("SetParamNames(const Char_t* name,...)", "OK");
#endif
}

//_____________________________________________________________________________
void KVDBParameterSet::Print(Option_t*) const
{
   cout << ">>>> KVDBParameterSet :" << endl
        << GetName() << "  " << GetTitle() << endl
        << " Parameters :\t" << GetParamNumber() << endl;
   fParameters.Print();
   cout << endl << "<<<<<<<<<<" << endl;
}
