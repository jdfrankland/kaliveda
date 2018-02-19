#include "Riostream.h"
#include "KVReconstructedNucleus.h"
#include "KVIDTelescope.h"
#include "KVGroup.h"
#include "KVMultiDetArray.h"
#include "KVACQParam.h"
#include <KVGeoDNTrajectory.h>

using namespace std;

ClassImp(KVReconstructedNucleus);

//////////////////////////////////////////////////////////////////////////////////////////
//KVReconstructedNucleus
//
//Particles reconstructed from data measured in multidetector arrays using identification
//telescopes (see KVIDTelescope class).
//
//If the particle has been identified (IsIdentified()=kTRUE) and calibrated (IsCalibrated()=kTRUE)
//then information on its nature and kinematics can be obtained using the methods of parent
//classes KVNucleus and KVParticle (GetZ(), GetVelocity(), etc.).
//
//Information specific to the detection and identification of the particle:
//
//   GetDetectorList()         -  list of detectors passed through, in reverse order
//   GetNumDet()               -  number of detectors in list = number of detectors passed through
//   GetDetector(Int_t i)      -  ith detector in list (i=0 is stopping detector)
//   GetStoppingDetector()     -  detector in which particle stopped (first in list)
//
//   GetGroup()                -  group in which particle detected/stopped (see KVGroup)
//   GetTelescope()            -  telescope to which stopping detector belongs (see KVTelescope)
//
//   GetIdentifyingTelescope() -  ID telescope which identified this particle (see KVIDTelescope)
//      IsIdentified()               - =kTRUE if particle identification has been completed
//      IsCalibrated()             - =kTRUE if particle's energy has been set
//      GetIDTelescopes()     - list of ID telescopes through which particle passed.
//                                   this list is used to find an ID telescope which can identify the particle.
//    GetRealZ(), GetRealA()  -  return the Z and A determined with floating-point precision by the
//                               identification of the particle.
//

void KVReconstructedNucleus::init()
{
   //default initialisation
   fRealZ = fRealA = 0.;
   fDetNames = "/";
   fIDTelName = "";
   fIDTelescope = 0;
   fNSegDet = 0;
   fAnalStatus = 99;
   fTargetEnergyLoss = 0;
   ResetBit(kIsIdentified);
   ResetBit(kIsCalibrated);
   ResetBit(kCoherency);
   ResetBit(kZMeasured);
   ResetBit(kAMeasured);
}


KVReconstructedNucleus::KVReconstructedNucleus() : fIDResults("KVIdentificationResult", 5)
{
   //default ctor.
   init();
}

KVReconstructedNucleus::KVReconstructedNucleus(const KVReconstructedNucleus& obj)
   : KVNucleus(), fIDResults("KVIdentificationResult", 5)
{
   //copy ctor
   init();
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,4,0)
   obj.Copy(*this);
#else
   ((KVReconstructedNucleus&) obj).Copy(*this);
#endif
}

KVReconstructedNucleus::~KVReconstructedNucleus()
{
   //dtor
   // calls KVGroup::Reset() of the group in which this particle was detected
   if (GetGroup()) {
      GetGroup()->Reset();
   }
   init();
}

//______________________________________________________________________________

void KVReconstructedNucleus::Streamer(TBuffer& R__b)
{
   // Stream an object of class KVReconstructedNucleus.
   //Customized streamer.

   UInt_t R__s, R__c;
   if (R__b.IsReading()) {
      Version_t R__v = R__b.ReadVersion(&R__s, &R__c);
      if (R__v < 17) {
         // Before v17, fIDResults was a static array: KVIdentificationResult fIDresults[5]
         // We convert this to the new TClonesArray format
         KVNucleus::Streamer(R__b);
         fDetNames.Streamer(R__b);
         fIDTelName.Streamer(R__b);
         if (R__v >= 16) {
            R__b >> fNSegDet;
            R__b >> fAnalStatus;
         }
         R__b >> fRealZ;
         R__b >> fRealA;
         R__b >> fTargetEnergyLoss;
         int R__i;
         KVIdentificationResult id_array[5];
         for (R__i = 0; R__i < 5; R__i++) {
            id_array[R__i].Streamer(R__b);
            id_array[R__i].Copy(*GetIdentificationResult(R__i + 1));
         }
         R__b.CheckByteCount(R__s, R__c, KVReconstructedNucleus::IsA());
      } else
         R__b.ReadClassBuffer(KVReconstructedNucleus::Class(), this, R__v, R__s, R__c);
      // if the multidetector object exists, update some informations
      // concerning the detectors etc. hit by this particle
      if (gMultiDetArray) {
         MakeDetectorList();

         // Removing the leading '/' character from fDetNames gives us the title of the
         // trajectory used to reconstruct the particle
         TString traj_t = fDetNames.Strip(TString::kLeading, '/');
         //fReconTraj = GetStoppingDetector()->GetNode()->FindTrajectory(traj_t);
         if (R__v < 16) {
            ResetNSegDet();   // fNSegDet/fAnalStatus non-persistent before v.16
         }

         fIDTelescope = 0;
         if (fIDTelName != "") fIDTelescope = gMultiDetArray->GetIDTelescope(fIDTelName.Data());
         TIter next_det(&fDetList);
         KVDetector* det;
         while ((det = (KVDetector*)next_det())) {
            det->AddHit(this);
            det->SetAnalysed();
            //modify detector's counters depending on particle's identification state
            if (IsIdentified())
               det->IncrementIdentifiedParticles();
            else
               det->IncrementUnidentifiedParticles();
         }
      }
   } else {
      R__b.WriteClassBuffer(KVReconstructedNucleus::Class(), this);
   }
}

//___________________________________________________________________________

void KVReconstructedNucleus::Print(Option_t*) const
{

   int ndets = GetNumDet();
   if (ndets) {

      for (int i = ndets - 1; i >= 0; i--) {
         KVDetector* det = GetDetector(i);
         if (det) det->Print("data");
      }
      for (int i = 1; i <= GetNumberOfIdentificationResults(); i++) {
         KVIdentificationResult* idr = const_cast<KVReconstructedNucleus*>(this)->GetIdentificationResult(i);
         if (idr->IDattempted) idr->Print();
      }
   }
   if (GetStoppingDetector()) cout << "STOPPED IN : " <<
                                      GetStoppingDetector()->GetName() << endl;
   if (IsIdentified()) {
      if (GetIdentifyingTelescope()) cout << "IDENTIFIED IN : " <<
                                             GetIdentifyingTelescope()->GetName() << endl;
      cout << " =======> ";
      cout << " Z=" << GetZ() << " A=" << GetA();
      if (IsAMeasured()) cout << " Areal=" << GetRealA();
      else cout << " Zreal=" << GetRealZ();
   } else {
      cout << "(unidentified)" << endl;
   }
   if (IsCalibrated()) {
      cout << " Total Energy = " << GetEnergy() << " MeV,  Theta=" << GetTheta() << " Phi=" << GetPhi() << endl;
      cout << "    Target energy loss correction :  " << GetTargetEnergyLoss() << " MeV" << endl;
   } else {
      cout << "(uncalibrated)" << endl;
   }
   cout << "RECONSTRUCTION STATUS : " << endl;
   switch (GetStatus()) {
      case kStatusOK:
         cout <<
              "Particle alone in group, or identification independently of other"
              << endl;
         cout << "particles in group is directly possible." << endl;
         break;

      case kStatusOKafterSub:
         cout <<
              "Particle reconstructed after identification of others in group"
              << endl;
         cout <<
              "and subtraction of their calculated energy losses in common detectors."
              << endl;
         break;

      case kStatusOKafterShare:
         cout <<
              "Particle identification estimated after arbitrary sharing of"
              << endl;
         cout <<
              "energy lost in common detectors between several reconstructed particles."
              << endl;
         break;

      case kStatusStopFirstStage:
         cout <<
              "Particle stopped in first stage of telescope. Estimation of minimum Z."
              << endl;
         break;

      case kStatusPileupDE:
         cout <<
              "Undetectable pile-up in first member of identifying telesscope (apparent status=OK)."
              << endl;
         cout << "Would lead to incorrect identification by DE-E method (Z and/or A overestimated)."
              << endl;
         break;

      case kStatusPileupGhost:
         cout <<
              "Undetectable ghost particle in filtered simulation."
              << endl;
         cout << "Another particle passed through all of the same detectors (pile-up)."
              << endl;
         break;


      default:
         cout << GetStatus() << endl;
         break;
   }
   if (GetParameters()->GetNpar()) GetParameters()->Print();
}

//_______________________________________________________________________________

#if ROOT_VERSION_CODE >= ROOT_VERSION(3,4,0)
void KVReconstructedNucleus::Copy(TObject& obj) const
#else
void KVReconstructedNucleus::Copy(TObject& obj)
#endif
{
   //
   //Copy this to obj
   //
   KVNucleus::Copy(obj);
   ((KVReconstructedNucleus&) obj).SetIdentifyingTelescope(GetIdentifyingTelescope());
   ((KVReconstructedNucleus&) obj).fDetNames = fDetNames;
   ((KVReconstructedNucleus&) obj).SetRealZ(GetRealZ());
   ((KVReconstructedNucleus&) obj).SetRealA(GetRealA());
}



void KVReconstructedNucleus::Clear(Option_t* opt)
{
   //Reset nucleus. Calls KVNucleus::Clear.
   //Calls KVGroup::Reset for the group where it was reconstructed.

   KVNucleus::Clear(opt);
   if (GetGroup())
      GetGroup()->Reset();
   fDetList.Clear();
   fIDResults.Clear("C");
   init();
}

void KVReconstructedNucleus::AddDetector(KVDetector* det)
{
   //Add a detector to the list of those through which the particle passed.
   //Put reference to detector into fDetectors array, increase number of detectors by one.
   //As this is only used in initial particle reconstruction, we add 1 unidentified particle to the detector.

   //add name of detector to fDetNames
   fDetNames += det->GetName();
   fDetNames += "/";
   // store pointer to detector
   fDetList.Add(det);
   if (det->IsDetecting()) {
      //add 1 unidentified particle to the detector
      det->IncrementUnidentifiedParticles();
   }
}

//___________________________________________________________________________

void KVReconstructedNucleus::GetAnglesFromStoppingDetector(Option_t* opt)
{
   // Calculate angles theta and phi for reconstructed nucleus based
   // on detector in which it stopped*. The nucleus'
   // momentum is set using these angles, its mass and its kinetic energy.
   // The (optional) option string can be "random" or "mean".
   // If "random" (default) the angles are drawn at random between the
   // over the surface of the detector.
   // If "mean" the (theta,phi) position of the centre of the detector
   // is used to fix the nucleus' direction.
   //
   // *unless one of the detectors on the particle's trajectory to the stopping detector has
   //  a smaller solid angle, in which case we use that one

   //don't try if particle has no correctly defined energy
   if (GetEnergy() <= 0.0)
      return;
   if (!GetStoppingDetector())
      return;

   KVDetector* angle_det = GetStoppingDetector();
   Int_t ndets = GetNumDet();
   if (ndets > 1) {
      for (int id = 1; id < ndets; id++) {
         KVDetector* d = GetDetector(id);
         if (d->GetSolidAngle() < angle_det->GetSolidAngle())
            angle_det = d;
      }
   }
   if (!strcmp(opt, "random")) {
      //random angles
      TVector3 dir = angle_det->GetRandomDirection("random");
      SetMomentum(GetEnergy(), dir);
   } else {
      //middle of telescope
      TVector3 dir = angle_det->GetDirection();
      SetMomentum(GetEnergy(), dir);
   }
}

void KVReconstructedNucleus::MakeDetectorList()
{
   // Protected method, called when required to fill fDetList with pointers to
   // the detectors whose names are stored in fDetNames.
   // If gMultiDetArray=0x0, fDetList list will be empty.

   fDetList.Clear();
   if (gMultiDetArray) {
      fDetNames.Begin("/");
      while (!fDetNames.End()) {
         KVDetector* det = gMultiDetArray->GetDetector(fDetNames.Next(kTRUE));
         if (det) fDetList.Add(det);
      }
   }
}

void KVReconstructedNucleus::SetIdentification(KVIdentificationResult* idr)
{
   // Set identification of nucleus from informations in identification result object
   // The mass (A) information in KVIdentificationResult is only used if the mass
   // was measured as part of the identification. Otherwise the nucleus' mass formula
   // will be used to calculate A from the measured Z.

   SetIDCode(idr->IDcode);
   SetZMeasured(idr->Zident);
   SetAMeasured(idr->Aident);
   SetZ(idr->Z);
   if (idr->Aident) {
      SetA(idr->A);
      SetRealA(idr->PID);
   } else SetRealZ(idr->PID);
}

void KVReconstructedNucleus::SubtractEnergyFromAllDetectors()
{
   // Subtract the calculated energy loss of this particle from the measured energy
   // loss of all detectors it passed through.

   TIter nxt(GetDetectorList(), kIterBackward);
   KVDetector* det;
   Double_t Einc = GetEnergy() - GetTargetEnergyLoss(); // energy before first detector
   while ((det = (KVDetector*)nxt())) {
      Double_t Edet = det->GetEnergy();
      Double_t dE = det->GetDeltaE(GetZ(), GetA(), Einc); // calculate apparent energy loss in active layer
      Double_t Eres = det->GetERes(GetZ(), GetA(), Einc); // calculate energy after detector
      Edet -= dE;
      if (Edet < 0.1) Edet = 0.;
      det->SetEnergyLoss(Edet);
      Einc = Eres;
      if (Einc < 0.1) break;
   }
}

