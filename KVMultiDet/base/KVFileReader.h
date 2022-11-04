//Created by KVClassFactory on Tue Jul 13 11:52:58 2010
//Author: Eric Bonnet

#ifndef __KVFILEREADER_H
#define __KVFILEREADER_H

#include "KVBase.h"
#include "Riostream.h"
#include "TObjArray.h"
#include "KVString.h"
#include "TString.h"
#include "TObjString.h"

/**
  \class KVFileReader
  \ingroup Core
  \brief Handle reading text files
 */
class KVFileReader : public KVBase {
private:
   void init()
   {
      reading_line = "";
      nline = 0;
      skip_comments = !comment_string.IsNull();
   }

protected:
   std::unique_ptr<TObjArray> toks;//!
   KVString reading_line, file_name, comment_string;
   Int_t nline;
   Bool_t status;
   Bool_t skip_comments = false;

public:
   std::ifstream f_in;

   enum class ReadStatus {
      EmptyLine,
      OK,
      ParamMismatch,
      CommentLine,
      EndOfFile
   };

   KVFileReader(const KVString& comments = "")
      : comment_string(comments)
   {
      // \param[in] comments if given, any lines beginning with the given string will be ignored
      init();
   }
   KVFileReader(const KVFileReader&);
   virtual void Copy(TObject&) const;

   KVString GetFileName()
   {
      return file_name;
   }

   void Clear(Option_t* /*opt*/ = "")
   {
      init();
   }

   Bool_t PreparForReadingAgain()
   {
      CloseFile();
      Clear();
      return OpenFileToRead(GetFileName());
   }

   Bool_t OpenFileToRead(KVString filename)
   {

      file_name = filename;

      f_in.open(filename.Data());
      status = f_in.good();

      if (!status)
         Error("OpenFileToRead", "Failed to open file %s", filename.Data());

      return status;

   }

   Bool_t IsOK()
   {
      return f_in.good();
   }

   void CloseFile()
   {
      if (f_in.is_open()) f_in.close();
   }

   void ReadLine(const Char_t* pattern)
   {
      reading_line.ReadLine(f_in);
      nline++;
      if (skip_comments && reading_line.BeginsWith(comment_string)) return;
      if (pattern)
         StoreParameters(pattern);
   }

   void ReadLineAndAdd(const Char_t* pattern)
   {
      reading_line.ReadLine(f_in);
      nline++;
      if (skip_comments && reading_line.BeginsWith(comment_string)) return;
      if (pattern)
         AddParameters(pattern);
   }

   ReadStatus ReadLineAndCheck(Int_t nexpect, const Char_t* pattern)
   {
      reading_line.ReadLine(f_in);
      if (!IsOK()) return ReadStatus::EndOfFile;

      nline++;
      if (skip_comments && reading_line.BeginsWith(comment_string)) return ReadStatus::CommentLine;

      GetCurrentLine().RemoveAllExtraWhiteSpace();

      if (GetCurrentLine().IsNull()) {
         return ReadStatus::EmptyLine;
      }
      StoreParameters(pattern);
      if (GetNparRead() == nexpect) {
         return ReadStatus::OK;
      }
      else {
         return ReadStatus::ParamMismatch;
      }

   }
   ReadStatus ReuseLineAndCheck(Int_t nexpect, const Char_t* pattern)
   {
      // Same as ReadLineAndCheck, except that instead of reading a
      // new line we re-use the last read line
      StoreParameters(pattern);
      if (GetNparRead() == nexpect) {
         return ReadStatus::OK;
      }
      else {
         return ReadStatus::ParamMismatch;
      }

   }

   KVString GetCurrentLine()
   {
      return reading_line;
   }

   void StoreParameters(const Char_t* pattern)
   {
      toks.reset(GetCurrentLine().Tokenize(pattern));
   }

   void AddParameters(const Char_t* pattern)
   {
      std::unique_ptr<TObjArray> tamp(GetCurrentLine().Tokenize(pattern));
      Int_t ne = tamp->GetEntries();
      // toks may be uninitialized
      if (toks.get() == nullptr) toks.reset(new TObjArray);
      for (Int_t kk = 0; kk < ne; kk += 1) toks->Add(tamp->RemoveAt(kk));
   }

   Int_t GetNparRead()
   {
      return toks->GetEntries();
   }
   Int_t GetNlineRead()
   {
      return nline;
   }

   Double_t GetDoubleReadPar(Int_t pos)
   {
      return GetReadPar(pos).Atof();
   }
   Int_t GetIntReadPar(Int_t pos)
   {
      return GetReadPar(pos).Atoi();
   }
   TString GetReadPar(Int_t pos)
   {
      return ((TObjString*)toks->At(pos))->GetString();
   }


   ClassDef(KVFileReader, 2) //Manage the reading of file
};

#endif
