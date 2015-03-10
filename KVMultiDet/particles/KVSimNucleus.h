//Created by KVClassFactory on Mon Jun 28 15:02:00 2010
//Author: bonnet

#ifndef __KVSIMNUCLEUS_H
#define __KVSIMNUCLEUS_H

#include "KVNucleus.h"
#include "KVNameValueList.h"

class TVector3;

class KVSimNucleus : public KVNucleus
{

	protected:
	TVector3 position; 	// vector position of the particle in fm
	TVector3 angmom; 	// angular momentum of the particle in ???
	
	public:
	
	KVSimNucleus();
   KVSimNucleus(Int_t z, Int_t a = 0, Double_t ekin = 0);
   KVSimNucleus(Int_t z, Double_t t, TVector3& p);
   KVSimNucleus(Int_t z, Int_t a, TVector3 p);
   KVSimNucleus(const Char_t* sym);
	virtual ~KVSimNucleus();
	
	void Copy(TObject& obj) const;
	
	void SetPosition(Double_t rx, Double_t ry, Double_t rz);
	const TVector3* GetPosition() const;
	
	void SetAngMom(Double_t lx, Double_t ly, Double_t lz);
	const TVector3* GetAngMom() const;
	
	ClassDef(KVSimNucleus,3)//Nuclear particle in a simulated event

};

#endif