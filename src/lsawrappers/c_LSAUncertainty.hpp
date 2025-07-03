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
#ifndef C_LSAUNCR_HPP
#define C_LSAUNCR_HPP

#include <LSAUncertainty.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAUncertainty* new_LSAUncr(const char *label);
    DLLExport void            delete_LSAUncr(LSAUncertainty *lsaUncertainty);

    // Label
    DLLExport void setUncrLabel(LSAUncertainty* lsaUncertainty, const char *label);

    // Sigma
    DLLExport void setUncrSigma(LSAUncertainty* lsaUncertainty, double value, const char *sunits);

    // PPM
    DLLExport void setUncrPPM(LSAUncertainty* lsaUncertainty, double value);

    // AT_CE
    DLLExport void setUncrAtCenterError(LSAUncertainty* lsaUncertainty, double value, const char *lunits);

    // FROM_CE
    DLLExport void setUncrFromCenterError(LSAUncertainty* lsaUncertainty, double value, const char *lunits);

    // TO_CE
    DLLExport void setUncrToCenterError(LSAUncertainty* lsaUncertainty, double value, const char *lunits);

    // LSAString
    DLLExport void getLSAUncrString(LSAUncertainty* lsaUncertainty, char * buffer);
}

#endif // C_LSAHGHT_HPP
