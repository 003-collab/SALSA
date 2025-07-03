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
#ifndef C_LSAHEIGHTDIFF_HPP
#define C_LSAHEIGHTDIFF_HPP

#include <LSAHeightDiff.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAHeightDiff* new_LSAHeightDiff(const char *fromLabel, const char *toLabel, double hDif, double sigma, const char *linUnits);
    DLLExport void     delete_LSAHeightDiff(LSAHeightDiff *lsaHeightDiff);

    // UNCER
    DLLExport void setHDifUNCRLabel(LSAHeightDiff *lsaHeightDiff, const char *label);

    // REFRACT
    DLLExport void setHDifRefraction(LSAHeightDiff *lsaHeightDiff, double value);

    // REDUCED
    DLLExport void setHDifIsReduced(LSAHeightDiff *lsaHeightDiff, bool isReduced);

    // VSCA
    DLLExport void setHDifVSCALabel(LSAHeightDiff *lsaHeightDiff, const char *label);
    DLLExport void setHDifVSCAValue(LSAHeightDiff *lsaHeightDiff, double value);

    //CURV
    DLLExport void setHDifCurvCorr(LSAHeightDiff *lsaHeightDiff, bool curvCorr);

    // LSAString
    DLLExport void getHDifString(LSAHeightDiff *lsaHeightDiff, char * buffer);
}

#endif // C_LSAHEIGHTDIFF_HPP
