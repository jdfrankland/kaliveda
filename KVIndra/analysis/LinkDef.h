#ifdef __CINT__
#include "KVConfig.h"
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#ifdef WITH_BUILTIN_GRU
#pragma link C++ class KVINDRARawDataAnalyser;
#pragma link C++ class KVINDRARawDataReconstructor;
#endif
#pragma link C++ class KVINDRADstToRootTransfert;
#pragma link C++ class KVINDRAReconDataAnalyser;
#pragma link C++ class KVINDRAGeneDataSelector;
#pragma link C++ class KVINDRAGeneDataAnalyser;
#pragma link C++ class KVOldINDRASelector+;
#pragma link C++ class KVINDRAEventSelector+;
#pragma link C++ class KVDataSelector;
#pragma link C++ class KVINDRAReconIdent+;
#pragma link C++ class KVReconIdent_e475s+;
#pragma link C++ class KVINDRAIdentRoot+;
#pragma link C++ class KVINDRAReconRoot+;
#ifdef WITH_GRULIB
#pragma link C++ class KVINDRAOnlineDataAnalyser+;
#endif
#pragma link C++ class KVDataPatch_INDRA_camp5_PHDcorrection+;
#pragma link C++ class KVDataPatch_CorrectEtalonModuleIDCode+;
#endif
