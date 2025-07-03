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
#ifndef C_LSAHANGLE_HPP
#define C_LSAHANGLE_HPP

#include <LSAHAngle.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAHAngle* new_LSAHAngle();
    DLLExport void delete_LSAHAngle(LSAHAngle *lsaHAngle);

    // Required Values
    DLLExport void setHAngleRequiredValues_DecDeg(LSAHAngle *lsaHAngle, const char * from, const char * to, double angle, double sig, const char * units);
    DLLExport void setHAngleRequiredValues_DMS(LSAHAngle *lsaHAngle, const char * from, const char * to, bool isDMSNeg, int degrees, int minutes, double seconds, double sig, const char * units);

    // From Height
    DLLExport void setHAngleHeightFromLabel(LSAHAngle* lsaHAngle, const char *label);
    DLLExport void setHAngleHeightFromValue(LSAHAngle* lsaHAngle, double value, const char *units);
    DLLExport void setHAngleHeightFromSigma(LSAHAngle* lsaHAngle, double value, const char *units);

    // To Height
    DLLExport void setHAngleHeightToLabel(LSAHAngle* lsaHAngle, const char *label);
    DLLExport void setHAngleHeightToValue(LSAHAngle* lsaHAngle, double value, const char *units);
    DLLExport void setHAngleHeightToSigma(LSAHAngle* lsaHAngle, double value, const char *units);

    // UNCER
    DLLExport void setHAngleUNCRLabel(LSAHAngle* lsaHAngle, const char *label);

    // REFRACT
    DLLExport void setHAngleRefraction(LSAHAngle *lsaHAngle, double value);

    // REDUCED
    DLLExport void setHAngleIsReduced(LSAHAngle* lsaHAngle, bool isReduced);

    // VSCA
    DLLExport void setHAngleVSCALabel(LSAHAngle* lsaHAngle, const char *label);
    DLLExport void setHAngleVSCAValue(LSAHAngle* lsaHAngle, double value);

    // LSAString
    DLLExport void getHAngleString(LSAHAngle* lsaHAngle, char * buffer);
}

#endif // C_LSAHANGLE_HPP
