/**
\class KVTrieurBloc
\brief Class for a sorting with detached cells
\ingroup Analysis

   This class returns an index determined via a a research in an array of
 values. The upper limit and the lower limit of a cell are not belonging to
 any of the other cells. The following methods are provided by this class:
~~~~{.cpp}
      virtual Int_t GetNumCase(Double_t x)   Gives an index corresponding to
                                             the value of x. If xmin and xmax
                                             are the arrays of nb_cells values,
                                             the returned value i is determined
                                             following this prescription:

                                             xmin[i-1] < x < xmax[i-1]

                                             if the x value do not correspond
                                             to any of the cells, -1 is
                                             returned.

      virtual Int_t GetNumCase(void *,...)   return -1,not to use.
~~~~
 BEWARE! : the index value ranges between 1 and nb_cells.

  For each index, the Xmin and Xmax values can be adjusted with the methods
~~~~{.cpp}
      virtual void SetXmin(Int_t i,Double_t x)
      virtual void SetXmax(Int_t i,Double_t x)
~~~~
 BEWARE! : the index value ranges between 1 and nb_cells.

  The name of the sorting variable can be set withe the method
      virtual void SetNomVar(Char_t *s)

 The number of indexes can be set with the method  virtual void SetNbCases(Int_t n).
This has to be done first

  Setting the number of indexes, the name of the sorting variable or the Xmin
 or Xmax values automatically generates the names for each index.

  In a treatment program, this can be used to set histogram titles and to
 manage efficiently arrays of histograms. Here is an example where the sorting
 variable is Ekin (calculated using KVEkin class).
 ~~~~{.cpp}
== Example ==================================================================================================
 // Declarations and initialisations
 ...
 KVEkin Sekin;
 Sekin.Init();
 KVZmax Zmax;
 Zmax.Init();
 ...
 KVTrieurBloc sortEkin;
 sortEkin.SetNbCases(5);        // 5 indexes
 sortEkin.SetNomVar("E_{kin}"); // name of the sorting variable
 sortEkin.SetXmin(1,0.);        // minimum value of Ekin for cell 1
 sortEkin.SetXmax(1,50.);       // maximum value of Ekin for cell 1
 sortEkin.SetXmin(2,100.);      // minimum value of Ekin for cell 2
 sortEkin.SetXmax(2,150.);      // maximum value of Ekin for cell 2
 sortEkin.SetXmin(3,150.);      // minimum value of Ekin for cell 3
 sortEkin.SetXmax(3,225.);      // maximum value of Ekin for cell 3
 sortEkin.SetXmin(4,300.);      // minimum value of Ekin for cell 4
 sortEkin.SetXmax(4,390.);      // maximum value of Ekin for cell 4
 sortEkin.SetXmin(5,500.);      // minimum value of Ekin for cell 5
 sortEkin.SetXmax(5,800.);      // maximum value of Ekin for cell 5
 ...
 // Declaration of histograms
 TList *lekin=new TList();      // list to store histograms
 for(Int_t i=0;i<sortEkin.GetNbCases();i++)
  {
  TString sname("histo"); // TString for the histogram name
  sname+=i;
  TString stitle("Z_{max} for "); // TString for the histogram title
  stitle+=sortEkin.GetNomCase(i+1);
  TH1F *h1=new TH1F(sname.Data(),stitle.Data(),100,0.5,100.5);
  lekin->Add(h1); // Add the histogram to the list
  }
 ...
 // Treatment loop for each event called for each event
 ...
 Sekin.Reset();
 Zmax.Reset();
 KVINDRAReconNuc *part = 0;
 while( (part = GetEvent()->GetNextParticle("ok")) ){//loop over particles with correct codes
  Sekin.Fill(part);
  Zmax.Fill(part);
 }
 // Filling the appropriate histogram
 Int_t index=sortEkin(Sekin());       // Determine the index value according to Sekin()
 if(index > 0)     // Check if SEkin() is not out of range (<0 or >800)
  {
  TH1F *h1=(TH1F *)lekin->At(index-1); // retrieve the histogram in the list
  h1->Fill(Zmax());                    // fill it!
  }
 ...
~~~~
*/
#include "KVTrieur.h"
#include "TVector.h"

class KVTrieurBloc: public KVTrieur {
public:
// Champs Statiques:
   static Int_t nb;
   static Int_t nb_crea;
   static Int_t nb_dest;

   TVector xmin;
   TVector xmax;
   Char_t nomVar[80];

// Methodes
public:
   void initKVTrieurBloc(void); // Initialisations
   void SetNomsCases(void);     // Initialisations des noms de cases

public:
   KVTrieurBloc(void);         // constructeur par defaut
   KVTrieurBloc(Char_t* nom);
   KVTrieurBloc(Int_t nbcases, Char_t* nom);
   KVTrieurBloc(const KVTrieurBloc& a);        // constructeur par copie

   virtual ~ KVTrieurBloc(void);       // destructeur

#if ROOT_VERSION_CODE >= ROOT_VERSION(3,4,0)
   virtual void Copy(TObject& obj) const;
#else
   virtual void Copy(TObject& obj);
#endif
   KVTrieurBloc& operator =(const KVTrieurBloc& a);    // operateur =



   virtual Int_t GetNumCase(void* argus ...);   // Pour une situation donnee,
   // retourne le numero de la case
   // correspondante.
   virtual Int_t GetNumCase(Double_t x);        // Pour un x donne,
   // retourne le numero de la case
   // correspondante.

   virtual void SetNomVar(Char_t* x);
   virtual void SetXmin(Int_t i, Double_t x);
   virtual void SetXmax(Int_t i, Double_t x);
   virtual const Char_t* GetNomVar(void);
   virtual Double_t GetXmin(Int_t i);
   virtual Double_t GetXmax(Int_t i);
   virtual void SetNbCases(Int_t n);    // ajuste le nombre de cases.

   ClassDef(KVTrieurBloc, 1)   // Class for sorting with detached cells
};