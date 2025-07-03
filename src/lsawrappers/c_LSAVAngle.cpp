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
#include <c_LSAVAngle.hpp>

LSAVAngle* new_LSAVAngle()
{
    return new LSAVAngle();
}

void delete_LSAVAngle(LSAVAngle *lsaVAngle)
{ 
    delete lsaVAngle;
}

void setVAngleRequiredValues_DecDeg(LSAVAngle *lsaVAngle, const char * from, const char * to, double angle, double sig, const char * units)
{
    lsaVAngle->From        = from;
    lsaVAngle->To          = to;
    lsaVAngle->angleDecDeg = angle;
    lsaVAngle->usesDecDeg  = true;
    lsaVAngle->sigma       = sig;
    lsaVAngle->sigmaUnits  = units;
}

void setVAngleRequiredValues_DMS(LSAVAngle *lsaVAngle, const char * from, const char * to, int degrees, int minutes, double seconds, double sig, const char * units)
{
    lsaVAngle->From       = from;
    lsaVAngle->To         = to;
    lsaVAngle->angleDeg   = degrees;
    lsaVAngle->angleMin   = minutes;
    lsaVAngle->angleSec   = seconds;
    lsaVAngle->usesDecDeg = false;
    lsaVAngle->sigma      = sig;
    lsaVAngle->sigmaUnits = units;
}

void setVAngleHeightFromLabel(LSAVAngle* lsaVAngle, const char *label)
{
    lsaVAngle->setHeightFromLabel(label);
}

void setVAngleHeightFromValue(LSAVAngle* lsaVAngle, double value, const char *units)
{
    lsaVAngle->setHeightFromValue(value, units);
}

void setVAngleHeightFromSigma(LSAVAngle* lsaVAngle, double value, const char *units)
{
    lsaVAngle->setHeightFromSigma(value, units);
}

void setVAngleHeightToLabel(LSAVAngle* lsaVAngle, const char *label)
{
    lsaVAngle->setHeightToLabel(label);
}

void setVAngleHeightToValue(LSAVAngle* lsaVAngle, double value, const char *units)
{
    lsaVAngle->setHeightToValue(value, units);
}

void setVAngleHeightToSigma(LSAVAngle* lsaVAngle, double value, const char *units)
{
    lsaVAngle->setHeightToSigma(value, units);
}

void setVAngleUNCRLabel(LSAVAngle* lsaVAngle, const char *label)
{
    lsaVAngle->setUncrLabel(label);
}

void setVAngleRefraction(LSAVAngle *lsaVAngle, double value)
{
    lsaVAngle->setRefract(value);
}

void setVAngleIsReduced(LSAVAngle* lsaVAngle, bool isReduced)
{
    lsaVAngle->setIsReduced(isReduced);
}

void setVAngleVSCALabel(LSAVAngle* lsaVAngle, const char *label)
{
    lsaVAngle->setVSCALabel(label);
}

void setVAngleVSCAValue(LSAVAngle* lsaVAngle, double value)
{
    lsaVAngle->setVSCAValue(value);
}

void getVAngleString(LSAVAngle* lsaVAngle, char *buffer)
{
    strcpy(buffer,lsaVAngle->getSingleLSAString().c_str());
}
