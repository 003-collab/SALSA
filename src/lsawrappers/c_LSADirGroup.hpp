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
#ifndef C_LSADIRGROUP_HPP
#define C_LSADIRGROOUP_HPP

#include <LSADirGroup.hpp>

#ifdef _WIN32
    #define DLLExport __declspec( dllexport )
#else
    #define DLLExport
#endif

// Python wrappers

extern "C"
{
    // Constructor Destructor
    DLLExport LSADirGroup* new_LSADirGroup(const char *label, const char *atStation);
    DLLExport void         delete_LSADirGroup(LSADirGroup *lsaDirGroup);

    // UNCER
    DLLExport void setDirGroupUNCRLabel(LSADirGroup* lsaDirGroup, const char *label);

    // REDUCED
    DLLExport void setDirGroupIsReduced(LSADirGroup* lsaDirGroup, bool isReduced);

    // VSCA
    DLLExport void setDirGroupVSCALabel(LSADirGroup* lsaDirGroup, const char *label);
    DLLExport void setDirGroupVSCAValue(LSADirGroup* lsaDirGroup, double value);

    // LSAString
    DLLExport void getLSADirGroupString(LSADirGroup* lsaDirGroup, char * buffer);
}

#endif // C_LSADIRGROUP_HPP
