/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "KVDBSystem.h"
#include "KV2Body.h"
#include "KVNumberList.h"
#include "KVUnits.h"
#include "TROOT.h"

using namespace std;

ClassImp(KVDBSystem);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      KVDBSystem
//
//      Database class used to store information on different colliding systems studied during an experiment.
//      A 'system' is a unique combination of projectile, target and beam energy.
//
//      Informations on Z & A of projectile and target and beam energy are here (GetZbeam(), GetZtarget(), etc.)
//
//      GetTarget() returns a pointer to a KVTarget object describing the physical characteristics of the
//      target for this system : such an object can be used to calculate energy losses of incident charged
//      particles etc.
//      NOTE: if the current run being analysed corresponds to this system, the target of INDRA will have
//      been set using this method. i.e. you can use gIndra->GetTarget() to obtain the KVTarget object of the current
//      run.
//
//      GetKinematics() returns a pointer to a KV2Body object which can be used to obtain information on the kinematics
//      of the colliding system. See the KV2Body class for more details.
//      Some of the more commonly used kinematical quantities are also available directly here: GetPtot(), GetZVtot(),
//      GetECM() etc.

//___________________________________________________________________________
KVDBSystem::KVDBSystem()
{
   fZtarget = fAtarget = fZbeam = fAbeam = 0;
   fEbeam = 0.;
   fCinema = 0;
   fTarget = 0;
   fRunlist = 0;
   fRuns = 0;
}

//___________________________________________________________________________
KVDBSystem::KVDBSystem(const Char_t* name): KVBase(name,
         "Physical System")
{
   fZtarget = fAtarget = fZbeam = fAbeam = 0;
   fEbeam = 0.;
   fCinema = 0;
   fTarget = 0;
   fRuns = 0;
}

//____________________________________________________________________________
KVDBSystem::~KVDBSystem()
{
   //Delete kinematics, target and associated runlist if they exist
   if (fCinema) {
      delete fCinema;
      fCinema = 0;
   }
   delete fTarget;
}

//___________________________________________________________________________

KV2Body* KVDBSystem::GetKinematics()
{
   // Create (if it doesn't already exist) and return pointer to a KV2Body object initialised
   // with the entrance channel corresponding to this system. Use this to obtain information
   // such as the recoil velocity of the CM, available energy, etc. (see KV2Body).
   //
   // If no projectile and/or target are defined for the system, we return 0x0.

   if (GetZbeam()*GetZtarget() == 0) return 0;

   if (!fCinema) {
      fCinema = new KV2Body();
      fCinema->SetProjectile(GetZbeam(), GetAbeam());
      fCinema->SetTarget(GetZtarget(), GetAtarget());
      fCinema->GetNucleus(1)->SetEnergy(GetEbeam() * GetAbeam());
      fCinema->CalculateKinematics();
   }
   return fCinema;
}
//___________________________________________________________________________

Bool_t KVDBSystem::IsCollision() const
{
   //retourne kTRUE, si le systeme est une collision ie projectile+cible
   return (GetZbeam() * GetZtarget() != 0);

}

//_____________________________________________________________________________

Double_t KVDBSystem::GetZVtot() const
{
   //Returns product of atomic number and velocity component parallel to beam axis of projectile nucleus in laboratory frame
   //Units are cm/ns (velocity units)
   KV2Body* kin = const_cast < KVDBSystem* >(this)->GetKinematics();
   if (!kin) return 0.;
   return (fZbeam * kin->GetNucleus(1)->GetVpar());
}

//_____________________________________________________________________________

Double_t KVDBSystem::GetPtot() const
{
   //Returns momentum component parallel to beam axis of projectile nucleus in laboratory frame
   //Units are MeV/c
   KV2Body* kin = const_cast < KVDBSystem* >(this)->GetKinematics();
   if (!kin) return 0.;
   return (kin->GetNucleus(1)->GetMomentum().Z());
}

//_____________________________________________________________________________

Double_t KVDBSystem::GetEtot() const
{
   //Returns total (mass + kinetic) energy of entrance channel corresponding to system
   //Units are MeV
   KV2Body* kin = const_cast < KVDBSystem* >(this)->GetKinematics();
   if (!kin) return 0.;
   return (kin->GetNucleus(1)->E() + kin->GetNucleus(2)->E());
}

//_____________________________________________________________________________

Double_t KVDBSystem::GetECM() const
{
   //Returns total available (CM) kinetic energy of entrance channel corresponding to system
   //Units are MeV
   KV2Body* kin = const_cast < KVDBSystem* >(this)->GetKinematics();
   if (!kin) return 0.;
   return (kin->GetCMEnergy());
}

//_____________________________________________________________________________

Int_t KVDBSystem::Compare(const TObject* obj) const
{
   //Function used to sort lists of systems.
   //Systems are sorted according to the number of the first run in the
   //(sorted) list of runs associated to the system.
   //Systems with lower first run numbers appear earlier in the list.

   KVDBSystem* other_sys =
      dynamic_cast < KVDBSystem* >(const_cast < TObject* >(obj));
   if (!other_sys)
      return 0;
   Int_t first = fRunlist.First();
   Int_t other_first = other_sys->GetRunList().First();
   return (first == other_first ? 0 : (other_first > first ? -1 : 1));
}

//___________________________________________________________________________

void KVDBSystem::Save(ostream& f) const
{
   //Write informations on system in the format used in Systems.dat files:
   //
   //+155Gd + 238U 36 MeV/A   '+'  followed by name of system
   //155 64 238 92 36.0    Aproj Zproj Atarg Ztarg Ebeam
   //Target: 3 0.0                    target with 3 layers, angle 0 degrees
   //C 0.02                            1st layer : carbon, 20 g/cm2
   //238U 0.1                          2nd layer : uranium-238, 100 g/cm2
   //C 0.023                           3rd layer : carbon, 23 g/cm2
   //Runs: 770-804             list of runs in KVNumberList format

   f << "+" << GetName() << endl;
   if (fZbeam) f << fAbeam << " " << fZbeam << " " << fAtarget << " " << fZtarget << " " << fEbeam << endl;
   if (fTarget) {
      f << "Target: " << fTarget->NumberOfLayers() << " " <<  fTarget->GetAngleToBeam() << endl;
      TIter next(fTarget->GetLayers());
      KVMaterial* lay;
      while ((lay = (KVMaterial*)next())) {
         if (lay->IsIsotopic()) f << Form("%d%s", (Int_t)lay->GetMass(), lay->GetType());
         else f << lay->GetType();
         f << " " << lay->GetAreaDensity() / KVUnits::mg << endl;
      }
   }
   f << "Runs: " << fRunlist.AsString() << endl;
}

//___________________________________________________________________________

void KVDBSystem::Load(istream& f)
{
   //Read and set informations on system in the format used in Systems.dat files:
   //
   //+155Gd + 238U 36 MeV/A   '+'  followed by name of system
   //155 64 238 92 36.0    Aproj Zproj Atarg Ztarg Ebeam
   //Target: 3 0.0                    target with 3 layers, angle 0 degrees
   //C 0.02                            1st layer : carbon, 20 ug/cm2
   //238U 0.1                          2nd layer : uranium-238, 100 ug/cm2
   //C 0.023                           3rd layer : carbon, 23 ug/cm2
   //Runs: 770-804             list of runs in KVNumberList format

   TString line;
   Float_t target_thickness;
   fAbeam = fZbeam = fAtarget = fZtarget = 0;
   fEbeam = target_thickness = 0;
   fRunlist.Clear();
   //'peek' at first character of next line
   char next_char = f.peek();
   if (next_char == '+') {
      line.ReadLine(f, kFALSE);
      line.Remove(0, 1);
      SetName(line.Data());
      cout << "New System : " << line.Data() << endl;
   } else {
      Error("Load", "Should read system name : %s\n", line.Data());
      return;
   }
   next_char = f.peek();
   while (next_char != '+' && f.good() && !f.eof()) {
      if ((next_char >= '0') && (next_char <= '9')) {
         line.ReadLine(f, kFALSE);
         if (sscanf(line.Data(), "%u %u %u %u %f %f", &fAbeam, &fZbeam, &fAtarget, &fZtarget, &target_thickness, &fEbeam) == 6) {
            cout << "Zproj = " << fZbeam << " Ztarg = " << fZtarget << " targ_thick = " << target_thickness << " Ebeam = " << fEbeam << endl;
         } else if (sscanf(line.Data(), "%u %u %u %u %f", &fAbeam, &fZbeam, &fAtarget, &fZtarget, &fEbeam) == 5) {
            cout << "Zproj = " << fZbeam << " Ztarg = " << fZtarget << " Ebeam = " << fEbeam << endl;
         }
      } else {
         line.ReadLine(f, kFALSE);
         if (line.BeginsWith("Target")) {
            fTarget = new KVTarget;
            line.Remove(0, line.Index(":") + 1);
            Int_t nlay;
            Float_t angle;
            sscanf(line.Data(), "%d %f", &nlay, &angle);
            Char_t mat[10];
            Float_t thick;
            for (int i = 0; i < nlay; i++) {
               line.ReadLine(f);
               sscanf(line.Data(), "%s %f", mat, &thick);
               fTarget->AddLayer(mat, thick);
            }
            fTarget->SetAngleToBeam(angle);
            fTarget->Print();
         } else if (line.BeginsWith("Runs")) {
            line.Remove(0, line.Index(":") + 1);
            fRunlist.SetList(line.Data());
            cout << "Runs : " << line.Data() << endl;
         } else if (line.BeginsWith("Run Range")) {
            line.Remove(0, line.Index(":") + 1);
            Int_t frun, lrun;
            sscanf(line.Data(), "%i %i", &frun, &lrun);
            fRunlist.Add(Form("%i-%i", frun, lrun));
            cout << "Run range : " << line.Data() << endl;
         }
      }
      next_char = f.peek();
   }
   //set target if not already done (old versions)
   if (!fTarget && target_thickness > 0 && fZtarget > 0) {
      KVNucleus n(fZtarget, fAtarget);
      fTarget = new KVTarget(n.GetSymbol(), target_thickness);
      fTarget->Print();
   }
}


void KVDBSystem::AddRun(Int_t run)
{
   //Associate the given run with this system.
   fRunlist.Add(run);
}

//___________________________________________________________________________

void KVDBSystem::RemoveRun(Int_t run)
{
   //Unassociate the given run from this system.
   fRunlist.Remove(run);
}

//___________________________________________________________________________

void KVDBSystem::RemoveAllRuns()
{
   //Unassociate all runs from this system.
   fRunlist.Clear();
}

void KVDBSystem::Print(Option_t*) const
{
   cout << "________________________________________________________" <<
        endl << "System : " << GetName() << endl;
   cout << "Runs : " << fRunlist.AsString() << endl;
   cout << "  Zbeam : " << fZbeam
        << endl << "  Abeam : " << fAbeam << endl << "  Ebeam : " << fEbeam
        << " A.MeV" << endl << "  Ztarget : " << fZtarget << endl <<
        "  Atarget : " << fAtarget << endl << "  Target Thickness : " <<
        const_cast <
        KVDBSystem*
        >(this)->
        GetTargetThickness() << " mg/cm2" << endl <<
        "________________________________________________________" << endl;

}

void KVDBSystem::ls(Option_t*) const
{
   cout << "KVDBSystem : " << GetName() << " Runs : " << fRunlist.AsString() << endl;
}

const Char_t* KVDBSystem::GetBatchName()
{
   // Gives name of system in compact form with all (unix-)illegal characters
   // replaced by '_'. Can be used for naming batch jobs, files, etc.

   static KVString tmp;
   tmp = "";
   if (GetKinematics()) {
      if (GetKinematics()->GetNucleus(1)) {
         tmp = GetKinematics()->GetNucleus(1)->GetSymbol();
      }
      if (GetKinematics()->GetNucleus(2)) {
         tmp += GetKinematics()->GetNucleus(2)->GetSymbol();
      }
      if (GetEbeam() > 0) {
         tmp += TMath::Nint(GetEbeam());
      }
   }
   if (tmp == "") {
      tmp = GetName();
      tmp.ReplaceAll(" ", "_");
      tmp.ReplaceAll("/", "_");
   }
   return tmp.Data();
}

