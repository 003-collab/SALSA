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
#include <c_LSAZang.hpp>

LSAZAngle* new_LSAZAngle()
{
    return new LSAZAngle();
}

void delete_LSAZangle(LSAZAngle *lsaZangle)
{ 
    delete lsaZangle;
}

void setZangRequiredValues_DecDeg(LSAZAngle *lsaZAngle, const char * from, const char * to, double angle, double sig, const char * units)
{
    lsaZAngle->From        = from;
    lsaZAngle->To          = to;
    lsaZAngle->angleDecDeg = angle;
    lsaZAngle->usesDecDeg  = true;
    lsaZAngle->sigma       = sig;
    lsaZAngle->sigmaUnits  = units;
}

void setZangRequiredValues_DMS(LSAZAngle *lsaZAngle, const char * from, const char * to, int degrees, int minutes, double seconds, double sig, const char * units)
{
    lsaZAngle->From       = from;
    lsaZAngle->To         = to;
    lsaZAngle->angleDeg   = degrees;
    lsaZAngle->angleMin   = minutes;
    lsaZAngle->angleSec   = seconds;
    lsaZAngle->usesDecDeg = false;
    lsaZAngle->sigma      = sig;
    lsaZAngle->sigmaUnits = units;
}

void setZangHeightFromLabel(LSAZAngle* lsaZAngle, const char *label)
{
    lsaZAngle->setHeightFromLabel(label);
}

void setZangHeightFromValue(LSAZAngle* lsaZAngle, double value, const char *units)
{
    lsaZAngle->setHeightFromValue(value, units);
}

void setZangHeightFromSigma(LSAZAngle* lsaZAngle, double value, const char *units)
{
    lsaZAngle->setHeightFromSigma(value, units);
}

void setZangHeightToLabel(LSAZAngle* lsaZAngle, const char *label)
{
    lsaZAngle->setHeightToLabel(label);
}

void setZangHeightToValue(LSAZAngle* lsaZAngle, double value, const char *units)
{
    lsaZAngle->setHeightToValue(value, units);
}

void setZangHeightToSigma(LSAZAngle* lsaZAngle, double value, const char *units)
{
    lsaZAngle->setHeightToSigma(value, units);
}

void setZangUNCRLabel(LSAZAngle* lsaZAngle, const char *label)
{
    lsaZAngle->setUncrLabel(label);
}

void setZangRefraction(LSAZAngle *lsaZAngle, double value)
{
    lsaZAngle->setRefract(value);
}

void setZangIsReduced(LSAZAngle* lsaZAngle, bool isReduced)
{
    lsaZAngle->setIsReduced(isReduced);
}

void setZangVSCALabel(LSAZAngle* lsaZAngle, const char *label)
{
    lsaZAngle->setVSCALabel(label);
}

void setZangVSCAValue(LSAZAngle* lsaZAngle, double value)
{
    lsaZAngle->setVSCAValue(value);
}

void getZangString(LSAZAngle* lsaZAngle, char *buffer)
{
    strcpy(buffer,lsaZAngle->getSingleLSAString().c_str());
}
