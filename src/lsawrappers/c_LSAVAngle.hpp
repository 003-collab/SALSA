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
#ifndef C_LSAVANGLE_HPP
#define C_LSAVANGLE_HPP

#include <LSAVAngle.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAVAngle* new_LSAVAngle();
    DLLExport void delete_LSAVAngle(LSAVAngle *lsaVAngle);

    // Required Values
    DLLExport void setVAngleRequiredValues_DecDeg(LSAVAngle *lsaVAngle, const char * from, const char * to, double angle, double sig, const char * units);
    DLLExport void setVAngleRequiredValues_DMS(LSAVAngle *lsaVAngle, const char * from, const char * to, int degrees, int minutes, double seconds, double sig, const char * units);

    // From Height
    DLLExport void setVAngleHeightFromLabel(LSAVAngle* lsaVAngle, const char *label);
    DLLExport void setVAngleHeightFromValue(LSAVAngle* lsaVAngle, double value, const char *units);
    DLLExport void setVAngleHeightFromSigma(LSAVAngle* lsaVAngle, double value, const char *units);


    // To Height
    DLLExport void setVAngleHeightToLabel(LSAVAngle* lsaVAngle, const char *label);
    DLLExport void setVAngleHeightToValue(LSAVAngle* lsaVAngle, double value, const char *units);
    DLLExport void setVAngleHeightToSigma(LSAVAngle* lsaVAngle, double value, const char *units);


    // UNCER
    DLLExport void setVAngleUNCRLabel(LSAVAngle* lsaVAngle, const char *label);

    // REFRACT
    DLLExport void setVAngleRefraction(LSAVAngle *lsaVAngle, double value);

    // REDUCED
    DLLExport void setVAngleIsReduced(LSAVAngle* lsaVAngle, bool isReduced);

    // VSCA
    DLLExport void setVAngleVSCALabel(LSAVAngle* lsaVAngle, const char *label);
    DLLExport void setVAngleVSCAValue(LSAVAngle* lsaVAngle, double value);

    // LSAString
    DLLExport void getVAngleString(LSAVAngle* lsaVAngle, char * buffer);
}

#endif // C_LSAVANGLE_HPP
