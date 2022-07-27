/*
$Id: KVParticleCondition.h,v 1.3 2007/03/26 10:14:56 franklan Exp $
$Revision: 1.3 $
$Date: 2007/03/26 10:14:56 $
*/

//Created by KVClassFactory on Thu Nov 16 14:20:38 2006
//Author: franklan

#ifndef __KVTEMPLATEPARTICLECONDITION_H
#define __KVTEMPLATEPARTICLECONDITION_H

#include "KVBase.h"
#include "KVString.h"
#include "KVHashList.h"
#include <vector>
#include <functional>
#include <KVClassFactory.h>
#include "TROOT.h"

class KVClassFactory;

/**
\class KVTemplateParticleCondition
\brief An object for handling particle selection
\ingroup AnalysisInfra
\ingroup NucEvents
\tparam ParticleType class which is at least a base class of the particles to be selected

A KVTemplateParticleCondition object can be used to select particles to include in data analysis
(see KVEventSelector::SetParticleConditions()), in the calculation of global
variables (see KVVarGlob::SetSelection()), or in iterations over events (see KVTemplateEvent).

The Test() method returns true or false
for a given nucleus depending on whether or not the condition is fulfilled. Combinations of
selections can be performed using Boolean logic operations `&&` and `||`.

Lambda expressions were introduced in C++11 and provide an easy way to define small functions
on the fly inside code. The lambda must take a `const ParticleType*` pointer as argument and return
a boolean, e.g.:
~~~~~~{.cpp}
KVTemplateParticleCondition<KVNucleus> l1("Z>2", [](const KVNucleus* nuc){ return nuc->GetZ()>2; });
KVTemplateParticleCondition<KVReconstructedNucleus> l2("Emeasured & Vpar>0", [](const KVReconstructedNucleus* nuc){ return nuc->IsCalibrated() && nuc->GetVpar()>0; });
~~~~~~
Note the first argument to the constructor is a name which the user is free to define
in order to remember what the condition does.

Like any lambda expressions, variables can be 'captured' from the surrounding scope, which
can be useful in some situations. For example, given the following definitions:
~~~~~~{.cpp}
int zmin = 3;
KVParticleCondition l3("Z>zmin", [&](const KVNucleus* nuc){ return nuc->GetZ()>=zmin; });
~~~~~~
then the limit for the selection can be changed dynamically like so:
~~~~~~{.cpp}
KVNucleus N("7Li");
l3.Test(&N);      ==> returns true
zmin=5;
l3.Test(&N);      ==> returns false
~~~~~~

\note KVParticleCondition is an alias for KVTemplateParticleCondition<KVNucleus>.
*/

template<typename ParticleType>
class KVTemplateParticleCondition : public KVBase {
   static KVHashList fgOptimized;// list of optimized particle conditions
   mutable Int_t fNUsing;//! number of classes using this as an optimized condition
   using LambdaFunc = std::function<bool(const ParticleType*)>;
   mutable LambdaFunc fLambdaCondition;
   LambdaFunc fSavedLambda1, fSavedLambda2;// used by || and &&
   enum class LogOp { AND, OR } fOpType;

   void logical_operator_lambda_condition_test() const
   {
      // check if we were created by && or || and haven't been initialized yet
      if (IsLambda() && !IsSet()) {
         switch (fOpType) {
            case LogOp::AND:
               fLambdaCondition = [this](const ParticleType * nuc) {
                  return (fSavedLambda1(nuc) && fSavedLambda2(nuc));
               };
               break;
            case LogOp::OR:
               fLambdaCondition = [this](const ParticleType * nuc) {
                  return (fSavedLambda1(nuc) || fSavedLambda2(nuc));
               };
               break;
         }
      }
   }

protected:

   KVString fCondition;//string containing selection criteria with ";" at end
   KVString fCondition_raw;//'raw' condition, i.e. no ';'
   KVString fCondition_brackets;//condition with '(' and ')' around it

   mutable const KVTemplateParticleCondition* fOptimal;//!
   KVString fClassName;//!
   mutable KVClassFactory* cf;//! used to generate code for optimisation
   KVString fOptimizedClassName;//! name of generated class used for optimisation
   mutable Bool_t fOptOK;//!false if optimisation failed (can't load generated code)

   void Optimize() const
   {
      //Generate a new class which inherits from KVParticleCondition but having a Test()
      //method which tests explicitly the condition which is set by the user.
      //
      //If needed, the KVNucleus pointer argument will be upcasted to the type given to SetParticleClassName().
      //
      //The new class is added to the list of plugins of type KVParticleCondition,
      //then an instance of the class is generated and a pointer to it stored in
      //member KVParticleCondition::fOptimal.
      //
      //This object is then used in the Test() method of this object to test the condition.
      //
      //If compilation fails, the condition will evaluate to kFALSE for all subsequent calls.

      fOptimal = (KVTemplateParticleCondition*)fgOptimized.FindObject(GetName());
      if (fOptimal) {  /* check that the same condition has not already been optimized */
         //Info("Optimize", "Using existing optimized condition %p", fOptimal);
         fOptimal->fNUsing++;
         fOptOK = kTRUE;
         return;
      }
      Info("Optimize", "Optimization of KVParticleCondition : %s", fCondition.Data());

      CreateClassFactory();
      KVString created_class_name = cf->GetClassName();
      //add Test() method
      cf->AddMethod("optimized_test", "Bool_t", "public", false, true);
      cf->AddMethodArgument("optimized_test", "const KVNucleus*", "nuc");
      cf->AddHeaderIncludeFile("KVNucleus.h");

      //write body of method
      KVString body("   //Optimized Test method for particle condition\n");
      KVString pointer = "nuc";
      if (fClassName != "") {
         pointer.Form("((%s*)nuc)", fClassName.Data());
         //upcasting pointer - we need to add corresponding #include to '.cpp' file
         cf->AddImplIncludeFile(Form("%s.h", fClassName.Data()));
      }
      KVString tmp;
      tmp = fCondition;
      tmp.ReplaceAll("_NUC_", pointer.Data());
      body += "   return ";
      body += tmp;

      cf->AddMethodBody("optimized_test", body);

      //generate .cpp and .h for new class
      cf->GenerateCode();

      //add plugin for new class
      gROOT->GetPluginManager()->AddHandler("KVParticleCondition", cf->GetClassName(), cf->GetClassName(),
                                            Form("%s+", cf->GetImpFileName()), Form("%s()", cf->GetClassName()));
      //load plugin
      TPluginHandler* ph;
      if (!(ph = LoadPlugin("KVParticleCondition", cf->GetClassName()))) {
         Error("Optimize", " *** Optimization failed for KVParticleCondition : %s", fCondition.Data());
         Error("Optimize", " *** Use method AddExtraInclude(const Char_t*) to give the names of all necessary header files for compilation of your condition.");
         Fatal("Optimize", " *** THIS CONDITION WILL BE EVALUATED AS kFALSE FOR ALL PARTICLES!!!");
         delete cf;
         cf = 0;
         //we set fOptimal to a non-zero value to avoid calling Optimize
         //every time that Test() is called subsequently.
         fOptimal = this;
         fOptOK = kFALSE;
         return;
      }
      fOptOK = kTRUE;
      delete cf;
      cf = 0;
      //execute constructor
      fOptimal = (KVTemplateParticleCondition*) ph->ExecPlugin(0);

      Info("Optimize", "fOptimal = %p", fOptimal);
      if (!fOptimal) {
         Error("Optimize", " *** Optimization failed for KVParticleCondition : %s", fCondition.Data());
         Error("Optimize", " *** Use method AddExtraInclude(const Char_t*) to give the names of all necessary header files for compilation of your condition.");
         Fatal("Optimize", " *** THIS CONDITION WILL BE EVALUATED AS kFALSE FOR ALL PARTICLES!!!");
         //we set fOptimal to a non-zero value to avoid calling Optimize
         //every time that Test() is called subsequently.
         fOptimal = this;
         fOptOK = kFALSE;
      }
      // add to list of optimized conditions
      const_cast<KVTemplateParticleCondition*>(fOptimal)->SetName(GetName());
      const_cast<KVTemplateParticleCondition*>(fOptimal)->fOptimizedClassName = created_class_name;
      fgOptimized.Add(const_cast<KVTemplateParticleCondition*>(fOptimal));
      fOptimal->fNUsing++;
      Info("Optimize", "Success");
   }

   virtual bool optimized_test(const ParticleType*) const
   {
      return kFALSE;
   }
   void CreateClassFactory() const
   {
      //Initialises KVClassFactory object used for optimization if it doesn't exist

      if (cf) return;

      // unique name for new class
      TUUID unique;
      KVString new_class = unique.AsString();
      // only first 8 characters are unique
      new_class.Remove(8);
      new_class.Prepend("KVParticleCondition_");

      //create new class
      cf = new KVClassFactory(new_class.Data(), "Particle condition to test", "KVParticleCondition");
      cf->SetInheritAllConstructors(kFALSE); // avoid generating ctor with LambdaFunc argument!!!
   }

   void SetClassFactory(KVClassFactory* CF)
   {
      //PRIVATE METHOD
      //Used by Copy
      CreateClassFactory();
      CF->Copy(*cf);
   }

public:
   KVTemplateParticleCondition()
      : KVBase("KVParticleCondition", "Particle selection criteria")
   {
      //default ctor
      fOptimal = nullptr;
      cf = nullptr;
      fOptOK = kFALSE;
      fNUsing = 0;
   }

   KVTemplateParticleCondition(const KVString& cond)
      : KVBase(cond, "KVParticleCondition")
   {
      //Create named object and set condition.
      //   This must be a valid C++ expression using `_NUC_` instead and in place of
      //   a `const KVNucleus*` pointer to the particle to be tested, for example
      //   ~~~~{.cpp}
      //   KVParticleCondition c1("_NUC_->GetZ()>2");
      //   KVParticleCondition c2("_NUC_->GetVpar()>0");
      //   ~~~~
      //   Note that the methods used in the selection
      //   do not have to be limited to the methods of the KVNucleus class.
      //   The 'real' class of the object
      //   passed to Test() can be used to cast the base pointer up (or is it down?) to the
      //   required pointer type at execution. In this case, you must call the method
      //   SetParticleClassName() with the name of the class to use in the cast.
      //
      //   Note that the first call to Test() automatically causes the 'optimization' of the
      //   KVParticleCondition, which means that a class implementing the required condition is generated
      //   and compiled on the fly before continuing (see method Optimize()).
      fOptimal = nullptr;
      Set(cond);
      cf = nullptr;
      fOptOK = kFALSE;
      fNUsing = 0;
   }

   KVTemplateParticleCondition(const Char_t* cond)
      : KVBase(cond, "KVParticleCondition")
   {
      //Create named object and set condition.
      //   This must be a valid C++ expression using `_NUC_` instead and in place of
      //   a `const KVNucleus*` pointer to the particle to be tested, for example
      //   ~~~~{.cpp}
      //   KVParticleCondition c1("_NUC_->GetZ()>2");
      //   KVParticleCondition c2("_NUC_->GetVpar()>0");
      //   ~~~~
      //   Note that the methods used in the selection
      //   do not have to be limited to the methods of the KVNucleus class.
      //   The 'real' class of the object
      //   passed to Test() can be used to cast the base pointer up (or is it down?) to the
      //   required pointer type at execution. In this case, you must call the method
      //   SetParticleClassName() with the name of the class to use in the cast.
      //
      //   Note that the first call to Test() automatically causes the 'optimization' of the
      //   KVParticleCondition, which means that a class implementing the required condition is generated
      //   and compiled on the fly before continuing (see method Optimize()).
      fOptimal = nullptr;
      Set(cond);
      cf = nullptr;
      fOptOK = kFALSE;
      fNUsing = 0;
   }

   KVTemplateParticleCondition(const KVTemplateParticleCondition& obj)
      : KVBase("KVParticleCondition", "Particle selection criteria")
   {
      // Copy constructor. Create new condition which is a copy of existing condition, obj.
      fOptimal = nullptr;
      cf = nullptr;
      fOptOK = kFALSE;
      fNUsing = 0;
      obj.Copy(*this);
   }

   KVTemplateParticleCondition(const KVString& name, const LambdaFunc& F)
      : KVBase(name, "KVParticleCondition"), fLambdaCondition(F)
   {
      // Create named object using lambda expression for condition
      // The lambda must take a `const ParticleType*` pointer as argument and return
      // a boolean, e.g.:
      // ~~~~~~{.cpp}
      // KVTemplateParticleCondition<KVNucleus> l1("Z>2", [](const KVNucleus* nuc){ return nuc->GetZ()>2; });
      // KVTemplateParticleCondition<KVReconstructedNucleus> l2("Emeasured & Vpar>0", [](const KVReconstructedNucleus* nuc){ return nuc->IsCalibrated() && nuc->GetVpar()>0; });
      // ~~~~~~
      // Note the first argument to the constructor is a name which the user is free to define
      // in order to remember what the condition does.
      //
      // Like any lambda expressions, variables can be 'captured' from the surrounding scope, which
      // can be useful in some situations. For example, given the following definitions:
      // ~~~~~~{.cpp}
      // int zmin = 3;
      // KVParticleCondition l3("Z>zmin", [&](const KVNucleus* nuc){ return nuc->GetZ()>=zmin; });
      // ~~~~~~
      // then the limit for the selection can be changed dynamically like so:
      // ~~~~~~{.cpp}
      // KVNucleus N("7Li");
      // l3.Test(&N);      ==> returns true
      // zmin=5;
      // l3.Test(&N);      ==> returns false
      // ~~~~~~
      fOptimal = nullptr;
      cf = nullptr;
      fOptOK = kFALSE;
      fNUsing = 0;
   }
   bool IsLambda() const
   {
      // \returns true if this condition is defined using a lambda expression
      return (bool)fLambdaCondition || ((bool)fSavedLambda1 && (bool)fSavedLambda2);
   }

   ~KVTemplateParticleCondition()
   {
      //default dtor
      if (fOptimal) {
         // do not delete optimized condition unless we are the last to use it
         --(fOptimal->fNUsing);
         if (!(fOptimal->fNUsing)) {
            fgOptimized.Remove(const_cast<KVTemplateParticleCondition*>(fOptimal));
            delete fOptimal;
            fOptimal = nullptr;
         }
      }
      SafeDelete(cf);
   }

   void Set(const KVString& name, const LambdaFunc& F)
   {
      // Set condition using lambda expression (replace any existing definition).
      //
      // The lambda must take a `const KVNucleus*` pointer as argument and return
      // a boolean:
      // ~~~~~~{.cpp}
      // KVParticleCondition l1("Z>2", [](const KVNucleus* nuc){ return nuc->GetZ()>2; });
      // KVParticleCondition l2("Vpar>0", [](const KVNucleus* nuc){ return nuc->GetVpar()>0; });
      // ~~~~~~
      // Note the first argument to the constructor is a name which the user is free to define
      // in order to remember what the condition does.
      fLambdaCondition = F;
      SetName(name);
   }

   void Set(const KVString& cond)
   {
      //Set particle condition criteria.
      //
      //These must be valid C++ expressions using _NUC_ instead and in place of
      //a pointer to the particle to be tested. Note that the methods used in the selection
      //do not have to be limited to the KVNucleus class. The 'real' class of the object
      //passed to Test() will be used to cast the base (KVNucleus) pointer up to the
      //required pointer type at execution.

      //Deprecate("Prefer to use lambda functions to define KVParticleCondition objects.");

      fCondition = cond;
      Ssiz_t ind = fCondition.Index(";");
      if (ind < 0) {
         fCondition_raw = fCondition;
         fCondition += ";";    //we add a ";" if there isn't already
      }
      else {
         fCondition_raw = fCondition.Strip(TString::kTrailing, ';');
      }
      fCondition_brackets = "(" + fCondition_raw + ")";
   }

   Bool_t Test(const ParticleType* nuc) const
   {
      //Evaluates the condition for the particle in question
      //
      //If no condition has been set (object created with default ctor) this returns
      //kTRUE for all nuclei.
      //
      //If optimisation fails (see method Optimize()), the condition will always
      //be evaluated as 'kFALSE' for all particles

      logical_operator_lambda_condition_test();
      if (!IsSet()) return kTRUE;
      if (IsLambda()) return fLambdaCondition(nuc);
      if (!fOptimal) Optimize();

      return (fOptOK ? fOptimal->optimized_test(nuc) : kFALSE);
   }

   Bool_t Test(const ParticleType& nuc) const
   {
      //Evaluates the condition for the particle in question
      //
      //If no condition has been set (object created with default ctor) this returns
      //kTRUE for all nuclei.
      //
      //If optimisation fails (see method Optimize()), the condition will always
      //be evaluated as 'kFALSE' for all particles

      return Test(&nuc);
   }

   void SetParticleClassName(const Char_t* cl)
   {
      fClassName = cl;
   }
   void AddExtraInclude(const Char_t* inc_file)
   {
      //Optimisation of KVParticleCondition::Test() implies the automatic generation
      //of a new class which implements the selection required by the user (see Optimize()).
      //
      //If the user's condition depends on objects of classes other than the family
      //of particle classes (TLorentVector <- KVParticle <- KVNucleus ...) there will
      //not be by default the necessary '#include' directive for the classes in question
      //in the generated class; the required plugin for Test() to function will not
      //load. In this case, the user should call this method with the name of each
      //'#include' file to be added to the class implementation.
      //
      //Example:
      //~~~~~{.cpp}
      //    KVParticleCondition p("_NUC_->GetVpar()>=gDataAnalyser->GetKinematics()->GetNucleus(1)->GetVpar()");
      //~~~~~
      //Optimization will not work, as:
      //  - gDataAnalyser pointer to current analysis manager (KVDataAnalyser object), defined in KVDataAnalyser.h
      //  - gDataAnalyser->GetKinematics() returns a pointer to a KV2Body object, defined in KV2Body.h
      //
      //Therefore, for this condition to work, the user must first call the methods :
      //
      //~~~~~{.cpp}
      //    p.AddExtraInclude("KVDataAnalyser.h");
      //    p.AddExtraInclude("KV2Body.h");
      //~~~~~
      //
      //before the first call to p.Test() (when optimization occurs).

      CreateClassFactory();
      cf->AddImplIncludeFile(inc_file);
   }

   void Copy(TObject& obj) const
   {
      //Copy this to obj
      KVBase::Copy(obj);
      ((KVTemplateParticleCondition&) obj).fCondition = fCondition;
      ((KVTemplateParticleCondition&) obj).fCondition_raw = fCondition_raw;
      ((KVTemplateParticleCondition&) obj).fCondition_brackets = fCondition_brackets;
      ((KVTemplateParticleCondition&) obj).fLambdaCondition = fLambdaCondition;
      ((KVTemplateParticleCondition&) obj).fSavedLambda1 = fSavedLambda1;
      ((KVTemplateParticleCondition&) obj).fSavedLambda2 = fSavedLambda2;
      ((KVTemplateParticleCondition&) obj).fOpType = fOpType;
      ((KVTemplateParticleCondition&) obj).fOptOK = fOptOK;
      // force first call to Test to try optimization
      // if existing optimized version exists in static list, pointer will be reset
      ((KVTemplateParticleCondition&) obj).fOptimal = nullptr;
      if (fClassName != "")((KVTemplateParticleCondition&) obj).SetParticleClassName(fClassName.Data());
      if (cf) {
         ((KVTemplateParticleCondition&) obj).SetClassFactory(cf);
      }
   }

   KVTemplateParticleCondition& operator=(const KVTemplateParticleCondition& obj)
   {
      // Set condition to be same as for existing KVParticleCondition object
      if (&obj != this) obj.Copy(*this);
      return (*this);
   }

   KVTemplateParticleCondition& operator=(const KVString& sel)
   {
      // Set condition using pseudo-code in string (replacing any previous definition).
      //
      //   This must be a valid C++ expression using `_NUC_` instead and in place of
      //   a `const KVNucleus*` pointer to the particle to be tested, for example
      //   ~~~~{.cpp}
      //   KVParticleCondition c1("_NUC_->GetZ()>2");
      //   KVParticleCondition c2("_NUC_->GetVpar()>0");
      //   ~~~~
      //   Note that the methods used in the selection
      //   do not have to be limited to the methods of the KVNucleus class.
      //   The 'real' class of the object
      //   passed to Test() can be used to cast the base pointer up (or is it down?) to the
      //   required pointer type at execution. In this case, you must call the method
      //   SetParticleClassName() with the name of the class to use in the cast.
      Set(sel);
      return (*this);
   }

   KVTemplateParticleCondition& operator=(const LambdaFunc& f)
   {
      // Set condition using lambda expression (replacing any previous definition).
      fLambdaCondition = f;
      return (*this);
   }

   friend KVTemplateParticleCondition operator&&(const KVTemplateParticleCondition& A, const KVTemplateParticleCondition& B)
   {
      //Perform boolean AND between the two selection conditions
      //
      //If SetParticleClassName() has been called for either of the two conditions,
      //it will be called for the resulting condition with the same value
      //
      // Both conditions must be of same type, i.e. if one uses a lambda expression, the other
      // must also use a lambda expression.
      //
      // If one or other of the conditions is not set, we just return the condition which has been set.

      A.logical_operator_lambda_condition_test();
      B.logical_operator_lambda_condition_test();

      if (!(A.IsSet() && B.IsSet())) {
         // one or both conditions is/are not set
         if (!(A.IsSet() || B.IsSet())) {
            // neither is set: return blank (unset) condition
            return KVTemplateParticleCondition();
         }
         else if (A.IsSet()) return KVTemplateParticleCondition(A);
         else return KVTemplateParticleCondition(B);
      }
      // if lambdas are used (error if not both ?)
      if (A.IsLambda() || B.IsLambda()) {
         if (A.IsLambda() && B.IsLambda()) {
            KVTemplateParticleCondition tmp;
            tmp.fSavedLambda1 = A.fLambdaCondition;
            tmp.fSavedLambda2 = B.fLambdaCondition;
            tmp.fOpType = KVTemplateParticleCondition::LogOp::AND;
            tmp.SetName(Form("(%s) && (%s)", A.GetName(), B.GetName()));
            return tmp;
         }
         else {
            ::Error("KVTemplateParticleCondition::operator&&", "Both KVParticleCondition objects must use lambda captures in order to do this");
            return KVTemplateParticleCondition();
         }
      }
      KVTemplateParticleCondition tmp;
      tmp.Set(A.fCondition_brackets + " && " + B.fCondition_brackets);
      if (A.fClassName != "") tmp.SetParticleClassName(A.fClassName);
      else if (B.fClassName != "") tmp.SetParticleClassName(B.fClassName);
      return tmp;
   }

   friend KVTemplateParticleCondition operator||(const KVTemplateParticleCondition& A, const KVTemplateParticleCondition& B)
   {
      //Perform boolean OR between the two selection conditions
      //
      //If SetParticleClassName has been called for either of the two conditions,
      //it will be called for the resulting condition with the same value
      //
      // Both conditions must be of same type, i.e. if one uses a lambda expression, the other
      // must also use a lambda expression.
      //
      // If one or other of the conditions is not set, we just return the condition which has been set.

      A.logical_operator_lambda_condition_test();
      B.logical_operator_lambda_condition_test();

      if (!(A.IsSet() && B.IsSet())) {
         // one or both conditions is/are not set
         if (!(A.IsSet() || B.IsSet())) {
            // neither is set: return blank (unset) condition
            return KVTemplateParticleCondition();
         }
         else if (A.IsSet()) return KVTemplateParticleCondition(A);
         else return KVTemplateParticleCondition(B);
      }
      // if lambdas are used (error if not both ?)
      if (A.IsLambda() || B.IsLambda()) {
         if (A.IsLambda() && B.IsLambda()) {
            KVTemplateParticleCondition tmp;
            tmp.fSavedLambda1 = A.fLambdaCondition;
            tmp.fSavedLambda2 = B.fLambdaCondition;
            tmp.fOpType = KVTemplateParticleCondition::LogOp::OR;
            tmp.SetName(Form("(%s) || (%s)", A.GetName(), B.GetName()));
            return tmp;
         }
         else {
            ::Error("operator&&", "Both KVParticleCondition objects must use lambda captures in order to do this");
            return KVTemplateParticleCondition();
         }
      }
      KVTemplateParticleCondition tmp;
      tmp.Set(A.fCondition_brackets + " || " + B.fCondition_brackets);
      if (A.fClassName != "") tmp.SetParticleClassName(A.fClassName);
      else if (B.fClassName != "") tmp.SetParticleClassName(B.fClassName);
      return tmp;
   }

   KVTemplateParticleCondition& operator|=(const KVTemplateParticleCondition& other)
   {
      // Replace current condition with a logical 'OR' between itself and other
      //
      // Both conditions must be of same type, i.e. if one uses a lambda expression, the other
      // must also use a lambda expression.

      KVTemplateParticleCondition tmp = *this || other;
      tmp.Copy(*this);
      return *this;
   }

   KVTemplateParticleCondition& operator&=(const KVTemplateParticleCondition& other)
   {
      // Replace current condition with a logical 'AND' between itself and other
      //
      // Both conditions must be of same type, i.e. if one uses a lambda expression, the other
      // must also use a lambda expression.

      KVTemplateParticleCondition tmp = *this && other;
      tmp.Copy(*this);
      return *this;
   }
   void Print(Option_t* opt = "") const
   {
      //Print informations on object
      if (fCondition != "") {
         Info("Print", "object name = %s, address = %p", GetName(), this);
         std::cout << " * condition = " << fCondition.Data() << std::endl;
         std::cout << " * classname = " << fClassName.Data() << std::endl;
         std::cout << " * fOptimal = " << fOptimal << std::endl;
         std::cout << " * fNUsing = " << fNUsing << std::endl;
         if (cf) {
            std::cout << " * classfactory :" << std::endl;
            cf->Print();
         }
      }
      else {
         std::cout << GetName() << std::endl;
      }
   }

   static void PrintOptimizedList()
   {
      fgOptimized.Print();
   }
   Bool_t IsSet() const
   {
      // Return kTRUE if a condition/selection has been defined
      //
      // If this is not true, Test() will return true for all and any nuclei.

      return (fLambdaCondition || fCondition != "");
   }

   ClassDef(KVTemplateParticleCondition, 1) //Implements parser of particle selection criteria
};

template<typename ParticleClass>
KVHashList KVTemplateParticleCondition<ParticleClass>::fgOptimized;
#endif
