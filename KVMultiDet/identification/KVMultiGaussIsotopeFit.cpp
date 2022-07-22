#include "KVMultiGaussIsotopeFit.h"
#include "TGraph.h"
#include "TRandom.h"
#include <TCanvas.h>

KVMultiGaussIsotopeFit::KVMultiGaussIsotopeFit(int z, int Ngauss, double PID_min, double PID_max, const KVNumberList& alist, std::vector<double> pidlist)
   : TF1(Form("MultiGaussIsotopeFit_Z=%d", z), this, &KVMultiGaussIsotopeFit::FitFunc, PID_min, PID_max, total_number_parameters(Ngauss)),
     Z{z},
     Niso{Ngauss},
     PIDmin{PID_min}, PIDmax{PID_max},
     Alist{alist.GetArray()},
     PIDlist{pidlist}
{
   // Constructor used to initialize and prepare a new fit of isotope PID spectrum
   FixParameter(0, Niso);
   SetParLimits(fit_param_index::bkg_cst, -1.e+5, 1e+5);
   SetParameter(fit_param_index::bkg_cst, 4.);
   SetParName(fit_param_index::bkg_cst, "Norm");
   SetParLimits(fit_param_index::bkg_slp, -10, 0);
   SetParameter(fit_param_index::bkg_slp, -0.2);
   SetParName(fit_param_index::bkg_slp, "Bkg. slope");
   SetParName(fit_param_index::gauss_wid, "Sigma");
   SetParLimits(fit_param_index::gauss_wid, min_sigma, max_sigma);
   SetParameter(fit_param_index::gauss_wid, 0.1);

   TGraph pid_vs_a;
   for (int ig = 1; ig <= Niso; ++ig) {
      SetParName(get_gauss_norm_index(ig), Form("Norm. A=%d", Alist[ig - 1]));
      SetParLimits(get_gauss_norm_index(ig), 1.e-3, 1.e+06);
      SetParameter(get_gauss_norm_index(ig), 1.);
      SetParName(get_mass_index(ig, Niso), Form("A_%d", ig));
      FixParameter(get_mass_index(ig, Niso), Alist[ig - 1]);

#if ROOT_VERSION_CODE >= ROOT_VERSION(6,24,0)
      pid_vs_a.AddPoint(Alist[ig - 1], PIDlist[ig - 1]);
#else
      pid_vs_a.SetPoint(pid_vs_a.GetN(), Alist[ig - 1], PIDlist[ig - 1]);
#endif
   }

   // do initial fit of centroids
   TF1 centroidFit("centroidFit", this, &KVMultiGaussIsotopeFit::centroid_fit, 0., 100., 3);
   centroidFit.SetParLimits(0, -50, 50);
   centroidFit.SetParameter(0, 0);
   centroidFit.SetParLimits(1, 1.e-2, 5.);
   centroidFit.SetParameter(1, 1.e-1);
   centroidFit.SetParLimits(2, -2.e-2, 1.);
   centroidFit.SetParameter(2, 1.e-3);
   pid_vs_a.Fit(&centroidFit, "N");

   for (int i = 0; i < 3; ++i) {
      FixParameter(fit_param_index::pidvsA_a0 + i, centroidFit.GetParameter(i));
      SetParName(fit_param_index::pidvsA_a0 + i, Form("PIDvsA_a%d", i));
   }

   SetLineColor(kBlack);
   SetLineWidth(2);
   SetNpx(500);
}

KVMultiGaussIsotopeFit::KVMultiGaussIsotopeFit(int z, int Ngauss, double PID_min, double PID_max,
      const KVNumberList& alist, double bkg_cst, double bkg_slp,
      double gaus_wid, double pidvsa_a0, double pidvsa_a1, double pidvsa_a2)
   : TF1(Form("MultiGaussIsotopeFit_Z=%d", z), this, &KVMultiGaussIsotopeFit::FitFunc, PID_min, PID_max, total_number_parameters(Ngauss)),
     Z{z},
     Niso{Ngauss},
     PIDmin{PID_min}, PIDmax{PID_max},
     Alist{alist.GetArray()}
{
   // Constructor which can be used with existing fit results (not to perform new fits)
   //
   // Use SetGaussianNorm() to set the normalisation parameters for each gaussian

   SetParameter(0, Niso);
   SetParameter(fit_param_index::bkg_cst, bkg_cst);
   SetParameter(fit_param_index::bkg_slp, bkg_slp);
   SetParameter(fit_param_index::gauss_wid, gaus_wid);
   SetParameter(fit_param_index::pidvsA_a0, pidvsa_a0);
   SetParameter(fit_param_index::pidvsA_a1, pidvsa_a1);
   SetParameter(fit_param_index::pidvsA_a2, pidvsa_a2);

   SetParName(fit_param_index::bkg_cst, "Norm");
   SetParName(fit_param_index::bkg_slp, "Bkg. slope");
   SetParName(fit_param_index::gauss_wid, "Sigma");

   for (int ig = 1; ig <= Niso; ++ig) {
      SetParName(get_gauss_norm_index(ig), Form("Norm. A=%d", Alist[ig - 1]));
      SetParName(get_mass_index(ig, Niso), Form("A_%d", ig));
      FixParameter(get_mass_index(ig, Niso), Alist[ig - 1]);
   }
   for (int i = 0; i < 3; ++i)
      SetParName(fit_param_index::pidvsA_a0 + i, Form("PIDvsA_a%d", i));

   SetLineColor(kBlack);
   SetLineWidth(2);
   SetNpx(500);
}

KVMultiGaussIsotopeFit::KVMultiGaussIsotopeFit(int Z, const KVNameValueList& fitparams)
   : KVMultiGaussIsotopeFit(Z, fitparams.GetIntValue("Ng"), fitparams.GetDoubleValue("PIDmin"),
                            fitparams.GetDoubleValue("PIDmax"), fitparams.GetStringValue("Alist"),
                            fitparams.GetDoubleValue("Bkg_cst"), fitparams.GetDoubleValue("Bkg_slp"),
                            fitparams.GetDoubleValue("GausWid"),
                            fitparams.GetDoubleValue("PIDvsA_a0"),
                            fitparams.GetDoubleValue("PIDvsA_a1"),
                            fitparams.GetDoubleValue("PIDvsA_a2"))
{
   // initialize from previous fit with parameters stored in KVNameValueList
   for (int ig = 1; ig <= fitparams.GetIntValue("Ng"); ++ig)
      SetGaussianNorm(ig, fitparams.GetDoubleValue(Form("Norm_%d", ig)));
}

void KVMultiGaussIsotopeFit::UnDraw(TVirtualPad* pad) const
{
   // Remove the graphical representation of this fit from the given pad

   auto old_fit = pad->FindObject(get_name_of_multifit(Z));
   if (old_fit) delete old_fit;
   for (auto a : Alist) {
      UnDrawGaussian(Z, a, pad);
   }
}

void KVMultiGaussIsotopeFit::DrawFitWithGaussians(Option_t* opt) const
{
   // Draw the overall fit plus the individual gaussians for each isotope

   DrawCopy(opt)->SetName(get_name_of_multifit(Z));

   TF1 fgaus("fgaus", "gausn", PIDmin, PIDmax);
   // give a different colour to each gaussian
   auto cstep = TColor::GetPalette().GetSize() / (Niso + 1);
   int ig = 1;
   for (auto& a : Alist) {
      fgaus.SetParameters(GetGaussianNorm(ig), GetCentroid(ig), GetGaussianWidth(ig));
      fgaus.SetNpx(500);
      fgaus.SetLineColor(TColor::GetPalette()[cstep * ig]);
      fgaus.SetLineWidth(2);
      fgaus.SetLineStyle(9);
      fgaus.DrawCopy("same")->SetName(get_name_of_isotope_gaussian(Z, a));
      ++ig;
   }
}

int KVMultiGaussIsotopeFit::GetMostProbableA(double PID, double& P) const
{
   // for a given PID, calculate the most probable value of A, P is its probability.

   auto total = Eval(PID);
   std::map<double, int> probabilities;
   int ig = 1;
   for (auto& a : Alist) {
      probabilities[evaluate_gaussian(ig, PID) / total] = a;
      ++ig;
   }
   // the largest probability is now the last element in the map:
   // get a reverse iterator to the beginning of the reversed map
   auto it = probabilities.rbegin();
   P = it->first;
   return it->second;
}

double KVMultiGaussIsotopeFit::GetMeanA(double PID) const
{
   // for a given PID, calculate the weighted sum of A
   auto total = Eval(PID);
   int ig = 1;
   double amean(0), totprob(0);
   for (auto& a : Alist) {
      auto weight = evaluate_gaussian(ig, PID) / total;
      amean += weight * a;
      totprob += weight;
      ++ig;
   }
   return totprob > 0 ? amean / totprob : -1.;
}

std::map<int, double> KVMultiGaussIsotopeFit::GetADistribution(double PID) const
{
   // For the given PID, the map is filled with all possible values of A and the associated probability
   auto total = Eval(PID);
   std::map<int, double> Adist;
   int ig = 1;
   for (auto& a : Alist) {
      Adist[a] = evaluate_gaussian(ig, PID) / total;
      ++ig;
   }
   return Adist;
}

int KVMultiGaussIsotopeFit::GetA(double PID, double& P) const
{
   // Probabilistic method to determine A from PID.
   //
   // The A returned will be drawn at random from the probability distribution given by the
   // sum of all gaussians (and the background) for the given PID.
   //
   // The result of the draw may be that this PID is part of the background noise:
   // in this case we return 0
   //
   // P is the probability of the chosen result.

   auto total = Eval(PID);
   double p_tot = 0;
   int ig = 1;
   auto X = gRandom->Uniform();
   for (auto& a : Alist) {
      auto w = evaluate_gaussian(ig, PID) / total;
      p_tot += w;
      if (X < w) {
         P = w;
         return a;
      }
      X -= w;
      ++ig;
   }
   P = 1. - p_tot;
   return 0; // background noise
}

double KVMultiGaussIsotopeFit::GetProbability(int A, double PID) const
{
   // \param[in] A isotope mass number
   // \param[in] PID value of PID associated with A
   // \return the probability that A is the correct mass number for a given PID value
   // \return zero if A is not associated with a Gaussian in the fit

   auto it = std::find(std::begin(Alist), std::end(Alist), A);
   if (it != std::end(Alist)) {
      int ig = std::distance(std::begin(Alist), it);
      return evaluate_gaussian(ig, PID) / Eval(PID);
   }
   return 0;
}



void KVMultiGaussIsotopeFit::SetFitRange(double min, double max)
{
   // Change range of fit

   SetRange(min, max);
   PIDmin = min;
   PIDmax = max;
}

ClassImp(KVMultiGaussIsotopeFit)
