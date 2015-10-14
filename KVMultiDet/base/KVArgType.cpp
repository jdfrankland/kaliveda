#include "KVArgType.h"

NamespaceImp(KVArgType)

namespace KVArgType {
   // Special operations
   // Nuclear properties: Z, A, N, N/Z, N-Z, N-Z/A
   ImplementNumericTypeAddition(MassNumber, NeutronNumber, AtomicNumber)
   ImplementNumericTypeSubtraction(NeutronExcess, NeutronNumber, AtomicNumber)
   ImplementNumericTypeDivision(ChargeAsymmetry, NeutronExcess, MassNumber)
   ImplementNumericTypeDivision(NOverZ, NeutronNumber, AtomicNumber)
   ImplementNumericTypeDivision(ExcitationEnergyPerNucleon, ExcitationEnergy, MassNumber)
   ImplementNumericTypeDivision(BindingEnergyPerNucleon, BindingEnergy, MassNumber)
   // Kinematics
   ImplementNumericTypeDivision(KineticEnergyPerNucleon, KineticEnergy, MassNumber)
   RelativeVelocity operator-(Velocity a, Velocity b)
   {
      return (const TVector3&)a - (const TVector3&)b;
   }
}

