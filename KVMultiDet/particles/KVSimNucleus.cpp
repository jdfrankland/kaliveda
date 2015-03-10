//Created by KVClassFactory on Mon Jun 28 15:02:00 2010
//Author: bonnet

#include "KVSimNucleus.h"

ClassImp(KVSimNucleus)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVSimNucleus</h2>
<h4>Classe d�riv�e de KVNucleus pour g�rer des simulations dans KaliVeda</h4>
<!-- */
// --> END_HTML
//Deux TVector3 sont ajout�s en champs de la classe : 
// - le vecteur position
// - le vecteur moment angulaire
//
//Cette classe est coupl�e � KVSimEvent
////////////////////////////////////////////////////////////////////////////////

//___________________________
KVSimNucleus::KVSimNucleus()
{
	// Default
}
	
//________________________________________________________________

KVSimNucleus::KVSimNucleus(Int_t z, Int_t a, Double_t ekin) : KVNucleus(z, a, ekin)
{
   // Z, A, and kinetic energy (MeV)
	// See KVNucleus(Int_t z, Int_t a = 0, Double_t ekin = 0)
}

//________________________________________________________________

KVSimNucleus::KVSimNucleus(Int_t z, Double_t t, TVector3& p) : KVNucleus(z, t, p)
{
   // Z, kinetic energy (MeV), and unit direction
	// See KVNucleus(Int_t z, Double_t t, TVector3 & p)
}

//________________________________________________________________

KVSimNucleus::KVSimNucleus(Int_t z, Int_t a, TVector3 p) : KVNucleus(z, a, p)
{
   // Z, A, and momentum (MeV/c)
	// See KVNucleus(Int_t z, Int_t a, TVector3 p)
}

//________________________________________________________________

KVSimNucleus::KVSimNucleus(const Char_t* sym) : KVNucleus(sym)
{
   // Create a nucleus defined by symbol e.g. "12C", "34Mg", "42Si" etc. etc.
	// See KVNucleus(const Char_t *)
}

//___________________________
KVSimNucleus::~KVSimNucleus()
{
}

//___________________________
void KVSimNucleus::Copy(TObject& obj) const
{

	//Copy l'object "this" vers obj
	
	KVNucleus::Copy(obj);	
	((KVSimNucleus &)obj).position = position;
	((KVSimNucleus &)obj).angmom = angmom;
}

//___________________________
void KVSimNucleus::SetPosition(Double_t rx, Double_t ry, Double_t rz)
{
	//set the position of the nucleus in position space
	position.SetXYZ(rx,ry,rz);
}

//___________________________
const TVector3* KVSimNucleus::GetPosition() const 
{
	//return the position of the nucleus as TVector3
	return &position; 
}

//___________________________
void KVSimNucleus::SetAngMom(Double_t lx, Double_t ly, Double_t lz)
{
	//set the angular momentum of the nucleus
	angmom.SetXYZ(lx,ly,lz);
}

//___________________________
const TVector3* KVSimNucleus::GetAngMom() const
{
	//return the angular momentum of the nucleus as TVector3
	return &angmom;
}