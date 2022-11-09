#ifndef __KVISOSCALING_H
#define __KVISOSCALING_H

//Isoscaling class
#include <vector>
#include <utility>

//Kaliveda libs
#include "KVNumberList.h"
#include "KVList.h"
#include "KVHashList.h"

//ROOT libs
#include "TMultiGraph.h"

/**
\class KVIsoscaling
\brief Class used to estimate isoscaling alpha coefficient and extract Csym/T from input data files (Z fixed)
\ingroup Analysis
\author Quentin Fable (quentin.fable@l2it.in2p3.fr)
\date Tue Mar  3 12:41:49 2020

See Phys. Rev. Lett. 86, 5023 (2001) and also Phys. Rev. C 75, 044605 (2007) for more details about the isoscaling method.
This class was used in Fable et al., Phys. Rev. C 106 024605 (2022).

Isoscaling corresponds to the scaling behaviour obtained from the ratio \f$ R_{21}(N,Z) \f$ of the same isotope measured with two colliding systems differing in their neutron-to-proton content,
\f$ Y_{(1)}(N,Z) \f$ and \f$ Y_{(2)}(N,Z) \f$, where \f$ (2) \f$ stands for the neutron-rich system.

In a variety of HIC an exponential dependence of the ratio on N and Z is observed, such as
\f[
R_{21}(N,Z) = \frac{Y_{(2)}(N,Z)}{Y_{(1)}(N,Z)} = C\,exp\left[ \alpha N + \beta Z \right]
\f]

where \f$ C \f$ is an overall normalization constant while \f$ \alpha \f$ and \f$ \beta \f$ are the isoscaling parameters.

For a fixed charge \f$ Z \f$, \f$ \alpha(Z) \f$ can be extracted from a linear fit to the natural logarithm of the yield ratio \f$ R_{21} \f$ as a function of the neutron number \f$ N \f$.

A Gaussian approximation of the yields in the grand-canonical approximation allows to link the \f$\alpha\f$ parameter to the symmetry energy coefficient \f$ C_{sym} \f$ divided by the temperature \f$ T \f$:
\f[
\frac{4 C_{sym}(Z)}{T} = \frac{\alpha(Z)}{\Delta (Z)}
\f]

with
\f[
\Delta (Z) = \left( \frac{Z}{\langle A_{1}(Z) \rangle} \right)^{2} - \left( \frac{Z}{\langle A_{2}(Z) \rangle} \right)^{2}
\f]

where \f$ \langle A_{1}(Z)\rangle \f$ and \f$ \langle A_{2}(Z)\rangle \f$ are the average masses corresponding to the isotope charge Z for systems \f$ (1) \f$ and \f$ (2) \f$, respectively.

## Example of use

### Initialization
First, initialize the isoscaling class:
   ~~~~~{.cpp}
   KVIsoscaling kvi();
   ~~~~~

Then import (at least) two ASCII files (one for each collding system) containing the yields for each charge.
Each colliding system is referenced by an index set by the user at the importation.

Format of the data in an input file must be the following:
   ~~~~~{.cpp}
   #Z   A   Yield   Yield_err
   5    10  0.4     0.02
   5    11  0.3     0.015
   ...
   10   20  0.2     0.02
   10   21  0.4     0.01
   ...
   ~~~~~

Assuming a string (`const Char_t *my_path`) representing the name of the ASCII file to open,
containing the yields data for a given system to be referenced by `Int_t system_idx`.
The yields can be imported using:
   ~~~~~{.cpp}
   kvi.ReadYieldsFile(Int_t system_idx, const Char_t *my_path);
   ~~~~~

After the importation, individual gaussian fits will be applied the yields in order to extract the \f$ \langle A(Z)\rangle \f$ values
(see BuildGaussianPlots() method for more details).

Results of the importation can be accessed from various functions:
   ~~~~~{.cpp}
   KVNumberList KVIsoscaling::GetZNumberList(Int_t system)                          // returns a KVNumberList of all the charges set for `my_system`
   KVNumberList KVIsoscaling::GetANumberList(Int_t system, Int_t zz)                // returns a KVNumberList of all the masses set `my_system` and charge `zz`
   KVNumberList KVIsoscaling::GetSharedZNumberList(Int_t system_1, Int_t system_2)  // returns a KVNumberList of the charges shared between two imported systems
   ...
   ~~~~~

### Set tolerance threshold for the Gaussian approximation
Isoscaling is expected in a mass region where the Gaussian approximation is verified.
For this reason, it can be necessary limit the region of experimental points where the isoscaling fit procedure will be applied.

The approach we use is to limit the fit in a region around \f$ \langle A(Z)\rangle \pm tol \cdot rms \f$,
where \f$ rms \f$ is the standard deviation of \f$ \langle A(Z)\rangle \f$ and \f$ tol \f$ is a threshold that can be set by the user.

By default, \f$ tol = 2.5 \f$.
It can be modified by calling:
   ~~~~~{.cpp}
   kvi.SetRMSTolerance(Double_t tol);
   ~~~~~

It is also possible to test "by-hand" and optimize the \f$ tol \f$ threshold.
Assuming two systems referenced by `Int_t system_1` and `Int_t system_2`,
for a given charge (`Int_t zz`) the quality of the Gaussian approximation can be tested with the following method:
   ~~~~~{.cpp}
   kvi.TestGaussianApprox(Int_t system_1, Int_t system_2, Int_t zz, Double_t tol);
   ~~~~~
This will plot the graph with and without the selected tolerance applied along with the gaussian fits.

### Build the graphs
Asumming two systems referenced by `Int_t system_1` and `Int_t system_2`, the \f$ lnR_{21} \f$ vs \f$ N \f$ graphs are built by calling:
   ~~~~~{.cpp}
   kvi.BuildLnR21vsNPlots(system_1, system_2);
   ~~~~~
Note that the tolerance threshold of the Gaussian approximation is applied here (see SetRMSTolerance method).

### Fit the graphs
To fit the graphs use:
   ~~~~~{.cpp}
   kvi.FitLnR21vsNPlots(system_1, system_2);
   ~~~~~

### Use the fit results
See the list of methods below that can then be used to exploit the fit results.

   ~~~~~{.cpp}
   void KVIsoscaling::BuildAlphaPlot(int system_1, int system_2)        // build alpha vs Z plot
   void KVIsoscaling::BuildDeltaZA2Plot(int system_1, int system_2)     // build Delta vs Z plot
   void KVIsoscaling::BuildAlphavsDeltaPlot(int system_1, int system_2) // build alpha vs Delta plot
   ~~~~~

### Export results
Results can be saved in ASCII file using:
   ~~~~~{.cpp}
   kvi.SaveResultsASCII(ascii_file_path);
   ~~~~~

Results can be saved in ROOT file using:
   ~~~~~{.cpp}
   kvi.SaveResultsROOT(root_file_path);
   ~~~~~
*/

class KVIsoscaling : public TObject {
public:

   KVIsoscaling()
   {
      // Default constructor
      fhlist_lnR21N_.SetOwner(kTRUE);
      fhlist_lnR21N_all_.SetOwner(kTRUE);
      fhlist_fit_.SetOwner(kTRUE);
      fhlist_csymT_.SetOwner(kTRUE);
      fhlist_yields_.SetOwner(kTRUE);
      fhlist_delta_.SetOwner(kTRUE);
      fhlist_alpha_.SetOwner(kTRUE);
      fhlist_alpha_delta_.SetOwner(kTRUE);

      ftol_  = 2.5;

      fdebug_ = kFALSE;
   }

   Bool_t ReadYieldsFile(Int_t system, const Char_t* file_path);

   void TestGaussianApprox(Int_t system1, Int_t system2, Int_t zz, Double_t tol); //use to test the effect of the rms tolerance for the given system combination and charge, return reduced chisquare
   void BuildLnR21vsNPlots(Int_t system1, Int_t system2);

   void FitLnR21vsNPlots(Int_t system1, Int_t system2, Option_t* option = "MNVR", Option_t* gooption = "goff");

   void DrawAlphavsNFits(Int_t system1, Int_t system2);
   void DrawLnR21vsNFits(Int_t system1, Int_t system2);
   void BuildAlphaPlot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw = kFALSE);
   void BuildDeltaZA2Plot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw = kFALSE);
   void BuildCsymOverTPlot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw = kFALSE);
   void BuildAlphavsDeltaPlot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw = kFALSE);
   void CreateCsymOverTMultiGraph(TMultiGraph* mgr);
   void SaveResultsROOT(const Char_t* file_name = "./isoscaling_output_file.root");
   void SaveResultsASCII(const Char_t* file_name = "./isoscaling_output_file.txt");

   // --- Getters ---
   Int_t   GetNSystems()
   {
      return int(fvec_sys_.size());
   }
   Bool_t  GetAMean(Int_t system, Int_t zz, Float_t& meanA, Float_t& meanA_err);
   Bool_t  GetAlpha(Int_t system1, Int_t system2, Int_t zz, Float_t& alpha, Float_t& alpha_err);
   Bool_t  GetDeltaZA2(Int_t system1, Int_t system2, Int_t zz, Float_t& denum, Float_t& denum_err, Bool_t debug);
   Bool_t  GetCsymOverT(Int_t system1, Int_t system2, Int_t zz, Float_t& csymT, Float_t& csymT_err, Bool_t debug);
   Int_t   FindZFromAmean(Int_t system, Int_t aa);
   Double_t GetRMSTolerance()
   {
      return ftol_;
   }

   // --- Vectors getters ---
   std::vector<Int_t>    GetZVector(Int_t system)
   {
      return fvec_sys_z_.at(GetSystemPos(system));
   }
   std::vector<Int_t>    GetAVector(Int_t system, Int_t zz);        //return the vector of A for a given (system, Z) combination
   std::vector<Float_t>  GetYieldVector(Int_t system, Int_t zz);    //return the vector of yields for a given (system, Z) combination
   std::vector<Float_t>  GetYieldErrVector(Int_t system, Int_t zz); //return the vector of A for a given (system, Z) combination
   std::vector<Float_t>  GetAMeanVector(Int_t system)
   {
      return fvec_sys_z_meanA_.at(GetSystemPos(system));
   }
   std::vector<Float_t>  GetAMeanErrVector(Int_t system)
   {
      return fvec_sys_z_meanerrA_.at(GetSystemPos(system));
   }

   // --- Lists ---
   KVHashList* GetLnR21vsNGraphList()
   {
      return &fhlist_lnR21N_;
   }
   KVHashList* GetLnR21vsNFitList()
   {
      return &fhlist_fit_;
   }
   KVHashList* GetAlphaPlots()
   {
      return &fhlist_alpha_;
   }
   KVHashList* GetDeltaZA2Plots()
   {
      return &fhlist_delta_;
   }
   KVHashList* GetAlphavsDeltaPlots()
   {
      return &fhlist_alpha_delta_;
   }
   KVHashList* GetCsymOverTPlots()
   {
      return &fhlist_csymT_;
   }

   KVNumberList* GetZNumberList(Int_t system)
   {
      return (KVNumberList*)(flist_z_.At(GetSystemPos(system)));
   }
   KVNumberList* GetANumberList(Int_t system, Int_t zz);
   KVNumberList GetSharedZNumberList(Int_t system1, Int_t system2);
   KVNumberList GetSharedANumberList(Int_t system1, Int_t system2, Int_t zz);
   KVList* GetAList()
   {
      return &flist_a_;
   }

   // --- Printers ---
   void PrintYieldsList();
   void PrintSystemsList();

   // --- inline methods ---
   inline void SetVerbose(Bool_t debug)
   {
      fdebug_ = debug;
   }

   inline void SetRMSTolerance(Double_t tol)
   {
      ftol_ = tol;
   }

protected:
   Bool_t fdebug_;   //verbose mode for debugging
   Double_t ftol_;  //tolerance for the gaussian approximation (in sigma)

   void BuildGaussianPlots(Int_t system);

   // --- (system, Z, A, yields, yields_err) vectors for Isoscaling ---
   std::vector<Int_t>               fvec_sys_;     //vector of systems
   std::vector< std::vector<Int_t> >   fvec_sys_z_;   //vectors of charge Z (one vector per system)
   std::vector< std::vector< std::vector<Int_t> > >   fvec_sys_z_a_;       //vectors of A(Z) (one vector per (system, Z) combination)
   std::vector< std::vector< std::vector<Float_t> > >    fvec_sys_z_yields_;  //vector of yields (one vector per (system, Z) combination - same order than fvec_sys_z_a_)
   std::vector< std::vector< std::vector<Float_t> > >    fvec_sys_z_yields_err_; //vector of yields uncertainties (one vector per (system, Z) combination - same order than fvec_sys_z_a_)

   // --- (system, Z, <A(Z)>, <A(Z)>_err) vectors for Isoscaling ---
   std::vector< std::vector<Float_t> > fvec_sys_z_meanA_;   //vectors of <A(Z)> (one vector per system)
   std::vector< std::vector<Float_t> > fvec_sys_z_meanerrA_;   //vectors of <A(Z)> uncertainties (one vector per system)

   // --- list of isoscaling systems ---
   std::vector< std::pair<Int_t, Int_t> > fvec_iso_pairs_;

   // --- Lists ---
   KVNumberList fnl_sys_;           // KVNumberList of treated systems (yield files have been read without error)
   KVList flist_z_;            // contains all lists of charges Z (one KVNumberList per system, in system order)
   KVList flist_a_;            // contains all lists of masses A (one list of KVNumberList per (system, Z) combination)
   KVHashList fhlist_lnR21N_;       // contains all graphs of lnR21 vs N (with tolerance applied)
   KVHashList fhlist_lnR21N_all_;  // contains all graphs of lnR21 vs N (without tolerance applied)
   KVHashList fhlist_fit_;         // contains all linear fits attempted from lnR21_vs_N graphs
   KVHashList fhlist_yields_;      // contains all yield plots vs N
   KVHashList fhlist_delta_;       // contains all Delta vs Z graphs
   KVHashList fhlist_alpha_;       // contains all Aplha vs Z graphs
   KVHashList fhlist_alpha_delta_; // contains all Alpha vs Delta graphs
   KVHashList fhlist_csymT_;     // contains all Csym/T vs Z graphs

   // --- Getters ---
   Int_t   GetSystemPos(Int_t system); // returns the system position in fvec_sys_
   Int_t   GetZPos(Int_t system, Int_t zz); // returns the position of the charge Z (for the provided system) in the vector
   Int_t   GetAPos(Int_t system, Int_t zz, Int_t aa);

   std::vector<Int_t>    GetAVectorFromPos(Int_t pos_system, Int_t pos_zz);        //return the vector of A from (system, zz) positions in corresponding vectors
   std::vector<Float_t>  GetYieldVectorFromPos(Int_t pos_system, Int_t pos_zz);    //return the vector of yields from (system, zz) positions in corresponding vectors
   std::vector<Float_t>  GetYieldErrVectorFromPos(Int_t pos_system, Int_t pos_zz); //return the vector of A from (system, zz) positions in corresponding vectors

   ClassDef(KVIsoscaling, 1) //KVClass
};

#endif
