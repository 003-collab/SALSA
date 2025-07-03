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
#ifndef C_LSAPOSC_HPP
#define C_LSAPOSC_HPP

#include <LSAPosC.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAPosC* new_LSAPosC(const char *label, double x, double y, double z, const char *units);
    DLLExport void     delete_LSAPosC(LSAPosC *LSAPosC);

    // Covariance
    DLLExport void setPosCCovariance(LSAPosC *lsaPosC, double cxx, double cxy, double cxz, double cyy, double cyz, double czz);

    // VSCA
    DLLExport void setPosCVSCALabel(LSAPosC* lsaPosC, const char *label);
    DLLExport void setPosCVSCAValue(LSAPosC* lsaPosC, double value);

    // Fixed state
    DLLExport void setPosCStateFloating(LSAPosC *lsaPosC)    { lsaPosC->fixedState = LSAFixedState::FLOATING; }
    DLLExport void setPosCStateConstrained(LSAPosC *lsaPosC) { lsaPosC->fixedState = LSAFixedState::CONSTRAINED; }
    DLLExport void setPosCStateFixed(LSAPosC *lsaPosC)       { lsaPosC->fixedState = LSAFixedState::FIXED; }
    
    // LSAString
    DLLExport void getLSAPosCString(LSAPosC* LSAPosC, char * buffer);
}

#endif // C_LSAPOSC_HPP
