#ifndef __KVMULTIGAUSSISOTOPEFIT_H
#define __KVMULTIGAUSSISOTOPEFIT_H

#include "TF1.h"
#include "TVirtualPad.h"
#include "TColor.h"
#include "TArrayI.h"
#include "KVNumberList.h"
#include "KVList.h"

#include <KVNameValueList.h>

/**
 \class KVMultiGaussIsotopeFit
 \brief Function for fitting PID mass spectra

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author eindra
 \date Wed Jun 15 10:56:41 2022
*/

class KVMultiGaussIsotopeFit : public TF1 {
   enum fit_param_index {
      bkg_cst = 1,
      bkg_slp = 2,
      gauss_wid = 3,
      pidvsA_a0 = 4,
      pidvsA_a1 = 5,
      pidvsA_a2 = 6
   };
   double centroid_fit(double* x, double* p)
   {
      /*
         centroids of gaussians are expected to increase linearly with mass A
         x[0]=A
         p[0]=offset
         p[1]=slope
         */
      return p[0] + p[1] * x[0] + p[2] * x[0] * x[0];
   }

   double FitFunc(double* x, double* p)
   {
      /*
         x[0] = PID
         p[0] = number of gaussians = Ng (fixed)
         p[1],p[2] = background parameters: exp(p[1]+p[2]*x)
         p[3]      = sigma for all gaussians
         p[4],p[5],p[6] = offset & slope for centroid PID vs. A correlation
         p[7]...p[6+Ng] = norm of each gaussian
         p[6+Ng+1]...p[6+2*Ng] = A of each gaussian

         Total number of parameters is 7+2*Ng
      */
      int Ng = p[0];
      double background = TMath::Exp(p[fit_param_index::bkg_cst] + p[fit_param_index::bkg_slp] * x[0]);
      for (int i = 1; i <= Ng; ++i) {
         background +=
            p[get_gauss_norm_index(i)] * TMath::Gaus(x[0], centroid_fit(&p[get_mass_index(i, Ng)],
                  &p[fit_param_index::pidvsA_a0]), p[fit_param_index::gauss_wid], kTRUE);
      }
      return background;
   }
   int get_gauss_norm_index(int ig) const
   {
      return 6 + ig;
   }
   int get_mass_index(int ig, int ng) const
   {
      return 6 + ng + ig;
   }
   int total_number_parameters(int ng) const
   {
      return 7 + 2 * ng;
   }
   int Z; // atomic number of the isotopes
   int Niso; // number of isotopes to fit = number of gaussians
   double PIDmin, PIDmax; // PID limits for current set of isotopes
   std::vector<int> Alist; // list of masses of isotopes (in increasing order)
   std::vector<double> PIDlist; // list of initial centroid (PID) of each isotope (in increasing order)
   double min_sigma = 1.e-2; // lower limit for width of gaussians
   double max_sigma = 1.e-1; // upper limit for width of gaussians

   double evaluate_gaussian(int i, double pid) const
   {
      // return weight of gaussian i (i=1,2,...) for given value of pid
      return GetGaussianNorm(i) * TMath::Gaus(pid, GetCentroid(i), GetGaussianWidth(i), kTRUE);
   }
public:
   KVMultiGaussIsotopeFit() : TF1() {}
   KVMultiGaussIsotopeFit(int z, std::vector<int> alist)
      : TF1(),
        Z{z},
        Alist{alist}
   {
      // This constructor cannot be used to perform fits, but can be used to UnDraw() an existing fit
   }
   KVMultiGaussIsotopeFit(int z, int Ngauss, double PID_min, double PID_max, const KVNumberList& alist, std::vector<double> pidlist);
   KVMultiGaussIsotopeFit(int z, int Ngauss, double PID_min, double PID_max, const KVNumberList& alist,
                          double bkg_cst, double bkg_slp, double gaus_wid,
                          double pidvsa_a0, double pidvsa_a1, double pidvsa_a2);
   KVMultiGaussIsotopeFit(int z, const KVNameValueList&);

   void ReleaseCentroids()
   {
      // Release the constraint on the positions of the centroids
      SetParLimits(fit_param_index::pidvsA_a0, -50, 50);
      SetParLimits(fit_param_index::pidvsA_a1, 1.e-2, 5.);
      SetParLimits(fit_param_index::pidvsA_a2, -2.e-2, 5.);
   }

   double GetPIDvsAfit_a0() const
   {
      // \returns constant term in PID vs. A relation
      return GetParameter(fit_param_index::pidvsA_a0);
   }
   double GetPIDvsAfit_a1() const
   {
      // \returns coefficient of linear term in PID vs. A relation
      return GetParameter(fit_param_index::pidvsA_a1);
   }
   double GetPIDvsAfit_a2() const
   {
      // \returns coefficient of quadratic term in PID vs. A relation
      return GetParameter(fit_param_index::pidvsA_a2);
   }

   void UnDraw(TVirtualPad* pad = gPad) const;

   static void UnDrawGaussian(int z, int a, TVirtualPad* pad = gPad)
   {
      // Remove the graphical representation of the given gaussian from the given pad

      auto old_fit = pad->FindObject(get_name_of_isotope_gaussian(z, a));
      if (old_fit) delete old_fit;
   }
   static void UnDrawAnyGaussian(int z, TVirtualPad* pad = gPad)
   {
      // Remove the graphical representation of any gaussian for this Z from the given pad

      TIter it(pad->GetListOfPrimitives());
      TObject* ob;
      KVList to_delete;
      while ((ob = it())) {
         TString obname = ob->GetName();
         if (obname.BeginsWith(get_root_name_of_isotope_gaussian(z))) to_delete.Add(ob);
      }
   }

   void DrawFitWithGaussians(Option_t* opt = "") const;

   int GetMostProbableA(double PID, double& P) const;
   double GetMeanA(double PID) const;
   std::map<int, double> GetADistribution(double PID) const;
   int GetA(double PID, double& P) const;
   double GetProbability(int A, double PID) const;
   double GetInterpolatedA(double PID) const
   {
      // calculate interpolated A from PID using the quadratic fit parameters

      auto a = GetPIDvsAfit_a2();
      auto b = GetPIDvsAfit_a1();
      auto c = GetPIDvsAfit_a0() - PID;
      return (TMath::Sqrt(b * b - 4.*a * c) - b) / 2. / a;
   }

   static TString get_name_of_multifit(int z)
   {
      return Form("multigauss_fit_Z=%d", z);
   }
   static TString get_name_of_isotope_gaussian(int z, int a)
   {
      return Form("gauss_fit_Z=%d_A=%d", z, a);
   }
   static TString get_root_name_of_isotope_gaussian(int z)
   {
      return Form("gauss_fit_Z=%d_", z);
   }

   double GetBackgroundConstant() const
   {
      // \returns fitted parameter constant term in exponential background
      return GetParameter(fit_param_index::bkg_cst);
   }
   double GetBackgroundSlope() const
   {
      // \returns fitted parameter slope term in exponential background
      return GetParameter(fit_param_index::bkg_slp);
   }
   double GetCentroid(int i) const
   {
      // \returns the fitted centroid position of the ith gaussian (i=1,2,...,Niso)
      assert(i > 0 && i <= Niso);
      return GetParameter(fit_param_index::pidvsA_a0)
             + (GetParameter(fit_param_index::pidvsA_a1)
                + GetParameter(fit_param_index::pidvsA_a2) * Alist[i - 1]) * Alist[i - 1];
   }
   double GetGaussianWidth(int) const
   {
      // \returns the fitted width (sigma) used for all gaussians
      return GetParameter(fit_param_index::gauss_wid);
   }
   double GetGaussianNorm(int i) const
   {
      // \returns the fitted normalisation constant of the ith gaussian (i=1,2,...,Niso)
      assert(i > 0 && i <= Niso);
      return GetParameter(get_gauss_norm_index(i));
   }
   void SetGaussianNorm(int i, double v)
   {
      // set the normalisation constant of the ith gaussian (i=1,2,...,Niso)
      assert(i > 0 && i <= Niso);
      SetParameter(get_gauss_norm_index(i), v);
   }

   void SetFitRange(double min, double max);

   double GetPIDmin() const
   {
      return PIDmin;
   }
   double GetPIDmax() const
   {
      return PIDmax;
   }

   double GetMinSigma() const
   {
      return min_sigma;
   }
   double GetMaxSigma() const
   {
      return max_sigma;
   }
   void SetSigmaLimits(double smin, double smax)
   {
      min_sigma = smin;
      max_sigma = smax;
      SetParLimits(fit_param_index::gauss_wid, min_sigma, max_sigma);
   }

   ClassDef(KVMultiGaussIsotopeFit, 1) //Function for fitting PID mass spectrum
};

#endif
