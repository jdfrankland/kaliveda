//Created by KVClassFactory on Mon Jan 24 16:54:04 2022
//Author: John Frankland,,,

#ifndef __KV_CCIN2P3_SLURM_H
#define __KV_CCIN2P3_SLURM_H

#include "KVBatchSystem.h"

/**
  \class KV_CCIN2P3_Slurm
\brief Interface to CCIN2P3 Grid Engine batch job management system
\ingroup Infrastructure
 */

class KV_CCIN2P3_Slurm : public KVBatchSystem {
protected:

   Bool_t fMultiJobs;  //set to kTRUE if several jobs are to be submitted for the runlist set in fAnalyser
   Int_t fRunsPerJob; //number of runs per job submitted in multi job mode (default=1)
   KVNumberList fCurrJobRunList;//runlist for (multi job mode) job being submitted
   KVString fDefJobTime; // default job length
   KVString fDefJobMem; // default job memory allocation (with units, e.g. "512M")
   Bool_t fTimeSet;
   Bool_t fMemSet;

   virtual void ChangeDefJobOpt(KVDataAnalyser*);

public:

   KV_CCIN2P3_Slurm(const Char_t* name);

   void SetJobTime(const Char_t* h = "");      /* Set CPU time for batch job */
   void SetJobMemory(const Char_t* h = ""); /* Set max memory for batch job */
   void PrintJobs(Option_t* opt = "");  /* Print list of all jobs submitted to system */

   virtual Bool_t CheckJobParameters();        /* Checks job parameters */
   void SetMultiJobsMode(Bool_t on = kTRUE)
   {
      fMultiJobs = on;
   }
   Bool_t MultiJobsMode() const
   {
      return fMultiJobs;
   }

   void SetRunsPerJob(Int_t n)
   {
      // Set number of runs per job submitted in multi jobs mode (default=1)
      fRunsPerJob = n;
   }
   Int_t GetRunsPerJob() const
   {
      // Returns number of runs per job submitted in multi jobs mode
      return fRunsPerJob;
   }

   const Char_t* GetJobTime(void) const;    /* Get CPU time for batch job */
   const Char_t* GetJobMemory(void) const;       /* Get max memory for batch job */

   void ChooseJobTime(void);    /* Choose CPU time for batch job */
   void ChooseJobMemory(void);       /* Choose max memory for batch job */

   virtual void Clear(Option_t* opt = "");

   virtual void WriteBatchEnvFile(TEnv*);
   virtual void ReadBatchEnvFile(TEnv*);
   virtual void Print(Option_t* /*option*/ = "") const;

   virtual void SanitizeJobName() const;

   void Run();
   const Char_t* GetJobName() const;
   virtual void GetBatchSystemParameterList(KVNameValueList&);
   virtual void SetBatchSystemParameters(const KVNameValueList&);

   ClassDef(KV_CCIN2P3_Slurm, 1) //Interface to new CCIN2P3 Slurm batch system (2022)
};

#endif
