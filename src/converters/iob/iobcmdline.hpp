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
/// @file iobcmdline.hpp  Include file for iobcmdline.cpp, a variation on
///                       lsacmdline.cpp

#ifndef LSA_IOBCMDLINE_INCLUDE
#define LSA_IOBCMDLINE_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


// prototypes
int GetCommandLine(int argc, char **argv);
int ValidateInput(void);
int PreProcess(void);

#endif   // LSA_IOBCMDLINE_INCLUDE

