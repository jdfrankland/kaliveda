/***************************************************************************
                          kvparticle.h  -  description
                             -------------------
    begin                : Sun May 19 2002
    copyright            : (C) 2002 by J.D. Frankland
    email                : frankland@ganil.fr

$Id: KVParticle.h,v 1.41 2008/05/21 13:19:56 ebonnet Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KVPARTICLE_H
#define KVPARTICLE_H

#include "TVector3.h"
#include "TLorentzVector.h"
#include "KVBase.h"
#include "TRef.h"
#include "TMath.h"
#include "KVList.h"
#include "KVUniqueNameList.h"
#include "TObjString.h"
#include "KVNameValueList.h"

#include <KVArgType.h>

using namespace KVArgType;

class KVList;
class KVParticleCondition;

class KVParticle: public TLorentzVector {

   TString fName;                       //!non-persistent name field - Is useful
   TString fFrameName;                  //!non-persistent frame name field, sets when calling SetFrame method
   KVList fBoosted;                     //!list of momenta of the particle in different Lorentz-boosted frames
   KVUniqueNameList fGroups;            //!list of TObjString for manage different group name
   static Double_t kSpeedOfLight;       //speed of light in cm/ns

protected:

   KVNameValueList fParameters;//a general-purpose list of parameters associated with this particle

   virtual void AddGroup_Withcondition(Name, KVParticleCondition*);
   virtual void AddGroup_Sanscondition(Name newgroup, Name parentgroup = "");
   void CreateGroups();
   void SetGroups(KVUniqueNameList* un);
   void AddGroups(KVUniqueNameList* un);

public:

   Bool_t HasFrame(const Char_t* frame);
   Int_t GetNumberOfDefinedFrames(void)
   {
      return fBoosted.GetEntries();
   };
   Int_t GetNumberOfDefinedGroups(void);
   KVUniqueNameList* GetGroups() const;

   enum {
      kIsOK = BIT(14),          //acceptation/rejection flag
      kIsOKSet = BIT(15),       //flag to indicate flag is set
      kIsDetected = BIT(16)     //flag set when particle is slowed by some KVMaterial
   };

   static Double_t C();

   KVParticle();
   KVParticle(Mass, Momentum);
   KVParticle(Mass, Momentum::XComponent, Momentum::YComponent, Momentum::ZComponent);
   KVParticle(const KVParticle&);
   virtual ~ KVParticle();
   void init();
   virtual void Copy(TObject&) const;
   virtual void Clear(Option_t* opt = "");

   virtual void SetMass(Mass m)
   {
      SetXYZM(Px(), Py(), Pz(), m);
   }
   Mass GetMass() const
   {
      return M();
   }
   void SetMomentum(Momentum v)
   {
      SetXYZM(v->X(), v->Y(), v->Z(), M());
   }
   void SetMomentum(Momentum::XComponent, Momentum::YComponent, Momentum::ZComponent, MomentumComponentType = "cart");
   void SetMomentum(KineticEnergy T, Direction dir);
   void SetRandomMomentum(KineticEnergy T, PolarAngle pmin, PolarAngle pmax, AzimuthalAngle amin, AzimuthalAngle amax, AngularDistributionType = "isotropic");
   virtual void Print(Option_t* t = "") const;
   void Set4Mom(FourMomentum p)
   {
      SetVect(p->Vect());
      SetT(p->E());
   }
   void SetE(KineticEnergy a)
   {
      SetKE(a);
   }
   void SetKE(KineticEnergy ecin);
   void SetEnergy(KineticEnergy e)
   {
      SetKE(e);
   }
   void SetVelocity(Velocity);
   Momentum GetMomentum() const
   {
      return Vect();
   }
   KineticEnergy GetKE() const
   {
      Double_t e =  E();
      Double_t m = M();
      //return (E() - M());
      return e - m;
   }
   KineticEnergy GetEnergy() const
   {
      return GetKE();
   }
   KineticEnergy GetTransverseEnergy() const
   {
      Double_t etran = TMath::Sin(Theta());
      etran = TMath::Power(etran, 2.0);
      etran *= GetKE();
      return etran;
   }
   KineticEnergy GetEtran() const
   {
      return GetTransverseEnergy();
   }
   KineticEnergy GetRTransverseEnergy() const
   {
      Double_t etran = Mt() - GetMass();
      return etran;
   }
   KineticEnergy GetREtran() const
   {
      return GetRTransverseEnergy();
   }
   KineticEnergy GetE() const
   {
      return GetKE();
   }
   Velocity GetVelocity() const;
   Velocity GetV() const
   {
      return GetVelocity();
   }
   Velocity::ZComponent GetVpar() const
   {
      return GetV()->Z();
   }
   Velocity::TransverseComponent GetVperp() const;
   PolarAngle GetTheta() const
   {
      return TMath::RadToDeg() * Theta();
   }
   AzimuthalAngle GetPhi() const
   {
      Double_t phi = TMath::RadToDeg() * Phi();
      return (phi < 0 ? 360. + phi : phi);
   }
   void SetTheta(PolarAngle theta)
   {
      TLorentzVector::SetTheta(TMath::DegToRad() * theta);
   }
   void SetPhi(AzimuthalAngle phi)
   {
      TLorentzVector::SetPhi(TMath::DegToRad() * phi);
   }

   virtual Bool_t IsOK();
   void SetIsOK(Bool_t flag = kTRUE);
   void ResetIsOK()
   {
      ResetBit(kIsOKSet);
   }

   KVList* GetListOfFrames(void)
   {
      return (KVList*)&fBoosted;
   }

   void SetIsDetected()
   {
      SetBit(kIsDetected);
   }
   Bool_t IsDetected()
   {
      return TestBit(kIsDetected);
   }
   KVParticle& operator=(const KVParticle& rhs);

   const Char_t* GetName() const;
   void SetName(Name);

   void AddGroup(Name group, Name parent = "");
   void AddGroup(Name group, KVParticleCondition*);

   Bool_t BelongsToGroup(Name group) const;
   void RemoveGroup(Name groupname);
   void RemoveAllGroups();
   void ListGroups(void) const;

   void SetFrame(Name frame, Velocity boost, Bool_t beta = kFALSE);
   void SetFrame(Name frame, const TLorentzRotation& rot);
   void SetFrame(Name frame, const TRotation& rot);
   void SetFrame(Name frame, Velocity boost, TRotation& rot, Bool_t beta = kFALSE);
   void SetFrame(Name newframe, Name oldframe, Velocity boost, Bool_t beta = kFALSE);
   void SetFrame(Name newframe, Name oldframe, const TLorentzRotation& rot);
   void SetFrame(Name newframe, Name oldframe, const TRotation& rot);
   void SetFrame(Name newframe, Name oldframe, Velocity boost, TRotation& rot, Bool_t beta = kFALSE);

   KVParticle* GetFrame(Name frame);

   Name GetFrameName(void) const
   {
      return fFrameName;
   }
   void SetFrameName(Name framename)
   {
      fFrameName = framename->Data();
   }

   KVNameValueList* GetParameters() const
   {
      return (KVNameValueList*)&fParameters;
   }
   template<typename ValType> void SetParameter(const Char_t* name, ValType value) const
   {
      GetParameters()->SetValue(name, value);
   }

   ClassDef(KVParticle, 9)      //General base class for all massive particles
};

#endif
