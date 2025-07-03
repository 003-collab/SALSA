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
#include <c_LSAPosG.hpp>

LSAPosG* new_LSAPosG(const char * label)
{
    formatExponent();
    return new LSAPosG(label);
}

void delete_LSAPosG(LSAPosG *lsaPosG)
{ 
    delete lsaPosG;
}

void setPosGRequiredValues_DecDeg(LSAPosG *lsaPosG, double latDecDeg, const char* latDir, double lonDecDeg, const char * lonDir, double height, const char * heightUnits)
{
    lsaPosG->useDecimalDegrees = true;

    lsaPosG->latDecDeg   = latDecDeg;
    lsaPosG->latDir      = latDir;

    lsaPosG->lonDecDeg   = lonDecDeg;
    lsaPosG->lonDir      = lonDir;

    lsaPosG->height      = height;
    lsaPosG->heightUnits = heightUnits;
}

void setPosGRequiredValues_DMS(LSAPosG *lsaPosG, int latDeg, int latMin, double latSec, const char* latDir,
                                                 int lonDeg, int lonMin, double lonSec, const char* lonDir,
                                                 double height, const char * heightUnits)
{     
    lsaPosG->useDecimalDegrees = false;

    lsaPosG->latDeg   = latDeg;
    lsaPosG->latMin   = latMin;
    lsaPosG->latSec   = latSec;
    lsaPosG->latDir   = latDir;

    lsaPosG->lonDeg   = lonDeg;
    lsaPosG->lonMin   = lonMin;
    lsaPosG->lonSec   = lonSec;
    lsaPosG->lonDir   = lonDir;

    lsaPosG->height      = height;
    lsaPosG->heightUnits = heightUnits;
}

void setPosGCovariance(LSAPosG *lsaPosG, double cnn, double cne, double cnu,
                       double cee, double ceu, double cuu)
{
    lsaPosG->setCovariance(cnn, cne, cnu, cee, ceu, cuu);
}

void setPosGVSCALabel(LSAPosG* lsaPosG, const char *label)
{
    lsaPosG->setVSCALabel(label);
}

void setPosGVSCAValue(LSAPosG* lsaPosG, double value)
{
    lsaPosG->setVSCAValue(value);
}

void getPosGString(LSAPosG* lsaPosG, char *buffer)
{
    strcpy(buffer,lsaPosG->getSingleLSAString().c_str());
}
