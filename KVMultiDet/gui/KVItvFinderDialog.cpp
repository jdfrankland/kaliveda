//Created by KVClassFactory on Mon Jan 23 10:03:13 2017
//Author: Diego Gruyer

#include "KVItvFinderDialog.h"
#include "TRootEmbeddedCanvas.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TGMsgBox.h"
#include "TGFileDialog.h"
#include "KVTestIDGridDialog.h"
#include "KVIdentificationResult.h"
#include "KVNameValueListGUI.h"
#include <KVMultiGaussIsotopeFit.h>
#include "KeySymbols.h"
#include <thread>

ClassImp(KVItvFinderDialog)
//ClassImp(interval_painter)

// Set default values for mass-fit parameters
KVNameValueList KVItvFinderDialog::mass_fit_parameters{
   {"Limit range of fit", false},
   {"PID min for fit", 0.},
   {"PID max for fit", 10.},
   {"Minimum probability [%]", 50.},
   {"Minimum #sigma", 1.e-2},
   {"Maximum #sigma", 5.e-2}
};

void KVItvFinderDialog::delete_painter_from_painter_list(KVPIDIntervalPainter* p)
{
   // remove painter from list and modify the 'left_painter' and 'right_painter' references
   // in any adjacent painters/intervals, then delete painter

   std::unique_ptr<KVPIDIntervalPainter> _p(p);
   auto pleft = p->get_left_interval();
   auto pright = p->get_right_interval();
   if (pleft) pleft->set_right_interval(pright);
   if (pright) pright->set_left_interval(pleft);
   fItvPaint.Remove(p);
}

KVItvFinderDialog::KVItvFinderDialog(KVIDZAFromZGrid* gg, TH2* hh)//:fSpectrum(7,1)
{
   fGrid  = gg;
   fHisto = hh;

   fPoints  = new TGraph;
   fPoints->SetMarkerStyle(23);
   fNPoints = 0;

   gStyle->SetOptStat(0);
   gStyle->SetOptTitle(0);

   gROOT->ProcessLine(Form("KVItvFinderDialog* _dummy_itv=(KVItvFinderDialog*)%p", this));

   fMain = new TGTransientFrame(gClient->GetDefaultRoot(), gClient->GetDefaultRoot(), 10, 10);
   // Here is the recipe for cleanly closing a window
   // (see https://root-forum.cern.ch/t/error-in-rootx11errorhandler-baddrawable-quot/8095/6)
   fMain->Connect("CloseWindow()", "KVItvFinderDialog", this, "DoClose()");
   fMain->DontCallClose();
   fMain->SetCleanup(kDeepCleanup);
   // afterwards, in the destructor do fMain->CloseWindo(), and in DoClose(), do 'delete this'

   // Default constructor
   TGHorizontalFrame* fCanvasFrame = new TGHorizontalFrame(fMain, 627, 7000, kHorizontalFrame);
   //    fCanvasFrame->SetBackgroundColor(fColor);


   TRootEmbeddedCanvas* fRootEmbeddedCanvas615 = new TRootEmbeddedCanvas(0, fCanvasFrame, 800, 440);
   // to replace the TCanvas in a TRootEmbeddedCanvas, just delete the original like so:
   // (see https://root-forum.cern.ch/t/error-in-rootx11errorhandler-baddrawable-quot/8095/6)
   auto WID = fRootEmbeddedCanvas615->GetCanvasWindowId();
   delete fRootEmbeddedCanvas615->GetCanvas();
   fCanvas = new TCanvas("c123", 10, 10, WID);
   fCanvas->AddExec("toto", "if(_dummy_itv)_dummy_itv->HandleKey();");
   fRootEmbeddedCanvas615->AdoptCanvas(fCanvas);
   fPad = fCanvas->cd();
//   fPad->AddExec("test","printf('coucou\n')");
   fCanvas->SetRightMargin(0.02);
   fCanvas->SetTopMargin(0.02);
   fCanvas->SetLeftMargin(0.08);
   fCanvas->SetBottomMargin(0.07);

   fCanvasFrame->AddFrame(fRootEmbeddedCanvas615, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
   fMain->AddFrame(fCanvasFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 0, 0, 0, 0));

   TGVerticalFrame* fControlOscillo  = new TGVerticalFrame(fCanvasFrame, 2000, 7000, kVerticalFrame);
   //    fControlOscillo->SetBackgroundColor(fColor);


   {
      const char* xpms[] = {
         "filesaveas.xpm",
         "profile_t.xpm",
         "bld_copy.png",
         "ed_new.png",
         "refresh2.xpm",
         "sm_delete.xpm",
         "h1_t.xpm",
         "query_new.xpm",
         "tb_back.xpm",
         "bld_colorselect.png",
         "latex.xpm",
         "move_cursor.png",
         0
      };
      // toolbar tool tip text
      const char* tips[] = {
         "Save intervals in current grid",
         "Find intervals",
         "Create a new interval set",
         "Create a new interval",
         "Update interval lists",
         "Remove selected intervals",
         "Multigauss fit to isotopes in interval set",
         "Set parameters for fit",
         "Remove fit from selected interval set",
         "Test identification",
         "Set log scale on y axis",
         "Unzoom the histogram",
         0
      };
      int spacing[] = {
         5,
         20,
         0,
         0,
         0,
         20,
         50,
         0,
         0,
         50,
         80,
         0,
         0
      };
      const char* method[] = {
         "SaveGrid()",
         "Identify()",
         "NewIntervalSet()",
         "NewInterval()",
         "UpdateLists()",
         "RemoveInterval()",
         "FitIsotopes()",
         "SetFitParameters()",
         "RemoveFit()",
         "TestIdent()",
         "SetLogy()",
         "UnzoomHisto()",
         0
      };
      fNbButtons = 0;
      ToolBarData_t t[50];
      fToolBar = new TGToolBar(fControlOscillo, 450, 80);
      int i = 0;
      while (xpms[i]) {
         t[i].fPixmap = xpms[i];
         t[i].fTipText = tips[i];
         t[i].fStayDown = kFALSE;
         t[i].fId = i + 1;
         t[i].fButton = NULL;
         TGButton* bb = fToolBar->AddButton(fControlOscillo, &t[i], spacing[i]);
         bb->Connect("Clicked()", "KVItvFinderDialog", this, method[i]);
         fNbButtons++;
         i++;
      }
      fControlOscillo->AddFrame(fToolBar, new TGLayoutHints(kLHintsTop | kLHintsExpandX));
   }

   fIntervalSetListView = new KVListView(interval_set::Class(), fControlOscillo, 450, 200);
   fIntervalSetListView->SetDataColumns(3);
   fIntervalSetListView->SetDataColumn(0, "Z", "GetZ", kTextLeft);
   fIntervalSetListView->SetDataColumn(1, "PIDs", "GetNPID", kTextCenterX);
   fIntervalSetListView->SetDataColumn(2, "Masses", "GetListOfMasses", kTextLeft);
   fIntervalSetListView->Connect("SelectionChanged()", "KVItvFinderDialog", this, "DisplayPIDint()");
   fIntervalSetListView->SetDoubleClickAction("KVItvFinderDialog", this, "ZoomOnCanvas()");
   fIntervalSetListView->AllowContextMenu(kFALSE);
   fControlOscillo->AddFrame(fIntervalSetListView, new TGLayoutHints(kLHintsTop | kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));

   fIntervalListView = new KVListView(interval::Class(), fControlOscillo, 450, 180);
   fIntervalListView->SetDataColumns(5);
   fIntervalListView->SetDataColumn(0, "Z", "GetZ", kTextLeft);
   fIntervalListView->SetDataColumn(1, "A", "GetA", kTextCenterX);
   fIntervalListView->SetDataColumn(2, "min", "GetPIDmin", kTextCenterX);
   fIntervalListView->SetDataColumn(3, "pid", "GetPID", kTextCenterX);
   fIntervalListView->SetDataColumn(4, "max", "GetPIDmax", kTextCenterX);



   {
      const char* xpms[] = {
         "arrow_down.xpm",
         "arrow_up.xpm",
         0
      };
      const char* tips[] = {
         "Decrease A by one",
         "Increase A by one",
         0
      };
      int spacing[] = {
         120,
         0,
         0
      };
      const char* method[] = {
         "MassesDown()",
         "MassesUp()",
         0
      };
      fNbButtons = 0;
      ToolBarData_t t[50];
      fToolBar2 = new TGToolBar(fControlOscillo, 450, 80);
      int i = 0;
      while (xpms[i]) {
         t[i].fPixmap = xpms[i];
         t[i].fTipText = tips[i];
         t[i].fStayDown = kFALSE;
         t[i].fId = i + 1;
         t[i].fButton = NULL;
         TGButton* bb = fToolBar2->AddButton(fControlOscillo, &t[i], spacing[i]);
         bb->Connect("Clicked()", "KVItvFinderDialog", this, method[i]);
         fNbButtons++;
         i++;
      }
      fControlOscillo->AddFrame(fToolBar2, new TGLayoutHints(kLHintsTop | kLHintsExpandX));
   }

   //    fCurrentView->ActivateSortButtons();
   fIntervalListView->Connect("SelectionChanged()", "KVItvFinderDialog", this, "SelectionITVChanged()");
   //    fCurrentView->SetDoubleClickAction("FZCustomFrameManager",this,"ChangeParValue()");
   fIntervalListView->AllowContextMenu(kFALSE);

   fControlOscillo->AddFrame(fIntervalListView, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 2, 2, 2));

   fCanvasFrame->AddFrame(fControlOscillo, new TGLayoutHints(kLHintsExpandY, 0, 0, 0, 0));


   //layout and display window
   fMain->MapSubwindows();
   fMain->Resize(fMain->GetDefaultSize());

   // position relative to the parent's window
   fMain->CenterOnParent();

   fMain->SetWindowName("Masses Identification");
   fMain->RequestFocus();
   fMain->MapWindow();

   fIntervalSetListView->Display(((KVIDZAFromZGrid*)fGrid)->GetIntervalSets());
   current_interval_set = nullptr;
   fPad->cd();

   LinearizeHisto(100);
   fLinearHisto->SetLineColor(kBlack);
   fLinearHisto->SetFillColor(kGray + 1);
   fLinearHisto->Draw("hist");

   int tmp[30] = {3, 3, 3, 4, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};
   for (int ii = 0; ii < 30; ii++) fNpeaks[ii] = tmp[ii];

   fSig = 0.1;
   fRat = 0.0001;

   DrawIntervals();

}

//____________________________________________________________________________//

void KVItvFinderDialog::DisplayPIDint()
{
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   Int_t nSelected = list->GetSize();
   if (nSelected == 1) {
      current_interval_set = (interval_set*)list->At(0);
      fIntervalListView->Display(current_interval_set->GetIntervals());
   }
   else {
      current_interval_set = nullptr;
      fIntervalListView->RemoveAll();
   }
   SelectionITVChanged();
}

void KVItvFinderDialog::SelectionITVChanged()
{
   fPad->cd();
   fItvPaint.Execute("HighLight", "0");

   if (fIntervalListView->GetLastSelectedObject()) {
      int zz = ((interval*)fIntervalListView->GetLastSelectedObject())->GetZ();
      int aa = ((interval*)fIntervalListView->GetLastSelectedObject())->GetA();
      KVPIDIntervalPainter* painter = (KVPIDIntervalPainter*)fItvPaint.FindObject(Form("%d_%d", zz, aa));
      if (!painter) Info("SelectionITVChanged", "%d %d not found...", zz, aa);
      painter->HighLight();
   }
   fCanvas->Modified();
   fCanvas->Update();
}

void KVItvFinderDialog::UpdatePIDList()
{
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   Int_t nSelected = list->GetSize();
   if (nSelected == 1) {
      current_interval_set = (interval_set*)list->At(0);
      fIntervalListView->Display(current_interval_set->GetIntervals());
   }
   else {
      current_interval_set = nullptr;
      fIntervalListView->RemoveAll();
   }
}

void KVItvFinderDialog::ZoomOnCanvas()
{
   // Display the interval set for a given Z when the user double clicks on it

   if (!fIntervalSetListView->GetLastSelectedObject()) {
      current_interval_set = nullptr;
      return;
   }
   current_interval_set = dynamic_cast<interval_set*>(fIntervalSetListView->GetLastSelectedObject());
   auto zz = current_interval_set->GetZ();

   fLinearHisto->GetXaxis()->SetRangeUser(zz - 0.5, zz + 0.5);

   fItvPaint.Execute("SetDisplayLabel", "0");

   std::unique_ptr<KVSeqCollection> tmp(fItvPaint.GetSubListWithMethod(Form("%d", zz), "GetZ"));

   tmp->Execute("SetDisplayLabel", "1");

   // Check - if no mass fit is displayed, we look to see if the grid has a saved
   // multigauss fit from a previous session, and if so we display it
   if (!fPad->GetPrimitive(KVMultiGaussIsotopeFit::get_name_of_multifit(zz))) {
      if (fGrid->GetParameters()->HasParameter(Form("MASSFIT_%d", zz))) {
         KVString massfit = fGrid->GetParameters()->GetTStringValue(Form("MASSFIT_%d", zz));
         massfit.ReplaceAll(":", "=");
         KVNameValueList fitparams;
         fitparams.Set(massfit);
         KVMultiGaussIsotopeFit fitfunc(zz, fitparams);
         fitfunc.DrawFitWithGaussians("same");
         // deactivate intervals in painter
         tmp->Execute("DeactivateIntervals", "");
      }
   }

   fCanvas->Modified();
   fCanvas->Update();
}

void KVItvFinderDialog::DrawIntervals()
{
   interval_set* itvs = 0;
   last_drawn_interval = nullptr;
   TIter it(fGrid->GetIntervalSets());
   while ((itvs = (interval_set*)it())) {
      DrawInterval(itvs);
   }
}

void KVItvFinderDialog::DrawInterval(interval_set* itvs, bool label)
{
   fPad->cd();
   interval* itv = nullptr;
   // give a different colour to each interval
   auto nitv = itvs->GetIntervals()->GetEntries();
   auto cstep = TColor::GetPalette().GetSize() / (nitv + 1);
   int i = 1;
   auto deactivate_intervals = fGrid->GetParameters()->HasParameter(Form("MASSFIT_%d", itvs->GetZ()));

   TIter itt(itvs->GetIntervals());
   while ((itv = (interval*)itt())) {
      KVPIDIntervalPainter* dummy = new KVPIDIntervalPainter(itv, fLinearHisto, TColor::GetPalette()[cstep * i], last_drawn_interval);
      ++i;
      if (label) dummy->SetDisplayLabel();
      if (deactivate_intervals) dummy->DeactivateIntervals();
      dummy->Draw();
      dummy->SetCanvas(fCanvas);
      dummy->Connect("IntMod()", "KVItvFinderDialog", this, "UpdatePIDList()");
      fItvPaint.Add(dummy);
      last_drawn_interval = dummy;
   }
}

void KVItvFinderDialog::ClearInterval(interval_set* itvs)
{
   // empty an interval set, effectively removing it from the interval sets which will be saved with the grid.
   //
   // we also remove any previous fits from the grid's parameters

   std::vector<int> alist;
   for (int ii = 0; ii < itvs->GetNPID(); ii++) {
      interval* itv = (interval*)itvs->GetIntervals()->At(ii);
      KVPIDIntervalPainter* pid = (KVPIDIntervalPainter*)fItvPaint.FindObject(Form("%d_%d", itv->GetZ(), itv->GetA()));
      delete_painter_from_painter_list(pid);
      alist.push_back(itv->GetA());
   }
   itvs->GetIntervals()->Clear();
   UpdateLists();
   // remove any fit displayed in pad
   KVMultiGaussIsotopeFit fitfunc(itvs->GetZ(), alist);
   fitfunc.UnDraw(fPad);
   // mop up any stray gaussians (from intervals which have been removed)
   KVMultiGaussIsotopeFit::UnDrawAnyGaussian(itvs->GetZ(), fPad);

   // remove from grid parameters
   if (fGrid->GetParameters()->HasParameter(Form("MASSFIT_%d", itvs->GetZ()))) {
      fGrid->GetParameters()->RemoveParameter(Form("MASSFIT_%d", itvs->GetZ()));
      KVNumberList zlist(fGrid->GetParameters()->GetStringValue("MASSFITS"));
      zlist.Remove(itvs->GetZ());
      fGrid->GetParameters()->SetValue("MASSFITS", zlist.AsString());
   }
   fCanvas->Modified();
   fCanvas->Update();
}

void KVItvFinderDialog::LinearizeHisto(int nbins)
{
   Double_t zmin  = ((KVIDentifier*)fGrid->GetIdentifiers()->First())->GetPID() - 1.0;
   Double_t zmax = 0;

   for (int iz = 1; iz < fGrid->GetIdentifiers()->GetSize() + 1; iz++)  {
      KVIDentifier* tmp = (KVIDentifier*)fGrid->GetIdentifiers()->At(iz);
      if (tmp && tmp->GetPID() > zmax) zmax = tmp->GetPID();
   }

   Int_t zbins = (Int_t)(zmax - zmin) * nbins;

   fLinearHisto = new TH1F("fLinearHisto", "fLinearHisto", zbins, zmin, zmax);

   fGrid->SetOnlyZId();

   // use multi-threading capacities
   auto available_cpu = WITH_MULTICORE_CPU;
   Int_t xbins_per_cpu = fHisto->GetNbinsX() / available_cpu;

   std::vector<std::thread> jobs; // threads to do the work

   // to clean up copies of grid
   KVList grid_copies;

   // do not add copies of grid to ID grid manager
   auto save_auto_add = KVIDGraph::GetAutoAdd();
   KVIDGraph::SetAutoAdd(false);

   std::cout << "Will run " << available_cpu << " threads, each for " << xbins_per_cpu << " bins in X" << std::endl;

   int nthreads = available_cpu;
   // join threads
   std::cout << "Histo linearization using " << nthreads << " threads..." << std::endl;

   for (int job = 0; job < available_cpu; ++job) {
      auto imin = 1 + job * xbins_per_cpu;
      auto imax = (job + 1) * xbins_per_cpu;
      if (job == available_cpu - 1) imax = fHisto->GetNbinsX();

      // make new copy of grid
      auto grid_copy = new KVIDZAFromZGrid(*fGrid);
      grid_copy->Initialize();
      grid_copies.Add(grid_copy);

      // start new thread
      jobs.push_back(std::thread([ =, &nthreads]() {

         KVIdentificationResult idr;

         bool no_mass_id_zone_defined = (grid_copy->GetInfos()->FindObject("MassID") == nullptr);

         for (int i = imin; i <= imax; ++i) {
            for (int j = 1; j <= fHisto->GetNbinsY(); j++) {
               Stat_t poids = fHisto->GetBinContent(i, j);
               if (poids == 0) continue;

               Axis_t x0 = fHisto->GetXaxis()->GetBinCenter(i);
               Axis_t y0 = fHisto->GetYaxis()->GetBinCenter(j);
               Axis_t wx = fHisto->GetXaxis()->GetBinWidth(i);
               Axis_t wy = fHisto->GetYaxis()->GetBinWidth(j);

               if (x0 < 4) continue;

               Double_t x, y;
               Int_t kmax = (Int_t) TMath::Min(20., poids);
               Double_t weight = (kmax == 20 ? poids / 20. : 1.);
               for (int k = 0; k < kmax; k++) {
                  x = gRandom->Uniform(x0 - .5 * wx, x0 + .5 * wx);
                  y = gRandom->Uniform(y0 - .5 * wy, y0 + .5 * wy);
                  if (grid_copy->IsIdentifiable(x, y)) {
                     idr.Clear();
                     grid_copy->KVIDZAGrid::Identify(x, y, &idr);
                     if (no_mass_id_zone_defined || idr.HasFlag(grid_copy->GetName(), "MassID")) {
                        Float_t PID = idr.PID;
                        fLinearHisto->Fill(PID, weight);
                     }
                  }
               }
            }
         }
         --nthreads;
         std::cout << "...remaining threads: " << nthreads << std::endl;
      }));
   }
   for (auto& j : jobs) {
      if (j.joinable()) j.join();
   }

   // reset automatic grid adding to previous state
   KVIDGraph::SetAutoAdd(save_auto_add);
}

void KVItvFinderDialog::Identify()
{
   //    KVBase::OpenContextMenu("Identify(double,double)",this);
   Identify(0.1, 0.001);
}

void KVItvFinderDialog::Identify(double sigma, double ratio)
{
   fSig = sigma;
   fRat = ratio;

   fPad->cd();
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   if (!list->GetSize()) {
      ProcessIdentification(1, TMath::Min(fGrid->GetIdentifiers()->GetSize(), 25));
      for (int ii = 0; ii < fGrid->GetIntervalSets()->GetSize(); ii++) DrawInterval((interval_set*)fGrid->GetIntervalSets()->At(ii), 0);
   }
   else {
      for (int ii = 0; ii < list->GetSize(); ii++) {
         interval_set* itvs = (interval_set*) list->At(ii);
         ProcessIdentification(itvs->GetZ(), itvs->GetZ());
         DrawInterval(itvs, 0);
      }
   }

   fCanvas->Modified();
   fCanvas->Update();

}

void KVItvFinderDialog::SaveGrid()
{
   ExportToGrid();

   TString currentdir(gSystem->ExpandPathName("."));

   TString fn = fHisto->GetName();
   fn += ".dat";

   Int_t ret_code;
   new TGMsgBox(
      gClient->GetRoot(),
      gClient->GetDefaultRoot(),
      "KVIDGridEditor::SaveCurrentGrid", Form("Do you wat to save the grid here : %s", fn.Data()),
      kMBIconExclamation, kMBYes | kMBNo, &ret_code
   );

   if (ret_code == kMBYes) {
      fGrid->WriteAsciiFile(Form("%s", fn.Data()));
      return;
   }

   static TString dir(".");
   const char* filetypes[] = {
      "ID Grid files", "*.dat",
      "All files", "*",
      0, 0
   };
   TGFileInfo fi;
   fi.fFileTypes = filetypes;
   fi.fIniDir = StrDup(dir);
   new TGFileDialog(gClient->GetDefaultRoot(), gClient->GetDefaultRoot(), kFDSave, &fi);
   if (fi.fFilename) {
      TString filenam(fi.fFilename);
      if (filenam.Contains("toto")) filenam.ReplaceAll("toto", fGrid->GetName());
      if (!filenam.Contains('.')) filenam += ".dat";
      fGrid->WriteAsciiFile(filenam.Data());
   }
   dir = fi.fIniDir;
   fGrid->ReloadPIDRanges();

   fIntervalSetListView->Display(((KVIDZAFromZGrid*)fGrid)->GetIntervalSets());
   current_interval_set = nullptr;
   fIntervalListView->RemoveAll();

   fItvPaint.Clear();
   DrawIntervals();
}

void KVItvFinderDialog::ExportToGrid()
{
   // Write all PID intervals in grid parameters "PIDRANGE", "PIDRANGE%d", etc.

   fGrid->ClearPIDIntervals();
   KVNumberList pids;
   interval_set* itvs = 0;
   TIter npid(fGrid->GetIntervalSets());
   while ((itvs = (interval_set*)npid())) {
      if (!itvs->GetNPID()) continue;
      pids.Add(itvs->GetZ());
   }
   fGrid->GetParameters()->SetValue("PIDRANGE", pids.AsString());

   itvs = 0;
   TIter next(fGrid->GetIntervalSets());
   while ((itvs = (interval_set*)next())) {
      if (!itvs->GetNPID()) continue;
      KVString par = Form("PIDRANGE%d", itvs->GetZ());
      KVString val = "";
      interval* itv = 0;
      TIter ni(itvs->GetIntervals());
      while ((itv = (interval*)ni())) {
         val += Form("%d:%lf,%lf,%lf|", itv->GetA(), itv->GetPIDmin(), itv->GetPID(), itv->GetPIDmax());
      }
      val.Remove(val.Length() - 1);
      fGrid->GetParameters()->SetValue(par.Data(), val.Data());
   }
}

void KVItvFinderDialog::NewInterval()
{
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   if (!list->GetSize() || list->GetSize() > 1) {
      current_interval_set = nullptr;
      return;
   }

   current_interval_set = (interval_set*)list->At(0);

   fPad->WaitPrimitive("TMarker");
   auto mm = dynamic_cast<TMarker*>(fPad->GetListOfPrimitives()->Last());
   assert(mm);
   double pid = mm->GetX();
   delete mm;

   int aa = 0;
   int iint = 0;

   // try to guess mass from position (PID)
   // typical isotope separation in PID is 0.12 (e.g. for carbon isotopes)
   // assume that PID=z means A=2*Z
   auto aa_guessstimate = TMath::Nint(current_interval_set->GetZ() * 2 + (pid - current_interval_set->GetZ()) / 0.12);
//Info("NewINt","pid = %f guess a=%d",pid,aa_guessstimate);
   if (!current_interval_set->GetNPID()) {
      aa = aa_guessstimate;
      iint = 0;
   }
   else if (pid < ((interval*)current_interval_set->GetIntervals()->First())->GetPID()) { // to left of all others
      aa = ((interval*)current_interval_set->GetIntervals()->First())->GetA() - 1;
      // NO MORE : use guesstimate as long as it is smaller than A of previously defined interval with largest PID
      // NOW : use 'aa_guessstimate' only for first interval
      //Info("NewInt","before first interval, with a=%d",aa+1);
//      if (aa_guessstimate < aa + 1) aa = aa_guessstimate;
      iint = 0;
   }
   else if (pid > ((interval*)current_interval_set->GetIntervals()->Last())->GetPID()) { // to right of all others
      aa = ((interval*)current_interval_set->GetIntervals()->Last())->GetA() + 1;
      // NO MORE :  use guesstimate as long as it is larger than A of previously defined interval with smallest PID
      // NOW : use 'aa_guessstimate' only for first interval
      //Info("NewInt","after last interval, with a=%d",aa-1);
//      if (aa_guessstimate > aa - 1) aa = aa_guessstimate;
      iint = current_interval_set->GetNPID();
   }
   else {
      // look for intervals between which the new one is places
      for (int ii = 1; ii < current_interval_set->GetNPID(); ii++) {
         if (pid > ((interval*)current_interval_set->GetIntervals()->At(ii - 1))->GetPID()
               && pid < ((interval*)current_interval_set->GetIntervals()->At(ii))->GetPID()) {
            aa = ((interval*)current_interval_set->GetIntervals()->At(ii - 1))->GetA() + 1;
            // use guesstimate if it is in between masses of adjacent intervals (in terms of PID)
            if ((aa_guessstimate > (aa - 1)) &&
                  (aa_guessstimate < ((interval*)current_interval_set->GetIntervals()->At(ii))->GetA()))
               aa = aa_guessstimate;
            iint = ii;
            break;
         }
      }
   }

   interval* itv = new interval(current_interval_set->GetZ(), aa, mm->GetX(), mm->GetX() - 0.05, mm->GetX() + 0.05);
   current_interval_set->GetIntervals()->AddAt(itv, iint);

   // find intervals which are now left (smaller mass) and right (higher mass) than this one
   interval* left_interval = iint > 0 ? (interval*)current_interval_set->GetIntervals()->At(iint - 1) : nullptr;
   interval* right_interval = iint < current_interval_set->GetIntervals()->GetEntries() - 1 ? (interval*)current_interval_set->GetIntervals()->At(iint + 1) : nullptr;
   // find the corresponding painters
   KVPIDIntervalPainter* left_painter{nullptr}, *right_painter{nullptr};
   TIter next_painter(&fItvPaint);
   KVPIDIntervalPainter* pidpnt;
   while ((pidpnt = (KVPIDIntervalPainter*)next_painter())) {
      if (pidpnt->GetInterval() == left_interval) left_painter = pidpnt;
      else if (pidpnt->GetInterval() == right_interval) right_painter = pidpnt;
   }

   // give a different colour to each interval
   auto nitv = current_interval_set->GetIntervals()->GetEntries();
   auto cstep = TColor::GetPalette().GetSize() / (nitv + 1);

   KVPIDIntervalPainter* dummy = new KVPIDIntervalPainter(itv, fLinearHisto, TColor::GetPalette()[cstep * (iint + 1)],
         left_painter);
   // set up links between painters
   if (right_painter) {
      right_painter->set_left_interval(dummy);
      dummy->set_right_interval(right_painter);
   }
   fPad->cd();
   dummy->Draw();
   dummy->Connect("IntMod()", "KVItvFinderDialog", this, "UpdatePIDList()");
   dummy->SetDisplayLabel(1);
   dummy->SetCanvas(fCanvas);
   fItvPaint.Add(dummy);

   fIntervalListView->Display(current_interval_set->GetIntervals());

   fItvPaint.Execute("Update", "");

   fCanvas->Modified();
   fCanvas->Update();
}

void KVItvFinderDialog::AddInterval(double pid)
{
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   if (!list->GetSize() || list->GetSize() > 1) {
      current_interval_set = nullptr;
      return;
   }

   current_interval_set = (interval_set*)list->At(0);

   int aa = 0;
   int iint = 0;

   // try to guess mass from position (PID)
   // typical isotope separation in PID is 0.12 (e.g. for carbon isotopes)
   // assume that PID=z means A=2*Z
   auto aa_guessstimate = TMath::Nint(current_interval_set->GetZ() * 2 + (pid - current_interval_set->GetZ()) / 0.12);
//Info("NewINt","pid = %f guess a=%d",pid,aa_guessstimate);
   if (!current_interval_set->GetNPID()) {
      aa = aa_guessstimate;
      iint = 0;
   }
   else if (pid < ((interval*)current_interval_set->GetIntervals()->First())->GetPID()) { // to left of all others
      aa = ((interval*)current_interval_set->GetIntervals()->First())->GetA() - 1;
      // NO MORE : use guesstimate as long as it is smaller than A of previously defined interval with largest PID
      // NOW : use 'aa_guessstimate' only for first interval
      //Info("NewInt","before first interval, with a=%d",aa+1);
//      if (aa_guessstimate < aa + 1) aa = aa_guessstimate;
      iint = 0;
   }
   else if (pid > ((interval*)current_interval_set->GetIntervals()->Last())->GetPID()) { // to right of all others
      aa = ((interval*)current_interval_set->GetIntervals()->Last())->GetA() + 1;
      // NO MORE :  use guesstimate as long as it is larger than A of previously defined interval with smallest PID
      // NOW : use 'aa_guessstimate' only for first interval
      //Info("NewInt","after last interval, with a=%d",aa-1);
//      if (aa_guessstimate > aa - 1) aa = aa_guessstimate;
      iint = current_interval_set->GetNPID();
   }
   else {
      // look for intervals between which the new one is places
      for (int ii = 1; ii < current_interval_set->GetNPID(); ii++) {
         if (pid > ((interval*)current_interval_set->GetIntervals()->At(ii - 1))->GetPID()
               && pid < ((interval*)current_interval_set->GetIntervals()->At(ii))->GetPID()) {
            aa = ((interval*)current_interval_set->GetIntervals()->At(ii - 1))->GetA() + 1;
            // use guesstimate if it is in between masses of adjacent intervals (in terms of PID)
            if ((aa_guessstimate > (aa - 1)) &&
                  (aa_guessstimate < ((interval*)current_interval_set->GetIntervals()->At(ii))->GetA()))
               aa = aa_guessstimate;
            iint = ii;
            break;
         }
      }
   }

   interval* itv = new interval(current_interval_set->GetZ(), aa, pid, pid - 0.05, pid + 0.05);
   current_interval_set->GetIntervals()->AddAt(itv, iint);

   // find intervals which are now left (smaller mass) and right (higher mass) than this one
   interval* left_interval = iint > 0 ? (interval*)current_interval_set->GetIntervals()->At(iint - 1) : nullptr;
   interval* right_interval = iint < current_interval_set->GetIntervals()->GetEntries() - 1 ? (interval*)current_interval_set->GetIntervals()->At(iint + 1) : nullptr;
   // find the corresponding painters
   KVPIDIntervalPainter* left_painter{nullptr}, *right_painter{nullptr};
   TIter next_painter(&fItvPaint);
   KVPIDIntervalPainter* pidpnt;
   while ((pidpnt = (KVPIDIntervalPainter*)next_painter())) {
      if (pidpnt->GetInterval() == left_interval) left_painter = pidpnt;
      else if (pidpnt->GetInterval() == right_interval) right_painter = pidpnt;
   }

   // give a different colour to each interval
   auto nitv = current_interval_set->GetIntervals()->GetEntries();
   auto cstep = TColor::GetPalette().GetSize() / (nitv + 1);

   KVPIDIntervalPainter* dummy = new KVPIDIntervalPainter(itv, fLinearHisto, TColor::GetPalette()[cstep * (iint + 1)],
         left_painter);
   // set up links between painters
   if (right_painter) {
      right_painter->set_left_interval(dummy);
      dummy->set_right_interval(right_painter);
   }
   fPad->cd();
   dummy->Draw();
   dummy->Connect("IntMod()", "KVItvFinderDialog", this, "UpdatePIDList()");
   dummy->SetDisplayLabel(1);
   dummy->SetCanvas(fCanvas);
   fItvPaint.Add(dummy);

   fIntervalListView->Display(current_interval_set->GetIntervals());

   fItvPaint.Execute("Update", "");

   fCanvas->Modified();
   fCanvas->Update();

}

void KVItvFinderDialog::NewIntervalSet()
{
   if (fGrid->GetIntervalSets()->GetSize() == 0) fNextIntervalZ = 1;
   else fNextIntervalZ = ((interval_set*)fGrid->GetIntervalSets()->Last())->GetZ() + 1;
   //   KVBase::OpenContextMenu("SetNextIntervalZ()",this);
   fGrid->GetIntervalSets()->Add(new interval_set(fNextIntervalZ, KVIDZAFromZGrid::kIntType));
   UpdateLists();
}

void KVItvFinderDialog::remove_interval_from_interval_set(interval_set* itvs, interval* itv, bool remove_fit)
{
   KVPIDIntervalPainter* pid = (KVPIDIntervalPainter*)fItvPaint.FindObject(Form("%d_%d", itv->GetZ(), itv->GetA()));
   itvs->GetIntervals()->Remove(itv);
   delete_painter_from_painter_list(pid);
   if (remove_fit) {
      // remove any fits from pad corresponding to intervals
      KVMultiGaussIsotopeFit::UnDrawGaussian(itvs->GetZ(), itv->GetA(), fPad);
   }
}

void KVItvFinderDialog::RemoveInterval()
{
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   Int_t nSelected = list->GetSize();
   current_interval_set = nullptr;

   if (nSelected == 1) {
      current_interval_set = (interval_set*)list->At(0);
      list.reset(fIntervalListView->GetSelectedObjects());
      nSelected = list->GetSize();
      if (nSelected >= 1) {
         for (int ii = 0; ii < nSelected; ii++) {
            interval* itv = (interval*) list->At(ii);
            remove_interval_from_interval_set(current_interval_set, itv);
         }
         fIntervalListView->Display(current_interval_set->GetIntervals());
         fCanvas->Modified();
         fCanvas->Update();
      }
      else ClearInterval(current_interval_set);
   }
   else if (nSelected > 1) {
      for (int ii = 0; ii < nSelected; ii++) {
         auto itvs = (interval_set*)list->At(ii);
         ClearInterval(itvs);
      }
   }
}

void KVItvFinderDialog::MassesUp()
{
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   Int_t nSelected = list->GetSize();

   current_interval_set = nullptr;
   if (nSelected == 1) {
      current_interval_set = (interval_set*)list->At(0);

      list.reset(fIntervalListView->GetSelectedObjects());
      nSelected = list->GetSize();

      if (nSelected == 1) {
         interval* itv = (interval*) list->At(0);
         itv->SetA(itv->GetA() + 1);
         fItvPaint.Execute("Update", "");
         // change the name of any gaussian in the pad associated with this isotope
         auto gfit = (TNamed*)fPad->GetPrimitive(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA() - 1));
         if (gfit) gfit->SetName(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA()));
         fCanvas->Modified();
         fCanvas->Update();
      }
      else {
         TIter next_itv(list.get());
         interval* itv;
         while ((itv = (interval*)next_itv())) {
            itv->SetA(itv->GetA() + 1);
            // change the name of any gaussian in the pad associated with this isotope
            auto gfit = (TNamed*)fPad->GetPrimitive(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA() - 1));
            if (gfit) gfit->SetName(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA()));
         }
         fItvPaint.Execute("Update", "");
         fCanvas->Modified();
         fCanvas->Update();
      }
   }
}

void KVItvFinderDialog::MassesDown()
{
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   Int_t nSelected = list->GetSize();

   current_interval_set = nullptr;
   if (nSelected == 1) {
      current_interval_set = (interval_set*)list->At(0);
      list.reset(fIntervalListView->GetSelectedObjects());
      nSelected = list->GetSize();

      if (nSelected == 1) {
         interval* itv = (interval*) list->At(0);
         itv->SetA(itv->GetA() - 1);
         fItvPaint.Execute("Update", "");
         // change the name of any gaussian in the pad associated with this isotope
         auto gfit = (TNamed*)fPad->GetPrimitive(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA() + 1));
         if (gfit) gfit->SetName(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA()));
         fCanvas->Modified();
         fCanvas->Update();
      }
      else {
         TIter next_itv(list.get());
         interval* itv;
         while ((itv = (interval*)next_itv())) {
            itv->SetA(itv->GetA() - 1);
            // change the name of any gaussian in the pad associated with this isotope
            auto gfit = (TNamed*)fPad->GetPrimitive(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA() + 1));
            if (gfit) gfit->SetName(KVMultiGaussIsotopeFit::get_name_of_isotope_gaussian(itv->GetZ(), itv->GetA()));
         }
         fItvPaint.Execute("Update", "");
         fCanvas->Modified();
         fCanvas->Update();
      }
   }
}

void KVItvFinderDialog::UpdateLists()
{
   fIntervalSetListView->Display(((KVIDZAFromZGrid*)fGrid)->GetIntervalSets());
   std::unique_ptr<TList> list(fIntervalSetListView->GetSelectedObjects());
   Int_t nSelected = list->GetSize();
   interval_set* itvs = 0;
   if (nSelected == 1) {
      current_interval_set = (interval_set*)list->At(0);
      fIntervalListView->Display(current_interval_set->GetIntervals());
   }
   else {
      current_interval_set = nullptr;
      fIntervalListView->RemoveAll();
   }
}

void KVItvFinderDialog::TestIdent()
{
   //fGrid->SetOnlyZId(0);
   fGrid->Initialize();
   ExportToGrid();

   fGrid->ReloadPIDRanges();

   fIntervalSetListView->Display(((KVIDZAFromZGrid*)fGrid)->GetIntervalSets());
   current_interval_set = nullptr;
   fIntervalListView->RemoveAll();

   fItvPaint.Clear();
   DrawIntervals();

   new KVTestIDGridDialog(gClient->GetDefaultRoot(), gClient->GetDefaultRoot(), 10, 10, fGrid, fHisto);
}

void KVItvFinderDialog::SetLogy()
{
   fCanvas->SetLogy(!fCanvas->GetLogy());
   fCanvas->Modified();
   fCanvas->Update();
}

void KVItvFinderDialog::UnzoomHisto()
{
   fItvPaint.Execute("SetDisplayLabel", "0");
   current_interval_set = nullptr;
   fLinearHisto->GetXaxis()->UnZoom();
   fCanvas->Modified();
   fCanvas->Update();
}

void KVItvFinderDialog::FitIsotopes()
{
   // fit the PID spectrum for the currently selected interval set (Z).
   //
   // for an interval with N isotopes, we use N gaussians plus an exponential (decreasing)
   // background. each gaussian has the same width. the centroids of the gaussians are first
   // fixed to the positions of the PID markers, the intensity and width of the peaks
   // (plus the background) are fitted.
   // then another fit is performed without constraining the centroids.
   //
   // finally the PID markers (PID of each interval) are modified according to the fitted centroid positions.

   if (!current_interval_set) return;

   KVNumberList alist;
   std::vector<double> pidlist;
   TIter nxt_int(current_interval_set->GetIntervals());
   interval* intvl = nullptr;
   while ((intvl = (interval*)nxt_int())) {
      alist.Add(intvl->GetA());
      pidlist.push_back(intvl->GetPID());
   }
   KVMultiGaussIsotopeFit fitfunc(current_interval_set->GetZ(), current_interval_set->GetNPID(),
                                  current_interval_set->GetZ() - 0.5, current_interval_set->GetZ() + 0.5,
                                  alist, pidlist);

   // check user fit parameters
   if (mass_fit_parameters.GetBoolValue("Limit range of fit")) {
      // if the user's range is not valid for the current interval set, we ignore it
      if (mass_fit_parameters.GetDoubleValue("PID min for fit") >= current_interval_set->GetZ() - 0.5
            && mass_fit_parameters.GetDoubleValue("PID max for fit") <= current_interval_set->GetZ() + 0.5)
         fitfunc.SetFitRange(mass_fit_parameters.GetDoubleValue("PID min for fit"),
                             mass_fit_parameters.GetDoubleValue("PID max for fit"));

   }
   fitfunc.SetSigmaLimits(mass_fit_parameters.GetDoubleValue("Minimum #sigma"),
                          mass_fit_parameters.GetDoubleValue("Maximum #sigma"));

   fLinearHisto->Fit(&fitfunc, "NR");

   // now release the centroids
   fitfunc.ReleaseCentroids();

   fLinearHisto->Fit(&fitfunc, "NRME");

   // remove any previous fit from pad
   fitfunc.UnDraw();
   // mop up any stray gaussians (from intervals which have been removed)
   KVMultiGaussIsotopeFit::UnDrawAnyGaussian(current_interval_set->GetZ(), fPad);

   // draw fit with individual gaussians
   fitfunc.DrawFitWithGaussians("same");

   // deactivate intervals for fitted masses
   std::unique_ptr<KVSeqCollection> tmp(fItvPaint.GetSubListWithMethod(Form("%d", current_interval_set->GetZ()), "GetZ"));
   tmp->Execute("DeactivateIntervals", "");

   // set interval limits according to regions of most probable mass
   int most_prob_A = 0;
   nxt_int.Reset();
   // minimum probability for which isotopes are taken into account
   double min_proba = mass_fit_parameters.GetDoubleValue("Minimum probability [%]") / 100.;
   double delta_pid = 0.001;
   TList accepted_intervals;//any intervals not in this list at the end of the procedure will be removed
   for (double pid = fitfunc.GetPIDmin() ; pid <= fitfunc.GetPIDmax(); pid += delta_pid) {
      double proba;
      auto Amax = fitfunc.GetMostProbableA(pid, proba);
      if (proba > min_proba) {
         if (most_prob_A) {
            if (Amax > most_prob_A) {
               //std::cout << pid << "  " << Amax << "  " << proba << std::endl;
               //std::cout << "got PIDmax for A=" << intvl->GetA() << std::endl;
               intvl->SetPIDmax(pid - delta_pid);
               most_prob_A = Amax;
               intvl = (interval*)nxt_int();
               while (intvl->GetA() < most_prob_A) {
                  intvl = (interval*)nxt_int();
               }
               accepted_intervals.Add(intvl);
               intvl->SetPIDmin(pid);
               //std::cout << "got PIDmin for A=" << intvl->GetA() << std::endl;
            }
         }
         else {
            most_prob_A = Amax;
            intvl = (interval*)nxt_int();
            while (intvl->GetA() < most_prob_A) {
               intvl = (interval*)nxt_int();
            }
            accepted_intervals.Add(intvl);
            intvl->SetPIDmin(pid);
            //std::cout << pid << "  " << Amax << "  " << proba << std::endl;
            //std::cout << "got PIDmin for A=" << intvl->GetA() << std::endl;
         }
      }
      else if (most_prob_A) {
         intvl->SetPIDmax(pid - delta_pid);
         //std::cout << pid << "  " << Amax << "  " << proba << std::endl;
         //std::cout << "got PIDmax for A=" << intvl->GetA() << std::endl;
         most_prob_A = 0;
      }
   }
   nxt_int.Reset();
//   TList intervals_to_remove;
//   while ((intvl = (interval*)nxt_int())) {
//      if (!accepted_intervals.FindObject(intvl)) intervals_to_remove.Add(intvl);
//   }
//   if (intervals_to_remove.GetEntries()) {
//      // remove intervals below minimum probability (leave gaussians on display)
//      TIter it_rem(&intervals_to_remove);
//      while ((intvl = (interval*)it_rem())) remove_interval_from_interval_set(current_interval_set, intvl, false);
//   }
//   intervals_to_remove.Clear();
   int ig(1);
   // update PID positions from fitted centroids
   nxt_int.Reset();
   KVNumberList remaining_gaussians, remaining_alist;
   auto vec_alist = alist.GetArray();
   while ((intvl = (interval*)nxt_int())) {
      // in case we removed some peaks
      while (vec_alist[ig - 1] < intvl->GetA()) {
         ++ig;
      }
      intvl->SetPID(fitfunc.GetCentroid(ig));
      remaining_gaussians.Add(ig);
      remaining_alist.Add(intvl->GetA());
      ++ig;
   }
//   if (intervals_to_remove.GetEntries()) {
//      // remove intervals with centroids outside PID limits
//      TIter it_rem(&intervals_to_remove);
//      while ((intvl = (interval*)it_rem())) remove_interval_from_interval_set(current_interval_set, intvl, false);
//   }
   UpdatePIDList();
   fItvPaint.Execute("Update", "");

   fPad->Modified();
   fPad->Update();

   // save results in grid parameters
   KVNumberList zlist;
   if (fGrid->GetParameters()->HasStringParameter("MASSFITS"))
      zlist.Set(fGrid->GetParameters()->GetStringValue("MASSFITS"));
   zlist.Add(current_interval_set->GetZ());
   fGrid->GetParameters()->SetValue("MASSFITS", zlist.AsString());
   TString massfit = Form("MASSFIT_%d", current_interval_set->GetZ());
   KVNameValueList fitparams;
   fitparams.SetValue("Ng", alist.GetNValues());
   fitparams.SetValue("Alist", alist.AsQuotedString());
   fitparams.SetValue("PIDmin", fitfunc.GetPIDmin());
   fitparams.SetValue("PIDmax", fitfunc.GetPIDmax());
   fitparams.SetValue("Bkg_cst", fitfunc.GetBackgroundConstant());
   fitparams.SetValue("Bkg_slp", fitfunc.GetBackgroundSlope());
   fitparams.SetValue("GausWid", fitfunc.GetGaussianWidth(0));
   fitparams.SetValue("PIDvsA_a0", fitfunc.GetPIDvsAfit_a0());
   fitparams.SetValue("PIDvsA_a1", fitfunc.GetPIDvsAfit_a1());
   fitparams.SetValue("PIDvsA_a2", fitfunc.GetPIDvsAfit_a2());
   for (ig = 1; ig <= alist.GetNValues(); ++ig) {
      fitparams.SetValue(Form("Norm_%d", ig), fitfunc.GetGaussianNorm(ig));
   }
   auto sanitized = fitparams.Get().ReplaceAll("=", ":");
   fGrid->GetParameters()->SetValue(massfit, sanitized);
}

void KVItvFinderDialog::SetFitParameters()
{
   // Open dialog to modify parameters for multigauss mass fit

   bool cancel = false;
   auto dialog = new KVNameValueListGUI(fMain, &mass_fit_parameters, &cancel);
   dialog->EnableDependingOnBool("PID min for fit", "Limit range of fit");
   dialog->EnableDependingOnBool("PID max for fit", "Limit range of fit");
   dialog->DisplayDialog();
}

void KVItvFinderDialog::RemoveFit()
{
   // Remove fit of currently selected interval set from pad

   if (!current_interval_set) return;

   std::vector<int> alist;
   TIter nxt_int(current_interval_set->GetIntervals());
   interval* intvl = nullptr;
   while ((intvl = (interval*)nxt_int())) {
      alist.push_back(intvl->GetA());
   }
   KVMultiGaussIsotopeFit fitfunc(current_interval_set->GetZ(), alist);

   fitfunc.UnDraw(fPad);
   // mop up any stray gaussians (from intervals which have been removed)
   KVMultiGaussIsotopeFit::UnDrawAnyGaussian(current_interval_set->GetZ(), fPad);

   fPad->Modified();
   fPad->Update();
}

void KVItvFinderDialog::HandleKey()
{

   if (!fCanvas) return;
   if (fCanvas->GetEvent() == kMouseMotion) return;

   switch (fCanvas->GetEvent()) {
      case kButton2Up:
         FitIsotopes();
         break;
      case kButton1Double:
         fCanvas->SetLogy(!fCanvas->GetLogy());
         break;
      case kButton1Shift:
//         std::cout << fCanvas->GetEventX() << " " << fCanvas->AbsPixeltoX(fCanvas->GetEventX()) << std::endl;
         AddInterval(fCanvas->AbsPixeltoX(fCanvas->GetEventX()));
         break;
   }

}

void KVItvFinderDialog::FindPIDIntervals(Int_t zz)
{
   interval_set* itvs = fGrid->GetIntervalSet(zz);
   if (!itvs) {
      itvs = new interval_set(zz, KVIDZAFromZGrid::kIntType);
      fGrid->GetIntervalSets()->Add(itvs);
   }
   else ClearInterval(itvs);


   if (zz == 1) fLinearHisto->SetAxisRange(0.9, zz + 0.5, "X");
   else      fLinearHisto->SetAxisRange(zz - 0.5, zz + 0.5, "X");

   int nfound = fSpectrum.Search(fLinearHisto, fSig, "goff", ((zz == 2) ? 0.1 * fRat : fRat));

#if ROOT_VERSION_CODE > ROOT_VERSION(5,99,01)
   Double_t* xpeaks = fSpectrum.GetPositionX();
//    Double_t* ypeaks = fSpectrum.GetPositionY();
#else
   Float_t* xpeaks = fSpectrum.GetPositionX();
//    Float_t* ypeaks = fSpectrum.GetPositionY();
#endif

   nfound = TMath::Min(fNpeaks[zz - 1], nfound);

   int idx[nfound];
   TMath::Sort(nfound, xpeaks, idx, 0);

   int zrefs[] = {1, 4, 7, 9, 11, 12, 15, 16, 19, 21, 23, 25, 27, 29, 31, 34, 35, 38, 40, 42, 44, 47, 49, 51, 53};
   int zref = zrefs[zz - 1];

   int idref = -1;
   for (int p = 0; p < nfound; p++) {
      if (abs(xpeaks[idx[p]] - xpeaks[0]) < .0001) idref = p;
   }
   Info("FindPIDIntervals", "Z=%d : idref = %d ", zz, idref);

   for (int p = 0; p < nfound; p++) {
//        Info("FindPIDIntervals","Z=%d : (%.2lf %.2lf) (%d %d) ",zz,xpeaks[p],ypeaks[p],idx[p],p);
      double pid = xpeaks[idx[p]];//ff->GetParameter(3 * ii + 1);
      itvs->add(zref + (p - idref), pid, pid - 0.05, pid + 0.05);
   }


}

Double_t KVItvFinderDialog::fpeaks(Double_t* x, Double_t* par)
{
   Double_t result = 0;
   int np = par[0];
   for (Int_t p = 0; p < np; p++) {
      Double_t norm  = par[3 * p + 3];
      Double_t mean  = par[3 * p + 1];
      Double_t sigma = par[3 * p + 2];
      result += norm * TMath::Gaus(x[0], mean, sigma);
   }
   return result;
}
void KVItvFinderDialog::ProcessIdentification(Int_t zmin, Int_t zmax)
{

   if (zmin < 0) zmin = ((KVIDentifier*)fGrid->GetIdentifiers()->First())->GetZ();
   if (zmax < 0) zmax = ((KVIDentifier*)fGrid->GetIdentifiers()->Last())->GetZ();

   for (int z = zmin; z <= zmax; z++) FindPIDIntervals(z);

}








