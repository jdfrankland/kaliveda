if(NOT WITH_FITLTG)
	set(do_not_compile KVTGIDFitter KVTGIDFunctions KVTGIDGrid KVTGID KVTGIDManager KVTGIDZA KVTGIDZ)
else()
	include_directories(${FitLTG_SOURCE_DIR}/src)
endif()
#---Need ROOT with SQLite support for SQLiteDB
if(NOT WITH_RSQLITE)
   set(do_not_compile ${do_not_compile} KVSQLROOTIDGridManager)
endif()

BUILD_KALIVEDA_MODULE(identification
	PARENT ${KVSUBPROJECT}
	KVMOD_DEPENDS particles base
	LIB_EXCLUDE ${do_not_compile}
	EXTRA_LIBS ${FITLTG_LIB}
)
