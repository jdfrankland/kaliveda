class KVINDRAEventSelectorTemplate : public KVINDRAEventSelector {
   int Mult;
   int Z[1000], A[1000];//!
   double E[1000], Theta[1000], Phi[1000]; //!
   double Vx[1000], Vy[1000], Vz[1000]; //!

public:
   KVINDRAEventSelectorTemplate() {};
   virtual ~KVINDRAEventSelectorTemplate() {};

   virtual void InitRun();
   virtual void EndRun();
   virtual void InitAnalysis();
   virtual Bool_t Analysis();
   virtual void EndAnalysis();

   ClassDef(KVINDRAEventSelectorTemplate, 0);
};
