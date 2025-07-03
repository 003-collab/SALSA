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
#ifndef C_LSADELTA_HPP
#define C_LSADELTA_HPP

#include <LSADelta.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSADelta* new_LSADelta(const char *fromLabel, const char *toLabel, double dx, double dy, double dz, const char *units,
                                     double cxx, double cxy, double cxz, double cyy, double cyz, double czz);
    DLLExport void      delete_LSADelta(LSADelta *lsaDelta);

    // From Height
    DLLExport void setDeltaHeightFromLabel(LSADelta* lsaDelta, const char *label);
    DLLExport void setDeltaHeightFromValue(LSADelta* lsaDelta, double value, const char *units);
    DLLExport void setDeltaHeightFromSigma(LSADelta* lsaDelta, double value, const char *units);

    // To Height
    DLLExport void setDeltaHeightToLabel(LSADelta* lsaDelta, const char *label);
    DLLExport void setDeltaHeightToValue(LSADelta* lsaDelta, double value, const char *units);
    DLLExport void setDeltaHeightToSigma(LSADelta* lsaDelta, double value, const char *units);

    // UNCER and VSCA
    DLLExport void setDeltaUNCRLabel(LSADelta* lsaDelta, const char *label);
    DLLExport void setDeltaVSCALabel(LSADelta* lsaDelta, const char *label);
    DLLExport void setDeltaVSCAValue(LSADelta* lsaDelta, double value);

    // LSAString
    DLLExport void getLSADeltaString(LSADelta* lsaDelta, char * buffer);
    
    // Text Notes
    DLLExport void setConversionNotes(LSADelta* lsaDelta, const char *notes);
}

#endif // C_LSADELTA_HPP
