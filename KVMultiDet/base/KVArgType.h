#ifndef KVARGTYPE_H
#define KVARGTYPE_H

#include "TVector3.h"
#include "TLorentzVector.h"

namespace KVArgType {

   /* ArgTypes with a simple underlying built-in variable type
    * i.e. int,double
    * conversion operator automatically casts to built-in type
    **********************************************************/
   template<typename Type, typename BuiltIn>
   class ArgType {
   protected:
      BuiltIn _val;
   public:
      ArgType() : _val() {}
      ArgType(const BuiltIn& v) : _val(v) {}
      operator BuiltIn() const
      {
         return _val;
      }
      BuiltIn operator()() const
      {
         return _val;
      }
   };
   /* ArgTypes with an underlying user-defined type (class)
    * e.g. TVector3, TString
    * member access operator '->' allows to use methods of
    * class as though variable were a pointer
    **********************************************************/
   template<typename Type, typename Class>
   class ArgTypeClass {
   protected:
      Class _val;
   public:
      ArgTypeClass() : _val() {}
      ArgTypeClass(const Class& v) : _val(v) {}
      Class* operator->()
      {
         return &_val;
      }
      operator const Class& () const
      {
         return _val;
      }
   };

#define DeclareNewNumericType(_type,_var) \
   class _type : public ArgType<_type,_var>\
   {\
   public:\
      _type() : ArgType<_type,_var>() {}\
      _type(const _var& v) : ArgType<_type,_var>(v) {}\
      _type& operator+=(const _var& v)\
      {\
         _val+=v;\
         return (*this);\
      }\
      _type& operator-=(const _var& v)\
      {\
         _val-=v;\
         return (*this);\
      }\
      _type& operator*=(const _var& v)\
      {\
         _val*=v;\
         return (*this);\
      }\
      _type& operator/=(const _var& v)\
      {\
         _val/=v;\
         return (*this);\
      }\
      _type operator-()\
      {\
         _type tmp(-_val);\
         return tmp;\
      }\
      static const char* get_encapsulated_typename() { return #_var ; }\
      static const char* get_typename() { return #_type ; }\
      virtual ~_type() {}\
      ClassDef(_type,0)\
   }
#define DeclareNewClassType(_type,_var) \
   class _type : public ArgTypeClass<_type,_var>\
   {\
   public:\
      _type() : ArgTypeClass<_type,_var>() {}\
      _type(const _var& v) : ArgTypeClass<_type,_var>(v) {}\
      static const char* get_encapsulated_typename() { return #_var ; }\
      static const char* get_typename() { return #_type ; }\
      virtual ~_type() {}\
      ClassDef(_type,0)\
   }
#define DeclareNewVectorType(_type) \
   class _type : public ArgTypeClass<_type,TVector3>\
   {\
   public:\
      virtual ~_type() {}\
      _type() : ArgTypeClass<_type,TVector3>() {}\
      _type(const TVector3& v) : ArgTypeClass<_type,TVector3>(v) {}\
      _type(Double_t x,Double_t y,Double_t z) : ArgTypeClass<_type,TVector3>(TVector3(x,y,z)) {}\
      _type& operator+=(const TVector3& v)\
      {\
         _val+=v;\
         return (*this);\
      }\
      _type& operator-=(const TVector3& v)\
      {\
         _val-=v;\
         return (*this);\
      }\
      static const char* get_encapsulated_typename() { return "TVector3" ; }\
      static const char* get_typename() { return #_type ; }\
      DeclareNewNumericType(XComponent,Double_t);\
      DeclareNewNumericType(YComponent,Double_t);\
      DeclareNewNumericType(ZComponent,Double_t);\
      DeclareNewNumericType(TransverseComponent,Double_t);\
      DeclareNewNumericType(Magnitude,Double_t);\
      XComponent X() const { return _val.X(); }\
      YComponent Y() const { return _val.Y(); }\
      ZComponent Z() const { return _val.Z(); }\
      TransverseComponent Perp() const { return _val.Perp(); }\
      Magnitude Mag() const { return _val.Mag(); }\
      ClassDef(_type,0)\
   }
#define DeclareNewStringType(_type) \
   class _type : public ArgTypeClass<_type,TString>\
   {\
   public:\
      _type() : ArgTypeClass<_type,TString>() {}\
      _type(const TString& v) : ArgTypeClass<_type,TString>(v) {}\
      _type(const char* v) : ArgTypeClass<_type,TString>(v) {}\
      operator const char* () const { return _val.Data(); }\
      friend std::ostream& operator<<(std::ostream& os, const _type& obj)\
      {\
         os << obj._val.Data();\
         return os;\
      }\
      Bool_t operator==(const TString& v) const { return _val==v; }\
      Bool_t operator!=(const TString& v) const { return _val!=v; }\
      Bool_t operator==(const char* v) const { return _val==v; }\
      Bool_t operator!=(const char* v) const { return _val!=v; }\
      static const char* get_encapsulated_typename() { return "TString" ; }\
      static const char* get_typename() { return #_type ; }\
      virtual ~_type() {}\
      ClassDef(_type,0)\
   }

#define DeclareNumericTypeOperator(_op,_ret,_t1,_t2) _ret operator _op (const _t1 & A, const _t2 & B)
#define DeclareNumericTypeOperatorCommutative(_op,_ret,_t1,_t2)\
   DeclareNumericTypeOperator(_op,_ret,_t1,_t2);\
   DeclareNumericTypeOperator(_op,_ret,_t2,_t1)
#define DeclareNumericTypeSubtraction(_ret,_t1,_t2)\
   DeclareNumericTypeOperatorCommutative(-,_ret,_t1,_t2);\
   DeclareNumericTypeOperatorCommutative(+,_t1,_ret,_t2);\
   DeclareNumericTypeOperatorCommutative(-,_t2,_t1,_ret)
#define DeclareNumericTypeAddition(_ret,_t1,_t2)\
   DeclareNumericTypeOperatorCommutative(+,_ret,_t1,_t2);\
   DeclareNumericTypeOperatorCommutative(-,_t1,_ret,_t2);\
   DeclareNumericTypeOperatorCommutative(-,_t2,_t1,_ret)
#define DeclareNumericTypeDivision(_ret,_t1,_t2)\
   DeclareNumericTypeOperator(/,_ret,_t1,_t2);\
   DeclareNumericTypeOperator(/,_t2,_t1,_ret);\
   DeclareNumericTypeOperatorCommutative(*,_t1,_t2,_ret)
#define DeclareNumericTypeMultiplication(_ret,_t1,_t2)\
   DeclareNumericTypeOperator(/,_t1,_ret,_t2);\
   DeclareNumericTypeOperator(/,_t2,_ret,_t1);\
   DeclareNumericTypeOperatorCommutative(*,_ret,_t1,_t2)
#define ImplementNumericTypeOperator(_op,_ret,_t1,_t2)\
   _ret operator _op (const _t1 & A, const _t2 & B)\
   {\
      return _ret(A() _op B());\
   }
#define ImplementNumericTypeOperatorCommutative(_op,_ret,_t1,_t2)\
   ImplementNumericTypeOperator(_op,_ret,_t1,_t2)\
   ImplementNumericTypeOperator(_op,_ret,_t2,_t1)
#define ImplementNumericTypeSubtraction(_ret,_t1,_t2)\
   ImplementNumericTypeOperatorCommutative(-,_ret,_t1,_t2)\
   ImplementNumericTypeOperatorCommutative(+,_t1,_ret,_t2)\
   ImplementNumericTypeOperatorCommutative(-,_t2,_t1,_ret)
#define ImplementNumericTypeAddition(_ret,_t1,_t2)\
   ImplementNumericTypeOperatorCommutative(+,_ret,_t1,_t2)\
   ImplementNumericTypeOperatorCommutative(-,_t1,_ret,_t2)\
   ImplementNumericTypeOperatorCommutative(-,_t2,_t1,_ret)
#define ImplementNumericTypeDivision(_ret,_t1,_t2)\
   ImplementNumericTypeOperator(/,_ret,_t1,_t2)\
   ImplementNumericTypeOperator(/,_t2,_t1,_ret)\
   ImplementNumericTypeOperatorCommutative(*,_t1,_t2,_ret)
#define ImplementNumericTypeMultiplication(_ret,_t1,_t2)\
   ImplementNumericTypeOperator(/,_t1,_ret,_t2)\
   ImplementNumericTypeOperator(/,_t2,_ret,_t1)\
   ImplementNumericTypeOperatorCommutative(*,_ret,_t1,_t2)

// basic properties
   DeclareNewNumericType(Number, Int_t);
   DeclareNewStringType(Name);
   DeclareNewNumericType(Energy, Double_t);
   typedef Energy Mass;
// geometry
   DeclareNewNumericType(Distance, Double_t);
   DeclareNewVectorType(Direction);
   DeclareNewNumericType(PolarAngle, Double_t);
   DeclareNewNumericType(AzimuthalAngle, Double_t);
   DeclareNewStringType(AngularDistributionType);
// particles & nuclei
   DeclareNewNumericType(ExcitationEnergy, Double_t);
   DeclareNewNumericType(BindingEnergy, Double_t);
   DeclareNewNumericType(BindingEnergyPerNucleon, Double_t);
   DeclareNewNumericType(ExcitationEnergyPerNucleon, Double_t);
   DeclareNewNumericType(KineticEnergyPerNucleon, Double_t);
   DeclareNewNumericType(AtomicNumber, Int_t);
   DeclareNewNumericType(NeutronNumber, Int_t);
   DeclareNewNumericType(Lifetime, Double_t);
   DeclareNewNumericType(MassNumber, Int_t);
   DeclareNewNumericType(ChargeAsymmetry, Double_t);
   DeclareNewNumericType(NeutronExcess, Int_t);
   DeclareNewNumericType(NOverZ, Double_t);
   DeclareNewNumericType(AOverZ, Double_t);
// kinematics
   DeclareNewNumericType(KineticEnergy, Double_t);
   DeclareNewVectorType(Velocity);
   DeclareNewVectorType(RelativeVelocity);
   DeclareNewVectorType(Momentum);
   DeclareNewVectorType(Spin);
   DeclareNewVectorType(AngularMomentum);
   DeclareNewClassType(FourMomentum, TLorentzVector);
   DeclareNewStringType(MomentumComponentType);

// Special operations
   DeclareNumericTypeAddition(MassNumber, NeutronNumber, AtomicNumber);
   DeclareNumericTypeSubtraction(NeutronExcess, NeutronNumber, AtomicNumber);
   DeclareNumericTypeDivision(ChargeAsymmetry, NeutronExcess, MassNumber);
   DeclareNumericTypeDivision(NOverZ, NeutronNumber, AtomicNumber);
   DeclareNumericTypeDivision(AOverZ, MassNumber, AtomicNumber);
   RelativeVelocity operator-(Velocity, Velocity);
   DeclareNumericTypeDivision(BindingEnergyPerNucleon, BindingEnergy, MassNumber);
   DeclareNumericTypeDivision(ExcitationEnergyPerNucleon, ExcitationEnergy, MassNumber);
   DeclareNumericTypeDivision(KineticEnergyPerNucleon, KineticEnergy, MassNumber);
}
#endif // KVARGTYPE_H
