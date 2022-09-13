#include "KVSQLROOTIDGridManager.h"

#include <KVUnownedList.h>

ClassImp(KVSQLROOTIDGridManager)

KVSQLROOTIDGridManager::KVSQLROOTIDGridManager(const TString& grid_db)
   : KVIDGridManager(),
     fGridDB(grid_db)
{
   // Open the given (full path) to grid database
}

void KVSQLROOTIDGridManager::LoadGridsForRun(UInt_t run)
{
   // Load into memory all identification grids which are valid for the run.
   //
   // Any previously loaded grids are first deleted.

   Clear();
   KVIDGraph::SetAutoAdd(kFALSE);//otherwise we end up with all grids appearing twice
   fGridDB.FillListOfObjectsWithSelection(GetGrids(), "", "Runlist", run);
   KVIDGraph::SetAutoAdd(kTRUE);
}
