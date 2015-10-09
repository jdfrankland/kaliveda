set(do_not_build KVRawDataAnalyser KVGANILDataReader KVGRUNetClientGanilReader KVGRUTapeGanilReader KVRTGIDManager)
if(WITH_GRULIB)
	list(REMOVE_ITEM do_not_build KVGRUTapeGanilReader)
endif()
if(WITH_BUILTIN_GRU)
	list(REMOVE_ITEM do_not_build KVRawDataAnalyser KVGANILDataReader)
endif()
if(WITH_FITLTG)
	list(REMOVE_ITEM do_not_build KVRTGIDManager)
endif()

BUILD_KALIVEDA_MODULE(exp_events
	PARENT ${KVSUBPROJECT}
	KVMOD_DEPENDS analysis globvars daq_cec identification base particles geometry stopping events data_management db
   LIB_EXCLUDE ${do_not_build} KVASMultiDetArray KVElasticScatter KVElasticScatterEvent
)
