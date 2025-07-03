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
#include <c_LSAAzim.hpp>

LSAAzimuth* new_LSAAzimuth()
{
    return new LSAAzimuth();
}

void delete_LSAAzimuth(LSAAzimuth *lsaAzimuth)
{ 
    delete lsaAzimuth;
}

void setAzimRequiredValues_DecDeg(LSAAzimuth *lsaAzimuth, const char * from, const char * to, double angle, double sig, const char * units)
{
    lsaAzimuth->From        = from;
    lsaAzimuth->To          = to;
    lsaAzimuth->angleDecDeg = angle;
    lsaAzimuth->usesDecDeg  = true;
    lsaAzimuth->sigma       = sig;
    lsaAzimuth->sigmaUnits  = units;
}

void setAzimRequiredValues_DMS(LSAAzimuth *lsaAzimuth, const char * from, const char * to, int degrees, int minutes, double seconds, double sig, const char * units)
{
    lsaAzimuth->From       = from;
    lsaAzimuth->To         = to;
    lsaAzimuth->angleDeg   = degrees;
    lsaAzimuth->angleMin   = minutes;
    lsaAzimuth->angleSec   = seconds;
    lsaAzimuth->usesDecDeg = false;
    lsaAzimuth->sigma      = sig;
    lsaAzimuth->sigmaUnits = units;
}

void setAzimHeightFromLabel(LSAAzimuth* lsaAzimuth, const char *label)
{
    lsaAzimuth->setHeightFromLabel(label);
}

void setAzimHeightFromValue(LSAAzimuth* lsaAzimuth, double value, const char *units)
{
    lsaAzimuth->setHeightFromValue(value, units);
}

void setAzimHeightFromSigma(LSAAzimuth* lsaAzimuth, double value, const char *units)
{
    lsaAzimuth->setHeightFromSigma(value, units);
}

void setAzimHeightToLabel(LSAAzimuth* lsaAzimuth, const char *label)
{
    lsaAzimuth->setHeightToLabel(label);
}

void setAzimHeightToValue(LSAAzimuth* lsaAzimuth, double value, const char *units)
{
    lsaAzimuth->setHeightToValue(value, units);
}

void setAzimHeightToSigma(LSAAzimuth* lsaAzimuth, double value, const char *units)
{
    lsaAzimuth->setHeightToSigma(value, units);
}

void setAzimUNCRLabel(LSAAzimuth* lsaAzimuth, const char *label)
{
    lsaAzimuth->setUncrLabel(label);
}

void setAzimIsReduced(LSAAzimuth* lsaAzimuth, bool isReduced)
{
    lsaAzimuth->setIsReduced(isReduced);
}

void setAzimVSCALabel(LSAAzimuth* lsaAzimuth, const char *label)
{
    lsaAzimuth->setVSCALabel(label);
}

void setAzimVSCAValue(LSAAzimuth* lsaAzimuth, double value)
{
    lsaAzimuth->setVSCAValue(value);
}

void getAzimString(LSAAzimuth* lsaAzimuth, char *buffer)
{
    strcpy(buffer,lsaAzimuth->getSingleLSAString().c_str());
}
