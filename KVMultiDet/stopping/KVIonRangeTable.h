//Created by KVClassFactory on Thu Feb  3 10:04:41 2011
//Author: frankland,,,,

#ifndef KVIONRANGETABLE_H
#define KVIONRANGETABLE_H

#include "KVBase.h"
#include "KVUnits.h"

/**
 \class KVIonRangeTable
 \ingroup Stopping
 \brief Abstract base class for calculation of range & energy loss of charged particles in matter
 */
class TGeoMaterial;
class KVIonRangeTableMaterial;
class TGeoManager;
class TVector3;

class KVIonRangeTable : public KVBase {

protected:
   virtual KVIonRangeTableMaterial* GetMaterialWithPointer(TGeoMaterial*) const;
   virtual KVIonRangeTableMaterial* GetMaterialWithNameOrType(const Char_t* material) const = 0;

public:
   enum SolType {
      kEmax,
      kEmin
   };

   KVIonRangeTable(const Char_t* name = "", const Char_t* title = "");
   virtual ~KVIonRangeTable();

   static KVIonRangeTable* GetRangeTable(const Char_t* name);

   virtual KVIonRangeTableMaterial* AddElementalMaterial(Int_t z, Int_t a = 0) const
   {
      // Adds a material composed of a single chemical element.
      //
      // \param[in] z atomic number of element \f$Z\f$
      // \param[in] a [optional] mass number of isotope \f$A\f$
      //
      // If the isotope \a a is not specified, we create a material containing the naturally
      // occuring isotopes of the given element, weighted according to their natural abundances.
      //
      // If the element name is "X", this material will be called "natX", for "naturally-occuring X".

      IGNORE_UNUSED(z);
      IGNORE_UNUSED(a);
      return nullptr;
   }
   virtual KVIonRangeTableMaterial* AddCompoundMaterial(
      const Char_t* name, const Char_t* symbol,
      Int_t nelem, Int_t* z, Int_t* a, Int_t* natoms, Double_t density = -1.0) const
   {
      // Adds a compound material with a simple formula composed of different elements
      //
      // \param[in] name name for the new compound (no spaces)
      // \param[in] symbol chemical symbol for compound
      // \param[in] nelem number of elements in compound
      // \param[in] z[nelem] atomic numbers of elements
      // \param[in] a[nelem] mass numbers of elements
      // \param[in] natoms[nelem] number of atoms of each element
      // \param[in] density in \f$g/cm^{3}\f$, only required if compound is a solid

      IGNORE_UNUSED(name);
      IGNORE_UNUSED(symbol);
      IGNORE_UNUSED(nelem);
      IGNORE_UNUSED(z);
      IGNORE_UNUSED(a);
      IGNORE_UNUSED(natoms);
      IGNORE_UNUSED(density);
      return nullptr;
   }
   virtual KVIonRangeTableMaterial* AddMixedMaterial(
      const Char_t*  name, const Char_t*  symbol,
      Int_t  nelem, Int_t*  z, Int_t*  a, Int_t*  natoms, Double_t*  proportion,
      Double_t  density  = -1.0) const
   {
      // Adds a material which is a mixture of either elements or compounds:
      //
      // \param[in] name name for the new mixture (no spaces)
      // \param[in] symbol chemical symbol for mixture
      // \param[in] nelem number of elements in mixture
      // \param[in] z[nelem] atomic numbers of elements
      // \param[in] a[nelem] mass numbers of elements
      // \param[in] natoms[nelem] number of atoms of each element
      // \param[in] proportion[nelem] proportion by mass in mixture of element
      // \param[in] density in \f$g/cm^{3}\f$, if mixture is a solid

      IGNORE_UNUSED(name);
      IGNORE_UNUSED(symbol);
      IGNORE_UNUSED(nelem);
      IGNORE_UNUSED(z);
      IGNORE_UNUSED(a);
      IGNORE_UNUSED(natoms);
      IGNORE_UNUSED(proportion);
      IGNORE_UNUSED(density);
      return nullptr;
   }

   // Create and fill a list of all materials for which range tables exist.
   // Each entry is a TNamed with the name and type (title) of the material.
   virtual TObjArray* GetListOfMaterials() = 0;

   // Create and return pointer to TGeoMaterial/Mixture corresponding to material
   virtual TGeoMaterial* GetTGeoMaterial(const Char_t* material);

   // Return maximum incident energy (in MeV) for which range table is valid for given
   // material and (Z,A) of incident ion
   virtual Double_t GetEmaxValid(const Char_t* material, Int_t Z, Int_t A);

   // Returns density (g/cm**3) of a material in the range tables
   virtual Double_t GetDensity(const Char_t*);

   // Set temperature (in degrees celsius) and pressure (in torr) for a given material.
   // This usually has no effect except for gaseous materials, for which T & P determine the density (in g/cm**3).
   virtual void SetTemperatureAndPressure(const Char_t*, Double_t temperature, Double_t pressure);

   // Returns atomic number of a material in the range tables
   virtual Double_t GetZ(const Char_t*);

   // Returns atomic mass of a material in the range tables
   virtual Double_t GetAtomicMass(const Char_t*);

   // Returns pointer to material of given name or type.
   KVIonRangeTableMaterial* GetMaterial(const Char_t* material) const;
   KVIonRangeTableMaterial* GetMaterial(TGeoMaterial*) const;

   // Return kTRUE if material is in range tables
   virtual Bool_t IsMaterialKnown(const Char_t*);

   virtual Bool_t IsMaterialKnown(TGeoMaterial*);

   // Return kTRUE if material is gaseous
   virtual Bool_t IsMaterialGas(const Char_t*);

   // Return name of material of given type or name if it is in range tables
   const Char_t* GetMaterialName(const Char_t*);

   // Return type of material of given type or name if it is in range tables
   const Char_t* GetMaterialType(const Char_t*);

   // Returns range (in g/cm**2) of ion (Z,A) with energy E (MeV) in material.
   // Give Amat to change default (isotopic) mass of material,
   // give temperature (degrees C) & pressure (torr) (T,P) for gaseous materials.
   virtual Double_t GetRangeOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t E,
                                  Double_t Amat = 0., Double_t T = -1., Double_t P = -1.);

   // Returns linear range (in cm) of ion (Z,A) with energy E (MeV) in material.
   // Give Amat to change default (isotopic) mass of material,
   // give temperature (degrees C) & pressure (torr) (T,P) for gaseous materials.
   virtual Double_t GetLinearRangeOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t E,
                                        Double_t Amat = 0., Double_t T = -1., Double_t P = -1.);

   // Returns energy lost (in MeV) by ion (Z,A) with energy E (MeV) after thickness r (in g/cm**2).
   // Give Amat to change default (isotopic) mass of material,
   // give temperature (degrees C) & pressure (torr) (T,P) for gaseous materials.
   virtual Double_t GetDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t E, Double_t r,
                                   Double_t Amat = 0., Double_t T = -1., Double_t P = -1.);

   // Returns energy lost (in MeV) by ion (Z,A) with energy E (MeV) after thickness d (in cm).
   // Give Amat to change default (isotopic) mass of material,
   // give temperature (degrees C) & pressure (torr) (T,P) for gaseous materials.
   virtual Double_t GetLinearDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t E, Double_t d,
                                         Double_t Amat = 0., Double_t T = -1., Double_t P = -1.);

   virtual Double_t GetEResOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t E, Double_t r,
                                 Double_t Amat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetLinearEResOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t E, Double_t d,
                                       Double_t Amat = 0., Double_t T = -1., Double_t P = -1.);

   virtual Double_t GetEIncFromEResOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t Eres, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetLinearEIncFromEResOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t Eres, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);

   virtual Double_t GetEIncFromDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t DeltaE, Double_t e, enum SolType type = kEmax, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetLinearEIncFromDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t DeltaE, Double_t e, enum SolType type = kEmax, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);

   virtual Double_t GetDeltaEFromEResOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t ERes, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetLinearDeltaEFromEResOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t ERes, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);

   virtual Double_t GetPunchThroughEnergy(const Char_t* mat, Int_t Z, Int_t A, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetLinearPunchThroughEnergy(const Char_t* mat, Int_t Z, Int_t A, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);

   virtual Double_t GetMaxDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetEIncOfMaxDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetLinearMaxDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);
   virtual Double_t GetLinearEIncOfMaxDeltaEOfIon(const Char_t* mat, Int_t Z, Int_t A, Double_t e, Double_t isoAmat = 0., Double_t T = -1., Double_t P = -1.);

   virtual void Print(Option_t* = "") const;

   virtual Bool_t CheckIon(Int_t, Int_t) const
   {
      AbstractMethod("CheckIon");
      return kTRUE;
   }
   virtual Bool_t ReadMaterials(const Char_t*) const = 0;

   ClassDef(KVIonRangeTable, 1) //Abstract base class for calculation of range & energy loss of charged particles in matter
};

#endif
