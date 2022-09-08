#ifndef __KVSQLROOTIDGRIDMANAGER_H
#define __KVSQLROOTIDGRIDMANAGER_H

#include "KVIDGridManager.h"
#include "KVSQLROOTFile.h"

/**
 \class KVSQLROOTIDGridManager
 \brief ID grid manager using KVSQLROOTFile backend

 Write a detailed documentation for your class here, see doxygen manual for help.

 \author John Frankland
 \date Thu Sep  8 14:11:03 2022
*/

class KVSQLROOTIDGridManager : public KVIDGridManager {
   KVSQLROOTFile fGridDB;

public:
   KVSQLROOTIDGridManager(const TString& grid_db);

   bool IsSQLROOT() const
   {
      return true;
   }

   void LoadGridsForRun(UInt_t);

   ClassDef(KVSQLROOTIDGridManager, 1) //ID grid manager using KVSQLROOTFile backend
};

#endif
