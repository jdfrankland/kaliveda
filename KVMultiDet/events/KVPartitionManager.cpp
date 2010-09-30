//Created by KVClassFactory on Thu Apr  1 14:11:10 2010
//Author: bonnet

#include "KVPartitionManager.h"
#include "KVString.h"
#include "KVIntegerList.h"
#include "TMethodCall.h"
#include "TMath.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TTree.h"
#include "KVNameValueList.h"
#include "TStopwatch.h"
#include "TArrayI.h"

ClassImp(KVPartitionManager)

////////////////////////////////////////////////////////////////////////////////
/*
BEGIN_HTML
<h2>KVPartitionManager</h2>
<h4>Permet d'enregistrer, de classer et compter des partitions d'entiers via la classe KVIntegerList</h4>
END_HTML
Classe heritant de KVList, elle est decomposee en sous listes afin de rendre le traitement d'un grand nombre de partitions
plus rapides "diviser pour mieux regner" http://fr.wikipedia.org/wiki/Diviser_pour_r�gner_(informatique)
Le nombre de partitions par sous liste est donn� par le champs Nmax (valeur par defaut 200)

Le remplissage s'effectue par la methode TestPartition(KVIntegerList* par), si "par", la partition en question
existe deja dans la liste la population de la partition existante KVIntegerList::GetPopulation() est incr�ment�
sinon on enregistre la partition "par" dans la liste

Apres un certain nombre de remplissage, on appelle la methode KVPartitionManager::ReduceSubLists()
qui compare les partitions d'une sous liste par rapport a l'autre, et qui reduit progressivement 
le nombre d'objet KVIntegerList en jouant sur la population

On peut ensuite r�effectuer une phase de remplissage et ensuite reduire etc ...
Lorsque toutes les partitions a etudier sont l�, on appelle la methode KVPartitionManager::TransfertToMainList()
qui va migrer l'ensemble des partitions enregistr�es dans les sous listes reduites vers la liste principale "this"

A partir de l� on peut : 

- creer des histos via TH1F* KVPartitionManager::GenereHisto(KVString method,Int_t nb,Double_t min,Double_t max), ou "method" est une methode 
de KVIntegerList, ex KVIntegerList::GetZ1(), KVIntegerList::GetMtot(), etc ..., cette methode ne marche que pour les methodes de KVIntegerList
ne possedant pas d'argument ou un avec une valeur par defaut
- trier les partitions suivant egalement des methodes de KVIntegerList, de mani�re croissante down=kFALSE ou decroissante (down=kTRUE)
Int_t* KVPartitionManager::GetIndex(KVString method, Bool_t down)
- creer  un arbre via la m�thode TTree* KVPartitionManager::GenereTree(KVString tree_name,Bool_t Compress,Bool_t AdditionalValue)
o� :
	- "Compress" indique si on enregistre la partition avec sa population, on si on enregistre "population" fois la partition 
	- "AdditionalValue" indique si on enregistre egalement les valeurs additionnionelles definies dans la methode KVIntegerList::CalculValeursAdditionnelles ou non

Un exemple de remplissage et de gestion de KVPartitionManager est donn� dans la classe KVBreakUp	

Ci dessous une petite routine de remplissage et de visualisation de partitions determinees aleatoirement
BEGIN_HTML
<pre>
<code>
void Test(){

KVPartitionManager* mg = new KVPartitionManager(); 
Int_t *tab; 
KVIntegerList* par = 0;
for (Int_t pp=1; pp<=10000; pp+=1){ 
	
	par = new KVIntegerList(120,4); 
	Int_t mm = TMath::Nint(gRandom->Uniform(0.9,10.1));
	
	tab = new Int_t[mm];	 
	for (Int_t nn=0; nn<mm; nn+=1){
		tab[nn] = TMath::Nint(gRandom->Uniform(0.9,60.1));
	}
	par->Fill(tab,mm);
	if (!mg->TestPartition(par)) delete par; 
	delete [] tab;

}

mg->ReduceSubLists(); 	//Reduction
mg->TransfertToMainList(); //Transfert
mg->PrintInfo();

TH1F* h1 = mg->GenereHisto("GetZ1",70,0.5,70.5); 
h1->SetLineColor(4);
h1->Draw();
 
TH1F* h2 = mg->GenereHisto("GetZ2",70,0.5,70.5); 
h2->SetLineColor(2);
h2->Draw("same");

TTree* tt = mg->GenereTree("test",kTRUE,kTRUE);
tt->GetListOfBranches()->ls();
tt->SetLineColor(4);
tt->SetMarkerStyle(24);
tt->Draw("Z1","pop","same,P");
tt->SetMarkerStyle(21);
tt->Draw("Z1","","same,P");

}

</code>
</pre>
END_HTML
*/
////////////////////////////////////////////////////////////////////////////////

KVPartitionManager::KVPartitionManager()
{
   // Default constructor
	init();
}

KVPartitionManager::KVPartitionManager(KVString option)
{
   // Default constructor
	init();
	SetFillingMode(option);
}

KVPartitionManager::~KVPartitionManager()
{
   // Destructor

	Reset();

	while (this->GetEntries()>0)
		delete this->RemoveAt(0);
	
	
}

void KVPartitionManager::init(){
	//
	listdepassage = new KVList(kFALSE);
	CreationSousListe();
	Nmax=200;
	nombre_diff=0;
	nombre_total=0;
	SetName("MainListe");
	lgen.Clear();
	kcheck=kTRUE;
	
}

void KVPartitionManager::Reset(){

	KVList* li = GetIntermediate();
	Int_t tot = li->GetEntries();
	for (Int_t ii=0; ii<tot; ii+=1) {
		KVList* sl = (KVList* )li->RemoveAt(0);	
		Int_t toti = sl->GetEntries();
		for (Int_t jj=0;jj<toti; jj+=1)
			delete (KVIntegerList* )sl->RemoveAt(0);
		delete sl;
	}
	
	Clear();
	CreationSousListe();
	UpdateCompteurs();
	
}

void KVPartitionManager::CreationSousListe(){

	GetIntermediate()->Add(new KVList(kFALSE));
	KVList* sl = (KVList* )GetIntermediate()->Last();
	KVString snom; snom.Form("SousListe_%d",GetIntermediate()->GetEntries());
	sl->SetName(snom.Data());
	
}

Bool_t KVPartitionManager::TestPartition(KVIntegerList* par){

	KVList* sl = (KVList* )GetIntermediate()->Last();
	Bool_t IsNew = kFALSE;
	
	KVIntegerList* pp = 0;
	if ( !(pp = (KVIntegerList* )sl->FindObject(par->GetName())) ){
		sl->Add(par);
		IsNew=kTRUE;
		if (sl->GetEntries()==Nmax){
			CreationSousListe();
		}
	}	
	else pp->AddPopulation(par->GetPopulation());
	
	return IsNew;

}

Bool_t KVPartitionManager::FillPartition(KVIntegerList* par){

	
	Add(par);
	return kTRUE;

}


Bool_t KVPartitionManager::Fill(KVIntegerList* par){

	if (kcheck) return TestPartition(par);
	else 			return FillPartition(par);

}

void KVPartitionManager::ReduceSubLists(){

	TStopwatch st; st.Start();
	
	KVList* ll = GetIntermediate();
	Info("ReduceSubLists","nbre de sous listes : %d",ll->GetEntries());

	KVList *sl0,*sl1;
	Int_t passage=2;
	Int_t nsl = ll->GetEntries();
	Int_t entries;
	while (nsl>1){
		
		Int_t pas =1;
		for (Int_t nn=0;nn<ll->GetEntries();nn+=pas){
			
			sl0  = (KVList* )ll->At(nn);
			entries = sl0->GetEntries();
			
			if (nn+1<ll->GetEntries()){
				sl1  = (KVList* )ll->At(nn+1);
				if (!sl1) Info("ReduceSubLists","pas de liste sl1 a la position %d",nn+1);
			
				KVIntegerList* par0=0,*par1=0;
				while (sl1->GetEntries()>0){
			
					par1 = (KVIntegerList* )sl1->RemoveAt(0);
			
					if ( (par0 = (KVIntegerList* )sl0->FindObject(par1->GetName())) ){
						par0->AddPopulation(par1->GetPopulation());
						delete par1;
					}
					else{
						sl0->Add(par1);
					}
				}
				delete (KVList* )ll->RemoveAt(nn+1);
			}
			
		}
		
		passage+=1;
		nsl = ll->GetEntries();
		
		Info("ReduceSubLists","nbre de sous-liste : %d",nsl);	
	}
		
	UpdateCompteurs((KVList* )ll->At(0));
	st.Stop();
	Info("ReduceSubLists","Temps de traitement %lf",st.RealTime());
}

void KVPartitionManager::TransfertToMainList(){
	
	KVList* sl  = (KVList* )GetIntermediate()->RemoveAt(0);
	if (GetEntries()==0) {
		Int_t tot = sl->GetEntries();
		for (Int_t mm=0;mm<tot;mm+=1){
			Add(sl->RemoveAt(0));
		}
	}
	else {
		ReduceWithOne(sl);
	}
	
	delete sl;
	CreationSousListe();
	
	UpdateCompteurs();
	
	Info("TransfertToMainList","%d partitions enregistrees dont %d differentes",
		GetNombrepartitionsTotale(), 
		GetNombrepartitionsDifferentes()
	);

}

void KVPartitionManager::ReduceWithOne(KVList* sl){

	KVIntegerList* par0=0,*par1=0;
	while (sl->GetEntries()>0){
		par1 = (KVIntegerList* )sl->RemoveAt(0);
		if ( (par0 = FindPartition(par1->GetName())) ){
			par0->AddPopulation(par1->GetPopulation());
			delete par1;
		}
		else{
			Add(par1);
		}
	}
}

void KVPartitionManager::UpdateCompteurs(KVList* current){
	
	/*
	if (!current) {
		nombre_diff = GetEntries();
		nombre_total = 0;
		Float_t ztot_min=10000,z_min=10000,mtot_min=10000;
		Float_t ztot_max=-10000,z_max=-10000,mtot_max=-10000;
		
		KVIntegerList* par = 0;
		for (Int_t nn=0;nn<nombre_diff;nn+=1) {
			par = GetPartition(nn);
			nombre_total += par->GetPopulation();
			if (par->GetZtot()>ztot_max) 	ztot_max=par->GetZtot();
			if (par->GetZtot()<ztot_min) 	ztot_min=par->GetZtot();
			if (par->GetMtot()>mtot_max) 	mtot_max=par->GetMtot();
			if (par->GetMtot()<mtot_min)	mtot_min=par->GetMtot();
			if (par->GetZmax()>z_max) 		z_max=par->GetZmax();
			if (par->GetZmin()<z_min) 		z_min=par->GetZmin();
		}
		
		lgen.SetValue("Ztot_max",ztot_max);
		lgen.SetValue("Ztot_min",ztot_min);
		lgen.SetValue("Mtot_max",mtot_max);
		lgen.SetValue("Mtot_min",mtot_min);
		lgen.SetValue("Z_max",z_max);
		lgen.SetValue("Z_min",z_min);
	}
	else {
		Int_t n_diff=0;
		Int_t n_total=0;	
		n_diff = current->GetEntries();
		for (Int_t nn=0;nn<n_diff;nn+=1) n_total += ((KVIntegerList* )current->At(nn))->GetPopulation();
	
		Info("UpdateCompteurs","%s %d partitions enregistrees dont %d differentes",
			current->GetName(),n_total,n_diff);
	}
	*/
	if (!current) {
		nombre_diff = GetEntries();
		nombre_total = 0;
		Float_t mtot_min=10000;
		Float_t mtot_max=-10000;
		
		KVIntegerList* par = 0;
		for (Int_t nn=0;nn<nombre_diff;nn+=1) {
			if (nn%5000==0)
				Info("UpdateCompteurs","%d partitions traitees",nn);
			par = GetPartition(nn);
			nombre_total += par->GetPopulation();
			if (par->GetNbre()>mtot_max) 	mtot_max=par->GetNbre();
			if (par->GetNbre()<mtot_min)	mtot_min=par->GetNbre();
		}
		
		lgen.SetValue("Mtot_max",mtot_max);
		lgen.SetValue("Mtot_min",mtot_min);
	}
	else {
		Int_t n_diff=0;
		Int_t n_total=0;	
		n_diff = current->GetEntries();
		for (Int_t nn=0;nn<n_diff;nn+=1) n_total += ((KVIntegerList* )current->At(nn))->GetPopulation();
	
		Info("UpdateCompteurs","%s %d partitions enregistrees dont %d differentes",
			current->GetName(),n_total,n_diff);
	}

}



/*
TH1F* KVPartitionManager::GenereHisto(KVString method,Int_t nb,Double_t min,Double_t max){

	if (GetEntries()==0) return 0;
	
	TMethodCall meth;
   meth.InitWithPrototype(this->At(0)->IsA(), method.Data(),"");
	
	KVString snom; snom.Form("%s_%s",this->At(0)->Class_Name(),method.Data());
	
	TH1F* h1 = (TH1F* )gROOT->FindObject(snom.Data());
	if (h1) delete h1;
	h1 = new TH1F(snom.Data(),"From_KVParticleManager",nb,min,max);
	
	if (meth.IsValid()){
		if ( meth.ReturnType()==TMethodCall::kLong) {
			Int_t* val_Long = GetValues_Long(meth);
			for (Int_t nn=0;nn<GetEntries();nn+=1)
				h1->Fill(val_Long[nn]);
			delete [] val_Long;	
		}
		else if (meth.ReturnType()==TMethodCall::kDouble) {
			Double_t* val_Double = GetValues_Double(meth);
			for (Int_t nn=0;nn<GetEntries();nn+=1)
				h1->Fill(val_Double[nn]);
			delete [] val_Double;	
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
	
	return h1;

}
*/
/*
Int_t* KVPartitionManager::GetIndex(KVString method, Bool_t down){

	if (GetEntries()==0) return 0;
	
	TMethodCall meth;
   meth.InitWithPrototype(this->At(0)->IsA(), method.Data(),"");
	Int_t* idx = new Int_t[GetEntries()];
	
	
	if (meth.IsValid()){
		if ( meth.ReturnType()==TMethodCall::kLong) {
			Int_t* val_Long = GetValues_Long(meth);
			TMath::Sort(GetEntries(),val_Long,idx,down);
			delete [] val_Long;
		}
		else if ( meth.ReturnType()==TMethodCall::kDouble) {
			Double_t* val_Double = GetValues_Double(meth);
			TMath::Sort(GetEntries(),val_Double,idx,down);
			delete [] val_Double;
		}
		else {
			delete idx;
			return 0;
		}
	}
	else {
		delete idx;
		return 0;
	}
	
	return idx;

}


Int_t* KVPartitionManager::GetValues_Long(TMethodCall& meth){

	Int_t dim=GetEntries();
	Int_t* val = new Int_t[dim];
	Long_t ret;
	for (Int_t kk=0;kk<dim;kk+=1){
		meth.Execute(At(kk),"",ret);
		val[kk]=ret;
	}
	return val;
}
	
	
Double_t* KVPartitionManager::GetValues_Double(TMethodCall& meth){

	Int_t dim=GetEntries();
	Double_t* val = new Double_t[dim];
	Double_t ret;
	for (Int_t kk=0;kk<dim;kk+=1){
		meth.Execute(At(kk),"",ret);
		val[kk]=ret;
	}
	return val;
}
*/	
TTree* KVPartitionManager::GenereTree(KVString treename,Bool_t Compress){	//,Bool_t AdditionalValue){
	
	//UpdateCompteurs();
	
	if (GetEntries()==0) return 0;
	
	Int_t mmax = lgen.GetIntValue("Mtot_max");
	Info("GenereTree","Multiplicity max entregistree %d",mmax);
	Int_t* tabz = new Int_t[1000];
	Int_t mtot;
	Int_t pop;
	Info("GenereTree","Nbre de partitions entregistrees %d",GetEntries());
	
	TTree* tree = new TTree(treename.Data(),"FromKVPartitionManager");
	tree->Branch("mtot",			&mtot,	"mtot/I");
	tree->Branch("tabz",			tabz,		"tabz[mtot]/I");
	if (Compress)
		tree->Branch("pop",	&pop,		"pop/I");
	
	KVIntegerList* par;
	TArrayI* table = 0;
	for (Int_t kk=0;kk<GetEntries();kk+=1){
		par = GetPartition(kk);
		table = par->GetTableOfValues();
		mtot = par->GetNbre();
		if (kk%5000==0)
			Info("GenereTree","%d partitions traitees",kk);
		pop = par->GetPopulation();
		for (Int_t mm=0;mm<mtot;mm+=1)
			tabz[mm] = table->At(mm);
		
		if (Compress){ tree->Fill(); }
		else{
			for (Int_t pp=0; pp<pop; pp+=1)  tree->Fill();
		}
		par->Reduce();
		//delete [] table;	
	}
	
	tree->ResetBranchAddresses();
	delete [] tabz;
	return tree;

}

void KVPartitionManager::SaveAsTree(KVString filename,KVString treename,Bool_t Compress,Option_t* option){
	TFile* file = new TFile(filename.Data(),option);
	GenereTree(treename)->Write();
	file->Close();
}
	
void KVPartitionManager::PrintInfo(){

	Info("PrintInfo","Intervalles en multiplicite des partitions enregistrees");
	printf(" - Mtot : %d %d\n",lgen.GetIntValue("Mtot_min"),lgen.GetIntValue("Mtot_max"));
	
}
