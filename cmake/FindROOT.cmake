# - Finds ROOT instalation
# This module sets up ROOT information
# It defines:
# ROOT_FOUND          If the ROOT is found
# ROOT_INCLUDE_DIR    PATH to the include directory
# ROOT_INCLUDE_DIRS   PATH to the include directories (not cached)
# ROOT_LIBRARIES      Most common libraries
# ROOT_<name>_LIBRARY Full path to the library <name>
# ROOT_LIBRARY_DIR    PATH to the library directory
#
# Updated by K. Smith (ksmith37@nd.edu) to properly handle
#  dependencies in ROOT_GENERATE_DICTIONARY

include(FindPackageMessage)
find_package_message(ROOT "Using KaliVeda-provided FindROOT.cmake" "[${CMAKE_CURRENT_SOURCE_DIR}]")

find_program(ROOT_CONFIG_EXECUTABLE root-config
  PATHS $ENV{ROOTSYS}/bin)

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --prefix
    OUTPUT_VARIABLE ROOTSYS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --version
    OUTPUT_VARIABLE ROOT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --incdir
    OUTPUT_VARIABLE ROOT_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(ROOT_INCLUDE_DIRS ${ROOT_INCLUDE_DIR})

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE ROOT_LIBRARY_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(ROOT_LIBRARY_DIRS ${ROOT_LIBRARY_DIR})

execute_process(
    COMMAND ${ROOT_CONFIG_EXECUTABLE} --bindir
    OUTPUT_VARIABLE ROOT_BINARY_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

set(ROOT_LIBRARIES)
foreach(_cpt Core Cint ${ROOT_FIND_COMPONENTS})
  find_library(ROOT_${_cpt}_LIBRARY ${_cpt} HINTS ${ROOT_LIBRARY_DIR})
  if(ROOT_${_cpt}_LIBRARY)
    mark_as_advanced(ROOT_${_cpt}_LIBRARY)
    list(APPEND ROOT_LIBRARIES ${ROOT_${_cpt}_LIBRARY})
    list(REMOVE_ITEM ROOT_FIND_COMPONENTS ${_cpt})
  endif()
endforeach()
list(REMOVE_DUPLICATES ROOT_LIBRARIES)

#----------------------------------------------------------------------------
# Locate the tools
set(ROOT_ALL_TOOLS genreflex genmap root rootcint)
foreach(_cpt ${ROOT_ALL_TOOLS})
  if(NOT ROOT_${_cpt}_CMD)
    find_program(ROOT_${_cpt}_CMD ${_cpt} HINTS ${ROOT_BINARY_DIR})
    if(ROOT_${_cpt}_CMD)
      mark_as_advanced(ROOT_${_cpt}_CMD)
      list(REMOVE_ITEM ROOT_FIND_COMPONENTS ${_cpt})
    endif()
  endif()
endforeach()

set(ROOT_DEFINITIONS ${ROOT_CXX_FLAGS})
separate_arguments(ROOT_DEFINITIONS)
set(ROOT_EXECUTABLE ${ROOT_root_CMD})

#--set all ROOT flags corresponding to enabled features
execute_process(
  COMMAND ${ROOT_CONFIG_EXECUTABLE} --features
  OUTPUT_VARIABLE _root_options
  OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(_root_options)
foreach(_opt ${_root_options})
  set(ROOT_${_opt}_FOUND TRUE)
  set(${_opt} TRUE)
endforeach()
#--unset 'soversion' if enabled - it will cause problems
set(soversion FALSE)

#---set ROOT module include path depending on major version
if(${ROOT_VERSION} VERSION_LESS 6)
   message(FATAL_ERROR "ROOT6 (at least v6.02) required [${ROOT_VERSION} found]")
#	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/root5)
#	set(ROOT_USE_FILE ${PROJECT_SOURCE_DIR}/cmake/root5/ROOTUseFile.cmake)
else()
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/root6)
#	set(ROOT_USE_FILE ${PROJECT_SOURCE_DIR}/cmake/root6/ROOTUseFile.cmake)
   #---for ROOT6 option 'cxx11' should always be set, but sometimes it is not
   #   in the root-config --features list (???)
   set(ROOT_cxx11_FOUND TRUE)
   set(cxx11 TRUE)
endif()
set(ROOT_USE_FILE ${PROJECT_SOURCE_DIR}/cmake/ROOTUseFile.cmake)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	ROOT
	REQUIRED_VARS ROOTSYS ROOT_CONFIG_EXECUTABLE ROOT_VERSION ROOT_INCLUDE_DIR ROOT_LIBRARIES ROOT_LIBRARY_DIR
	ROOT_BINARY_DIR
	VERSION_VAR ROOT_VERSION
	HANDLE_COMPONENTS
   FAIL_MESSAGE "You need ROOT6 (at least v6.02) in order to build this version of KaliVeda"
)

mark_as_advanced(ROOT_CONFIG_EXECUTABLE)
