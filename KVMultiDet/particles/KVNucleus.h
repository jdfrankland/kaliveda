/***************************************************************************
$Id: KVNucleus.h,v 1.40 2009/04/02 09:32:55 ebonnet Exp $
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KVNUCLEUS_H
#define KVNUCLEUS_H

#include "TVector3.h"
#include "TEnv.h"
#include "KVParticle.h"
#include "KVNumberList.h"
#include "KVParticleCondition.h"
#include "TLorentzRotation.h"
#include "KVString.h"

class KVLifeTime;
class KVMassExcess;
class KVAbundance;
class KVChargeRadius;

class KVNucleus: public KVParticle {


private:
   UChar_t fA;                  //nuclear mass number
   UChar_t fZ;                  //nuclear charge number (atomic number)
   UChar_t fMassFormula;        //mass formula for calculating A from Z
   static UInt_t fNb_nuc;       //!counts number of existing KVNucleus objects
   static Char_t fElements[][3];        //!symbols of chemical elements
   TString fSymbolName;        //!

   enum {
      kIsHeavy = BIT(17)        //flag when mass of nucleus is > 255
   };

protected:
   virtual void AddGroup_Withcondition(Name, KVParticleCondition*);

public:
   enum {                       //determines how to calculate mass from Z
      kBetaMass,
      kVedaMass,
      kEALMass,
      kEALResMass,
      kEPAXMass
   };
   enum {                       //determines how to calculate radius from Mass
      kLDModel,
      kEMPFunc,
      kELTON
   };

   enum {
      kDefaultFormula,
      kHinde1987,
      kViola1985,
      kViola1966
   };


   static Mass kAMU;        //atomic mass unit in MeV
   static Mass kMe;        //electron mass in MeV/c2
   static Mass u(void);
   static Double_t hbar;   // hbar*c in MeV.fm
   static Double_t e2;    // e^2/(4.pi.epsilon_0) in MeV.fm

   inline void SetMassFormula(UChar_t mt);
   inline Int_t GetMassFormula() const
   {
      return (Int_t)fMassFormula;
   }

   void init();
   KVNucleus();
   KVNucleus(const KVNucleus&);
   virtual void Clear(Option_t* opt = "");
   KVNucleus(AtomicNumber, MassNumber = 0, KineticEnergy = 0);
   KVNucleus(AtomicNumber, KineticEnergy, Direction);
   KVNucleus(AtomicNumber, MassNumber, Momentum);
   KVNucleus(Name, KineticEnergyPerNucleon = 0);

   virtual void Copy(TObject&) const;

   Bool_t IsSortable() const
   {
      return kTRUE;
   }
   Int_t Compare(const TObject* obj) const;

   virtual ~ KVNucleus();
   static MassNumber GetAFromZ(Double_t, Char_t mt);
   static NeutronNumber GetNFromZ(Double_t, Char_t mt);
   static Double_t GetRealAFromZ(Double_t, Char_t mt);
   static Double_t GetRealNFromZ(Double_t, Char_t mt);
   const Char_t* GetSymbol(Option_t* opt = "") const;
   const Char_t* GetLatexSymbol(Option_t* opt = "") const;

   static AtomicNumber GetZFromSymbol(const Char_t*);
   void SetZFromSymbol(const Char_t*);
   void Set(const Char_t*);
   static MassNumber IsMassGiven(const Char_t*);

   void SetZ(AtomicNumber, Char_t = -1);
   void SetA(MassNumber);
   void SetN(NeutronNumber);
   void SetZandA(AtomicNumber, MassNumber);
   void SetZandN(AtomicNumber, NeutronNumber);
   void SetZAandE(AtomicNumber, MassNumber, KineticEnergy);

   virtual void Print(Option_t* t = "") const;
   AtomicNumber GetZ() const;
   MassNumber GetA() const;
   NeutronNumber GetN() const;

   AOverZ GetAsurZ() const
   {
      return GetA() / GetZ();
   }
   NOverZ GetNsurZ() const
   {
      return GetN() / GetZ();
   }
   ChargeAsymmetry GetChargeAsymetry() const
   {
      // The charge asymmetry  = (neutrons-protons)/nucleons

      return (GetN() - GetZ()) / GetA();
   }
   KineticEnergyPerNucleon GetEnergyPerNucleon();
   KineticEnergyPerNucleon GetAMeV();

   void CheckZAndA(AtomicNumber&, MassNumber&) const;

   Mass GetMassExcess(AtomicNumber = -1, MassNumber = -1) const;
   KVArgType::Energy GetExtraMassExcess(AtomicNumber = -1, MassNumber = -1) const;
   KVMassExcess* GetMassExcessPtr(AtomicNumber = -1, MassNumber = -1) const;
   Mass GetAtomicMass(AtomicNumber = -1, MassNumber = -1) const ;
   Double_t GetNaturalA(AtomicNumber = -1) const ;

   BindingEnergy GetBindingEnergy(AtomicNumber = -1, MassNumber = -1) const;
   BindingEnergyPerNucleon GetBindingEnergyPerNucleon(AtomicNumber = -1, MassNumber = -1) const;

   KVNumberList GetKnownARange(AtomicNumber = -1, Lifetime min = 0) const;
   const Char_t* GetIsotopesList(AtomicNumber min, AtomicNumber max, Lifetime Tmin = 0) const;
   MassNumber GetAWithMaxBindingEnergy(AtomicNumber z = -1);

   static Double_t LiquidDrop_BrackGuet(UInt_t A, UInt_t Z);

   Bool_t IsKnown(AtomicNumber = -1, MassNumber = -1) const;
   Bool_t IsStable(Lifetime min = 1.0e+15/*seconds*/) const;
   Bool_t IsResonance() const;
   KVArgType::Energy GetWidth() const;

   void SetExcitEnergy(ExcitationEnergy e);

   ExcitationEnergy GetExcitEnergy() const
   {
      // Return the excitation energy of the nucleus in MeV.
      // This is the difference between the (relativistic) rest mass
      // and the ground state mass of the nucleus
      return ExcitationEnergy(GetMass() - GetMassGS());
   }
   Mass GetMassGS() const
   {
      // Return the ground state mass of the nucleus in MeV.
      return (kAMU * GetA()() + GetMassExcess());
   }

   Lifetime GetLifeTime(AtomicNumber = -1, MassNumber = -1) const;
   KVLifeTime* GetLifeTimePtr(AtomicNumber = -1, MassNumber = -1) const;

   Double_t GetAbundance(AtomicNumber = -1, MassNumber = -1) const;
   KVAbundance* GetAbundancePtr(AtomicNumber = -1, MassNumber = -1) const;
   MassNumber GetMostAbundantA(AtomicNumber = -1) const;

   Distance GetChargeRadius(AtomicNumber = -1, MassNumber = -1) const;
   KVChargeRadius* GetChargeRadiusPtr(AtomicNumber = -1, MassNumber = -1) const;
   Distance GetExtraChargeRadius(AtomicNumber = -1, MassNumber = -1, Int_t rct = 2) const;

   KVNucleus& operator=(const KVNucleus& rhs);
   KVNucleus operator+(const KVNucleus& rhs);
   KVNucleus operator-(const KVNucleus& rhs);
   KVNucleus& operator+=(const KVNucleus& rhs);
   KVNucleus& operator-=(const KVNucleus& rhs);

   // TH2F* GetKnownNucleiChart(KVString method="GetBindingEnergyPerNucleon");
   KineticEnergy DeduceEincFromBrho(Double_t Brho, Int_t ChargeState = 0);
   RelativeVelocity::Magnitude GetRelativeVelocity(KVNucleus* nuc);
   RelativeVelocity::Magnitude GetViolaVelocity(KVNucleus* nuc = 0, Int_t formula = kDefaultFormula /* kHinde1987 kViola1985 kViola1966 */);

   static RelativeVelocity::Magnitude vrelHinde1987(Double_t z1, Double_t a1, Double_t z2, Double_t a2);
   static RelativeVelocity::Magnitude vrelViola1985(Double_t z, Double_t a);
   static RelativeVelocity::Magnitude vrelViola1966(Double_t z, Double_t a);
   
   Bool_t IsElement(Int_t Z) const
   {
      return GetZ()==Z;
   }
   Bool_t IsIsotope(Int_t Z, Int_t A) const
   {
      return (GetZ()==Z && GetA()==A);
   }

   ClassDef(KVNucleus, 6)      //Class describing atomic nuclei
};

inline void KVNucleus::SetMassFormula(UChar_t mt)
{
   //Set mass formula used for calculating A from Z for this nucleus
   fMassFormula = mt;
   SetA(GetAFromZ(GetZ(), fMassFormula));       //recalculate A and mass
}

#endif
