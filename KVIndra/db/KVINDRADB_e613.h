//Created by KVClassFactory on Mon Oct 24 14:38:16 2011
//Author: bonnet

#ifndef __KVINDRADB_E613_H
#define __KVINDRADB_E613_H

#include "KVINDRADB.h"

class KVINDRADB_e613 : public KVINDRADB {

protected:
   virtual void ReadGainList();
   virtual void ReadPedestalList();
   virtual void ReadChannelVolt();
   virtual void ReadVoltEnergyChIoSi();
   virtual void ReadOoOACQParams();

public:
   KVINDRADB_e613();
   KVINDRADB_e613(const Char_t* name);
   virtual ~KVINDRADB_e613();

   KVNameValueList GetGains(int run);
   virtual TString GetListOfOoOACQPar(int run);

   ClassDef(KVINDRADB_e613, 1) //Child class for e613 experiment
};

#endif
