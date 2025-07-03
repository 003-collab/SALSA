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
#ifndef C_LSADIST_HPP
#define C_LSADIST_HPP

#include <LSADist.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSADist* new_LSADist(const char *fromLabel, const char *toLabel, double distance, double sigma, const char *linUnits);
    DLLExport void     delete_LSADist(LSADist *lsaDist);

    // From Height
    DLLExport void setDistHeightFromLabel(LSADist* lsaDist, const char *label);
    DLLExport void setDistHeightFromValue(LSADist* lsaDist, double value, const char *units);
    DLLExport void setDistHeightFromSigma(LSADist* lsaDist, double value, const char *units);

    // To Height
    DLLExport void setDistHeightToLabel(LSADist* lsaDist, const char *label);
    DLLExport void setDistHeightToValue(LSADist* lsaDist, double value, const char *units);
    DLLExport void setDistHeightToSigma(LSADist* lsaDist, double value, const char *units);

    // UNCER and VSCA
    DLLExport void setDistUNCRLabel(LSADist* lsaDist, const char *label);
    DLLExport void setDistVSCALabel(LSADist* lsaDist, const char *label);
    DLLExport void setDistVSCAValue(LSADist* lsaDist, double value);

    // LSAString
    DLLExport void getLSADistString(LSADist* lsaDist, char * buffer);
}

#endif // C_LSADIST_HPP
