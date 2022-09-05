//Created by KVClassFactory on Thu Jun 20 15:36:03 2019
//Author: John Frankland,,,

#include "KVDetector.h"
#include "KVDetectorSignalExpression.h"

ClassImp(KVDetectorSignalExpression)

KVDetectorSignalExpression::KVDetectorSignalExpression(const Char_t* type, const KVString& _expr, KVDetector* det)
   : KVDetectorSignal(type, det)
{
   // \param[in] type the typename for this expression, will be used as an alias for the expression
   // \param[in] _expr a mathematical expression using names of signals already defined for detector \a det. See TFormula
   //             class for valid operators/functions.
   // \param[in] det the detector to which this expression is to be associated
   //
   // If no valid signals are contained in the expression (i.e. signals not already defined for the
   // detector \a det), IsValid() returns kFALSE and the expression should not be used

   int nsigs = 0;
   KVString expr = _expr;
   SetLabel(_expr);
   TIter it_sig(&det->GetListOfDetectorSignals());
   KVDetectorSignal* ds;
   fRaw = kTRUE;
   while ((ds = (KVDetectorSignal*)it_sig())) {
      if (expr.Contains(ds->GetName())) {
         fSignals.push_back(ds);
         if (!ds->IsRaw()) fRaw = kFALSE;
         expr.ReplaceAll(ds->GetName(), Form("[%d]", nsigs));
         if (expr.CompareTo(_expr) != 0) ++nsigs; // a replacement was made => a signal was found
      }
   }
   if (nsigs) {
      fFormula.reset(new TFormula(type, expr));
      fValid = kTRUE;
   }
   else
      fValid = kFALSE;

   SetTitle(Form("Signal calculated as %s for detector %s", _expr.Data(), det->GetName()));
}

KVDetectorSignalExpression::KVDetectorSignalExpression(const Char_t* type, const KVString& _expr, const KVSeqCollection* dets)
   : KVDetectorSignal(type)
{
   // Constructor to be used with expressions which use explict references to detector signals
   // in the form [DET]::[SIG] where [DET] is the detector label and [SIG] is the signal name.
   // The list must contain the detectors whose labels are referenced.
   //
   // \note any signals which are not defined will be systematically evaluated as zero
   //
   // \param[in] type the typename for this expression, will be used as an alias for the expression
   // \param[in] _expr a mathematical expression using explicit references to names of detector signals
   // \param[in] dets a list of pointers to the detectors whose signals are to be used in the expression

   int nsigs = 0;
   KVString expr = _expr;
   SetLabel(_expr);
   fRaw = kTRUE;
   // examine each term in expression
   _expr.Begin("+-*/()");
   bool multidetexpr = false;
   bool explicit_det_reference = false;
   KVDetector* det = nullptr;
   while (!_expr.End()) {
      KVString t = _expr.Next();
      // check if we have an explicit reference to a detector
      if (t.Contains("::")) {
         explicit_det_reference = true;
         // check if more than 1 detector is referenced
         t.Begin("::");
         auto det_label = t.Next();
         auto _det = (KVDetector*)dets->FindObjectByLabel(det_label);
         if (!_det) {
            // reference to unknown detector
            Error("KVDetectorSignalExpression(const Char_t*,const KVString&,const KVSeqCollection*)",
                  "Use of reference to unknown detector with label %s in expression %s",
                  det_label.Data(), _expr.Data());
            fValid = kFALSE;
            return;
         }
         auto sig_name = t.Next();
         auto dsig = _det->GetDetectorSignal(sig_name);
         KVString det_sig_ref = Form("%s::%s", det_label.Data(), sig_name.Data());
         if (!dsig) {
            // reference to unknown signal
            Error("KVDetectorSignalExpression(const Char_t*,const KVString&,const KVSeqCollection*)",
                  "Use of reference to undefined signal %s for detector %s in expression %s : will evaluate as 0",
                  sig_name.Data(), _det->GetName(), _expr.Data());
            expr.ReplaceAll(det_sig_ref, "0");
         }
         else {
            if (expr.Contains(det_sig_ref)) {
               expr.ReplaceAll(det_sig_ref, Form("[%d]", nsigs));
               fSignals.push_back(dsig);
               ++nsigs;
               if (!dsig->IsRaw()) fRaw = kFALSE;
            }
         }
         if (det && (_det != det)) multidetexpr = true;
         det = _det;
      }
   }
   if (!explicit_det_reference) {
      fValid = kFALSE;
      Error("KVDetectorSignalExpression(const Char_t*,const KVString&,const KVSeqCollection*)",
            "Expression %s must contain explicit references to detector labels",
            _expr.Data());
      return;
   }
   if (nsigs) {
      fFormula.reset(new TFormula(type, expr));
      fValid = kTRUE;
   }
   else
      fValid = kFALSE;

   if (multidetexpr)
      SetTitle(Form("Signal calculated as %s", _expr.Data()));
   else {
      // only 1 detector is referenced in the expression
      SetDetector(det);
      SetTitle(Form("Signal calculated as %s for detector %s", _expr.Data(), det->GetName()));
   }
}

Double_t KVDetectorSignalExpression::GetValue(const KVNameValueList& params) const
{
   // \param[in] params comma-separated list of "param=value" pairs if extra parameters are required to evaluate any signals in the expression
   // \returns value of the expression using all current values of signals

   int nsigs = fSignals.size();
   for (int i = 0; i < nsigs; ++i) {
      fFormula->SetParameter(i, fSignals[i]->GetValue(params));
   }
   return fFormula->Eval(0);
}
