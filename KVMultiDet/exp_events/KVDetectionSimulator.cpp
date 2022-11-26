//Created by KVClassFactory on Sat Oct 10 09:37:42 2015
//Author: John Frankland,,,

#include "KVDetectionSimulator.h"
#include "KVGeoNavigator.h"

ClassImp(KVDetectionSimulator)

KVDetectionSimulator::KVDetectionSimulator(KVMultiDetArray* a, Double_t e_cut_off) :
   KVBase(Form("DetectionSimulator_%s", a->GetName()),
          Form("Simulate detection of particles or events in detector array %s", a->GetTitle())),
   fArray(a), fCalcTargELoss(kTRUE)
{
   // Initialise a detection simulator
   //
   // The detector array is put into simulation mode, and the minimum cut-off energy
   // for propagation of particles is set (1 keV is default)
   SetArray(a);
}

void KVDetectionSimulator::DetectEvent(KVEvent* event, const Char_t* detection_frame)
{
   //Simulate detection of event by multidetector array.
   //
   // optional argument detection_frame(="" by default) can be used to give name of
   // inertial reference frame (defined for all particles of 'event') to be used.
   // e.g. if the simulated event's default reference frame is the centre of mass frame, before calling this method
   // you should create the 'laboratory' or 'detector' frame with KVEvent::SetFrame(...), and then give the
   // name of the 'LAB' or 'DET' frame as 3rd argument here.
   //
   //For each particle in the event we calculate first its energy loss in the target (if the target has been defined, see KVMultiDetArray::SetTarget).
   //By default these energy losses are calculated from a point half-way along the beam-direction through the target (taking into account the orientation
   //of the target), if you want random depths for each event call GetTarget()->SetRandomized() before using DetectEvent().
   //
   //If the particle escapes the target then we look for the group in the array that it will hit. If there is one, then the detection of this particle by the
   //different members of the group is simulated.
   //
   //The detectors concerned have their fEloss members set to the energy lost by the particle when it crosses them.
   //
   //Give tags to the simulated particles via KVNucleus::AddGroup() method
   //Two general tags :
   //   - DETECTED : cross at least one active layer of one detector
   //   - UNDETECTED : go through dead zone or stopped in target
   //We add also different sub group :
   //   - For UNDETECTED particles : "NO HIT", "NEUTRON", "DEAD ZONE", "STOPPED IN TARGET" and "THRESHOLD", the last one concerns particles
   //     which go through the first detection stage of the multidetector array but stopped in an absorber (ie an inactive layer)
   //   - For DETECTED particles :
   //       *  "PUNCH THROUGH" corrresponds to particle which cross all the materials in front of it
   //         (high energy particle punch through)
   //       *  "INCOMPLETE"  corresponds to particles which stopped in the first detection stage of the multidetector
   //         in a detector which can not give alone a clear identification,
   //

   fDetectionFrame = detection_frame;

   if (get_array_navigator()->IsTracking()) {
      // clear any tracks created by last event
      gGeoManager->ClearTracks();
      get_array_navigator()->ResetTrackID();
   }

   // Reset detectors in array hit by any previous events
   ClearHitGroups();

   // iterate through the particles of the event
   for (auto& part : EventIterator(event)) {
      // reference to particle in requested detection frame
      auto part_to_detect = (KVNucleus*)part.GetFrame(detection_frame, kFALSE);

      // store initial energy of particle in detection frame
      part_to_detect->SetE0();
      part.SetParameter("SIM:Z", part.GetZ());
      part.SetParameter("SIM:A", part.GetA());
      part.SetParameter("SIM:ENERGY", part_to_detect->GetE());
      part.SetParameter("SIM:THETA", part_to_detect->GetTheta());
      part.SetParameter("SIM:PHI", part_to_detect->GetPhi());

      // neutral particles & those with less than the cut-off energy are not detected
      if ((part.GetZ() == 0) && !get_array_navigator()->IsTracking()) {
         // neutrons are included in tracking, if active
         part.SetParameter("UNDETECTED", "NEUTRON");
      }
      else if (part.GetZ() && !get_array_navigator()->CheckIonForRangeTable(part.GetZ(), part.GetA())) {
         // ignore charged particles which range table cannot handle
         part.SetParameter("UNDETECTED", "NOT IN RANGE TABLE");
      }
      else if (!fGeoFilter && (part_to_detect->GetEnergy() < GetMinKECutOff())) {
         part.SetParameter("UNDETECTED", "AT REST");
      }
      else {
         if (IncludeTargetEnergyLoss() && GetTarget() && part.GetZ()) {
            //simulate passage through target material
            auto ebef = part_to_detect->GetE();
            GetTarget()->DetectParticle(part_to_detect);
            auto eLostInTarget = ebef - part_to_detect->GetE();
            part.SetParameter("ENERGY LOSS IN TARGET", eLostInTarget);
            if (part_to_detect->GetE() < GetMinKECutOff())
               part.SetParameter("UNDETECTED", "STOPPED IN TARGET");
         }

         if (fGeoFilter || (part_to_detect->GetE() > GetMinKECutOff())) {

            auto nvl = PropagateParticle(part_to_detect);

            if (nvl.IsEmpty()) {
               if (part.GetZ() == 0) {
                  // tracking
                  part.SetParameter("UNDETECTED", "NEUTRON");
               }
               else {
                  if (part.GetParameters()->HasParameter("DEADZONE")) {
                     // deadzone
                     part.SetParameter("UNDETECTED", "DEAD ZONE");
                  }
                  else {
                     // missed all detectors
                     part.SetParameter("UNDETECTED", "NO HIT");
                  }
               }
            }
            else {
               // check for incomplete stopping of particle
               // note that energy losses are not calculated in DEADZONE volume; as soon as a particle enters
               // a DEADZONE volume its propagation stops. therefore particles which lose energy in one or more
               // active material volumes and then hit a DEADZONE may have residual kinetic energy, but not
               // because they crossed all detector layers without losing all their energy, which is the meaning of "DETECTED=PUNCH THROUGH"
               if (!part.GetParameters()->HasParameter("DEADZONE") && part_to_detect->GetE() > GetMinKECutOff()) {
                  part.SetParameter("RESIDUAL ENERGY", part_to_detect->GetE());
                  part.SetParameter("DETECTED", "PUNCH THROUGH");
               }
               else
                  part.SetParameter("DETECTED", "OK");
            }

            if (!nvl.IsEmpty()) {
               for (Int_t ii = 0; ii < nvl.GetNpar(); ++ii) {
                  part.SetParameter(nvl.GetNameAt(ii), nvl.GetDoubleValue(ii));
               }
            }

         }
      }

      part_to_detect->SetMomentum(part_to_detect->GetPInitial());
   }

}

//__________________________________________________________________________________

KVNameValueList KVDetectionSimulator::PropagateParticle(KVNucleus* part)
{
   // Simulate detection of a single particle
   //
   // Propagate particle through the array,
   // calculating its energy losses in all absorbers, and setting the
   // energy loss members of the active detectors on the way.
   //
   // Returns a list containing the name and energy loss of each
   // detector hit in array (list is empty if none i.e. particle
   // in beam pipe or dead zone of the multidetector)

   auto nparams = part->GetParameters()->GetNpar();

   get_array_navigator()->PropagateParticle(part);

   // particle missed all detectors
   if (part->GetParameters()->GetNpar() == nparams ||
         ((part->GetParameters()->GetNpar() - nparams) == 1 && part->GetParameters()->HasParameter("DEADZONE")))
      return KVNameValueList();

   // list of energy losses in active layers of detectors
   KVNameValueList NVL;

   // find detectors in array hit by particle
   KVDetector* last_detector = nullptr;
   TIter next(part->GetParameters()->GetList());
   KVNamedParameter* param;
   while ((param = (KVNamedParameter*)next())) {
      KVString pname(param->GetName());
      pname.Begin(":");
      KVString pn2 = pname.Next();
      KVString pn3 = pname.Next();
      if (pn2 == "DE") {
         pn3.Begin("/");
         KVString det_name = pn3.Next();
         if (pn3.End() || pn3.Next().BeginsWith("ACTIVE")) {
            // energy loss in active layer of detector
            KVDetector* curDet = fArray->GetDetector(det_name);
            if (curDet) {
               last_detector = curDet;
               Double_t de = param->GetDouble();
               NVL.SetValue(curDet->GetName(), de);
               // add detector to name of trajectory followed by particle
               TString traj;
               if (part->GetParameters()->HasStringParameter("TRAJECTORY")) {
                  traj = part->GetParameters()->GetStringValue("TRAJECTORY");
                  traj.Prepend(Form("%s/", det_name.Data()));
               }
               else {
                  traj = Form("%s/", det_name.Data());
               }
               part->SetParameter("TRAJECTORY", traj);
               // set number of group where detected
               part->SetParameter("GROUP", (int)curDet->GetGroupNumber());
            }
         }
      }
   }
   // add hit group to list if not already in it
   if (last_detector) {
      fHitGroups.AddGroup(last_detector->GetGroup());
      part->SetParameter("STOPPING DETECTOR", last_detector->GetName());
   }

   return NVL;
}

