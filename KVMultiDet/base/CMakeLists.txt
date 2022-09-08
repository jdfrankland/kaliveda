set(extra_libs ${ROOT_Gui_LIBRARY} ${ROOT_Geom_LIBRARY} ${GRU_LIB})
#---Need ROOT with SQLite support for SQLiteDB
if(WITH_RSQLITE)
   set(extra_libs ${extra_libs} ${ROOT_RSQLite_LIBRARY})
   set(lib_exclude)
else()
   set(lib_exclude SQLiteDB KVSQLROOTFile)
endif()
set(dict_exclude KVError.h)

BUILD_KALIVEDA_MODULE(base
	PARENT ${KVSUBPROJECT}
        LIB_EXCLUDE ${lib_exclude}
        DICT_EXCLUDE ${dict_exclude}
        EXTRA_LIBS ${extra_libs}
)

#--- generation of KVGitInfo.h
include(CreateVersionControlHeader)
create_version_control_header()

#--- declare dependency of library (in fact, KVBase.o) on KV***Info.h
add_dependencies(KVMultiDetbase vcsinfos)
