#include "KVSimReader_ELIE2023.h"



Bool_t KVSimReader_ELIE2023::ReadHeader()
{
   // File header no longer contains all info on simulation, this can be found in accompanying '.input' file
   //
   // File header now looks like:
   //~~~~
   //zproj aproj zcibl acibl ebeam number_of_events run_number
   //~~~~
   //
   // while '.input' file contains
   //~~~~
   //alevel=7.87
   //entropy_max=1.25
   //g_al=1.
   //thermal=1
   //fermi_gas=1
   //geometry=1
   // ...
   //~~~~

   auto res = ReadLineAndCheck(7, " ");
   switch (res) {
      case KVFileReader::ReadStatus::EmptyLine:
         Info("ReadHeader", "Can't read file header infos");
         return kFALSE;
      case KVFileReader::ReadStatus::OK:
         AddInfo("Aproj", GetReadPar(1));
         AddInfo("Zproj", GetReadPar(0));
         AddInfo("Atarg", GetReadPar(3));
         AddInfo("Ztarg", GetReadPar(2));
         AddInfo("Ebeam", GetReadPar(4));
         AddInfo("Nevents", GetReadPar(5));
         AddInfo("RunNumber", GetReadPar(6));
         proj.SetZAandE(GetIntReadPar(0), GetIntReadPar(1), GetIntReadPar(1)*GetDoubleReadPar(4));
         targ.SetZandA(GetIntReadPar(2), GetIntReadPar(3));
         ebeam = GetDoubleReadPar(4);
         break;
      default:
         Info("ReadHeader", "res=%d", (int)res);
         return kFALSE;
   }

   // derive '.input' file name from name of file being read
   auto input_file = GetFileName();
   auto suff_ind = input_file.Index("_primary.output");
   if (suff_ind < 0) suff_ind = input_file.Index("_secondary.output");
   if (suff_ind < 0) {
      Warning("ReadHeader", "Cannot deduce name of '.input' file from filename:%s", input_file.Data());
      Warning("ReadHeader", "Informations on input parameters of calculation will not be read/kept");
      return kFALSE;
   }
   input_file.Remove(suff_ind);
   input_file.Append(".input");

   // open '.input' file and read infos
   KVFileReader input_file_reader("#");// comments start with '#' in file
   if (!input_file_reader.OpenFileToRead(input_file)) {
      Warning("ReadHeader", "Failed to open file %s", input_file.Data());
      Warning("ReadHeader", "Informations on input parameters of calculation will not be read/kept");
      return kFALSE;
   }

   read_elie_params(input_file_reader);

   return kTRUE;
}

Bool_t KVSimReader_ELIE2023::ReadEvent()
{
   // New event header structure is:
   //~~~~
   //event_number multiplicity b_reduit reduced_density tempe_part tempe_qp tempe_qt sigma* pauli_factor
   //~~~~
   //
   // These are the names used for the event parameters

   evt->Clear();

   auto res = ReadLineAndCheck(9, " ");
   Int_t mult = 0;
   switch (res) {
      case KVFileReader::ReadStatus::EmptyLine:
      case KVFileReader::ReadStatus::EndOfFile:
         return kFALSE;
      case KVFileReader::ReadStatus::OK:
         evt->SetNumber(GetIntReadPar(0));
         mult = GetIntReadPar(1);
         evt->GetParameters()->SetValue("bred", GetDoubleReadPar(2));
         evt->GetParameters()->SetValue("mult", mult);
         evt->GetParameters()->SetValue("reduced_density", GetDoubleReadPar(3));
         evt->GetParameters()->SetValue("tempe_part", GetDoubleReadPar(4));
         evt->GetParameters()->SetValue("tempe_qp", GetDoubleReadPar(5));
         evt->GetParameters()->SetValue("tempe_qt", GetDoubleReadPar(6));
         evt->GetParameters()->SetValue("sigma*", GetDoubleReadPar(7));
         evt->GetParameters()->SetValue("pauli_factor", GetDoubleReadPar(8));
         break;

      default:
         return kFALSE;
   }
   for (Int_t mm = 0; mm < mult; mm++) {
      nuc = (KVSimNucleus*)evt->AddParticle();
      if (!ReadNucleus()) return kFALSE;
   }
   nevt++;

   // if in lab, transform to CM frame
   if (elie_params->GetIntValue("lab_frame") > 0) transform_to_cm();
   else evt->SetFrameName("CM");

   return kTRUE;
}

Bool_t KVSimReader_ELIE2023::ReadNucleus()
{
   // New particle structure is:
   //~~~~
   //particule_number z_part a_part teta_part phi_part e_part exci_part origine_part
   //~~~~
   //
   //
   // The origin of secondary decay particles is stored in a parameter named "ORIGIN"
   // As particles in a KVEvent are numbered 1,2,... we add 1 to the value read in

   auto res = ReadLineAndCheck(8, " ");
   switch (res) {
      case KVFileReader::ReadStatus::EndOfFile:
         Info("ReadNucleus", "premature end of file?");
         return kFALSE;
      case KVFileReader::ReadStatus::EmptyLine:
         Info("ReadNucleus", "case 0 line est vide");
         return kFALSE;

      case KVFileReader::ReadStatus::OK:
         nuc->SetZ(GetIntReadPar(1));
         nuc->SetA(GetIntReadPar(2));
         nuc->SetExcitEnergy(GetDoubleReadPar(6));
         nuc->SetEnergy(GetDoubleReadPar(5));
         nuc->SetTheta(GetDoubleReadPar(3));
         nuc->SetPhi(GetDoubleReadPar(4));
         nuc->SetParameter("ORIGIN", GetIntReadPar(7) + 1);
         return kTRUE;
         break;

      default:
         return kFALSE;
   }

   return kFALSE;
}

ClassImp(KVSimReader_ELIE2023)


