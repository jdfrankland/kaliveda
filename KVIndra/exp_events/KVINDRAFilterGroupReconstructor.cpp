#include "KVINDRAFilterGroupReconstructor.h"
#include "KVINDRA.h"

#include <KVIDGCsI.h>

ClassImp(KVINDRAFilterGroupReconstructor)

KVReconstructedNucleus* KVINDRAFilterGroupReconstructor::ReconstructTrajectory(const KVGeoDNTrajectory* traj, const KVGeoDetectorNode* node)
{
   // Handle the special case of particles stopping in CsI behind an etalon telescope
   //
   // If the CsI fired, we seed a particle if:
   //  - EITHER we are on the SILI/SI75 trajectory and the SILI also fired
   //  - OR we are on the CSI/CI trajectory and the SILI did not fire

   auto d = node->GetDetector();
   if (d->Fired(GetPartSeedCond()) &&
         !d->IsAnalysed() &&
         d->IsType("CSI") &&
         node->GetNTrajForwards() > 1) {
      auto sili = GetGroup()->GetDetector(Form("SILI_%d", dynamic_cast<KVINDRADetector*>(d)->GetRingNumber()));
      auto sili_fired = sili->Fired();
      auto on_etalon_traj = traj->Contains(sili->GetName());
      if ((sili_fired && on_etalon_traj) || (!sili_fired && !on_etalon_traj)) {
         nfireddets += d->Fired(); // only count fired CsI once, when used to seed particle
         return GetEventFragment()->AddParticle();
      }
      return nullptr;
   }
   return KVFilterGroupReconstructor::ReconstructTrajectory(traj, node);
}

void KVINDRAFilterGroupReconstructor::identify_particle(KVIDTelescope* idt, KVIdentificationResult* IDR, KVReconstructedNucleus& nuc)
{
   // Treat special case of code 5 in CI-SI/SI75/CSI identifications

   KVFilterGroupReconstructor::identify_particle(idt, IDR, nuc);
   if (!IDR->IDOK && (IDR->IsType("CI_SI") || IDR->IsType("CI_CSI") || IDR->IsType("CI_SI75"))
         && IDR->IDcode == gIndra->GetIDCodeForParticlesStoppingInFirstStageOfTelescopes()) {
      IDR->IDOK = true;
      IDR->Zident = false;
      IDR->Aident = false;
   }
}

void KVINDRAFilterGroupReconstructor::Reconstruct()
{
   // If after reconstruction no particles are created in the group, we check if the CI (if present)
   // fired and if so add a particle stopped in the CI.

   KVFilterGroupReconstructor::Reconstruct();

   if (GetEventFragment()->GetMult() == 0) {
      if (theChIo && theChIo->Fired(GetPartSeedCond())) {
         auto kvdp = GetEventFragment()->AddParticle();
         ReconstructParticle(kvdp,
                             dynamic_cast<KVGeoDNTrajectory*>(theChIo->GetNode()->GetTrajectories()->First()),
                             theChIo->GetNode());
         AnalyseParticles();
         PostReconstructionProcessing();
      }
   }
}

void KVINDRAFilterGroupReconstructor::IdentifyParticle(KVReconstructedNucleus& PART)
{
   //UNIDENTIFIED PARTICLES
   //Unidentified particles receive the general ID code for non-identified particles (kIDCode14)
   //EXCEPT if their CsI identification was attempted but they are too heavy (Z>5) for CsI identification
   //(Zmin) then they are relabelled "Identified" with IDcode = 9 (ident. incomplete dans CsI ou Phoswich (Z.min))
   //Their "identifying" telescope is set to the CsI ID telescope

   KVFilterGroupReconstructor::IdentifyParticle(PART);

   if (!PART.IsIdentified()) {
      /*** general ID code for non-identified particles ***/
      PART.SetIDCode(KVINDRA::IDCodes::NO_IDENTIFICATION);
      auto csirl = id_by_type.find(gIndra->GetCsIIDType().Data());
      if (csirl != id_by_type.end()) {
         if (csirl->second->IDattempted) {
            auto idt = (KVIDTelescope*)PART.GetReconstructionTrajectory()->GetIDTelescopes()->FindObjectByType(gIndra->GetCsIIDType());
            if (!idt->CanIdentify(PART.GetParameters()->GetIntValue("SIM:Z"), PART.GetParameters()->GetIntValue("SIM:A"))) {
               PART.SetIsIdentified();
               csirl->second->IDcode = KVINDRA::IDCodes::ID_CSI_FRAGMENT;
               partID = *(csirl->second);
               identifying_telescope = idt;
               PART.SetIdentification(&partID, identifying_telescope);
               PART.GetReconstructionTrajectory()->IterateFrom();
               KVGeoDetectorNode* node;
               while ((node = PART.GetReconstructionTrajectory()->GetNextNode())) {
                  --number_unidentified[node->GetName()];
               }
            }
         }
      }
   }
}
