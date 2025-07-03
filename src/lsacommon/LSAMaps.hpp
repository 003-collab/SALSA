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
#ifndef LSAMAPS_INCLUDE
#define LSAMAPS_INCLUDE

#include <map>
#include <LSAHeight.hpp>
#include "LSAUncertainty.hpp"
#include "LSAVarianceScaling.hpp"

class LSAHeight;
class LSAUncertainty;
class LSAVarScaling;

typedef std::map<std::string, LSAHeight*>      LSAHGHTMap;
typedef std::map<std::string, LSAUncertainty*> LSAUNCRMap;
typedef std::map<std::string, LSAVarScaling*>  LSAVSCAMap;

#endif // LSAMAPS_INCLUDE
