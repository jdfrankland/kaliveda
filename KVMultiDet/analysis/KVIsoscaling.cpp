#include "KVIsoscaling.h"
#include "KVString.h"

#include "TGraphErrors.h"
#include "TString.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TMath.h"
#include "TF1.h"
#include "TFile.h"
#include "TAxis.h"

#include <fstream>

ClassImp(KVIsoscaling)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVIsoscaling</h2>
<h4>KVClass</h4>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////



//____________________________________________________________________________//
Bool_t KVIsoscaling::ReadYieldsFile(Int_t system, const Char_t* file_path)
{
   // Read and save yields from a user's file
   // Construct the corresponding vectors
   // Input file format is :
   // Z     A  Yield Yield_err
   // \param[in] system index
   // \param[in] path for ASCII file

   Bool_t ok = kTRUE;

   // --- Check that the system was not set before ---
   if (fnl_sys_.Contains(system)) {
      ok = kFALSE;
      Warning("ReadYieldsFile", "system %d already set, %s will be ignored", system, file_path);
   }
   else if (fdebug_) Info("ReadYieldsFile", "===== Starting reading system %d =====\n", system);

   // --- Open the file for a new system ---
   std::ifstream my_file;
   my_file.open(file_path);

   if (ok && my_file.is_open()) {
      // --- Init the buffers ---
      Int_t oldZ = -666;
      KVNumberList my_a_nl;         //buffer number list of A for a given (system, Z)
      std::vector<Int_t> my_a_vec;     //buffer list of A (for the current (system, Z))
      std::vector<Float_t> my_yield_vec;  //buffer list of Yields (for the current (system, Z))
      std::vector<Float_t> my_yield_err_vec; //buffer list of Yields (for the current (system, Z))

      // --- Init the variables ---
      std::vector<Int_t>        my_z_vec;       //list of Z for the current system
      std::vector< std::vector<Int_t> >   my_z_a_vec;       //list of A for each Z of the current system
      std::vector< std::vector<Float_t> > my_z_yield_vec;   //list of Yields for each (A,Z) of the current system
      std::vector< std::vector<Float_t> > my_z_yielderr_vec;   //list of Yield errors for each (A,Z) of the current system

      KVNumberList* my_z_nl     = new KVNumberList();    //number list of Z for the current system
      KVList*      my_nla_list = new KVList();        //list of all number list of A for each (system, Z)

      // --- Read the file ---
      Int_t nline = 0;
      std::string line;
      while (getline(my_file, line)) {
         TString my_str(line);

         // --- Ignore comments ---
         if (my_str.BeginsWith("#")) continue;

         // --- Extract infos ---
         TObjArray* my_str_array = my_str.Tokenize(" \t,;");

         //debug
         /*if(fdebug_){
            Int_t nn = my_str_array->GetEntries();
            for(Int_t jj=0; jj<nn; jj++){
               printf("Tokenize[%d] = %s\n", jj, (((TObjString *) my_str_array->At(jj))->String()).Data());
            }
         }
         */

         Int_t zz    = ((TObjString*) my_str_array->At(0))->String().Atoi();
         Int_t aa    = ((TObjString*) my_str_array->At(1))->String().Atoi();
         Float_t yy  = ((TObjString*) my_str_array->At(2))->String().Atof();
         Float_t yy_err = ((TObjString*) my_str_array->At(3))->String().Atof();

         //--- Check of the charge Z ---
         // If new Z value :
         // - Add the new Z in the list of Z (for the current system)
         // - Save the current mass vector (for the former Z and the current system)
         // - Clear the mass buffer vector
         // - Reset the 'actualA' buffer
         if (zz != oldZ) {
            // - Save the current Z -
            my_z_vec.push_back(zz);
            my_z_nl->Add(zz);

            //debug
            if (fdebug_) Info("ReadYieldsFile", "=== New Z=%d ===", zz);

            // - Save the list of A for the former Z (except at initialization) -
            if (oldZ != -666) {
               my_z_a_vec.push_back(my_a_vec);
               my_z_yield_vec.push_back(my_yield_vec);
               my_z_yielderr_vec.push_back(my_yield_err_vec);
               my_nla_list->Add(new KVNumberList(my_a_nl.GetList()));

               if (fdebug_) {
                  KVNumberList* nl = (KVNumberList*) my_nla_list->Last();
                  printf("Saving the former Z(=%d) associated A list=(%s)\n", oldZ, nl->GetList());
               }
            }

            //- Reset -
            my_a_vec.clear();
            my_yield_vec.clear();
            my_yield_err_vec.clear();
            my_a_nl.Clear();
         }// end of new Z

         // --- In any case, add data ---
         // - We add the mass to the actual A list (for the current Z and system)
         // - We add the yield to the actual Y(system, A, Z) list
         // - We add the yield_err to the actual Y_err(suystem, A, Z) list
         my_a_vec.push_back(aa);
         my_yield_vec.push_back(yy);
         my_yield_err_vec.push_back(yy_err);
         my_a_nl.Add(aa);

         // --- Debug ---
         if (fdebug_) printf("(line=%d) (zz=%d, aa=%d, yy=%lf, yy_err=%lf)\n", nline, zz, aa, yy, yy_err);

         //- Update the Z buffer -
         oldZ = zz;

         nline++;
      }//end getline

      // - Save the last values -
      my_nla_list->Add(new KVNumberList(my_a_nl.GetList()));
      my_z_a_vec.push_back(my_a_vec);
      my_z_yield_vec.push_back(my_yield_vec);
      my_z_yielderr_vec.push_back(my_yield_err_vec);
      if (fdebug_) {
         KVNumberList* nl = (KVNumberList*) my_nla_list->Last();
         printf("Saving the former Z(=%d) associated A list=(%s)\n", oldZ, nl->GetList());
      }

      // --- Fill the global vectors ---
      fvec_sys_.push_back(system);
      fvec_sys_z_.push_back(my_z_vec);
      fvec_sys_z_a_.push_back(my_z_a_vec);
      fvec_sys_z_yields_.push_back(my_z_yield_vec);
      fvec_sys_z_yields_err_.push_back(my_z_yielderr_vec);

      // --- Fill the global number lists ---
      flist_z_.Add(my_z_nl);
      flist_a_.Add(my_nla_list);

      // --- Add the system to the number list --
      fnl_sys_.Add(system);

      // --- Close file ---
      my_file.close();

      //debug
      Info("ReadYieldsFile", "[sys=%d] file %s closed, masses and yields saved at position %d\n", system, file_path, int(fvec_sys_.size() - 1));

   }//end file.is_opened

   // --- Handle the errors ---
   else {
      if (!ok) Error("ReadYieldsFile", "!!! System=%d already set !!!", system);
      if (!my_file.is_open()) {
         Error("ReadYieldsFile", "!!! Can't open file '%s' !!!", file_path);
         ok = kFALSE;
      }
   }

   // --- Push back of meanA vectors for the given system ---
   // (It allows to keep the same ordering in systems for the <A(Z)> and <A(Z)>_err vectors )
   std::vector<Float_t> my_dumb_vec;
   fvec_sys_z_meanA_.push_back(my_dumb_vec);
   fvec_sys_z_meanerrA_.push_back(my_dumb_vec);

   // --- Apply a gaussian fit to the provided yields ---
   BuildGaussianPlots(system);

   return ok;
}

//____________________________________________________________________________//
void KVIsoscaling::BuildGaussianPlots(Int_t system)
{
   // Method to build the plots used to check the gaussian approximation
   // Also compute the average masses for each charge <A(Z)>
   // \param[in] system index

   if (fdebug_) Info("BuildGaussianPlots", "====== STARTING BUILDING GAUSSIAN PLOTS FOR SYSTEM %d ======", system);

   // ---- Check that system is OK ---
   if (fnl_sys_.Contains(system)) {

      // - vector of the mean values -
      std::vector<Float_t> my_meanA_vec;
      std::vector<Float_t> my_meanAerr_vec;

      // --- Iterate over existing Z ---
      Int_t pos = GetSystemPos(system);

      KVNumberList* nl_zz = GetZNumberList(system);
      nl_zz->Begin();
      while (!nl_zz->End()) {
         Int_t next_zz = nl_zz->Next();

         // - create the graphs -
         TGraphErrors* gr = new TGraphErrors();
         gr->SetName(Form("gr_yields_%d_vs_N_%d", system, next_zz));
         gr->GetXaxis()->SetTitle("N");
         gr->GetYaxis()->SetTitle("Y1");
         gr->SetMarkerStyle(20);
         fhlist_yields_.Add(gr);

         TGraphErrors* grln = new TGraphErrors();
         grln->SetName(Form("gr_lnyields_%d_vs_N_%d", system, next_zz));
         grln->GetXaxis()->SetTitle("N");
         grln->GetYaxis()->SetTitle("Ln(Y1)");
         grln->SetMarkerStyle(20);
         fhlist_yields_.Add(grln);

         if (fdebug_) Info("BuildGaussianPlots", "Graph %s created", gr->GetName());

         Int_t pos_zz = GetZPos(system, next_zz);
         std::vector<Int_t> aa_vec      = GetAVectorFromPos(pos, pos_zz);
         std::vector<Float_t> yy_vec    = GetYieldVectorFromPos(pos, pos_zz);
         std::vector<Float_t> yyerr_vec = GetYieldErrVectorFromPos(pos, pos_zz);

         KVNumberList* nl_aa = GetANumberList(system, next_zz);
         Int_t npoint = 0;
         nl_aa->Begin();
         while (!nl_aa->End()) {
            Int_t next_aa = nl_aa->Next();
            Int_t nn      = next_aa - next_zz;
            Int_t aa_pos  = GetAPos(system, next_zz, next_aa);

            Float_t yield    = yy_vec.at(aa_pos);
            Float_t yielderr = yyerr_vec.at(aa_pos);

            gr->SetPoint(npoint, nn, yield);
            gr->SetPointError(npoint, 0, yielderr);

            grln->SetPoint(npoint, nn, TMath::Log(yield));
            grln->SetPointError(npoint, 0, TMath::Abs(yielderr / yield));

            npoint++;

            if (fdebug_) Info("BuildGaussianPlots", "[Z=%d, N=%d, A=%d] Yield(%d)=%lf +/- %lf", next_zz, next_aa - next_zz, next_aa, system, yield, yielderr);
         }//end a iter

         // --- Apply individual gaussian fit and save the results ---
         TF1* gaus = new TF1(Form("gaus_yields_%d_vs_N_%d", system, next_zz), "gaus(0)", 0, 60);
         gr->Fit(Form("gaus_yields_%d_vs_N_%d", system, next_zz), "Q");
         fhlist_yields_.Add(gaus);

         // - Add the average <A(Z)> values -
         my_meanA_vec.push_back(gaus->GetParameter(1) + next_zz);
         my_meanAerr_vec.push_back(gaus->GetParError(1));

      }//end Z-iter

      // --- Save the <A(Z)> vectors ---
      Int_t syspos = GetSystemPos(system);
      if (syspos > -1) {
         fvec_sys_z_meanA_.at(syspos)    = my_meanA_vec;
         fvec_sys_z_meanerrA_.at(syspos) = my_meanAerr_vec;
      }
   }//end system ok

   if (fdebug_) Info("BuildGaussianPlots", "====== END BUILDING GAUSSIAN PLOTS FOR SYSTEM %d ======", system);
}

//____________________________________________________________________________//
void KVIsoscaling::TestGaussianApprox(Int_t system1, Int_t system2, Int_t zz, Double_t tol)
{
   // Method to test the tolerance threshold of the gaussian approximation for a fixed charge \f$Z\f$.
   // Assuming a tolerance \f$tol\f$ set by the user and an average mass \f$\langle A(Z)\rangle\f$ with a standard deviation \f$ rms(Z) \f$,
   // the region of experimental points will be limited to \f$ \langle A(Z)\rangle \pm tol \cdot rms(Z) \f$.
   // This method allows to test and plot the results of various tolerance thresholds.
   // \param[in] system1 n-deficient system index
   // \param[in] system2 n-rich system index
   // \param[in] zz fixed charge for the isotopic yields to be tested
   // \param[in] tol threshold to be tested

   TGraphErrors* gr_sys1_all = (TGraphErrors*) fhlist_yields_.FindObject(Form("gr_yields_%d_vs_N_%d", system1, zz));
   TGraphErrors* gr_sys2_all = (TGraphErrors*) fhlist_yields_.FindObject(Form("gr_yields_%d_vs_N_%d", system2, zz));
   TF1* gaus1 = (TF1*) fhlist_yields_.FindObject(Form("gaus_yields_%d_vs_N_%d", system1, zz));
   TF1* gaus2 = (TF1*) fhlist_yields_.FindObject(Form("gaus_yields_%d_vs_N_%d", system2, zz));

   // --- Build the graphs with thresholds and ratios of the yields ---
   // Use only the A (N) shared between the 2 systems
   TGraphErrors* gr_sys1_tol = new TGraphErrors();
   gr_sys1_tol->SetName(Form("gr_yields_%d_vs_N_%d_tol", system1, zz));
   TGraphErrors* gr_sys2_tol = new TGraphErrors();
   gr_sys2_tol->SetName(Form("gr_yields_%d_vs_N_%d_tol", system2, zz));

   TGraphErrors* gr_ratio_all = new TGraphErrors();
   gr_ratio_all->SetName(Form("gr_ratio_all_%d_%d_%d", system1, system2, zz));
   TGraphErrors* gr_ratio_tol = new TGraphErrors();
   gr_ratio_tol->SetName(Form("gr_ratio_tol_%d_%d_%d", system1, system2, zz));

   // - First find the position in vectors corresponding to our two systems -
   Int_t pos1 = GetSystemPos(system1);
   Int_t pos2 = GetSystemPos(system2);
   // - Then find charge pos. in vector and associated values -
   Int_t pos_zz1 = GetZPos(system1, zz);
   Int_t pos_zz2 = GetZPos(system2, zz);

   std::vector<Int_t> aa1_vec      = GetAVectorFromPos(pos1, pos_zz1);
   std::vector<Int_t> aa2_vec      = GetAVectorFromPos(pos2, pos_zz2);
   std::vector<Float_t> yy1_vec    = GetYieldVectorFromPos(pos1, pos_zz1);
   std::vector<Float_t> yy2_vec    = GetYieldVectorFromPos(pos2, pos_zz2);
   std::vector<Float_t> yyerr1_vec = GetYieldErrVectorFromPos(pos1, pos_zz1);
   std::vector<Float_t> yyerr2_vec = GetYieldErrVectorFromPos(pos2, pos_zz2);

   // - Extract all A shared between the 2 systems -
   KVNumberList nl_inter_a = GetSharedANumberList(system1, system2, zz);

   if (fdebug_) Info("TestRMSTolerance", "=== Starting iteration over shared A list for Z=%d ===", zz);

   // - Build graphs with tolerance applied -
   Int_t npoint_all = 0;
   Int_t np1 = 0;
   Int_t np2 = 0;
   Int_t npoint_tol = 0;
   nl_inter_a.Begin();
   while (!nl_inter_a.End()) {
      Int_t next_aa = nl_inter_a.Next();
      Int_t nn = next_aa - zz;
      Int_t aa1_pos = GetAPos(system1, zz, next_aa);
      Int_t aa2_pos = GetAPos(system2, zz, next_aa);

      Float_t yield1 = yy1_vec.at(aa1_pos);
      Float_t yield2 = yy2_vec.at(aa2_pos);

      Float_t yielderr1 = yyerr1_vec.at(aa1_pos);
      Float_t yielderr2 = yyerr2_vec.at(aa2_pos);

      Float_t ratio     = yield2 / yield1;
      Float_t ratio_err = TMath::Sqrt(TMath::Power(yielderr2 / yield1, 2.) + TMath::Power(yielderr1 * yield2 / yield1 / yield1, 2.));

      // - Fill the graph of the ratios with all points -
      gr_ratio_all->SetPoint(npoint_all, nn, ratio);
      gr_ratio_all->SetPointError(npoint_all, 0, ratio_err);
      npoint_all++;

      // - Apply tolerance cut for the gaussian approximation -
      Double_t diff1 = TMath::Abs(gaus1->GetParameter(1) - nn);
      Double_t diff2 = TMath::Abs(gaus2->GetParameter(1) - nn);

      if (diff1 < tol * gaus1->GetParameter(2)) {
         gr_sys1_tol->SetPoint(np1, nn, yield1);
         gr_sys1_tol->SetPointError(np1, 0., yielderr1);
         np1++;
      }

      if (diff2 < tol * gaus2->GetParameter(2)) {
         gr_sys2_tol->SetPoint(np2, nn, yield2);
         gr_sys2_tol->SetPointError(np2, 0., yielderr2);
         np2++;
      }

      if ((diff1 < tol * gaus1->GetParameter(2)) && (diff2 < tol * gaus2->GetParameter(2))) {
         gr_ratio_tol->SetPoint(npoint_tol, nn, ratio);
         gr_ratio_tol->SetPointError(npoint_tol, 0, ratio_err);
         npoint_tol++;
      }
   }//end shared A iter

   // --- Build ratio of the gaussian fits ---
   TF1* gaus_ratio = new TF1(Form("gaus_ratio_%d_%d_%d", system1, system2, zz), "gaus(0)/(gaus(3))", 0, 60);
   gaus_ratio->SetLineColor(kBlack);
   gaus_ratio->SetParameters(gaus2->GetParameter(0), gaus2->GetParameter(1), gaus2->GetParameter(2), gaus1->GetParameter(0), gaus1->GetParameter(1), gaus1->GetParameter(2));

   // --- Set some draw opt ---
   gr_sys1_all->SetMarkerColor(kRed);
   gr_sys1_all->SetMarkerStyle(24);
   gr_sys2_all->SetMarkerColor(kBlue);
   gr_sys2_all->SetMarkerStyle(26);
   gr_ratio_all->SetMarkerColor(kBlack);
   gr_ratio_all->SetMarkerStyle(25);

   gr_sys1_tol->SetMarkerColor(kRed);
   gr_sys1_tol->SetMarkerStyle(20);
   gr_sys2_tol->SetMarkerColor(kBlue);
   gr_sys2_tol->SetMarkerStyle(22);
   gr_ratio_tol->SetMarkerColor(kBlack);
   gr_ratio_tol->SetMarkerStyle(21);

   gaus1->SetLineColor(kRed);
   gaus2->SetLineColor(kBlue);
   gaus_ratio->SetLineColor(kBlack);

   // --- Draw ---
   gr_ratio_all->Draw("ap");
   //gPad->SetLogy();
   gr_sys1_all->Draw("p");
   gr_sys2_all->Draw("p");
   gaus1->Draw("same");
   gaus2->Draw("same");
   gaus_ratio->Draw("same");
   gr_sys1_tol->Draw("p");
   gr_sys2_tol->Draw("p");
   gr_ratio_tol->Draw("p");
}

//____________________________________________________________________________//
void KVIsoscaling::BuildLnR21vsNPlots(Int_t system1, Int_t system2)
{
   // Method to compute the Ln of the ratio 'LnR21' of the yields 'Yi(A,Z)' of 2 asymetric reactions noted '(2)' and '(1)'
   // (projectiles and targets of reactions (2) and (1) only differ in their respective number of neutrons)
   // We are interested in extracting the alpha isoscaling parameter, thus we want to draw LnR21 as a function of N for a given Z

   // Added 27/10/2021 : addition of "np" = number of points (isotopes) used to build the plots
   // if np=-1 use all the isotopes (as before)

   // [in] system1  n-deficient system index
   // [in] system2  n-rich system index

   if (fdebug_) Info("BuildLnR21vsNPlots", "====== STARTING BUILDING PLOTS FOR %d/%d ======", system1, system2);

   // --- Check the systems ---
   if (system1 != system2 && fnl_sys_.Contains(system1) && fnl_sys_.Contains(system2)) {
      // --- Create a vector of the ratios for each Z available ---
      // - First find the position in vectors corresponding to our two systems -
      Int_t pos1 = GetSystemPos(system1);
      Int_t pos2 = GetSystemPos(system2);

      // - Then create the corresponding number list of shared Z for the 2 systems
      KVNumberList nl_inter = GetSharedZNumberList(system1, system2);

      // - Debug -
      if (fdebug_) {
         Info("BuildLnR21vsNPlots", "system1=%d is at position %d | system2=%d is at position %d", system1, pos1, system2, pos2);
         printf("The list of shared Z is '%s'\n", nl_inter.GetList());
      }

      // - Now we have the list of shared Z to use -
      // We find the corresponding masses, yields and yields errors positions in vectors

      Int_t ip = 0;
      nl_inter.Begin();
      while (!nl_inter.End()) {
         Int_t next_zz = nl_inter.Next();

         // - Create the associated graph for ratios -
         TGraphErrors* gr_all = new TGraphErrors();
         gr_all->SetName(Form("gr_lnR21_vs_N_%d_%d_%d_all", system1, system2, next_zz));
         gr_all->GetXaxis()->SetTitle("N");
         gr_all->GetYaxis()->SetTitle("ln(R21)");
         gr_all->SetMarkerStyle(20);
         gr_all->SetMarkerColor(next_zz % 9 + 1);
         fhlist_lnR21N_all_.Add(gr_all);

         TGraphErrors* gr = new TGraphErrors();
         gr->SetName(Form("gr_lnR21_vs_N_%d_%d_%d", system1, system2, next_zz));
         gr->GetXaxis()->SetTitle("N");
         gr->GetYaxis()->SetTitle("ln(R21)");
         gr->SetMarkerStyle(20);
         gr->SetMarkerColor(next_zz % 9 + 1);
         fhlist_lnR21N_.Add(gr);

         if (fdebug_) Info("BuildLnR21vsNPlots", "Graph %s created", gr->GetName());

         // - Find individual fits of the yields -
         TF1* gaus_np = (TF1*) fhlist_yields_.FindObject(Form("gaus_yields_%d_vs_N_%d", system1, next_zz));
         TF1* gaus_nr = (TF1*) fhlist_yields_.FindObject(Form("gaus_yields_%d_vs_N_%d", system2, next_zz));
         Double_t par0     = gaus_nr->GetParameter(0);
         Double_t par0_err = gaus_nr->GetParError(0);
         Double_t par1     = gaus_nr->GetParameter(1);
         Double_t par1_err = gaus_nr->GetParError(1);
         Double_t par2     = gaus_nr->GetParameter(2);
         Double_t par2_err = gaus_nr->GetParError(2);
         Double_t par3     = gaus_np->GetParameter(0);
         Double_t par3_err = gaus_np->GetParError(0);
         Double_t par4     = gaus_np->GetParameter(1);
         Double_t par4_err = gaus_np->GetParError(1);
         Double_t par5     = gaus_np->GetParameter(2);
         Double_t par5_err = gaus_np->GetParError(2);

         // - Compute gauss2/gauss1 function -
         TF1* gaus_ratio = new TF1(Form("gaus_ratio_%d_%d_%d", system1, system2, next_zz), "gaus(0)/(gaus(3))", 0, 60);
         gaus_ratio->SetLineColor(kBlack);
         gaus_ratio->SetParameters(par0, par1, par2, par3, par4, par5);
         fhlist_yields_.Add(gaus_ratio);

         TF1* gaus_ratio_ln = new TF1(Form("gaus_ratio_ln_%d_%d_%d", system1, system2, next_zz), "TMath::Log(gaus(0)/(gaus(3)))", 0, 60);
         gaus_ratio_ln->SetLineColor(kBlack);
         gaus_ratio_ln->SetParameters(par0, par1, par2, par3, par4, par5);
         fhlist_yields_.Add(gaus_ratio_ln);

         // - Find charge pos. in vector and associated values -
         Int_t pos_zz1 = GetZPos(system1, next_zz);
         Int_t pos_zz2 = GetZPos(system2, next_zz);

         std::vector<Int_t> aa1_vec = GetAVectorFromPos(pos1, pos_zz1);
         std::vector<Int_t> aa2_vec = GetAVectorFromPos(pos2, pos_zz2);

         std::vector<Float_t> yy1_vec = GetYieldVectorFromPos(pos1, pos_zz1);
         std::vector<Float_t> yy2_vec = GetYieldVectorFromPos(pos2, pos_zz2);

         std::vector<Float_t> yyerr1_vec = GetYieldErrVectorFromPos(pos1, pos_zz1);
         std::vector<Float_t> yyerr2_vec = GetYieldErrVectorFromPos(pos2, pos_zz2);

         // --- Compute ln(R21) ---
         // - Extract all A shared between the 2 systems -
         KVNumberList nl_inter_a = GetSharedANumberList(system1, system2, next_zz);

         if (fdebug_) Info("BuildLnR21vsNPlots", "=== Starting iteration over shared A list for Z=%d ===", next_zz);

         Int_t npoint = 0;
         Int_t npoint_all = 0;
         nl_inter_a.Begin();
         while (!nl_inter_a.End()) {
            Int_t next_aa = nl_inter_a.Next();
            Int_t nn = next_aa - next_zz;
            Int_t aa1_pos = GetAPos(system1, next_zz, next_aa);
            Int_t aa2_pos = GetAPos(system2, next_zz, next_aa);

            Float_t yield1 = yy1_vec.at(aa1_pos);
            Float_t yield2 = yy2_vec.at(aa2_pos);

            Float_t lnr21  = TMath::Log(yield2 / yield1);

            Float_t yielderr1 = yyerr1_vec.at(aa1_pos);
            Float_t yielderr2 = yyerr2_vec.at(aa2_pos);
            Float_t err       = TMath::Sqrt(TMath::Power(yielderr2 / yield2, 2.) + TMath::Power(yielderr1 / yield1, 2.));
            //Float_t err       = TMath::Abs(yielderr2/yield2) + TMath::Abs(yielderr1/yield1);

            // --- Fill the graph with all points ---
            gr_all->SetPoint(npoint_all, nn, lnr21);
            gr_all->SetPointError(npoint_all, 0, err);
            npoint_all++;

            // --- Apply tolerance cut for the gaussian approximation ---
            Double_t diff1 = TMath::Abs(par1 - nn);
            Double_t diff2 = TMath::Abs(par4 - nn);
            if (diff1 < ftol_ * par2 && diff2 < ftol_ * par5) {
               gr->SetPoint(npoint, nn, lnr21);
               gr->SetPointError(npoint, 0, err);
               npoint++;
            }
            else if (fdebug_) {
               printf("[Z=%d, N=%d, A=%d] Point refused : (tol1=%lf, diff1=%lf), (tol2=%lf, diff2=%lf) - (mean1=%lf, sigma1=%lf), (mean2=%lf, sigma2=%lf) - ",
                      next_zz, next_aa - next_zz, next_aa, ftol_ * par2, diff1, ftol_ * par5, diff2, par1, par2, par4, par5);
            }
            if (fdebug_) Info("BuildLnR21vsNPlots", "[Z=%d, N=%d, A=%d] Yield1(%d)=%lf, Yield2(%d)=%lf", next_zz, next_aa - next_zz, next_aa, system1, yield1, system2, yield2);
         }

         ip++;
      }//end z list iter

      fvec_iso_pairs_.push_back(std::make_pair(system1, system2));

      if (fdebug_) Info("BuildLnR21vsNPlots", "====== END OF BUILDING PLOTS FOR %d/%d ======", system1, system2);

   }//end of systems OK
   else {
      Error("BuildLnR21Plots", "!!! Something is wrong in the input systems !!!");
   }

   return;
}

//____________________________________________________________________________//
void KVIsoscaling::FitLnR21vsNPlots(Int_t system1, Int_t system2, Option_t* option, Option_t* gooption)
{
   // This method apply the fit to extract alpha isoscaling parameters for each charge
   // \param[in] system1 n-deficient system index
   // \param[in] system2 n-rich system index
   // \param[in] option fit options
   // \param[in] gooption graphic fit options

   if (fdebug_) Info("FitLnR21vsNPlots", "====== STARTING FITS FOR %d/%d ======", system1, system2);

   TGraphErrors* gr = NULL;
   TF1*          func = NULL;

   Int_t ip = 0;
   TIter it(&fhlist_lnR21N_);
   while ((gr = (TGraphErrors*) it.Next())) {
      //Tokenize
      KVString my_str(gr->GetName());
      TObjArray* my_str_array = my_str.Tokenize("_"); //gr_lnR21_vs_N_%d_%d_%d

      Int_t sys1  = ((TObjString*) my_str_array->At(4))->String().Atoi();
      Int_t sys2  = ((TObjString*) my_str_array->At(5))->String().Atoi();
      Int_t zz            = ((TObjString*) my_str_array->At(6))->String().Atoi();

      if (sys1 == system1 && sys2 == system2) {
         KVNumberList nl = GetSharedANumberList(sys1, sys2, zz);
         Int_t nmin = nl.First() - zz - 2;
         Int_t nmax = nl.Last() - zz + 2;
         func = new TF1(Form("func_%d_%d_%d", system1, system2, zz), "pol1", float(nmin), float(nmax));
         func->SetLineColor(zz % 9 + 1);

         for (Int_t ii = 0; ii < 10; ii++) gr->Fit(func, option, gooption, float(nmin + 2), float(nmax - 2));

         fhlist_fit_.Add(func);

         ip++;
      }
   }

   if (fdebug_) Info("BuildLnR21vsNPlots", "====== END OF FITS FOR %d/%d ======", system1, system2);
}

//____________________________________________________________________________//
void KVIsoscaling::DrawLnR21vsNFits(Int_t system1, Int_t system2)
{
   // This method allows to draw the results of LnR21 vs N linear fits for the given system1/system2 combination
   // \param[in] system1 n-deficient system index
   // \param[in] system2 n-rich system index

   TMultiGraph* mgr = new TMultiGraph();

   TGraphErrors* gr = NULL;
   TIter it0(&fhlist_lnR21N_);
   while ((gr = (TGraphErrors*) it0.Next())) {
      //Tokenize
      KVString my_str(gr->GetName());
      TObjArray* my_str_array = my_str.Tokenize("_"); //gr_lnR21_vs_N_%d_%d_%d

      Int_t sys1  = ((TObjString*) my_str_array->At(4))->String().Atoi();
      Int_t sys2  = ((TObjString*) my_str_array->At(5))->String().Atoi();
      Int_t zz            = ((TObjString*) my_str_array->At(6))->String().Atoi();

      if (sys1 == system1 && sys2 == system2) {
         mgr->Add(gr);
      }
   }

   mgr->Draw("ap");
   mgr->GetXaxis()->SetTitle("N");
   mgr->GetYaxis()->SetTitle("ln(R21)");

   TF1* func = NULL;
   TIter it1(&fhlist_fit_);
   while ((func = (TF1*) it1.Next())) {
      //Tokenize
      KVString my_str(func->GetName());
      TObjArray* my_str_array = my_str.Tokenize("_"); //func_%d_%d_%d

      Int_t sys1  = ((TObjString*) my_str_array->At(1))->String().Atoi();
      Int_t sys2  = ((TObjString*) my_str_array->At(2))->String().Atoi();
      Int_t zz            = ((TObjString*) my_str_array->At(3))->String().Atoi();

      if (sys1 == system1 && sys2 == system2) func->Draw("same");
   }
}

//____________________________________________________________________________//
void KVIsoscaling::BuildAlphaPlot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw)
{
   // This method allows to create the alpha vs Z plots for a given system1/system2 combination
   // \param[in] system1 n-deficient system index
   // \param[in] system2 n-rich system index
   // \param[in] mcolor color to use for drawing
   // \param[in] mstyle style to use for drawing
   // \param[in] draw =kTRUE if we want to plot the result

   if (fdebug_) Info("BuildAlphaPlot", "====== STARTING BUILDING ALPHA PLOTS FOR %d/%d ======", system1, system2);

   // Compute then plot Csym/T as a function of Z for the given system1/system2 combination
   TGraphErrors* gr = new TGraphErrors();
   gr->SetName(Form("Alpha_%d_%d", system1, system2));
   gr->SetMarkerStyle(20);
   gr->GetXaxis()->SetTitle("Z");
   gr->GetYaxis()->SetTitle("#alpha");
   gr->SetMarkerStyle(mstyle);
   gr->SetMarkerColor(mcolor);

   Int_t np = 0;
   TF1* func = NULL;
   TIter it1(&fhlist_fit_);
   while ((func = (TF1*) it1.Next())) {
      //Tokenize
      KVString my_str(func->GetName());
      TObjArray* my_str_array = my_str.Tokenize("_"); //func_%d_%d_%d

      Int_t sys1  = ((TObjString*) my_str_array->At(1))->String().Atoi();
      Int_t sys2  = ((TObjString*) my_str_array->At(2))->String().Atoi();
      Int_t zz            = ((TObjString*) my_str_array->At(3))->String().Atoi();

      if (sys1 == system1 && sys2 == system2) {
         // --- Get the corresponding Csym/T ---
         Float_t alpha = -666.;
         Float_t alpha_err = -666.;
         Bool_t is_ok = GetAlpha(system1, system2, zz, alpha, alpha_err);

         if (is_ok) {
            gr->SetPoint(np, zz, alpha);
            gr->SetPointError(np, 0, alpha_err);

            np++;
         }
      }
   }//end iter

   fhlist_alpha_.Add(gr);

   if (draw) gr->Draw("ap");

   if (fdebug_) Info("BuildAlphaPlot", "====== END BUILDING ALPHA PLOTS FOR %d/%d ======", system1, system2);
}

//____________________________________________________________________________//
void KVIsoscaling::BuildDeltaZA2Plot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw)
{
   // This method allows to create the delta vs Z plots for a given system1/system2 combination
   // \param[in] system1 n-deficient system index
   // \param[in] system2 n-rich system index
   // \param[in] mcolor color to use for drawing
   // \param[in] mstyle style to use for drawing
   // \param[in] draw =kTRUE if we want to plot the result

   if (fdebug_) Info("BuildDeltaZA2Plot", "====== STARTING BUILDING DELTA PLOTS FOR %d/%d ======", system1, system2);

   // Compute then plot Csym/T as a function of Z for the given system1/system2 combination
   TGraphErrors* gr = new TGraphErrors();
   gr->SetName(Form("Delta_%d_%d", system1, system2));
   gr->SetMarkerStyle(20);
   gr->GetXaxis()->SetTitle("Z");
   gr->GetYaxis()->SetTitle("#Delta");
   gr->SetMarkerStyle(mstyle);
   gr->SetMarkerColor(mcolor);

   Int_t np = 0;
   KVNumberList nl = GetSharedZNumberList(system1, system2);
   nl.Begin();
   while (!nl.End()) {
      Int_t zz      = nl.Next();
      Float_t delta, delta_err;
      Bool_t is_ok = GetDeltaZA2(system1, system2, zz, delta, delta_err, fdebug_);

      if (is_ok) {
         gr->SetPoint(np, zz, delta);
         gr->SetPointError(np, 0., delta_err);
         np++;
      }
   }

   fhlist_delta_.Add(gr);

   if (draw) gr->Draw("ap");

   if (fdebug_) Info("BuildDeltaZA2Plot", "====== END BUILDING DELTA PLOTS FOR %d/%d ======", system1, system2);
}

//____________________________________________________________________________//
void KVIsoscaling::BuildAlphavsDeltaPlot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw)
{
   // This method allows to create the alpha vs delta plots for a given system1/system2 combination
   // \param[in] system1 n-deficient system index
   // \param[in] system2 n-rich system index
   // \param[in] mcolor color to use for drawing
   // \param[in] mstyle style to use for drawing
   // \param[in] draw =kTRUE if we want to plot the result

   if (fdebug_) Info("BuildAlphavsDeltaPlot", "====== STARTING BUILDING DELTA PLOTS FOR %d/%d ======", system1, system2);

   TGraphErrors* gr = new TGraphErrors();
   gr->SetName(Form("Alpha_vs_Delta_%d_%d", system1, system2));
   gr->SetMarkerStyle(20);
   gr->GetYaxis()->SetTitle("#alpha");
   gr->GetXaxis()->SetTitle("#Delta");
   gr->SetMarkerStyle(mstyle);
   gr->SetMarkerColor(mcolor);

   Int_t np = 0;
   KVNumberList nl = GetSharedZNumberList(system1, system2);
   nl.Begin();
   while (!nl.End()) {
      Int_t zz      = nl.Next();
      Float_t delta, delta_err, alpha, alpha_err;
      Bool_t is_ok_delta = GetDeltaZA2(system1, system2, zz, delta, delta_err, fdebug_);
      Bool_t is_ok_alpha = GetAlpha(system1, system2, zz, alpha, alpha_err);

      if (is_ok_delta && is_ok_alpha) {
         gr->SetPoint(np, delta, alpha);
         gr->SetPointError(np, delta_err, alpha_err);
         np++;
      }
   }

   fhlist_alpha_delta_.Add(gr);

   if (draw) gr->Draw("ap");

   if (fdebug_) Info("BuildAlphavsDeltaPlot", "====== END BUILDING DELTA PLOTS FOR %d/%d ======", system1, system2);
}

//____________________________________________________________________________//
void KVIsoscaling::BuildCsymOverTPlot(Int_t system1, Int_t system2, Int_t mcolor, Int_t mstyle, Bool_t draw)
{
   // This method allows to create the Csym/T vs Z plots for a given system1/system2 combination
   // \param[in] system1 n-deficient system index
   // \param[in] system2 n-rich system index
   // \param[in] mcolor color to use for drawing
   // \param[in] mstyle style to use for drawing
   // \param[in] draw =kTRUE if we want to plot the result

   // 2 ways to compute Csym/T (see for ex. Raduta & Gulminelli, PRC 75 044605 (2007):
   // - Eq.(16) : usual isoscaling formula : Csym(<A(Z)>)/T = alpha(Z)/delta; where delta = (Z/<A1(Z)>)**2-(Z/<A2(Z)>)**2
   // - Eq.(15) : Csym/T =
   if (fdebug_) Info("BuildCsymOverTPlot", "====== STARTING BUILDING PLOTS FOR %d/%d ======", system1, system2);

   // Compute then plot Csym/T as a function of Z for the given system1/system2 combination
   TGraphErrors* gr = new TGraphErrors();
   gr->SetName(Form("CsymOverT_%d_%d", system1, system2));
   gr->SetMarkerStyle(mstyle);
   gr->SetMarkerColor(mcolor);
   gr->GetXaxis()->SetTitle("Z");
   gr->GetYaxis()->SetTitle("#alpha/4#Delta");

   Int_t np = 0;
   TF1* func = NULL;
   TIter it1(&fhlist_fit_);
   while ((func = (TF1*) it1.Next())) {
      //Tokenize
      KVString my_str(func->GetName());
      TObjArray* my_str_array = my_str.Tokenize("_"); //func_%d_%d_%d

      Int_t sys1  = ((TObjString*) my_str_array->At(1))->String().Atoi();
      Int_t sys2  = ((TObjString*) my_str_array->At(2))->String().Atoi();
      Int_t zz            = ((TObjString*) my_str_array->At(3))->String().Atoi();

      if (sys1 == system1 && sys2 == system2) {
         // --- Get the corresponding Csym/T ---
         Float_t csymT = -666.;
         Float_t csymT_err = -666.;
         Bool_t is_ok = GetCsymOverT(system1, system2, zz, csymT, csymT_err, fdebug_);

         if (is_ok) {
            gr->SetPoint(np, zz, csymT);
            gr->SetPointError(np, 0, csymT_err);
            np++;
         }
      }
   }//end iter

   fhlist_csymT_.Add(gr);

   if (draw) gr->Draw("ap");

   if (fdebug_) Info("BuildCsymOverTPlot", "====== END BUILDING PLOTS FOR %d/%d ======", system1, system2);
}

//____________________________________________________________________________//
void KVIsoscaling::CreateCsymOverTMultiGraph(TMultiGraph* mgr)
{
   TGraphErrors* gr = NULL;

   Int_t nn = 0;
   TIter it(&fhlist_csymT_);
   while ((gr = (TGraphErrors*) it.Next())) {
      Info("CreateCsymOverTMultiGraph", "%s added to the MultiGraph", gr->GetName());
      mgr->Add(gr);
   }
}

//____________________________________________________________________________//
void KVIsoscaling::SaveResultsROOT(const Char_t* file_name)
{
   // This method allows to save the lnR21, fits and Csym/T graphs in a *.root file
   // \param[in] file_name output file path

   TFile my_file(Form("%s", file_name), "RECREATE");

   fhlist_lnR21N_all_.Write();
   fhlist_lnR21N_.Write();
   fhlist_fit_.Write();
   fhlist_yields_.Write();

   fhlist_delta_.Write();
   fhlist_alpha_.Write();
   fhlist_alpha_delta_.Write();
   fhlist_csymT_.Write();

   if (fdebug_) Info("SaveResultsROOT", "File '%s' saved", file_name);
}

//____________________________________________________________________________//
void KVIsoscaling::SaveResultsASCII(const Char_t* file_name)
{
   // This method allows to save results in a *.txt file
   // \param[in] file_name output file path
   KVString fname(file_name);

   std::ofstream my_file;
   my_file.open(fname.Data(), std::ofstream::out);
   my_file << "### system_1	system_2	Z	Alpha	d(Alpha)	<A_1>	d<A_1>	<A_2>	d<A_2>	denum	d(denum) Csym/T 	d(Csym/T) ###" << std::endl;

   for (std::vector< std::pair<Int_t, Int_t> >::iterator it = fvec_iso_pairs_.begin(); it != fvec_iso_pairs_.end(); ++it) {
      std::pair<Int_t, Int_t> my_pair = *it;
      Int_t sys1 = my_pair.first;
      Int_t sys2 = my_pair.second;

      my_file << "# " << sys1 << " / " << sys2 << " #" << std::endl;

      KVNumberList nl = GetSharedZNumberList(sys1, sys2);
      nl.Begin();
      while (!nl.End()) {
         Int_t next_z = nl.Next();

         // Alpha
         Float_t alpha = -666.;
         Float_t alpha_err = -666.;
         GetAlpha(sys1, sys2, next_z, alpha, alpha_err);

         // Denum
         Float_t denum = -666.;
         Float_t denum_err = -666.;
         GetDeltaZA2(sys1, sys2, next_z, denum, denum_err, kFALSE);

         //Csym/T
         Float_t csymT = -666.;
         Float_t csymT_err = -666.;
         GetCsymOverT(sys1, sys2, next_z, csymT, csymT_err, kFALSE);

         //<A1> and <A2>
         Float_t meanA1 = -666.;
         Float_t meanA1_err = -666.;
         Float_t meanA2 =  -666.;
         Float_t meanA2_err = -666.;
         GetAMean(sys1, next_z, meanA1, meanA1_err);
         GetAMean(sys2, next_z, meanA2, meanA2_err);


         KVString line(Form("%d %d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", sys1, sys2, next_z, alpha, alpha_err, meanA1, meanA1_err, meanA2, meanA2_err, denum, denum_err, csymT, csymT_err));
         my_file << line.Data() << std::endl;
      }

      if (fdebug_) Info("SaveResultsASCII", "[%d/%d] results saved with the following Z list : %s\n", sys1, sys2, nl.GetList());
   }

   my_file.close();

   if (fdebug_) Info("SaveResultsASCII", "File '%s' saved", fname.Data());
}


//============================
//==        GETTERS         ==
//============================
//____________________________________________________________________________//
Bool_t KVIsoscaling::GetAMean(Int_t system, Int_t zz, Float_t& meanA, Float_t& meanA_err)
{
   // return the <A(Z)> for the given system
   Bool_t is_ok = kTRUE;

   Int_t zpos = GetZPos(system, zz);
   KVNumberList* nl =  GetZNumberList(system);

   if (zpos > -1 && nl->Contains(zz)) {
      meanA     = GetAMeanVector(system).at(zpos);
      meanA_err = GetAMeanErrVector(system).at(zpos);
   }
   else {
      if (zpos <= -1) Error("GetAMean", "[sys=%d] Can't find mean A vector for z=%d ", system, zz);
      if (!nl->Contains(zz)) Error("GetAMean", "[sys=%d] Z=%d is out of range", system, zz);
   }

   return is_ok;
}

//____________________________________________________________________________//
Bool_t KVIsoscaling::GetAlpha(Int_t system1, Int_t system2, Int_t zz, Float_t& alpha, Float_t& alpha_err)
{
   // --- Find the corresponding alpha parameter in fit function list ---
   Bool_t is_ok = kFALSE;

   TF1* func = NULL;
   TIter it1(&fhlist_fit_);
   while ((func = (TF1*) it1.Next())) {
      //Tokenize
      KVString my_str(func->GetName());
      TObjArray* my_str_array = my_str.Tokenize("_"); //func_%d_%d_%d

      Int_t sys1        = ((TObjString*) my_str_array->At(1))->String().Atoi();
      Int_t sys2        = ((TObjString*) my_str_array->At(2))->String().Atoi();
      Int_t zzz        = ((TObjString*) my_str_array->At(3))->String().Atoi();

      if (sys1 == system1 && sys2 == system2 && zzz == zz) {
         is_ok = kTRUE;
         alpha    = func->GetParameter(1);
         alpha_err = func->GetParError(1);
      }
   }

   if (!is_ok) Error("GetAlpha", "!!! [Z=%d] (%d, %d) : Fit function not found !!!", zz, system1, system2);

   return is_ok;
}

//____________________________________________________________________________//
Bool_t KVIsoscaling::GetDeltaZA2(Int_t system1, Int_t system2, Int_t zz, Float_t& denum, Float_t& denum_err, Bool_t debug)
{
   // Return the value of the denumerator (Z/<A1>)**2-(Z/<A2>)**2 for Csym/T estimation

   Bool_t is_ok = kFALSE;

   Float_t meanA1     = -666.;
   Float_t meanA2     = -666.;
   Float_t meanA1_err = -666.;
   Float_t meanA2_err = -666.;

   Bool_t ok1 = GetAMean(system1, zz, meanA1, meanA1_err);
   Bool_t ok2 = GetAMean(system2, zz, meanA2, meanA2_err);

   if (ok1 && ok2) {
      is_ok = kTRUE;

      denum     = TMath::Power(1.*zz / meanA1, 2.) - TMath::Power(1.*zz / meanA2, 2.);
      denum_err = TMath::Sqrt(TMath::Power(zz * zz * 2.*meanA1_err / meanA1 / meanA1 / meanA1, 2.) + TMath::Power(zz * zz * 2.*meanA2_err / meanA2 / meanA2 / meanA2, 2.));

      if (debug) Info("GetDeltaZA2", "[Z=%d] (%d / %d) : denum=%lf +/- %lf", zz, system1, system2, denum, denum_err);
   }
   else {
      if (!ok1) Warning("GetDeltaZA2", "<A1> was not found for Z=%d of system %d/%d", zz, system1, system2);
      if (!ok2) Warning("GetDeltaZA2", "<A2> was not found for Z=%d of system %d/%d", zz, system1, system2);
   }

   return is_ok;
}

//____________________________________________________________________________//
Bool_t KVIsoscaling::GetCsymOverT(Int_t system1, Int_t system2, Int_t zz, Float_t& csymT, Float_t& csymT_err, Bool_t debug)
{
   Bool_t is_ok = kFALSE;
   Bool_t is_oka = kFALSE;
   Bool_t is_okd = kFALSE;

   // --- Get corresponding alpha isoscaling parameter ---
   Float_t alpha       = -666.;
   Float_t alpha_err = -666.;
   is_oka = GetAlpha(system1, system2, zz, alpha, alpha_err);

   // --- Get the denum ---
   Float_t denum       = -666.;
   Float_t denum_err = -666.;
   is_okd = GetDeltaZA2(system1, system2, zz, denum, denum_err, debug);

   // --- Compute Csym/T ---
   if (is_oka && is_okd) {
      is_ok = kTRUE;
      csymT     = alpha / 4. / denum;
      csymT_err = TMath::Sqrt(TMath::Power(alpha_err / 4. / denum, 2.) + TMath::Power(alpha * denum_err / 4. / denum / denum, 2.));

      if (debug) {
         Info("GetCsymOverT", "[Z=%d] (%d / %d) : alpha=%lf +/- %lf, denum=%lf +/- %lf, Csym/T=%lf +/- %lf", zz, system1, system2, alpha, alpha_err, denum, denum_err, csymT, csymT_err);
      }
   }

   return is_ok;
}

//____________________________________________________________________________//
Int_t KVIsoscaling::GetSystemPos(Int_t system)
{
   Int_t pos = -1;

   if (fnl_sys_.Contains(system)) {
      Bool_t found = kFALSE;
      Int_t ii = 0;

      while (!found || ii < int(fvec_sys_.size())) {
         Int_t sys = fvec_sys_.at(ii);
         if (sys == system) {
            found = kTRUE;
            pos = ii;
         }

         ii++;
      }
   }
   if (pos == -1) Warning("GetSystemPos", "!!! system %d was not set !!!", system);
   else if (fdebug_) Info("GetSystemPos", "system= %d found at position %d", system, pos);

   return pos;
}

//____________________________________________________________________________//
Int_t KVIsoscaling::GetZPos(Int_t system, Int_t ZZ)
{
// Returns the position of the charge 'ZZ' in the vector corresponding to 'system' in fvec_sys_z_

   Int_t pos_zz     = -1;
   Int_t pos_system = GetSystemPos(system);

   if (pos_system > -1) {
      std::vector<Int_t> my_vec_zz = fvec_sys_z_.at(pos_system);
      Bool_t found = kFALSE;
      Int_t ii = 0;

      while (!found && ii < int(my_vec_zz.size())) {
         Int_t zz = my_vec_zz.at(ii);
         if (zz == ZZ) {
            found = kTRUE;
            pos_zz = ii;
         }

         ii++;
      }

      if (!found) Warning("GetZPos", "!!! Charge Z=%d for system= %d was not found !!!", ZZ, system);
      else if (fdebug_) Info("GetZPos", "Charge Z=%d for system %d is at position %d", ZZ, system, pos_zz);
   }

   return pos_zz;
}

//____________________________________________________________________________//
Int_t KVIsoscaling::GetAPos(Int_t system, Int_t ZZ, Int_t AA)
{
   // Returns the position of the mass 'AA' in the vector corresponding to the (ZZ, system) combination in fvec_sys_z_a_.
   // The returned position can also be used for the 'yields' and 'yields_err' (respectively in fvec_sys_z_yields_ and  fvec_sys_z_yields_err_).

   Int_t pos_aa = -1;
   Int_t pos_system = GetSystemPos(system);
   Int_t pos_zz = GetZPos(system, ZZ);

   if (pos_zz > -1) {
      // first find the vectors of all A for the system (one vector per Z)
      std::vector< std::vector<Int_t> > my_vec_zz_aa = fvec_sys_z_a_.at(pos_system);
      // then find the vector of A for the given (system,Z)
      std::vector<Int_t> my_vec_aa =  my_vec_zz_aa.at(pos_zz);

      Bool_t found = kFALSE;
      Int_t ii = 0;

      while (!found && ii < int(my_vec_aa.size())) {
         Int_t aa = my_vec_aa.at(ii);
         if (aa == AA) {
            found = kTRUE;
            pos_aa = ii;
         }

         ii++;
      }

      if (!found) Warning("GetAPos", "!!! mass A=%d for (system= %d, Z= %d) was not found !!!", AA, system, ZZ);
      else if (fdebug_) Info("GetAPos", "mass A=%d for (system= %d, Z= %d) is at position %d", AA, system, ZZ, pos_aa);
   }

   return pos_aa;
}

//____________________________________________________________________________//
std::vector<Int_t> KVIsoscaling::GetAVector(Int_t system, Int_t ZZ)
{
   Int_t pos_system = GetSystemPos(system);
   Int_t pos_zz = GetZPos(system, ZZ);

   return GetAVectorFromPos(pos_system, pos_zz);
}

std::vector<Int_t> KVIsoscaling::GetAVectorFromPos(Int_t pos_system, Int_t pos_zz)
{
   std::vector< std::vector<Int_t> > my_vec_zz_aa = fvec_sys_z_a_.at(pos_system);
   std::vector<Int_t> my_vec_aa = my_vec_zz_aa.at(pos_zz);

   return my_vec_aa;
}

//____________________________________________________________________________//
std::vector<Float_t> KVIsoscaling::GetYieldVector(Int_t system, Int_t ZZ)
{
   Int_t pos_system = GetSystemPos(system);
   Int_t pos_zz = GetZPos(system, ZZ);

   return GetYieldVectorFromPos(pos_system, pos_zz);
}

std::vector<Float_t> KVIsoscaling::GetYieldVectorFromPos(Int_t pos_system, Int_t pos_zz)
{
   std::vector< std::vector<Float_t> > my_vec_zz_yy = fvec_sys_z_yields_.at(pos_system);
   std::vector<Float_t> my_vec_yy = my_vec_zz_yy.at(pos_zz);

   return my_vec_yy;
}

//____________________________________________________________________________//
std::vector<Float_t> KVIsoscaling::GetYieldErrVector(Int_t system, Int_t ZZ)
{
   Int_t pos_system = GetSystemPos(system);
   Int_t pos_zz = GetZPos(system, ZZ);

   return GetYieldErrVectorFromPos(pos_system, pos_zz);
}

std::vector<Float_t> KVIsoscaling::GetYieldErrVectorFromPos(Int_t pos_system, Int_t pos_zz)
{
   std::vector< std::vector<Float_t> > my_vec_zz_yy = fvec_sys_z_yields_err_.at(pos_system);
   std::vector<Float_t> my_vec_yy = my_vec_zz_yy.at(pos_zz);

   return my_vec_yy;
}

//____________________________________________________________________________//
KVNumberList* KVIsoscaling::GetANumberList(Int_t system, Int_t zz)
{
   Int_t sys_pos = GetSystemPos(system);
   Int_t zz_pos  = GetZPos(system, zz);

   if (fdebug_) Info("GetANumberList", "Looking at sys_pos=%d, z_pos=%d", sys_pos, zz_pos);

   KVList* mylist        = (KVList*)(flist_a_.At(sys_pos));
   KVNumberList* mynlist = (KVNumberList*)(mylist->At(zz_pos));

   return mynlist;
}

//____________________________________________________________________________//
KVNumberList KVIsoscaling::GetSharedZNumberList(Int_t system1, Int_t system2)
{
   // Returns the number list of all Z shared between the 2 systems

   KVNumberList* nl_z1 = GetZNumberList(system1);
   KVNumberList* nl_z2 = GetZNumberList(system2);
   KVNumberList nl_inter;
   nl_z1->Copy(nl_inter);
   nl_inter.Inter(*nl_z2);

   if (fdebug_) {
      Info("GetSharedZNumberList", "Zlist[%d]= %s, Zlist[%d]= %s, shared Zlist= %s",
           system1, nl_z1->GetList(), system2, nl_z2->GetList(), nl_inter.GetList()) ;
   }

   return nl_inter;
}

//____________________________________________________________________________//
KVNumberList KVIsoscaling::GetSharedANumberList(Int_t system1, Int_t system2, Int_t zz)
{
   // Returns the number list of all A shared between the 2 systems
   KVNumberList* nl_a1 = GetANumberList(system1, zz);
   KVNumberList* nl_a2 = GetANumberList(system2, zz);
   KVNumberList nl_inter_a;
   nl_a1->Copy(nl_inter_a);
   nl_inter_a.Inter(*nl_a2);

   if (fdebug_) {
      Info("GetSharedANumberList", "Z=%d : Alist[%d]= %s, Alist[%d]= %s, shared Alist= %s",
           zz, system1, nl_a1->GetList(), system2, nl_a2->GetList(), nl_inter_a.GetList()) ;
   }

   return nl_inter_a;
}

//____________________________________________________________________________//
Int_t KVIsoscaling::FindZFromAmean(Int_t system, Int_t aa)
{
   // Returns the charge ZZ associated to the provided mass AA such as <A(ZZ)> == AA

   Int_t zz = -666;
   Double_t min_val = 10.;

   KVNumberList* nl = GetZNumberList(system);
   nl->Begin();
   while (!nl->End()) {
      Int_t next_zz = nl->Next();

      // extract mean aa
      Float_t aa_real, aa_real_err;
      GetAMean(system, next_zz, aa_real, aa_real_err);

      Double_t  delta_aa = TMath::Abs(aa - aa_real);
      if (delta_aa < min_val) {
         zz = next_zz;
         min_val = delta_aa;

         if (fdebug_) Info("FindZFromAmean", "[%d - A=%d] : Z=%d -> <A(Z)>=%lf +/- %lf [FOUND] delta=%lf, min_val=%lf", system, aa, next_zz, aa_real, aa_real_err, delta_aa, min_val);
      }
      else if (fdebug_) Info("FindZFromAmean", "[%d - A=%d] : Z=%d -> <A(Z)>=%lf +/- %lf [NOT FOUND] delta=%lf, min_val=%lf", system, aa, next_zz, aa_real, aa_real_err, delta_aa, min_val);
   }

   return zz;
}

//============================
//==        PRINTERS        ==
//============================
//____________________________________________________________________________//
void KVIsoscaling::PrintYieldsList()
{
   Info("PrintYields", "Printing the list of yields : ");

   // --- Iterate over systems ---
   Int_t nn_sys = 0;
   for (std::vector<Int_t>::iterator sys_it = fvec_sys_.begin(); sys_it != fvec_sys_.end(); ++sys_it) {
      Int_t system = *sys_it;

      printf("	=====================================\n");

      // --- Iterate over Z for the given system ---
      std::vector<Int_t> zz_vec                 = fvec_sys_z_.at(nn_sys);
      std::vector< std::vector<Int_t> > aa_vec_vec       = fvec_sys_z_a_.at(nn_sys);
      std::vector< std::vector<Float_t> > yy_vec_vec    = fvec_sys_z_yields_.at(nn_sys);
      std::vector< std::vector<Float_t> > yyerr_vec_vec = fvec_sys_z_yields_err_.at(nn_sys);

      Int_t nn_zz = 0;
      for (std::vector<Int_t>::iterator zz_it = zz_vec.begin(); zz_it != zz_vec.end(); ++zz_it) {
         Int_t zz = *zz_it;

         std::vector<Int_t> aa_vec    = aa_vec_vec.at(nn_zz);
         std::vector<Float_t> yy_vec    = yy_vec_vec.at(nn_zz);
         std::vector<Float_t> yyerr_vec = yyerr_vec_vec.at(nn_zz);

         for (std::vector<Float_t>::size_type ii = 0; ii != aa_vec.size(); ii++) {
            Int_t aa      = aa_vec.at(ii);
            Float_t yy       = yy_vec.at(ii);
            Float_t yy_err = yyerr_vec.at(ii);

            printf("	[%d] : Y(%d, %d) = %lf +/- %lf\n", system, aa, zz, yy, yy_err);
         }

         nn_zz++;
      }

      nn_sys++;
   }

   Info("PrintYields", "That's all folks !");
}

//____________________________________________________________________________//
void KVIsoscaling::PrintSystemsList()
{
   fnl_sys_.Begin();
   Int_t nn = 0;

   Info("PrintSystemsList", "Printing the list of available systems : ");

   while (!fnl_sys_.End()) {
      Int_t next_val = fnl_sys_.Next();
      printf("[%d] %d\n", nn, next_val);

      nn++;
   }
}

