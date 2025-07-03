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
#ifndef C_LSAHDIR_HPP
#define C_LSAHDIR_HPP

#include <LSAHDir.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAHDir* new_LSAHDir();
    DLLExport void delete_LSAHDir(LSAHDir *lsaHDir);

    // Required Values
    DLLExport void setHDirRequiredValues_DecDeg(LSAHDir *lsaHDir, const char * dgrpLabel, const char * toStation, double angle, double sig, const char * units);
    DLLExport void setHDirRequiredValues_DMS(LSAHDir *lsaHDir, const char * dgrpLabel, const char * toStation, int degrees, int minutes, double seconds, double sig, const char * units);

    DLLExport void setHDirHeightToLabel(LSAHDir* lsaHDir, const char *label);
    DLLExport void setHDirHeightToValue(LSAHDir* lsaHDir, double value, const char *units);
    DLLExport void setHDirHeightToSigma(LSAHDir* lsaHDir, double value, const char *units);

    // LSAString
    DLLExport void getHDirString(LSAHDir* lsaHDir, char * buffer);
}

#endif // C_LSAHDIR_HPP
