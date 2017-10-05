/***************************************************************************
                          kvINDRADB.h  -  description
                             -------------------
    begin                : 9/12 2003
    copyright            : (C) 2003 by J.D. Frankland
    email                : frankland@ganil.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KV_INDRADB_H
#define KV_INDRADB_H

#include "KVExpDB.h"
#include "KVDataSet.h"
#include "KVINDRADBRun.h"
#include "KVDBSystem.h"
#include "KVDetector.h"
#include "Riostream.h"
#include "TEnv.h"
#include "KVINDRARunListReader.h"
#include "KVINDRAPulserDataTree.h"
#include "KVNumberList.h"
#include "KVDBChIoPressures.h"

class KVINDRADB: public KVExpDB, public KVINDRARunListReader {

private:

   std::ifstream __ifpeaks;          //!ifstream for calibration peaks file

protected:

   mutable unique_ptr<KVINDRAPulserDataTree> fPulserData;  //! mean values of pulsers for all detectors & runs

   class calib_file_reader {
      KVINDRADB* mydb;
      KVSQLite::table new_table;
   protected:
      KVSQLite::database& GetDB() const
      {
         return mydb->GetDB();
      }
      KVNumberList runrange;
      Int_t new_table_num;
      KVSQLite::table& get_table()
      {
         return new_table;
      }
      bool format_warning;
   public:
      calib_file_reader(KVINDRADB* a) : mydb(a), new_table_num(0) {}
      virtual ~calib_file_reader() {}

      void ReadCalib(const TString& calib_type,
                     const TString& caller_method_name,
                     const TString& informational_message);

      virtual void initial_setup_new_table() = 0;
      virtual void write_data_to_db(const TString&);
      virtual bool read_data_line(const char*) = 0;
      virtual void add_new_table(const TString&);
      virtual void insert_data_into_table() = 0;
   };

   virtual void ReadGainList();
   class gain_list_reader : public calib_file_reader {
      Float_t gain;
      Char_t det_name[80];

   public:
      gain_list_reader(KVINDRADB* a) : calib_file_reader(a) {}
      virtual ~gain_list_reader() {}

      void initial_setup_new_table();
      bool read_data_line(const char*);
      void insert_data_into_table();
   };

   virtual void ReadChIoPressures();
   virtual void ReadCsITotalLightGainCorrections();
   virtual void ReadChannelVolt();
   class channel_volt_reader : public calib_file_reader {
   protected:
      UInt_t cour, modu, sign;
      Float_t a0, a1, a2, dum1, dum2;
   public:
      channel_volt_reader(KVINDRADB* a) : calib_file_reader(a) {}
      virtual ~channel_volt_reader() {}
      void initial_setup_new_table();
      bool read_data_line(const char*);
      void insert_data_into_table();
   };
   class etalon_channel_volt_reader : public channel_volt_reader {
      Char_t det_name[80];
   public:
      etalon_channel_volt_reader(KVINDRADB* a) : channel_volt_reader(a) {}
      virtual ~etalon_channel_volt_reader() {}
      bool read_data_line(const char*);
      void insert_data_into_table();
   };

   virtual void ReadVoltEnergyChIoSi();
   class volt_energy_chiosi_reader : public calib_file_reader {
      Char_t det_name[80];
      Float_t a0, a1, chi;
   public:
      volt_energy_chiosi_reader(KVINDRADB* a) : calib_file_reader(a) {}
      virtual ~volt_energy_chiosi_reader() {}
      void initial_setup_new_table();
      bool read_data_line(const char*);
      void insert_data_into_table();
   };

   virtual void ReadLightEnergyCsI(const Char_t*);
   virtual void ReadCalibCsI();
   virtual void ReadPedestalList();

   virtual void ReadAbsentDetectors();
   virtual void ReadOoODetectors();
   virtual void ReadOoOACQParams();


   //calibration peaks database
   Bool_t OpenCalibrationPeakFile();
   void CloseCalibrationPeakFile();
   std::ifstream& GetPeakFileStream()
   {
      return __ifpeaks;
   }

   void init();

   virtual Bool_t ReadPedestals(const TString& filename, const TString& tablename);
   void ReadTEnvStatusFile(const TString& calling_method, const TString& informational, const TString& calibfilename,
                           const TString& status_table, const TString& status_table_column, const TString& info_table_basename);

   TString GetStatusList(int run, const TString& what);

public:
   KVINDRADB();
   KVINDRADB(const Char_t* name);
   virtual ~ KVINDRADB();

   virtual void Build();
   virtual void cd();

   virtual void GoodRunLine();
   KVINDRADBRun* GetRun(Int_t run) const
   {
      return (KVINDRADBRun*) KVExpDB::GetRun(run);
   }
   void ReadNewRunList();

   KVDBChIoPressures GetChIoPressures(int run);
   virtual KVNameValueList GetGains(int run);

   KVList* GetCalibrationPeaks(Int_t run, KVDetector* detector = 0,
                               Int_t peak_type = -1, Int_t signal_type = 0,
                               Double_t peak_energy = -1.0);

   Double_t GetEventCrossSection(Int_t run, Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetTotalCrossSection(Int_t run, Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetEventCrossSection(Int_t run1, Int_t run2,
                                 Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetTotalCrossSection(Int_t run1, Int_t run2,
                                 Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetEventCrossSection(KVNumberList runs,
                                 Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetTotalCrossSection(KVNumberList runs,
                                 Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetEventCrossSection(const Char_t* system, Int_t Mult_trigger,
                                 Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetTotalCrossSection(const Char_t* system, Int_t Mult_trigger,
                                 Double_t Q_apres_cible,
                                 Double_t Coul_par_top = 1.e-10) const;
   Double_t GetTotalCrossSection(TH1* events_histo, Double_t Q_apres_cible, Double_t Coul_par_top = 1.e-10);


   KVINDRAPulserDataTree* GetPulserData() const;

   virtual const Char_t* GetDBEnv(const Char_t* type) const;
   KVNameValueList GetPedestals_ChIoSi(int run);
   KVNameValueList GetPedestals_CsI(int run);
   virtual TString GetListOfAbsentDetectors(int run);
   virtual TString GetListOfOoODetectors(int run);
   virtual TString GetListOfOoOACQPar(int run);

   ClassDef(KVINDRADB, 5)       //DataBase of parameters for an INDRA campaign
};

//........ global variable
R__EXTERN KVINDRADB* gIndraDB;

#endif
