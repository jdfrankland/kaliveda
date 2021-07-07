#ifdef __CINT__
#include "RVersion.h"
#include "KVConfig.h"
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;

#pragma link C++ enum KVMultiDetArray::EFilterType;
#pragma link C++ global gMultiDetArray;
#pragma link C++ class KVDetectionSimulator+;
#pragma link C++ class KVEventReconstructor+;
#pragma link C++ class KVGroupReconstructor+;
#pragma link C++ class KVFilterGroupReconstructor+;
#pragma link C++ class KVFilterEventReconstructor+;
#pragma link C++ class KVRawDataReconstructor+;
#pragma link C++ class KVReconDataAnalyser+;
#pragma link C++ class KVReconRawDataAnalyser+;
#ifdef WITH_DATAFLOW
#ifdef WITH_ZMQ
#pragma link C++ class KVOnlineReconDataAnalyser+;
#endif
#endif
#pragma link C++ class KVReconEventSelector+;
#pragma link C++ class KVReconNucTrajectory+;
#pragma link C++ class KVReconstructedNucleus-;//customised streamer
#pragma link C++ class KVDetectorEvent+;
#pragma link C++ class KVTemplateParticleCondition<KVReconstructedNucleus>+;
#pragma link C++ class KVTemplateEvent<KVReconstructedNucleus>+;
#pragma link C++ class KVTemplateEvent<KVReconstructedNucleus>::Iterator+;
#pragma link C++ class KVTemplateEvent<KVReconstructedNucleus>::EventIterator+;
#pragma link C++ class KVTemplateEvent<KVReconstructedNucleus>::EventOKIterator+;
#pragma link C++ class KVTemplateEvent<KVReconstructedNucleus>::EventGroupIterator+;
#pragma link C++ class KVReconstructedEvent-;//customised streamer
#pragma link C++ class ReconEventIterator+;
#pragma link C++ class ReconEventGroupIterator+;
#pragma link C++ class ReconEventOKIterator+;
#pragma link C++ class KVIDTelescope+;
#pragma link C++ class KVIDCsI+;
#ifdef WITH_FITLTG
#pragma link C++ class KVRTGIDManager+;
#endif
#pragma link C++ class KVUpDater;
#pragma link C++ class KVRawDataAnalyser+;
#pragma link C++ class KVSimDirFilterAnalyser+;
#pragma link C++ class KVExpSetUp+;
#pragma link C++ class KVExpSetUpDB+;
#pragma link C++ class KVMultiDetArray+;
#pragma link C++ class KVASMultiDetArray+;
#pragma link C++ class KVGeoImport+;
#pragma link C++ class KVDataPatch+;
#pragma link C++ class KVDataPatchList+;
#pragma link C++ class KVArrayMult+;
#pragma link C++ class KVDataQualityAudit::isotope+;
#pragma link C++ class std::pair<int,KVDataQualityAudit::isotope>+;
#pragma link C++ class KVDataQualityAudit::element+;
#pragma link C++ class std::pair<int,KVDataQualityAudit::element>+;
#pragma link C++ class KVDataQualityAudit::idtelescope+;
#pragma link C++ class KVDataQualityAudit+;
#pragma link C++ class KVDataQualityAuditReportMaker+;
#pragma link C++ class KVDataQualityAuditReportMaker::telescope;
#pragma link C++ class KVDataQualityAuditSelector+;
#endif
