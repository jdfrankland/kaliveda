#include "Sive503.h"
#include "KVDataSet.h"

#include "KVINDRADB_e503.h"
#include "KVINDRAe503.h"

#include <cmath>
#include "TRandom3.h"

//Author: Maurycy Rejmund
ClassImp(Sive503)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>Siv</h2>
<p>
Part of the VAMOS analysis package kindly contributed by Maurycy Rejmund (GANIL).
</p>
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

Sive503::Sive503(LogFile* Log)
{
#ifdef DEBUG
   cout << "Si::Constructor" << endl;
#endif

   Ready = kFALSE;

   char line[255];
   int len = 255;
   int i;
   Float_t tmp1, tmp2;

   L = Log;

   for (i = 0; i < 2; i++)
      Counter[i] = 0;

   InitRaw();
   Init();

   Rnd = new TRandom3();

   ifstream inf;

   int tmp = 0;
   //  if(!gDataSet->OpenDataSetFile("Si_calib2.dat",inf))
   if (!gDataSet->OpenDataSetFile("Si.cal", inf)) {
      cout << "Could not open the calibration file Si.cal !" << endl;
      return;
   } else {
      cout.setf(ios::showpoint);
      cout << "Reading Si.cal" << endl;
      L->Log << "Reading Si.cal" << endl;
      inf.getline(line, len);
      //cout << line << endl;
      //L->Log << line << endl;
      while (!inf.eof()) {
         inf.getline(line, len);
         if (!inf.eof() && (tmp < 45)) { //originalement tmp<45
            if (tmp < 21) {
               sscanf(line, "%f %f %f %f %f", ECoef[tmp] + 0, ECoef[tmp] + 1, ECoef[tmp] + 2, &tmp1, &tmp2);
               for (i = 0; i < 3; i++)
                  ECoef[tmp][i] *= tmp1;
               //L->Log<<"ECoef : "<<ECoef[tmp][0]<<" "<<"-524 : "<<ECoef[0][0]<<endl;
               tmp++;
            } else if (tmp < 42) { //originalement tmp<42
               sscanf(line, "%f %f", TOff[tmp - 21] + 0, TOff[tmp - 21] + 1);
               //L->Log<<  "TO " <<TOff[tmp-21][0] << " " << TOff[tmp-21][1]<< endl;
               tmp++;
            } else {
               sscanf(line, "%f %f %f %f %f", TCoef[tmp - 42] + 0, TCoef[tmp - 42] + 1, TCoef[tmp - 42] + 2, TCoef[tmp - 42] + 3, TCoef[tmp - 42] + 4); // originalement tmp-42
               //L->Log <<  "TC " <<TCoef[tmp-42][0] << " " << TCoef[tmp-42][1]<< endl;
               tmp++;
            }
         }
      }
   }
#ifdef DEBUG
   for (int h = 0; h < 21; h++) {
      cout << "Ecoef[" << h << "][0]=" << ECoef[h][0] << " Ecoef[" << h << "][1]=" << ECoef[h][1] << "Ecoef[" << h << "][2]=" << ECoef[h][2] << endl << flush;
   }
#endif

   inf.close();


   Int_t  num;
   Float_t ref;
   Float_t propre;

   Float_t off328;
   Float_t off333;
   Float_t off339;
   Float_t off344;
   Float_t off348;
   Float_t off353;
   Float_t off355;

   Float_t off379;
   Float_t off384;
   Float_t off390;
   Float_t off398;
   Float_t off403;
   Float_t off404;
   Float_t off405;
   Float_t off407;

   Float_t off455;
   Float_t off467;
   Float_t off475;
   Float_t off484;
   Float_t off490;
   Float_t off493;

   Float_t off511;
   Float_t off525;
   Float_t off528;
   Float_t off531;
   Float_t off537;
   Float_t off543;
   Float_t off548;

   TString sline;

   ifstream in2;

   //ToF calibration and offsets according to the system and the run number
   if (!gDataSet->OpenDataSetFile("Offset.cal", in2)) {
      cout << "Could not open the calibration file Offset.cal !!!" << endl;
      return;
   } else {
      cout << "Reading Offset.cal" << endl;
      L->Log << "Reading Offset.cal" << endl;
      while (!in2.eof()) {
         sline.ReadLine(in2);
         if (!in2.eof()) {
            if (!sline.BeginsWith("+") && !sline.BeginsWith("|")) {
               sscanf(sline.Data(), "%d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &num, &ref, &propre, &off328, &off333, &off339, &off344, &off348, &off353, &off355, &off379,
                      &off384, &off390, &off398, &off403, &off404, &off405, &off407, &off455, &off467, &off475, &off484, &off490, &off493, &off511, &off525, &off528, &off531, &off537, &off543, &off548);
               //L->Log << "SI_" << num << ": Ref : "<< ref << endl;
               TRef[num] = ref;
               Tpropre_el[num] = propre;

               Offset328[num] = off328;
               Offset333[num] = off333;
               Offset339[num] = off339;
               Offset344[num] = off344;
               Offset348[num] = off348;
               Offset353[num] = off353;
               Offset355[num] = off355;

               Offset379[num] = off379;
               Offset384[num] = off384;
               Offset390[num] = off390;
               Offset398[num] = off398;
               Offset403[num] = off403;
               Offset404[num] = off404;
               Offset405[num] = off405;
               Offset407[num] = off407;

               Offset455[num] = off455;
               Offset467[num] = off467;
               Offset475[num] = off475;
               Offset484[num] = off484;
               Offset490[num] = off490;
               Offset493[num] = off493;

               Offset511[num] = off511;
               Offset525[num] = off525;
               Offset528[num] = off528;
               Offset531[num] = off531;
               Offset537[num] = off537;
               Offset543[num] = off543;
               Offset548[num] = off548;

               L->Log << "TRef[num] : " << TRef[num] << " Tpropre_el[num] :  " << Tpropre_el[num] << endl;
            }
         }
      }
   }
   in2.close();



   Int_t  numm;
   Int_t thi = 0;
   ifstream in;
   TString sline2;

   if (!gDataSet->OpenDataSetFile("thick.dat", in)) {
      cout << "Could not open the calibration file thick.dat !!!" << endl;
      return;
   } else {
      while (!in.eof()) {
         sline2.ReadLine(in);
         if (!in.eof()) {
            if (!sline2.BeginsWith("#")) {
               sscanf(sline2.Data(), "%d %d", &numm, &thi);
               si_thick[numm] = thi;
            }
         }
      }
   }
   in.close();



   Ready = kTRUE;
}
Sive503::~Sive503(void)
{

   delete Rnd;

#ifdef DEBUG
   cout << "Si::Destructor" << endl;
#endif

   PrintCounters();
}

void Sive503::PrintCounters(void)
{
#ifdef DEBUG
   cout << "Si::PrintCounters" << endl;
#endif

   cout << endl;
   cout << "Si::PrintCounters" << endl;
   cout << "Called: " << Counter[0] << endl;
   cout << "Present: " << Counter[1] << endl;

   L->Log << endl;
   L->Log << "Si::PrintCounters" << endl;
   L->Log << "Called: " << Counter[0] << endl;
   L->Log << "Present: " << Counter[1] << endl;

}


void Sive503::InitRaw(void)
{
#ifdef DEBUG
   cout << "Si::InitRaw" << endl;
#endif

   for (Int_t i = 0; i < 21; i++) {
      E_Raw[i] = 0;
      E_Raw_Nr[i] = 0;
      SiRaw[i] = 0;
   }
   for (Int_t i = 0; i < 3; i++) {
      T_Raw[i] = 0;
   }
   E_RawM = 0;
   T_Raw_SIHF = 0;
}

void Sive503::Init(void)
{
#ifdef DEBUG
   cout << "Sive503::Init" << endl;
#endif
   Present = false;

   for (Int_t i = 0; i < 21; i++) {
      E[i] = 0.0;
   }
   for (Int_t i = 0; i < 3; i++) {
      T[i] = 0.0;
   }
   Offset[0] = Offset[1]  = 0.0;
   Tfrag = 0.0;

   EM = 0;
   ETotal = 0.0;
   Number = -10;
   DetSi = 0;
}

void Sive503::Calibrate(void)
{

#ifdef DEBUG
   cout << "Si::Calibrate" << endl;
#endif

   for (Int_t i = 0; i < 3; i++) {
      //if(T_Raw[i] > 200)
      //for(j=0;j<2;j++)
      //{
      if (T_Raw[i] > TCoef[i][4])
         T[i] = (Float_t) TCoef[i][1] + (T_Raw[i] * TCoef[i][2]);
      if (T_Raw[i] <= TCoef[i][4])
         T[i] = (Float_t) TCoef[i][1] + (T_Raw[i] * TCoef[i][2]) + (TCoef[i][3] * powf((T_Raw[i] - TCoef[i][4]), 2));
      //cout<< "T["<<i<<"] = "<<T[i]<<endl;
      //cout << i << " " << j << " " <<TCoef[i][j] << endl;
      //}
   }

   //==Raw_signal class==
   T_Raw_SIHF = T_Raw[0];
   //====================

   //L->Log<<"Sive503.cpp :   "<<"T_Raw[0] : "<<T_Raw[0]<<" "<<"T[0] : "<<T[0]<<endl;
   //L->Log<<"TCoef[0][1] : "<<TCoef[0][1]<<" "<<"TCoef[0][2] : "<<TCoef[0][2]<<" "<<"TCoef[0][3] : "<<TCoef[0][3]<<" "<<"TCoef[0][4] : "<<TCoef[0][4]<<endl;

   for (Int_t i = 0; i < E_RawM; i++) {
      //cout<<"Si : E_Raw["<<i<<"] = "<<E_Raw[i]<<endl;
      // Rnd->Next();
      Rnd->SetSeed(clock());

      for (Int_t k = 0; k < 3; k++) {
//        E[E_Raw_Nr[i]] += powf((Float_t) E_Raw[i] + Rnd->Value(),
//            (Float_t) k)*ECoef[E_Raw_Nr[i]][k];
         E[E_Raw_Nr[i]] += powf((Float_t) E_Raw[i] + Rnd->Uniform(0, 1),
                                (Float_t) k) * ECoef[E_Raw_Nr[i]][k];

         //==Raw_signal class==
         SI_Raw = E_Raw[i];
         //====================
         //       cout<<"RANDOM-------------"<<Rnd->Uniform(0,1)<<endl<<flush;
         //       cout<<"ECoeff="<<ECoef[E_Raw_Nr[i]][k]<<endl<<flush;
      }
      if (E[E_Raw_Nr[i]] > 0) {
         ETotal = E[E_Raw_Nr[i]];
         //cout<<"ETotal Si = "<<E[E_Raw_Nr[i]]<<endl;
         //cout<<"E Si = "<<E<<endl;
         //L->Log<<"ETotal : "<<ETotal<<endl;

         Number = E_Raw_Nr[i];
         DetSi = int(Number) + 1; //numérotation 1-18

         //T[0]+= TOffset[E_Raw_Nr[i]];        //Add the offset to the TSi_HF depending on the Si detector
         Tfrag = TRef[E_Raw_Nr[i]] + Tpropre_el[E_Raw_Nr[i]] - T[0];

         //L->Log<<"Tfrag avant : "<<Tfrag<<endl;

         //=========================================================================
         //ToF offset according to the system and the run number
         //=========================================================================
         //System : 48Ca + 40Ca
         if (gIndra->GetCurrentRunNumber() > 327 && gIndra->GetCurrentRunNumber() < 333) {
            Tfrag -= Offset328[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset328[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 332 && gIndra->GetCurrentRunNumber() < 339) {
            Tfrag -= Offset333[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset333[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 338 && gIndra->GetCurrentRunNumber() < 344) {
            Tfrag -= Offset339[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset339[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 343 && gIndra->GetCurrentRunNumber() < 348) {
            Tfrag -= Offset344[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset344[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 347 && gIndra->GetCurrentRunNumber() < 351) {
            Tfrag -= Offset348[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset348[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 352 && gIndra->GetCurrentRunNumber() < 355) {
            Tfrag -= Offset353[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset353[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 354 && gIndra->GetCurrentRunNumber() < 361) {
            Tfrag -= Offset355[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset355[E_Raw_Nr[i]]<<endl;
         }
         //=========================================================================
         //System : 48Ca + 48Ca
         if (gIndra->GetCurrentRunNumber() > 378 && gIndra->GetCurrentRunNumber() < 384) {
            Tfrag -= Offset379[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset379[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 383 && gIndra->GetCurrentRunNumber() < 390) {
            Tfrag -= Offset384[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset384[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 389 && gIndra->GetCurrentRunNumber() < 398) {
            Tfrag -= Offset390[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset390[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 397 && gIndra->GetCurrentRunNumber() < 403) {
            Tfrag -= Offset398[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset398[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 402 && gIndra->GetCurrentRunNumber() < 404) {
            Tfrag -= Offset403[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset403[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() == 404) {
            Tfrag -= Offset404[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset404[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 404 && gIndra->GetCurrentRunNumber() < 407) {
            Tfrag -= Offset405[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset405[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 406 && gIndra->GetCurrentRunNumber() < 425) {
            Tfrag -= Offset407[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset407[E_Raw_Nr[i]]<<endl;
         }
         //=========================================================================
         //System : 40Ca + 48Ca
         if (gIndra->GetCurrentRunNumber() > 454 && gIndra->GetCurrentRunNumber() < 466) {
            Tfrag -= Offset455[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset455[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 465 && gIndra->GetCurrentRunNumber() < 475) {
            Tfrag -= Offset467[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset467[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 474 && gIndra->GetCurrentRunNumber() < 484) {
            Tfrag -= Offset475[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset475[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 483 && gIndra->GetCurrentRunNumber() < 490) {
            Tfrag -= Offset484[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset484[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 489 && gIndra->GetCurrentRunNumber() < 494) {
            Tfrag -= Offset490[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset490[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 493 && gIndra->GetCurrentRunNumber() < 510) {
            Tfrag -= Offset493[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset493[E_Raw_Nr[i]]<<endl;
         }
         //=========================================================================
         //System : 40Ca + 40Ca
         if (gIndra->GetCurrentRunNumber() > 510 && gIndra->GetCurrentRunNumber() < 525) {
            Tfrag -= Offset511[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset511[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 524 && gIndra->GetCurrentRunNumber() < 528) {
            Tfrag -= Offset525[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset525[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 527 && gIndra->GetCurrentRunNumber() < 531) {
            Tfrag -= Offset528[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset528[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 530 && gIndra->GetCurrentRunNumber() < 537) {
            Tfrag -= Offset531[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset531[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 536 && gIndra->GetCurrentRunNumber() < 543) {
            Tfrag -= Offset537[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset537[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 542 && gIndra->GetCurrentRunNumber() < 548) {
            Tfrag -= Offset543[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset543[E_Raw_Nr[i]]<<endl;
         }
         if (gIndra->GetCurrentRunNumber() > 547 && gIndra->GetCurrentRunNumber() < 551) {
            Tfrag -= Offset548[E_Raw_Nr[i]];
            //L->Log<<"offset : "<<Offset548[E_Raw_Nr[i]]<<endl;
         }
         //=========================================================================
         //L->Log<<"Tfrag apres : "<<Tfrag<<endl;

         //L->Log<<"TRef : "<<TRef[E_Raw_Nr[i]]<<" Tpropre_el : "<<Tpropre_el[E_Raw_Nr[i]]<<" Tpropre_frag : "<<T[0]<<endl;
         //L->Log<<"thick = "<<si_thick[E_Raw_Nr[i]]<<endl;
         EM++;
      }
   }


   if (EM != 1) {
      ETotal = 0.0;
      Number = 0;
      for (Int_t k = 0; k < 2; k++)
         Offset[k] = 0.0;
   }

   if (ETotal > 0) {
      Present = true;
      Counter[1]++;
   }
//delete Rnd;
}

void Sive503::Treat(void)
{
#ifdef DEBUG
   cout << "Sive503::Treat" << endl;
#endif
   if (!Ready) return;

   Counter[0]++;
   Init();
   Calibrate();
#ifdef DEBUG
   Show_Raw();
   Show();
#endif
}

void Sive503::inAttach(TTree* inT)
{

#ifdef DEBUG
   cout << "Si::inAttach" << endl;
#endif

#ifdef DEBUG
   cout << "Attaching Si variables" << endl;
#endif

#ifdef ACTIVEBRANCHES
   cout << "Activating Branches: Si" << endl;
   L->Log << "Activating Branches: Si" << endl;

   inT->SetBranchStatus("SIEM", 1);
   inT->SetBranchStatus("SIE", 1);
   inT->SetBranchStatus("SIENr", 1);

   inT->SetBranchStatus("TSI_HF", 1);
   inT->SetBranchStatus("TSI_SED", 1);
   inT->SetBranchStatus("TSED_HF", 1);



#endif

   inT->SetBranchAddress("SIEM", &E_RawM);
   inT->SetBranchAddress("SIE", E_Raw);
   inT->SetBranchAddress("SIENr", E_Raw_Nr);

   inT->SetBranchAddress("TSI_HF", T_Raw + 0);
   //inT->SetBranchAddress("TSI_SED",T_Raw+1);
   //inT->SetBranchAddress("TSED_HF",T_Raw+2);

}
void Sive503::outAttach(TTree* outT)
{

#ifdef DEBUG
   cout << "Si::inAttach" << endl;
#endif

#ifdef DEBUG
   cout << "Attaching Si variables" << endl;
#endif

   outT->Branch("SiEM", &EM, "SiEM/I");
   //outT->Branch("SiE",E,"SiE[21]/F");
   //outT->Branch("SiET",&ETotal,"SiET/F");

   //outT->Branch("SiERaw",&E_Raw[0],"SiERaw/S");
   //outT->Branch("SiERaw",SiRaw,"SiERaw[21]/S");
   /*outT->Branch("TSI_HF_raw",T_Raw+0,"TSI_HF_raw/s");
   outT->Branch("TSi_HFpropre",&T[0],"TSi_HFpropre/D");
   outT->Branch("Tfrag",&Tfrag,"Tfrag/D");*/
   outT->Branch("DetSi", &DetSi, "DetSi/I");
   //outT->Branch("TSi_SeD",&T[1],"TSi_SeD/F");
   //outT->Branch("TSeD_HF",&T[2],"TSeD_HF/F");

}
void Sive503::CreateHistograms(void)
{

#ifdef DEBUG
   cout << "Si::CreateHistograms" << endl;
#endif

}
void Sive503::FillHistograms(void)
{

#ifdef DEBUG
   cout << "Si::FillHistograms" << endl;
#endif

}


void Sive503::Show_Raw(void)
{
   Int_t i;
#ifdef DEBUG
   cout << "Si::Show_Raw" << endl;
#endif

   cout.setf(ios::showpoint);

   for (i = 0; i < E_RawM; i++) {
      cout << "CrER: " << E_Raw_Nr[i] << " " << E_Raw[i] << endl;
   }

}
void Sive503::Show(void)
{
   Int_t i;
#ifdef DEBUG
   cout << "Si::Show" << endl;
#endif

   cout.setf(ios::showpoint);

   cout << "Present: " << Present << endl;
   if (Present) {
      cout << "EM: " << EM << endl;
      for (i = 0; i < 21; i++) {
         if (E[i] > 0.) cout << "SiE: " << i << " " << E[i] << endl;
      }
   }
}



