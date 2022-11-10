#ifndef __KVFILEREADER_H
#define __KVFILEREADER_H

#include <string>
#include "KVBase.h"
#include "TSystem.h"
#include "KVString.h"
#include "TString.h"

/**
  \class KVFileReader
  \ingroup Core
  \brief Handle reading columns of numeric data in text files
\author Eric Bonnet (ebonnet@subatech.in2p3.fr)
\date Tue Jul 13 11:52:58 2010

This class can be used to read any text file containing numeric data in columns such as
~~~~
0.1   19    3.56    2
0.2   29    3.66    1
0.3   39    3.26    -1
~~~~
or
~~~~
0.1,19,3.56,2
0.2,29,3.66,1
0.3,39,3.26,-1
~~~~
i.e. columns of data separated by whitespace, commas, tabs, or any other separator character

Comment lines will be ignored if the signature for the beginning of a comment is given to the constructor,
e.g.
~~~~{.cpp}
KVFileReader data_file("#"); // comment lines begin with '#'

KVFileReader data_file2("//"); // comment lines begin with '//'
~~~~

To open the file to be read, give the full path (shell characters & environment variables can be used) to
method OpenFileToRead():
~~~~{.cpp}
auto ok = data_file.OpenFileToRead("$SOME_ENV/my_file.dat");
if( ok )
{
   [file was opened successfully]
}
~~~~

Each line of the file can then be read in using one of the following methods:
~~~~{.cpp}
 ReadLine(const KVString& pattern = "")
 ReadLineAndAdd(const KVString& pattern = "")
 ReadLineAndCheck(Int_t nexpect, const KVString& pattern)
 ReuseLineAndCheck(Int_t nexpect, const KVString& pattern)
~~~~

The values of the parameters which were read, if any, can then be retrieved using
~~~~{.cpp}
KVString GetReadPar(int pos);
Int_t GetIntReadPar(int pos);
Double_t GetDoubleReadPar(int pos);
~~~~

Assuming we are reading either of the files given above, the following examples illustrate usage:
~~~~{.cpp}
   KVFileReader p;
   p.OpenFileToRead("data.txt");

   while (p.IsOK()) {
      if (p.ReadLine(" \t,") == KVFileReader::ReadStatus::OK) {
         std::cout << p.GetDoubleReadPar(0) << " " << p.GetIntReadPar(1) << " " << p.GetDoubleReadPar(2) << " " << p.GetIntReadPar(3) << std::endl;
      }
   }
~~~~
will give the following output:
~~~~
0.1 19 3.56 2
0.2 29 3.66 1
0.3 39 3.26 -1
~~~~

If you want to check the right number of parameters is being read:
~~~~{.cpp}
   KVFileReader p;
   p.OpenFileToRead("data.txt");

   while (p.IsOK()) {
      if (p.ReadLineAndCheck(4," \t,") == KVFileReader::ReadStatus::OK) {
         std::cout << p.GetDoubleReadPar(0) << " " << p.GetIntReadPar(1) << " " << p.GetDoubleReadPar(2) << " " << p.GetIntReadPar(3) << std::endl;
      }
   }
~~~~
will give the following output:
~~~~
0.1 19 3.56 2
0.2 29 3.66 1
0.3 39 3.26 -1
~~~~

And finally if you want to cumulate several lines of parameters into a single list:
~~~~{.cpp}
   KVFileReader p;
   p.OpenFileToRead("data.txt");

   while (p.ReadLineAndAdd(" \t,") == KVFileReader::ReadStatus::OK) ;

   for(int i=0; i<p.GetNparRead(); ++i){
      std::cout << i << ":" << p.GetReadPar(i) << " ";
      if(!((i+1)%4)) std::cout << std::endl;
   }
~~~~
will give the following output:
~~~~
0:0.1 1:19 2:3.56 3:2
4:0.2 5:29 6:3.66 7:1
8:0.3 9:39 10:3.26 11:-1
~~~~
 */
class KVFileReader : public KVBase {
private:
   void init()
   {
      reading_line = "";
      nline = 0;
      skip_comments = !comment_string.IsNull();
   }
   void StoreParameters(const KVString& pattern)
   {
      // Store values of columnar data in last read line according to delimiter(s) in pattern
      items.clear();
      reading_line.Begin(pattern);
      while (!reading_line.End()) items.emplace_back(reading_line.Next(kTRUE).Data());
   }

   void AddParameters(const KVString& pattern)
   {
      // Add values of columnar data in last read line according to delimiter(s) in pattern
      // to list of values read from a previous line with either StoreParameters() or AddParameters()
      reading_line.Begin(pattern);
      while (!reading_line.End()) items.emplace_back(reading_line.Next(kTRUE).Data());
   }


protected:
   std::vector<std::string> items;//!
   KVString reading_line, file_name, comment_string;
   Int_t nline;
   Bool_t status;
   Bool_t skip_comments = false;

public:
   std::ifstream f_in;

   /**
      @brief status returned by each method used to read a line in the file
    */
   enum class ReadStatus {
      EmptyLine, ///< last line read was empty (only whitespace)
      OK,        ///< successful read and import of parameters from line
      ParamMismatch,///< the number of parameters read from line does not correspond to expectations
      CommentLine,///< last line read was a comment line
      EndOfFile ///< end of file reached
   };

   KVString GetReadStatus(ReadStatus s)
   {
      switch (s) {
         case ReadStatus::EmptyLine:
            return "EmptyLine";
         case ReadStatus::CommentLine:
            return "CommentLine";
         case ReadStatus::EndOfFile:
            return "EndOfFile";
         case ReadStatus::OK:
            return "OK";
         case ReadStatus::ParamMismatch:
            return "ParamMismatch";
      }
      return "???";
   }
   KVFileReader(const KVString& comments = "")
      : comment_string(comments)
   {
      // \param[in] comments if given, any lines beginning with the given string will be ignored
      init();
   }

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
      // Close current file, reset file reader, then reopen file for reading
      //
      // \returns true if file is open for reading, false in case of a problem opening the file
      CloseFile();
      Clear();
      return OpenFileToRead(GetFileName());
   }

   Bool_t OpenFileToRead(const KVString& filename)
   {
      // \param[in] filename name or full path to file to be read. shell characters, environment variables are accepted.
      //
      // Open file for reading
      //
      // \returns true if file is open for reading, false in case of a problem opening the file

      TString _filename = filename;
      gSystem->ExpandPathName(_filename);
      file_name = _filename;

      f_in.open(_filename.Data());
      status = f_in.good();

      if (!status)
         Error("OpenFileToRead", "Failed to open file %s", _filename.Data());

      return status;
   }

   Bool_t IsOK()
   {
      // \returns true if file is open and ready for reading
      return f_in.good();
   }

   void CloseFile()
   {
      // Close file if it is open
      if (f_in.is_open()) f_in.close();
   }

   ReadStatus ReadLine(const KVString& pattern = "")
   {
      // Read the next line from the file, store any parameter values read
      //
      // \returns status of file after reading
      //
      // @param[in] pattern if given, contains separator(s) to be used to separate & store values of columnar data (see KVString::Begin())
      reading_line.ReadLine(f_in);
      if (!IsOK()) return ReadStatus::EndOfFile;

      nline++;
      if (skip_comments && reading_line.BeginsWith(comment_string)) return ReadStatus::CommentLine;
      if (!pattern.IsNull())
         StoreParameters(pattern);

      return ReadStatus::OK;
   }

   ReadStatus ReadLineAndAdd(const KVString& pattern = "")
   {
      // Read the next line from the file, add any parameter values read to those already stored
      //
      // \returns status of file after reading
      //
      // @param[in] pattern if given, contains separator(s) to be used to separate & store values of columnar data (see KVString::Begin())
      reading_line.ReadLine(f_in);
      if (!IsOK()) return ReadStatus::EndOfFile;

      nline++;
      if (skip_comments && reading_line.BeginsWith(comment_string)) return ReadStatus::CommentLine;
      if (!pattern.IsNull())
         AddParameters(pattern);

      return ReadStatus::OK;
   }

   ReadStatus ReadLineAndCheck(Int_t nexpect, const KVString& pattern)
   {
      // Read the next line from the file, store any parameter values read
      //
      // \returns status of file after reading, notably KVFileReader::ReadStatus::ParamMismatch if incorrect number of values is found
      //
      // @param[in] pattern contains separator(s) to be used to separate & store values of columnar data (see KVString::Begin())
      // @param[in] nexpext the expected number of items of columnar data in the line
      reading_line.ReadLine(f_in);
      if (!IsOK()) return ReadStatus::EndOfFile;

      nline++;
      if (skip_comments && reading_line.BeginsWith(comment_string)) return ReadStatus::CommentLine;

      GetCurrentLine().RemoveAllExtraWhiteSpace();

      if (GetCurrentLine().IsNull()) {
         return ReadStatus::EmptyLine;
      }
      StoreParameters(pattern);
      if (GetNparRead() != nexpect) {
         return ReadStatus::ParamMismatch;
      }
      return ReadStatus::OK;
   }
   ReadStatus ReuseLineAndCheck(Int_t nexpect, const KVString& pattern)
   {
      // Same as ReadLineAndCheck() except that instead of reading a new line we re-use the last read line
      //
      // \returns status of file after reading, notably KVFileReader::ReadStatus::ParamMismatch if incorrect number of values is found
      //
      // @param[in] pattern contains separator(s) to be used to separate & store values of columnar data (see KVString::Begin())
      // @param[in] nexpext the expected number of items of columnar data in the line

      StoreParameters(pattern);
      if (GetNparRead() != nexpect) {
         return ReadStatus::ParamMismatch;
      }
      return ReadStatus::OK;
   }

   KVString GetCurrentLine()
   {
      return reading_line;
   }

   Int_t GetNparRead() const
   {
      return items.size();
   }
   Int_t GetNlineRead() const
   {
      return nline;
   }

   Double_t GetDoubleReadPar(Int_t pos) const
   {
      return GetReadPar(pos).Atof();
   }
   Int_t GetIntReadPar(Int_t pos) const
   {
      return GetReadPar(pos).Atoi();
   }
   KVString GetReadPar(Int_t pos) const
   {
      return items[pos].c_str();
   }

   ClassDef(KVFileReader, 2) //Manage the reading of file
};

#endif
