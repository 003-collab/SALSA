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
#ifndef C_LSAINCLUDE_HPP
#define C_LSAINCLUDE_HPP

#include <LSAInclude.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSAInclude *new_LSAInclude(const char *lsaPath, const char *absolutePath);
    DLLExport void delete_LSAInclude(LSAInclude *lsaInclude);

    // LSAString
    DLLExport void getLSAIncludeString(LSAInclude *lsaInclude, char * buffer);
}

#endif // C_LSAINCLUDE_HPP
