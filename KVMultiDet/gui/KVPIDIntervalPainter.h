//Created by KVClassFactory on Mon Jan 23 14:29:32 2017
//Author: Diego Gruyer

#ifndef __KVPIDINTERVALPAINTER_H
#define __KVPIDINTERVALPAINTER_H


#include "RQ_OBJECT.h"

#include "TH1.h"
#include "TLine.h"
#include "TMarker.h"
#include "TLatex.h"
#include "TCanvas.h"
#include "KVList.h"
#include "KVIDZAFromZGrid.h"

#include "TNamed.h"

/**
   \class KVPIDIntervalPainter
\brief Graphical representation of a PID interval in the KVIDZAFromZGrid mass assignation GUI
\ingroup GUI
\ingroup Identification
*/

class KVPIDIntervalPainter : public TNamed {
   RQ_OBJECT("KVPIDIntervalPainter")

   class pid_marker : public TMarker {
      KVPIDIntervalPainter* parent;
      TLatex fLabel;
      bool fDrawLabel{false};
      bool fHighlight{false};
      Color_t def_color{kBlack};
      double pid;
   public:
      using TMarker::TMarker;
      pid_marker() = default;
      pid_marker(KVPIDIntervalPainter* P, Color_t itv_col);

      void ExecuteEvent(Int_t event, Int_t px, Int_t py);
      void Paint(Option_t* option = "");
      void SetDrawLabel(bool yes = true)
      {
         fDrawLabel = yes;
      }
      bool GetDrawLabel() const
      {
         return fDrawLabel;
      }
      void HighLight(bool hi = true);
      void Update(int z, int a, double pid);
      bool CheckPosition(double x)
      {
         // only allow PID marker to move between lower & upper limits
         //
         // deactivated if parent->active_interval=false
         return (!parent->active_intervals)
                || (x >= parent->GetPIDIntervalLowerLimit() && x <= parent->GetPIDIntervalUpperLimit());
      }
   };
   class pid_line : public TLine {
   public:
      enum class limit_t {lower, upper};
      using TLine::TLine;
      pid_line() = default;
      pid_line(KVPIDIntervalPainter* p, double _pid, double ymin, double ymax, limit_t limit_type, Color_t itv_col)
         : parent{p},
           TLine(_pid, ymin, _pid, ymax),
           type{limit_type},
           def_color{itv_col},
           pid{_pid}
      {
         SetLineColor(def_color);
         SetLineWidth(2);
      }
      void ExecuteEvent(Int_t event, Int_t px, Int_t py);
      limit_t GetLimitType() const
      {
         return type;
      }
      bool IsUpperLimit() const
      {
         return GetLimitType() == limit_t::upper;
      }
      bool IsLowerLimit() const
      {
         return GetLimitType() == limit_t::lower;
      }
      void HighLight(bool hi = true);
      void SetX(double X)
      {
         SetX1(X);
         SetX2(X);
         if (pid != X) {
            // the line has moved
            pid = X;
            // update interval limits
            if (type == limit_t::lower)
               parent->GetInterval()->SetPIDmin(pid);
            else
               parent->GetInterval()->SetPIDmax(pid);
            // signal that something changed
            parent->IntMod();
         }
      }
      void Update(double x)
      {
         SetX1(x);
         SetX2(x);
         pid = x;
      }

      double GetX() const
      {
         assert(GetX1() == GetX2());
         return GetX1();
      }
      void Paint(Option_t* option = "");
      bool CheckPosition(double x);

   private:
      KVPIDIntervalPainter* parent;
      limit_t type;
      bool fHighlight{false};
      Color_t def_color{kBlack};
      double pid;
   };

   friend class pid_marker;

   TH1* fLinearHisto = nullptr;
   interval* fInterval = nullptr;
   TAxis* fXaxis{nullptr}, *fYaxis{nullptr};

   double pid, min, max;
   Int_t fA, fZ;

   pid_marker fMarker; // represents position of PID for interval
   pid_line   fLine1;  // represents lower limit of PID for interval
   pid_line   fLine2;  // represents upper limit of PID for interval

   KVPIDIntervalPainter* left_interval = nullptr; // interval to the left (i.e. smaller mass) than this one
   KVPIDIntervalPainter* right_interval = nullptr; // interval to the right (i.e. larger mass) than this one

   TCanvas* fCanvas = nullptr;

   Bool_t active_intervals = true;

public:
   void set_right_interval(KVPIDIntervalPainter* i)
   {
      right_interval = i;
   }
   void set_left_interval(KVPIDIntervalPainter* i)
   {
      left_interval = i;
   }
   KVPIDIntervalPainter* get_right_interval() const
   {
      return right_interval;
   }
   KVPIDIntervalPainter* get_left_interval() const
   {
      return left_interval;
   }
   KVPIDIntervalPainter(interval* itv, TH1* hh, Color_t itv_color, KVPIDIntervalPainter* last_interval)
      : TNamed(Form("%d_%d", itv->GetZ(), itv->GetA()), Form("%d_%d", itv->GetZ(), itv->GetA())),
        fLinearHisto{hh},
        fInterval{itv},
        fXaxis{fLinearHisto->GetXaxis()},
        fYaxis{fLinearHisto->GetYaxis()},
        pid{fInterval->GetPID()},
        min{fInterval->GetPIDmin()},
        max{fInterval->GetPIDmax()},
        fA{fInterval->GetA()},
        fZ{fInterval->GetZ()},
        fMarker{this, itv_color},
        fLine1{this, min, 0, 200, pid_line::limit_t::lower, itv_color},
        fLine2{this, max, 0, 200, pid_line::limit_t::upper, itv_color},
        left_interval{last_interval}
   {
      if (last_interval) last_interval->set_right_interval(this);
   }
   KVPIDIntervalPainter(const char* name, const char* title)
      : TNamed(name, title)
   {}
   KVPIDIntervalPainter(const TString& name, const TString& title)
      : TNamed(name, title)
   {}

   void DeactivateIntervals()
   {
      active_intervals = false;
      if (fCanvas) {
         fCanvas->GetListOfPrimitives()->Remove(&fLine1);
         fCanvas->GetListOfPrimitives()->Remove(&fLine2);
         fCanvas->Modified();
         fCanvas->Update();
      }
   }
   void ActivateIntervals()
   {
      active_intervals = true;
   }

   void Draw(Option_t*     option = "");
   void HighLight(bool hi = true);
   void IntMod()
   {
      Emit("IntMod()");
   }
   void SetCanvas(TCanvas* cc)
   {
      fCanvas = cc;
   }

   void Update();
   Int_t GetZ()
   {
      return fZ;
   }
   Int_t GetA()
   {
      return fA;
   }

   void SetDisplayLabel(bool dis = true)
   {
      fMarker.SetDrawLabel(dis);
   }
   bool GetDisplayLabel() const
   {
      return fMarker.GetDrawLabel();
   }
   interval* GetInterval() const
   {
      return fInterval;
   }
   TH1* GetHisto() const
   {
      return fLinearHisto;
   }

   Double_t GetHistoXAxisLowerLimit() const
   {
      return fXaxis->GetBinCenter(fXaxis->GetFirst());
   }
   Double_t GetHistoXAxisUpperLimit() const
   {
      return fXaxis->GetBinCenter(fXaxis->GetLast());
   }
   TCanvas* GetCanvas() const
   {
      return fCanvas;
   }

   double GetPIDIntervalLowerLimit() const
   {
      return fLine1.GetX();
   }
   double GetPIDIntervalUpperLimit() const
   {
      return fLine2.GetX();
   }
   double GetPIDPosition() const
   {
      return fMarker.GetX();
   }

   ClassDef(KVPIDIntervalPainter, 1) // Graphical representation of a PID interval in the KVIDZAFromZGrid mass assignation GUI
};

#endif
