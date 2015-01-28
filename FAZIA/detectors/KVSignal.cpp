//Created by KVClassFactory on Mon Dec 22 15:46:46 2014
//Author: ,,,

#include "KVSignal.h"
#include "KVString.h"

ClassImp(KVSignal)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVSignal</h2>
<h4>simple class to store TArray in a list</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVSignal::KVSignal()
{
   // Default constructor
   fYmin = fYmax = 0;
}

//________________________________________________________________

KVSignal::KVSignal(const char* name, const char* title) : TGraph()
{
   // Write your code here
   fYmin = fYmax = 0;
   SetNameTitle(name,title);
}

//________________________________________________________________

KVSignal::KVSignal(const TString& name, const TString& title) : TGraph()
{
   // Write your code here
   fYmin = fYmax = 0;
   SetNameTitle(name,title);
}

KVSignal::~KVSignal()
{
   // Destructor
}

//________________________________________________________________

void KVSignal::Copy(TObject& obj) const
{
   // This method copies the current state of 'this' object into 'obj'
   // You should add here any member variables, for example:
   //    (supposing a member variable KVSignal::fToto)
   //    CastedObj.fToto = fToto;
   // or
   //    CastedObj.SetToto( GetToto() );

   //TGraph::Copy((TGraph&)obj);
   //KVSignal& CastedObj = (KVSignal&)obj;
}

//________________________________________________________________

void KVSignal::SetData(Int_t nn, Double_t* xx, Double_t* yy)
{
	Set(nn);
   if (nn==0) return;
   Int_t np=0;
   fYmin=fYmax=yy[np];
   for (np=1;np<nn;np+=1){
   	SetPoint(np,xx[np],yy[np]);
      if (yy[np]<fYmin) fYmin = yy[np];
      if (yy[np]>fYmax) fYmax = yy[np];
	}
}

//________________________________________________________________
void KVSignal::DeduceFromName()
{

	fBlock = fQuartet = fTelescope = -1;
   fType = fDet = fTelName = fQuartetName = "";
	KVString tmp = GetName();
   KVString part = "";
   if (tmp.BeginsWith("B"))
   {
   	//FAZIA telescope denomination
   	tmp.Begin("-");
      part = tmp.Next(); part.ReplaceAll("B",""); fBlock = part.Atoi();
		part = tmp.Next(); part.ReplaceAll("Q",""); fQuartet = part.Atoi();
		part = tmp.Next(); part.ReplaceAll("T",""); fTelescope = part.Atoi();
		fType = tmp.Next();
      fDet = GetTitle(); fDet.ToUpper();
      
      fDetName.Form("%s-T%d-Q%d-B%03d",fDet.Data(),fTelescope,fQuartet,fBlock);
		fTelName.Form("B%03d-Q%d-T%d",fBlock,fQuartet,fTelescope);
     fQuartetName.Form("B%03d-Q%d",fBlock,fQuartet);
   }
   
}
//________________________________________________________________
void KVSignal::Print(Option_t* chopt) const
{
	Info("Print","\nName: %s - Title: %s",GetName(),GetTitle());
	if (fBlock!=-1){
   	printf("\tBlock# %d - Quartet# %d - Telescope# %d\n",fBlock,fQuartet,fTelescope);
   	printf("\tType: %s - Detecteur: %s\n",fType.Data(),fDet.Data());
   }
}
//________________________________________________________________

KVPSAResult* KVSignal::TreateSignal(void)
{
	//to be implemented in child class
   Info("TreateSignal","To be implemented in child classes");
	return 0;
}

//________________________________________________________________

void KVSignal::ComputeGlobals(void)
{
	Double_t xx,yy;
   Int_t np=0;
   GetPoint(np++,xx,yy);
   fYmin=fYmax=yy;
   
   for (np=1;np<GetN();np+=1){
   	GetPoint(np,xx,yy);
      if (yy<fYmin) fYmin = yy;
      if (yy>fYmax) fYmax = yy;
   }
}
