#include "KVINDRAFilterGroupReconstructor.h"
#include "KVINDRA.h"

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
