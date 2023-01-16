//Created by KVClassFactory on Mon Jan 24 16:54:04 2022
//Author: John Frankland,,,

#include "KV_CCIN2P3_Slurm.h"
#include "TSystem.h"
#include "TEnv.h"
#include "KVDataAnalyser.h"
#include "KVDataAnalysisTask.h"
#include "KVDataSetAnalyser.h"
#include "KVSimDirAnalyser.h"

using namespace std;

ClassImp(KV_CCIN2P3_Slurm)

KV_CCIN2P3_Slurm::KV_CCIN2P3_Slurm(const Char_t* name)
   : KVBatchSystem(name), fMultiJobs(kTRUE)
{
   //Default constructor
   //Sets default job time, memory and disk space as defined in $KVROOT/KVFiles/.kvrootrc

   fDefJobTime = gEnv->GetValue("GE.BatchSystem.DefaultJobTime", "00:05:00");
   fDefJobMem = gEnv->GetValue("GE.BatchSystem.DefaultJobMemory", "3G");
   fTimeSet = fMemSet = kFALSE;
   //default number of runs per job in multi jobs mode (default=1)
   SetRunsPerJob(gEnv->GetValue("GE.BatchSystem.RunsPerJob", 1));
}

//_______________________________________________________________________________//

void KV_CCIN2P3_Slurm::Clear(Option_t* opt)
{
   //Clear previously set parameters in order to create a new job submission command
   KVBatchSystem::Clear(opt);
   fTimeSet = fMemSet = kFALSE;
   fMultiJobs = kTRUE;
}

//_______________________________________________________________________________//

void KV_CCIN2P3_Slurm::SetJobTime(const Char_t* time)
{
   //Set CPU time for batch job.
   //      SetJobTime() => use default time
   KVString tmp(time);
   if (tmp == "") tmp = fDefJobTime;
   //time given as "hh:mm:ss"
   if (tmp.GetNValues(":") == 2) tmp.Prepend("00:");
   else if (tmp.GetNValues(":") == 1) tmp.Prepend("00:00:");
   fParList.SetValue("--time ", tmp);
   fTimeSet = kTRUE;
}

void KV_CCIN2P3_Slurm::SetJobMemory(const Char_t* mem)
{
   //Set maximum memory used by job.
   //Include units in string, i.e. "100M", "1G" etc.
   //If mem="", use default value
   KVString tmp(mem);
   if (tmp == "") tmp = fDefJobMem;
   fParList.SetValue("--mem ", tmp);
   fMemSet = kTRUE;
}

void KV_CCIN2P3_Slurm::PrintJobs(Option_t*)
{
   //Print list of owner's jobs.
   KVList* j = GetListOfJobs();
   j->ls();
   delete j;
}

Bool_t KV_CCIN2P3_Slurm::CheckJobParameters()
{
   // Checks the job and asks for any missing parameters

   if (!KVBatchSystem::CheckJobParameters()) return kFALSE;

   if (!fTimeSet) ChooseJobTime();

   if (!fMemSet) ChooseJobMemory();

   return kTRUE;
}

void KV_CCIN2P3_Slurm::ChooseJobTime()
{
   KVString tmp = "";
   cout << "Enter max CPU time per job (ss/mn:ss/hh:mn:ss) ["
        << fDefJobTime << "] : ";
   cout.flush();
   tmp.ReadToDelim(cin);
   if (!tmp.Length()) {
      SetJobTime();
      return;
   }
   else
      SetJobTime(tmp);
}

void KV_CCIN2P3_Slurm::ChooseJobMemory()
{
   KVString tmp = "";
   cout << "Enter max memory per job (xKB/xMB/xGB) ["
        << fDefJobMem.Data() << "] : ";
   cout.flush();
   tmp.ReadToDelim(cin);
   SetJobMemory(tmp.Data());
}

const Char_t* KV_CCIN2P3_Slurm::GetJobTime(void) const
{
// returns the parameter string corresponding to the job CPU time
   return fParList.GetStringValue("--time ");
}

const Char_t* KV_CCIN2P3_Slurm::GetJobMemory(void) const
{
// returns the parameter string corresponding to the job Memory
   return fParList.GetStringValue("--mem ");
}

//_______________________________________________________________________________//

void KV_CCIN2P3_Slurm::WriteBatchEnvFile(TEnv* env)
{
   //Store any useful information on batch system in the TEnv
   //(this method is used by KVDataAnalyser::WriteBatchEnvFile)
   KVBatchSystem::WriteBatchEnvFile(env);
   env->SetValue("BatchSystem.MultiJobs", MultiJobsMode());
   if (MultiJobsMode()) env->SetValue("BatchSystem.CurrentRunList", fCurrJobRunList.AsString());
   env->SetValue("BatchSystem.Time", GetJobTime());
   env->SetValue("BatchSystem.Memory", GetJobMemory());
   // if analysis of simulated data is being used, we copy the files to analyse to the
   // scratch disk of the batch job (make sure enough disk space is requested)
   env->SetValue("SimDirAnalyser.CopyFilesToWorkingDirectory", true);
}

//_______________________________________________________________________________//

void KV_CCIN2P3_Slurm::ReadBatchEnvFile(TEnv* env)
{
   //Read any useful information on batch system from the TEnv
   //(this method is used by KVDataAnalyser::ReadBatchEnvFile)
   KVBatchSystem::ReadBatchEnvFile(env);
   SetMultiJobsMode(env->GetValue("BatchSystem.MultiJobs", kFALSE));
   if (MultiJobsMode()) fCurrJobRunList.SetList(env->GetValue("BatchSystem.CurrentRunList", ""));
   SetJobTime(env->GetValue("BatchSystem.Time", ""));
   SetJobMemory(env->GetValue("BatchSystem.Memory", ""));
}

//_______________________________________________________________________________//

void KV_CCIN2P3_Slurm::Print(Option_t* option) const
{
   //if option="log", print infos for batch log file
   //if option="all", print detailed info on batch system
   if (!strcmp(option, "log")) {
      KVBatchSystem::Print(option);
      cout << "* MEM_REQ:         " << GetJobMemory() << "                             *" << endl;
   }
   else
      KVBatchSystem::Print(option);
}

//_______________________________________________________________________________//

void KV_CCIN2P3_Slurm::ChangeDefJobOpt(KVDataAnalyser* da)
{
   // PRIVATE method called by SubmitTask() at moment of job submission.
   // Depending on the current environment, the default job submission options
   // may be changed by this method.
   //
   // This method overrides and augments KVBatchSystem::ChangeDefJobOpt (which
   // changes the options as a function of the type of analysis task).
   // Here we add the CCIN2P3-specific case where the job is launched from a directory
   // on the /sps/ semi-permanent storage facility, or if the data being analysed is
   // stored in a repository on /sps/.
   //
   // Then we must declare the resource 'sps'.
   // As the 'sbatch' command does not allow multiple '-L' ressource declarations,
   // the whole thing has to be done here. 'xrootd' resource is declared by
   // default, 'sps' is added if needed.

   KVBatchSystem::ChangeDefJobOpt(da);
   KVString taskname = da->GetAnalysisTask()->GetName();
   KVString rootdir = da->GetRootDirectoryOfDataToAnalyse();
   Bool_t repIsSPS = rootdir.BeginsWith("/sps/");

   KVString wrkdir(gSystem->WorkingDirectory());
   KVString oldoptions(GetDefaultJobOptions());

   KVString ressource_list(" -L xrootd");
   Bool_t NeedToAddSPS = wrkdir.Contains("/sps/");
   if ((NeedToAddSPS || repIsSPS)) {
      ressource_list += ",sps";
   }
   oldoptions += ressource_list;
   SetDefaultJobOptions(oldoptions.Data());
}


void KV_CCIN2P3_Slurm::SanitizeJobName() const
{
   // Batch-system dependent sanitization of jobnames
   // Grid Engine does not allow:
   //   :
   // Any such character appearing in the current jobname will be replaced
   // with '_'

   fCurrJobName.ReplaceAll(":", "_");
}

void KV_CCIN2P3_Slurm::Run()
{
   //Processes the job requests for the batch system.
   //In normal mode, this submits one job for the data analyser fAnalyser
   //In multijobs mode, this submits one job for each run in the runlist associated to fAnalyser

   if (!CheckJobParameters()) return;

   if (MultiJobsMode()) {
      if (fAnalyser->InheritsFrom("KVDataSetAnalyser")) {
         //submit jobs for every GetRunsPerJob() runs in runlist
         KVDataSetAnalyser* ana = dynamic_cast<KVDataSetAnalyser*>(fAnalyser);
         KVNumberList runs = ana->GetRunList();
         runs.Begin();
         Int_t remaining_runs = runs.GetNValues();
         fCurrJobRunList.Clear();
         while (remaining_runs && !runs.End()) {
            Int_t run = runs.Next();
            remaining_runs--;
            fCurrJobRunList.Add(run);
            if ((fCurrJobRunList.GetNValues() == GetRunsPerJob()) || runs.End()) {
               // submit job for GetRunsPerJob() runs (or less if we have reached end of runlist 'runs')
               ana->SetRuns(fCurrJobRunList, kFALSE);
               ana->SetFullRunList(runs);
               SubmitJob();
               fCurrJobRunList.Clear();
            }
         }
         ana->SetRuns(runs, kFALSE);
      }
      else if (fAnalyser->InheritsFrom("KVSimDirAnalyser")) {
         // here we understand "run" to mean "file"
         KVSimDirAnalyser* ana = dynamic_cast<KVSimDirAnalyser*>(fAnalyser);
         TList* file_list = ana->GetFileList();
         Int_t remaining_runs = ana->GetNumberOfFilesToAnalyse();
         fCurrJobRunList.Clear();
         TList cur_file_list;
         TObject* of;
         TIter it(file_list);
         Int_t file_no = 1;
         while ((of = it())) {
            cur_file_list.Add(of);
            fCurrJobRunList.Add(file_no);
            remaining_runs--;
            file_no++;
            if ((fCurrJobRunList.GetNValues() == GetRunsPerJob()) || (remaining_runs == 0)) {
               // submit job for GetRunsPerJob() files (or less if we have reached end of list)
               ana->SetFileList(&cur_file_list);
               SubmitJob();
               fCurrJobRunList.Clear();
               cur_file_list.Clear();
            }
         }
         ana->SetFileList(file_list);
      }
   }
   else {
      SubmitJob();
   }

}

const Char_t* KV_CCIN2P3_Slurm::GetJobName() const
{
   //Returns name of batch job, either during submission of batch jobs or when an analysis
   //task is running in batch mode (access through gBatchSystem global pointer).
   //
   //In multi-job mode, the job name is generated from the base name set by SetJobName()
   //plus the extension "_Rxxxx-yyyy" with "xxxx" and "yyyy" the number of the first and last run
   //which will be analysed by the current job.
   //
   // Depending on the batch system, some sanitization of the jobname may be required
   // e.g. to remove "illegal" characters from the jobname. This is done by SanitizeJobName()
   // before the jobname is returned.

   if (!fAnalyser) {
      //stand-alone batch submission ?
      fCurrJobName = fJobName;
   }
   else {
      //replace any special symbols with their current values
      fCurrJobName = fAnalyser->ExpandAutoBatchName(fJobName.Data());
      if (MultiJobsMode() && !fAnalyser->BatchMode()) {
         KVString tmp;
         if (fCurrJobRunList.GetNValues() > 1)
            tmp.Form("_R%d-%d", fCurrJobRunList.First(), fCurrJobRunList.Last());
         else
            tmp.Form("_R%d", fCurrJobRunList.First());
         fCurrJobName += tmp;
      }
   }
   SanitizeJobName();
   return fCurrJobName.Data();
}

void KV_CCIN2P3_Slurm::GetBatchSystemParameterList(KVNameValueList& nl)
{
   // Fill the list with all relevant parameters for batch system,
   // set to their default values.
   //
   // Parameters defined here are:
   //   JobTime        [string]
   //   JobMemory      [string]
   //   MultiJobsMode  [bool]
   //   RunsPerJob     [int]
   //   EMailOnStart   [bool]
   //   EMailOnEnd     [bool]
   //   EMailAddress   [string]

   KVBatchSystem::GetBatchSystemParameterList(nl);
   nl.SetValue("JobTime", fDefJobTime);
   nl.SetValue("JobMemory", fDefJobMem);
   nl.SetValue("MultiJobsMode", MultiJobsMode());
   nl.SetValue("RunsPerJob", fRunsPerJob);
}

void KV_CCIN2P3_Slurm::SetBatchSystemParameters(const KVNameValueList& nl)
{
   // Use the parameters in the list to set all relevant parameters for batch system.

   KVBatchSystem::SetBatchSystemParameters(nl);
   SetJobTime(nl.GetStringValue("JobTime"));
   SetJobMemory(nl.GetStringValue("JobMemory"));
   SetMultiJobsMode(nl.GetBoolValue("MultiJobsMode"));
   SetRunsPerJob(nl.GetIntValue("RunsPerJob"));
}
