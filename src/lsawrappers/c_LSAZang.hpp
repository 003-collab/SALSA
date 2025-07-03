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
#ifndef C_LSAZANG_HPP
#define C_LSAZANG_HPP

#include <LSAZAngle.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAZAngle* new_LSAZAngle();
    DLLExport void delete_LSAZangle(LSAZAngle *lsaZangle);

    // Required Values
    DLLExport void setZangRequiredValues_DecDeg(LSAZAngle *lsaZAngle, const char * from, const char * to, double angle, double sig, const char * units);
    DLLExport void setZangRequiredValues_DMS(LSAZAngle *lsaZAngle, const char * from, const char * to, int degrees, int minutes, double seconds, double sig, const char * units);

    // From Height
    DLLExport void setZangHeightFromLabel(LSAZAngle* lsaZAngle, const char *label);
    DLLExport void setZangHeightFromValue(LSAZAngle* lsaZAngle, double value, const char *units);
    DLLExport void setZangHeightFromSigma(LSAZAngle* lsaZAngle, double value, const char *units);

    // To Height
    DLLExport void setZangHeightToLabel(LSAZAngle* lsaZAngle, const char *label);
    DLLExport void setZangHeightToValue(LSAZAngle* lsaZAngle, double value, const char *units);
    DLLExport void setZangHeightToSigma(LSAZAngle* lsaZAngle, double value, const char *units);

    // UNCER
    DLLExport void setZangUNCRLabel(LSAZAngle* lsaZAngle, const char *label);

    // REFRACT
    DLLExport void setZangRefraction(LSAZAngle *lsaZAngle, double value);

    // REDUCED
    DLLExport void setZangIsReduced(LSAZAngle* lsaZAngle, bool isReduced);

    // VSCA
    DLLExport void setZangVSCALabel(LSAZAngle* lsaZAngle, const char *label);
    DLLExport void setZangVSCAValue(LSAZAngle* lsaZAngle, double value);

    // LSAString
    DLLExport void getZangString(LSAZAngle* lsaZAngle, char * buffer);
}

#endif // C_LSAZANG_HPP
