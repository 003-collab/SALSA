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
#ifndef C_LSAAZIM_HPP
#define C_LSAAZIM_HPP

#include <LSAAzimuth.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAAzimuth* new_LSAAzimuth();
    DLLExport void delete_LSAAzimuth(LSAAzimuth *lsaAzimuth);

    // Required Values
    DLLExport void setAzimRequiredValues_DecDeg(LSAAzimuth *lsaAzimuth, const char * from, const char * to, double angle, double sig, const char * units);
    DLLExport void setAzimRequiredValues_DMS(LSAAzimuth *lsaAzimuth, const char * from, const char * to, int degrees, int minutes, double seconds, double sig, const char * units);

    // From Height
    DLLExport void setAzimHeightFromLabel(LSAAzimuth* lsaAzimuth, const char *label);
    DLLExport void setAzimHeightFromValue(LSAAzimuth* lsaAzimuth, double value, const char *units);
    DLLExport void setAzimHeightFromSigma(LSAAzimuth* lsaAzimuth, double value, const char *units);

    // To Height
    DLLExport void setAzimHeightToLabel(LSAAzimuth* lsaAzimuth, const char *label);
    DLLExport void setAzimHeightToValue(LSAAzimuth* lsaAzimuth, double value, const char *units);
    DLLExport void setAzimHeightToSigma(LSAAzimuth* lsaAzimuth, double value, const char *units);

    // UNCER
    DLLExport void setAzimUNCRLabel(LSAAzimuth* lsaAzimuth, const char *label);

    // REFRACT
    DLLExport void setAzimRefraction(LSAAzimuth *lsaAzimuth, double value);

    // REDUCED
    DLLExport void setAzimIsReduced(LSAAzimuth* lsaAzimuth, bool isReduced);

    // VSCA
    DLLExport void setAzimVSCALabel(LSAAzimuth* lsaAzimuth, const char *label);
    DLLExport void setAzimVSCAValue(LSAAzimuth* lsaAzimuth, double value);

    // LSAString
    DLLExport void getAzimString(LSAAzimuth* lsaAzimuth, char * buffer);
}

#endif // C_LSAAZIM_HPP
