#include "KVEventClassifier.h"

int KVEventClassifier::calc_where() const
{
   double value_to_test;
   if (fWithExpression) {
      int nval = 0;
      for (auto& p : fValues) {
         fFormula->SetParameter(nval, fVar->GetValue(p.c_str()));
         ++nval;
      }
      value_to_test = fFormula->Eval(0);
   }
   else if (fWithVal) {
      value_to_test = fVar->GetValue(fVal);
   }
   else {
      value_to_test = fVar->GetValue();
   }
   if (fIntegerVariable) {
      value_to_test = value_to_test + gRandom->Uniform();
   }
   std::vector<double>::const_iterator result = std::find_if(fCuts.begin(), fCuts.end(),
   [value_to_test](double x) {
      return x > value_to_test;
   }
                                                            );
   return std::distance(fCuts.begin(), result);
}

void KVEventClassifier::Init()
{
   // Sort cuts/bins into ascending order
   //
   // Also set up TFormula if we need to use an expression

   fIntegerVariable = false;
   fWithExpression = false;
   std::sort(fCuts.begin(), fCuts.end());
   if (fWithVal && !fVar->HasValue(fVal)) {
      // assume fVal is an expression using the named values of the variable
      int nval = 0;
      for (auto& p : fVar->GetValueNameList()) {
         if (fVal.Contains(p.GetName())) {
            fValues.push_back(std::string(p.GetName()));
            fVal.ReplaceAll(p.GetName(), Form("[%d]", nval));
            ++nval;
         }
      }
      if (nval) {
         fFormula.reset(new TFormula("EC_formula", fVal));
         fWithExpression = kTRUE;
      }
      else {
         Error("Init()", "Tried to make event classifier for variable %s using expression: %s", fVar->GetName(), fVal.Data());
      }
   }
   if (!fWithExpression) {
      // using simple single-valued variable (or the default value) or 1 named value
      // check if it is an integer value
      if (fWithVal)
         fIntegerVariable = (fVar->GetValueType(fVar->GetNameIndex(fVal)) == 'I');
      else
         fIntegerVariable = (fVar->GetValueType(0) == 'I');
   }
}

ClassImp(KVEventClassifier)


