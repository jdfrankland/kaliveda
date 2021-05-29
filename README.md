# KaliVeda Data Analysis Toolkit {#index}

KaliVeda is an object-oriented toolkit based on ROOT for the analysis of heavy-ion collisions in the Fermi energy domain.

## Build & Install

See http://indra.in2p3.fr/kaliveda

## Use in ROOT interactive session

The 'kaliveda' command launches a ROOT session with dynamic shared library paths set up so that all classes will be loaded as & when needed by the ROOT interpreter. Example of use:

    $ kaliveda
    
    /----------------------------------------------------------------------\
    | Welcome to KaliVeda 1.13/00             github:kaliveda-dev/kaliveda |
    | (c) 2002-2022, The KaliVeda development team                         |
    |                                                                      |
    | Built with ROOT 6.26.02 on 2022-04-20, 13:33:46                      |
    | From heads/dev@release-1.12.05-208-g7d6bd9a2                         |
    | See http://indra.in2p3.fr/kaliveda for help                          |
    \----------------------------------------------------------------------/
 
    kaliveda [0] 

## Compiling & linking with KaliVeda & ROOT libraries

    $ g++ `kaliveda-config --cflags` -c MyCode.cxx
    $ g++ MyCode.o `kaliveda-config --linklibs` 

## Use in CMake-based project

Given a C++ file using KaliVeda classes such as toto.cpp:

    #include "KVBase.h"
    int main()
    {
       KVBase::InitEnvironment();
       return 0;
    }

You can compile and link this executable with the following CMakeLists.txt file:

    cmake_minimum_required(VERSION 3.9)
    project(toto)
    find_package(KaliVeda REQUIRED)
    include(${KALIVEDA_USE_FILE})
    find_package(ROOT REQUIRED)
    include(SetUpROOTBuild)
    add_executable(toto toto.cpp)
    target_link_libraries(toto ${KALIVEDA_LIBRARIES})

Build the executable 'toto' by doing:

    $ mkdir build && cd build
    $ cmake ..
    $ make

See the wiki page https://github.com/kaliveda-dev/kaliveda/wiki/Using-KaliVeda-in-a-CMake-based-project for more detailed information.

## More information

See the website http://indra.in2p3.fr/kaliveda


