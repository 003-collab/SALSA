/*
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
*/
#ifndef C_LSAPOSG_HPP
#define C_LSAPOSG_HPP

#include <LSAPosG.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAPosG* new_LSAPosG(const char *label);
    DLLExport void delete_LSAPosG(LSAPosG *lsaPosG);

    // Required Values
    DLLExport void setPosGRequiredValues_DecDeg(LSAPosG *lsaPosG, double latDecDeg, const char *latDir,
                                                                  double lonDecDeg, const char *lonDir,
                                                                  double height, const char *heightUnits);
    DLLExport void setPosGRequiredValues_DMS(LSAPosG *lsaPosG, int latDeg, int latMin, double latSec, const char* latDir,
                                                               int lonDeg, int lonMin, double lonSec, const char* lonDir,
                                                               double height, const char * heightUnits);

    // Covariance
    DLLExport void setPosGCovariance(LSAPosG *lsaPosG, double cnn, double cne, double cnu, double cee, double ceu, double cuu);


    // Fixed state
    DLLExport void setPosGStateFloating(LSAPosG *lsaPosG)    { lsaPosG->fixedState = LSAFixedState::FLOATING; }
    DLLExport void setPosGStateConstrained(LSAPosG *lsaPosG) { lsaPosG->fixedState = LSAFixedState::CONSTRAINED; }
    DLLExport void setPosGStateFixed(LSAPosG *lsaPosG)       { lsaPosG->fixedState = LSAFixedState::FIXED; }

    // VSCA
    DLLExport void setPosGVSCALabel(LSAPosG* lsaPosG, const char *label);
    DLLExport void setPosGVSCAValue(LSAPosG* lsaPosG, double value);

    // LSAString
    DLLExport void getPosGString(LSAPosG* lsaPosG, char * buffer);
}

#endif // C_LSAPOSG_HPP
