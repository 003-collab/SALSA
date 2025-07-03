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
#ifndef LSAOUTPUTPARSER_HPP
#define LSAOUTPUTPARSER_HPP



#include <vector>
#include "StringUtils.hpp"


#include <iostream>
#include <string>
#include <sstream>
#include <fstream>


struct FinalAdjustedPosition
{
    std::string position;

    double      latDecDeg;
    std::string latDir;
    double      lonDecDeg;
    std::string lonDir;
    double      height;
    std::string heightUnits;

    double deltaN;
    double deltaE;
    double deltaU;

    double covNN;
    double covNE;
    double covNU;
    double covEE;
    double covEU;
    double covUU;

    /// These are reliability rectangle values for mapping/drawing, not table display
    double maj2Drr;
    double min2Drr;
    double azrr;
    double vertrr;
};

#endif // LSAOUTPUTPARSER_HPP
