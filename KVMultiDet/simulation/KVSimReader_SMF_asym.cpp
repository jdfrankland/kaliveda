//Created by KVClassFactory on Mon Jul  5 15:24:25 2010
//Author: bonnet

#include "KVSimReader_SMF_asym.h"
#include "KVString.h"

ClassImp(KVSimReader_SMF_asym)

KVSimReader_SMF_asym::KVSimReader_SMF_asym()
{
   // Default constructor
   init();
}

KVSimReader_SMF_asym::KVSimReader_SMF_asym(KVString filename)
{
   init();
   ConvertEventsInFile(filename);
}


KVSimReader_SMF_asym::~KVSimReader_SMF_asym()
{
   // Destructor
}

void KVSimReader_SMF_asym::ReadFile()
{

   while (IsOK()) {
      if (ReadHeader()) {
         for (Int_t nd = 0; nd < nv->GetIntValue("ndes"); nd += 1) {
            if (ReadEvent()) {
               if (nevt % 1000 == 0) Info("ReadFile", "%d evts lus", nevt);
               if (HasToFill()) FillTree();
            }
         }
      }
   }

}


Bool_t KVSimReader_SMF_asym::ReadHeader()
{

   KVString snom;
   auto res = ReadLineAndCheck(1, " ");
   switch (res) {
      case KVFileReader::ReadStatus::EmptyLine:
         return kFALSE;
      case KVFileReader::ReadStatus::OK:
         snom.Form("%s", GetReadPar(0).Data());
         snom.ReplaceAll("evt_", "");
         //Info("ReadHeader","lecture %d",snom.Atoi());
         nv->SetValue("event_number", snom.Atoi());

         break;
      default:
         return kFALSE;
   }

   res = ReadLineAndCheck(1, " ");
   switch (res) {
      case KVFileReader::ReadStatus::EmptyLine:
         return kFALSE;
      case KVFileReader::ReadStatus::OK:
         nv->SetValue("ndes", GetIntReadPar(0));
         ndes = 0;

         return kTRUE;
      default:
         return kFALSE;
   }


}

//-----------------------------------------

Bool_t KVSimReader_SMF_asym::ReadEvent()
{

   evt->Clear();

   Int_t mult = 0;
   auto res = ReadLineAndCheck(1, " ");
   switch (res) {
      case KVFileReader::ReadStatus::EmptyLine:
         return kFALSE;
      case KVFileReader::ReadStatus::OK:
         mult = GetIntReadPar(0);
         break;

      default:
         return kFALSE;
   }

   evt->SetNumber(nv->GetIntValue("event_number"));
   evt->GetParameters()->SetValue("sub_number", ndes);
   for (Int_t mm = 0; mm < mult; mm += 1) {
      nuc = (KVSimNucleus*)evt->AddParticle();
      if (!ReadNucleus()) return kFALSE;
   }

   nevt += 1;
   ndes += 1;

   return kTRUE;

}

//-----------------------------------------

Bool_t KVSimReader_SMF_asym::ReadNucleus()
{

   auto res = ReadLineAndCheck(5, " ");
   switch (res) {
      case KVFileReader::ReadStatus::EmptyLine:
         Info("ReadNucleus", "case 0 line est vide");
         return kFALSE;

      case KVFileReader::ReadStatus::OK:
         nuc->SetZ(GetIntReadPar(1));
         nuc->SetA(GetIntReadPar(0));

         //Axe "faisceau dans SMF z"
         nuc->SetMomentum(GetDoubleReadPar(2), GetDoubleReadPar(3), GetDoubleReadPar(4));

         return kTRUE;

      default:

         return kFALSE;
   }

}
