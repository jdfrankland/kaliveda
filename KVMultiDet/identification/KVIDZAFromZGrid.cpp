//Created by KVClassFactory on Tue Mar  8 10:00:16 2016
//Author: Diego Gruyer

#include "KVIDZAFromZGrid.h"
#include "TMultiGraph.h"
#include "KVIDZALine.h"
#include "TCanvas.h"

ClassImp(KVIDZAFromZGrid)
ClassImp(interval)
ClassImp(interval_set)

////////////////////////////////////////////////////////////////////////////////
// BEGIN_HTML <!--
/* -->
<h2>KVIDZAFromZGrid</h2>
<h4>Hybrid identification grid</h4>
Such a grid can identify simultaneously both the mass and charge of detected
particles (if contains a convertion table for the given element), or solely the charge
(if no convertion table).

The convertion table has to be included in the parameter list of the grid as follow:

<PARAMETER> PIDRANGE=[zmax]
<PARAMETER> PIDRANGE[z]=[a1]:[PID min],[PID mean],[PID max]|[a2]:[PID min],[PID mean],[PID max]|[a3]:[PID min],[PID mean],[PID max]
(...)

with:
<ul>
     <li> [zmax]            highest Z for witch A will be extracted</li>
     <li> [z]               charge corresponding to the following line</li>
     <li> [ai]              mass corresponding to the following interval</li>
     <li> [PID mean]        PID value of the peak of corresponding to mass [ai]</li>
     <li> [PID min/max]     PID interval corresponding to mass [ai] used to attribute the ICODE (optional)</li>
</ul>

<h3>Identification quality codes</h3>
After each identification attempt, the value returned by GetQualityCode() indicates whether the
identification was successful or not. The meaning of the different codes depends on the type
of identification.

<h4>Z & A (mass & charge) isotopic identification grid</h4>
<ul>
     <li> KVIDZAGrid::kICODE0,                   ok</li>
  <li>     KVIDZAGrid::kICODE1,                   Z ok, slight ambiguity of A, which could be larger</li>
  <li>     KVIDZAGrid::kICODE2,                   Z ok, slight ambiguity of A, which could be smaller</li>
    <li>   KVIDZAGrid::kICODE3,                   Z ok, slight ambiguity of A, which could be larger or smaller</li>
   <li>    KVIDZAGrid::kICODE4,                   point out of mass identification intervals, strong ambiguity of A</li>
  <li>     KVIDZAGrid::kICODE5,                   point is in between two isotopes of different Z, too far from either to be considered well-identified</li>
   <li>    KVIDZAGrid::kICODE6,                   (x,y) is below first line in grid</li>
   <li>    KVIDZAGrid::kICODE7,                   (x,y) is above last line in grid</li>
  <li>     KVIDZAGrid::kICODE8,                   no identification: (x,y) out of range covered by grid</li>
</ul>

<h4>Z-only charge identification grid</h4>
<ul>
     <li> KVIDZAGrid::kICODE0,                   ok</li>
  <li>     KVIDZAGrid::kICODE1,                   slight ambiguity of Z, which could be larger</li>
  <li>     KVIDZAGrid::kICODE2,                   slight ambiguity of Z, which could be smaller</li>
    <li>   KVIDZAGrid::kICODE3,                   slight ambiguity of Z, which could be larger or smaller</li>
   <li>    KVIDZAGrid::kICODE4,                   point is in between two lines of different Z, too far from either to be considered well-identified</li>
  <li>     KVIDZAGrid::kICODE5,                   point is in between two lines of different Z, too far from either to be considered well-identified</li>
   <li>    KVIDZAGrid::kICODE6,                   (x,y) is below first line in grid</li>
   <li>    KVIDZAGrid::kICODE7,                   (x,y) is above last line in grid</li>
  <li>     KVIDZAGrid::kICODE8,                   no identification: (x,y) out of range covered by grid</li>
</ul>

In both cases, an acceptable identification is achieved if the quality code is kICODE0, kICODE1, kICODE2, or kICODE3.<br>
Points with codes kICODE4 or kICODE5 are normally considered as "noise" and should be rejected.<br>
Points which are (vertically) out of range for this grid have code kICODE6 (point too far below) or kICODE7 (point too far above).<br>
Points with code kICODE8 are totally out of range.
<!-- */
// --> END_HTML
////////////////////////////////////////////////////////////////////////////////

KVIDZAFromZGrid::KVIDZAFromZGrid()
{
   // Default constructor
//    Info("KVIDZAFromZGrid","called...");
   init();
   fTables.SetOwner(kTRUE);
}

KVIDZAFromZGrid::~KVIDZAFromZGrid()
{
   // Destructor
}

//________________________________________________________________

void KVIDZAFromZGrid::Copy(TObject& obj) const
{
   // This method copies the current state of 'this' object into 'obj'
   // You should add here any member variables, for example:
   //    (supposing a member variable KVIDZAFromZGrid::fToto)
   //    CastedObj.fToto = fToto;
   // or
   //    CastedObj.SetToto( GetToto() );

   KVIDZAGrid::Copy(obj);
   //KVIDZAFromZGrid& CastedObj = (KVIDZAFromZGrid&)obj;
}


void KVIDZAFromZGrid::ReadFromAsciiFile(std::ifstream& gridfile)
{
//   fPIDRange = kFALSE;
//   KVIDGraph::ReadFromAsciiFile(gridfile);
//   if (GetParameters()->HasParameter("PIDRANGE")) {
//      fPIDRange = kTRUE;
//      fZmaxInt = GetParameters()->GetIntValue("PIDRANGE");
//      LoadPIDRanges();
//   }

   fPIDRange = kFALSE;
   fHasMassCut = kFALSE;
   KVIDGraph::ReadFromAsciiFile(gridfile);

   if (GetIdentifier("MassID")) fHasMassCut = kTRUE;

   if (GetParameters()->HasParameter("PIDRANGE")) {
      fPIDRange = kTRUE;
      TString pidrange = GetParameters()->GetStringValue("PIDRANGE");
      if (pidrange.Contains("-")) {
         TString min = (pidrange(0, pidrange.Index("-")));
         fZminInt = min.Atoi();
         min = (pidrange(pidrange.Index("-") + 1, pidrange.Length()));
         fZmaxInt = min.Atoi();
      } else {
         fZminInt = 1;
         fZmaxInt = pidrange.Atoi();
      }
      LoadPIDRanges();
   }

}

void KVIDZAFromZGrid::LoadPIDRanges()
{
   fZminInt = 100000;
   fZmaxInt = 0;

   KVIDentifier* id = 0;
   TIter it(GetIdentifiers());
   while ((id = (KVIDentifier*)it())) {
      int zz = id->GetZ();
      if (!GetParameters()->HasParameter(Form("PIDRANGE%d", zz))) continue;
      KVString mes = GetParameters()->GetStringValue(Form("PIDRANGE%d", zz));
      if (mes.IsWhitespace()) continue;
      int type = (mes.Contains(",") ? 2 : 1);
      interval_set* itv = new interval_set(zz, type);
      itv->SetName(GetName());
      mes.Begin("|");
      while (!mes.End()) {
         KVString tmp = mes.Next();
         tmp.Begin(":");
         int aa = tmp.Next().Atoi();
         KVString val = tmp.Next();
         double pidmin, pidmax, pid;
         if (type == 1) itv->add(aa, val.Atof());
         else if (type == 2) {
            val.Begin(",");
            pidmin = val.Next().Atof();
            pid = val.Next().Atof();
            pidmax = val.Next().Atof();
            itv->add(aa, pid, pidmin, pidmax);
//            itv->add(aa, pid, pid-0.02, pid+0.02);
         }
      }
      if (zz < fZminInt) fZminInt = zz;
      if (zz > fZmaxInt) fZmaxInt = zz;
      fTables.Add(itv);
   }
   fPIDRange = kTRUE;
   //    PrintPIDLimits();
}

void KVIDZAFromZGrid::ReloadPIDRanges()
{
   fTables.Clear("all");
   LoadPIDRanges();
}

interval_set* KVIDZAFromZGrid::GetIntervalSet(int zint)
{
   interval_set* itv = 0;
   TIter it(&fTables);
   while ((itv = (interval_set*)it())) if (itv->GetZ() == zint) return itv;
   return 0;
}

void KVIDZAFromZGrid::PrintPIDLimits()
{
//    ((interval_set*)fTables.At(12))->fIntervals.ls();

//   for (int zz = fZminInt; zz <= fZmaxInt; zz++) {
//      Info("PrintPIDLimits", "Z=%2d    [%.4lf  %.4lf]", zz, ((interval_set*)fTables.At(zz - fZminInt))->fPIDmins.at(0),
//           ((interval_set*)fTables.At(zz - fZminInt))->fPIDmaxs.at(((interval_set*)fTables.At(zz - fZminInt))->fNPIDs - 1));
//      }
}

void KVIDZAFromZGrid::ClearPIDIntervals()
{
   if (fPar->HasParameter("PIDRANGE")) fPar->RemoveParameter("PIDRANGE");
   for (int ii = 1; ii < 30; ii++) {
      if (fPar->HasParameter(Form("PIDRANGE%d", ii))) fPar->RemoveParameter(Form("PIDRANGE%d", ii));
   }
}

bool KVIDZAFromZGrid::is_inside(double pid)
{
   int zint = TMath::Nint(pid);
   interval_set* it = GetIntervalSet(zint);
   if (it) return it->is_inside(pid);
   else return kFALSE;
}

void KVIDZAFromZGrid::Initialize()
{
   // General initialisation method for identification grid.
   // This method MUST be called once before using the grid for identifications.
   // The ID lines are sorted.
   // The natural line widths of all ID lines are calculated.
   // The line with the largest Z (Zmax line) is found.

   KVIDGrid::Initialize();
   // Zmax should be Z of last line in sorted list

   TIter it(GetIdentifiers());
   KVIDentifier* id = 0;

   fZMax = 0;
   while ((id = (KVIDentifier*)it())) {
      if (!id->InheritsFrom("KVIDZALine")) continue;
      int zz = ((KVIDZALine*)id)->GetZ();
      if (zz > fZMax) fZMax = zz;
   }

}

void KVIDZAFromZGrid::Identify(Double_t x, Double_t y, KVIdentificationResult* idr) const
{
   // Fill the KVIdentificationResult object with the results of identification for point (x,y)
   // corresponding to some physically measured quantities related to a reconstructed nucleus.
   //
   // By default (OnlyZId()=kFALSE) this means identifying the Z & A of the nucleus.
   // In this case, we consider that the nucleus' Z & A have been correctly measured
   // if the 'quality code' returned by IdentZA() is < kICODE4:
   //   we set idr->Zident and idr->Aident to kTRUE if fICode<kICODE4
   //
   // If OnlyZId()=kTRUE, only the Z of the nucleus is established.
   // In this case, we consider that the nucleus' Z has been correctly measured
   // if the 'quality code' returned by IdentZ() is < kICODE4, thus:
   //   we set idr->Zident to kTRUE if fICode<kICODE4
   // The mass idr->A is set to the mass of the nearest line.
   //
   // Real & integer masses for isotopically identified particles
   // ===================================================
   // For points lying between two lines of same Z and different A (fICode<kIDCode4)
   // the "real" mass is given by interpolation between the two masses.
   // The integer mass is the A of the line closest to the point.
   // This means that the integer A is not always = nint("real" A),
   // as for example if a grid is drawn with lines for 7Be & 9Be but not 8Be
   // (usual case), then particles between the two lines can have "real" masses
   // between 7.5 and 8.5, but their integer A will be =7 or =9, never 8.
   //

   idr->IDOK = kFALSE;
   if (!const_cast<KVIDZAFromZGrid*>(this)->FindFourEmbracingLines(x, y, "above")) {
      const_cast < KVIDZAFromZGrid* >(this)->fICode = kICODE8;         // Z indetermine ou (x,y) hors limites
      idr->IDquality = kICODE8;
      idr->SetComment("no identification: (x,y) out of range covered by grid");
      return;
   }

   Double_t Z;
   const_cast < KVIDZAFromZGrid* >(this)->IdentZ(x, y, Z);
   idr->IDquality = fICode;
   if (fICode < kICODE4 || fICode == kICODE7) {
      idr->Zident = kTRUE;
   }
   if (fICode < kICODE4) {
      idr->IDOK = kTRUE;
   }
   idr->Z = Zint;
   idr->PID = Z;
   idr->Aident = kFALSE;

   if ((fPIDRange && (idr->IDOK) && (idr->Z <= fZmaxInt) && (idr->Z > fZminInt - 1) && (const_cast < KVIDZAFromZGrid* >(this)->is_inside(Z)))
         && ((!fHasMassCut) || (fHasMassCut && GetIdentifier("MassID")->IsInside(x, y)))) {
//       Info("Identify","try mass ID..");
      const_cast < KVIDZAFromZGrid* >(this)->DeduceAfromPID(idr); // IDQuality and comments assigned here
      if (idr->IDquality < kICODE4) { // should always be true: to be verified...
         idr->Aident = kTRUE;
         idr->IDOK = kTRUE;
      } else if (idr->IDquality == kICODE4) idr->IDquality = kICODE3;
   } else {
      switch (fICode) {
         case kICODE0:
            idr->SetComment("ok");
            break;
         case kICODE1:
            idr->SetComment("slight ambiguity of Z, which could be larger");
            break;
         case kICODE2:
            idr->SetComment("slight ambiguity of Z, which could be smaller");
            break;
         case kICODE3:
            idr->SetComment("slight ambiguity of Z, which could be larger or smaller");
            break;
         case kICODE4:
            idr->SetComment("point is in between two lines of different Z, too far from either to be considered well-identified");
            break;
         case kICODE5:
            idr->SetComment("point is in between two lines of different Z, too far from either to be considered well-identified");
            break;
         case kICODE6:
            idr->SetComment("(x,y) is below first line in grid");
            break;
         case kICODE7:
            idr->SetComment("(x,y) is above last line in grid");
            break;
         default:
            idr->SetComment("no identification: (x,y) out of range covered by grid");
      }
   }
}


double KVIDZAFromZGrid::DeduceAfromPID(KVIdentificationResult* idr)
{
   int zint = idr->Z;
   double res = 0.;
   interval_set* it = GetIntervalSet(zint);
   if (it) res = it->eval(idr);
   return res;
}



double interval_set::eval(KVIdentificationResult* idr)
{
   double pid = idr->PID;
   if (pid < 0.5) return 0.;
   double res = fPIDs.Eval(pid);
   int ares = 0;

   if (fType == KVIDZAFromZGrid::kIntType) {
      for (int ii = 0; ii < fNPIDs; ii++) {
//         if (pid > ((interval*)fIntervals.At(ii))->GetPIDmin() && pid < ((interval*)fIntervals.At(ii))->GetPIDmax()) {
         if (((interval*)fIntervals.At(ii))->is_inside(pid)) {
            ares = ((interval*)fIntervals.At(ii))->GetA();
            break;
         }
      }
      if (ares != 0) {
         idr->A = ares;
         idr->PID = res;
         idr->IDquality = KVIDZAGrid::kICODE0;
         idr->SetComment("ok");
      } else {
         ares = TMath::Nint(res);
         idr->A = ares;
         idr->PID = res;
         if (pid > ((interval*)fIntervals.At(0))->GetPIDmin() && pid < ((interval*)fIntervals.At(fNPIDs - 1))->GetPIDmax()) {
            idr->IDquality = KVIDZAGrid::kICODE3;
            idr->SetComment("slight ambiguity of A, which could be larger or smaller");
         } else {
            idr->IDquality = KVIDZAGrid::kICODE4;
            idr->SetComment("point out of mass identification intervals, strong ambiguity of A");
         }
      }
   } else {
      ares = TMath::Nint(res);
      idr->A = ares;
      idr->PID = res;
      if (ares > fPIDs.GetX()[0] && ares < fPIDs.GetX()[fNPIDs - 1]) {
         idr->IDquality = KVIDZAGrid::kICODE0;
         idr->SetComment("ok");
      } else {
         idr->IDquality = KVIDZAGrid::kICODE4;
         idr->SetComment("point out of mass identification intervals, strong ambiguity of A");
      }
   }
   return res;
}

bool interval_set::is_inside(double pid)
{
   if (fType != KVIDZAFromZGrid::kIntType) return kTRUE;

//   Info("is_inside","min: %d max:%d npids:%d", ((interval*)fIntervals.At(0))->GetA(), ((interval*)fIntervals.At(fNPIDs-1))->GetA(), fNPIDs);

   if (pid > ((interval*)fIntervals.At(0))->GetPIDmin() && pid < ((interval*)fIntervals.At(fNPIDs - 1))->GetPIDmax()) return kTRUE;
   else return kFALSE;
}


const char* interval_set::GetListOfMasses()
{
   if (!GetNPID()) return "-";
   KVNumberList alist;
   for (int ii = 0; ii < GetNPID(); ii++) alist.Add(((interval*)fIntervals.At(ii))->GetA());
   return alist.AsString();
}

interval_set::interval_set(int zz, int type)
{
   fType = type;
   fZ = zz;
   fNPIDs = 0;
   fIntervals.SetOwner(kTRUE);
}

void interval_set::add(int aa, double pid, double pidmin, double pidmax)
{
//   if (fNPIDs && pid < fPIDs.GetX()[fNPIDs - 1]) {
//      Error("add", "Please give me peaks in the right order for Z=%d and A=%d...", fZ, aa);
//      return;
//   }
   if (fType == KVIDZAFromZGrid::kIntType && !(pid > pidmin && pid < pidmax)) {
      Error("add", "Wrong interval for Z=%d and A=%d: [%.4lf  %.4lf  %.4lf] (%s)", fZ, aa, pidmin, pid, pidmax, GetName());
      return;
   }

   fPIDs.SetPoint(fNPIDs, pid, aa);
   if (fType == KVIDZAFromZGrid::kIntType) {
      if (pid) fIntervals.AddLast(new interval(fZ, aa, pid, pidmin, pidmax));
   }
   fNPIDs++;
}
