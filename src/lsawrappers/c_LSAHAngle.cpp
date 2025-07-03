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
#include <c_LSAHAngle.hpp>

LSAHAngle* new_LSAHAngle()
{
    return new LSAHAngle();
}

void delete_LSAHAngle(LSAHAngle *lsaHAngle)
{ 
    delete lsaHAngle;
}

void setHAngleRequiredValues_DecDeg(LSAHAngle *lsaHAngle, const char * from, const char * to, double angle, double sig, const char * units)
{
    lsaHAngle->From        = from;
    lsaHAngle->To          = to;
    lsaHAngle->angleDecDeg = angle;
    lsaHAngle->usesDecDeg  = true;
    lsaHAngle->sigma       = sig;
    lsaHAngle->sigmaUnits  = units;
}

void setHAngleRequiredValues_DMS(LSAHAngle *lsaHAngle, const char * from, const char * to, bool isDMSNeg, int degrees, int minutes, double seconds, double sig, const char * units)
{
    lsaHAngle->From       = from;
    lsaHAngle->To         = to;
    lsaHAngle->angleDMSIsNeg = isDMSNeg;
    lsaHAngle->angleDeg   = degrees;
    lsaHAngle->angleMin   = minutes;
    lsaHAngle->angleSec   = seconds;
    lsaHAngle->usesDecDeg = false;
    lsaHAngle->sigma      = sig;
    lsaHAngle->sigmaUnits = units;
}

void setHAngleHeightFromLabel(LSAHAngle* lsaHAngle, const char *label)
{
    lsaHAngle->setHeightFromLabel(label);
}

void setHAngleHeightFromValue(LSAHAngle* lsaHAngle, double value, const char *units)
{
    lsaHAngle->setHeightFromValue(value, units);
}

void setHAngleHeightFromSigma(LSAHAngle* lsaHAngle, double value, const char *units)
{
    lsaHAngle->setHeightFromSigma(value, units);
}

void setHAngleHeightToLabel(LSAHAngle* lsaHAngle, const char *label)
{
    lsaHAngle->setHeightToLabel(label);
}

void setHAngleHeightToValue(LSAHAngle* lsaHAngle, double value, const char *units)
{
    lsaHAngle->setHeightToValue(value, units);
}

void setHAngleHeightToSigma(LSAHAngle* lsaHAngle, double value, const char *units)
{
    lsaHAngle->setHeightToSigma(value, units);
}

void setHAngleUNCRLabel(LSAHAngle* lsaHAngle, const char *label)
{
    lsaHAngle->setUncrLabel(label);
}

void setHAngleIsReduced(LSAHAngle* lsaHAngle, bool isReduced)
{
    lsaHAngle->setIsReduced(isReduced);
}

void setHAngleVSCALabel(LSAHAngle* lsaHAngle, const char *label)
{
    lsaHAngle->setVSCALabel(label);
}

void setHAngleVSCAValue(LSAHAngle* lsaHAngle, double value)
{
    lsaHAngle->setVSCAValue(value);
}

void getHAngleString(LSAHAngle* lsaHAngle, char *buffer)
{
    strcpy(buffer,lsaHAngle->getSingleLSAString().c_str());
}
