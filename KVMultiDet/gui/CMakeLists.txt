BUILD_KALIVEDA_MODULE(gui
	PARENT ${KVSUBPROJECT}
   KVMOD_DEPENDS simulation analysis exp_events geometry data_management db stopping events base identification particles globvars
   EXTRA_LIBS ${ROOT_Spectrum_LIBRARY}
   LIB_EXCLUDE KVDataBaseBrowser KVDBSystemDialog
)
