/***************************************************************************
$Id: KVDBChIoPressures.cpp,v 1.3 2007/02/13 18:18:18 franklan Exp $
                          KVDBChIoPressures.cpp  -  description
                             -------------------
    begin                : mer mai 7 2003
    copyright            : (C) 2003 by Alexis Mignon
    email                : mignon@ganil.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "KVDBChIoPressures.h"
#include "Riostream.h"


using namespace std;

ClassImp(KVDBChIoPressures);
/////////////////////////////////////////////////////////////////////////////////////////////////
//             Class KVDBChIoPressures
// This class handles a set of parameters for ChIo pressures.
// It can read/write in from/to a stream with following format :
// ChIos 2_3    p1
// ChIos 4_5    p2
// ChIos 6_7    p3
// ChIos 8_12   p4
// ChIos 13_17  p5
//
//____________________________________________________________________________
KVDBChIoPressures::KVDBChIoPressures() : fPressures(5, 0.0)
{
   SetPressures(0., 0., 0., 0., 0.);
}

//____________________________________________________________________________
KVDBChIoPressures::KVDBChIoPressures(Float_t p1, Float_t p2, Float_t p3,
                                     Float_t p4, Float_t p5) : fPressures(5, 0.0)
{
   SetPressures(p1, p2, p3, p4, p5);
}

//____________________________________________________________________________
KVDBChIoPressures::KVDBChIoPressures(Float_t pressure[5]) : fPressures(5, 0.0)
{
   SetPressures(pressure);
}

//____________________________________________________________________________
KVDBChIoPressures::KVDBChIoPressures(const KVDBChIoPressures& chiopres) : KVBase(), fPressures(5, 0.0)
{
   fPressures = chiopres.fPressures;
}

//____________________________________________________________________________
KVDBChIoPressures::~KVDBChIoPressures()
{
}

KVDBChIoPressures& KVDBChIoPressures::operator=(const KVDBChIoPressures& other)
{
   if (&other == this) return (*this);
   fPressures = other.fPressures;
   return (*this);
}


//____________________________________________________________________________
void KVDBChIoPressures::Print(Option_t*) const
{
   cout << "_____________________________________________________________"
        << endl << "ChIo Pressures :" << endl << GetName() << " " <<
        GetTitle() << endl << "ChIo 2_3  : " << fPressures[CHIO_2_3] << endl
        << "ChIo 4_5  : " << fPressures[CHIO_4_5] << endl << "ChIo 6_7  : "
        << fPressures[CHIO_6_7] << endl << "ChIo 8_12 : " <<
        fPressures[CHIO_8_12] << endl << "ChIo 13_17: " <<
        fPressures[CHIO_13_17] << endl <<
        "______________________________________________________________" <<
        endl;
}
