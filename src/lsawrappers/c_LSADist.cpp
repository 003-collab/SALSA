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
#include <c_LSADist.hpp>

LSADist* new_LSADist(const char *fromLabel, const char *toLabel, double distance, double sigma, const char *linUnits)
{
    return new LSADist(fromLabel, toLabel, distance, sigma, linUnits);
}

void delete_LSADist(LSADist *lsaDist) 
{ 
	delete lsaDist; 
}

void setDistHeightFromLabel(LSADist* lsaDist, const char *label)
{
    lsaDist->setHeightFromLabel(label);
}

void setDistHeightFromValue(LSADist* lsaDist, double value, const char *units)
{
    lsaDist->setHeightFromValue(value, units);
}

void setDistHeightFromSigma(LSADist* lsaDist, double value, const char *units)
{
    lsaDist->setHeightFromSigma(value, units);
}

void setDistHeightToLabel(LSADist* lsaDist, const char *label)
{
    lsaDist->setHeightToLabel(label);
}

void setDistHeightToValue(LSADist* lsaDist, double value, const char *units)
{
    lsaDist->setHeightToValue(value, units);
}

void setDistHeightToSigma(LSADist* lsaDist, double value, const char *units)
{
    lsaDist->setHeightToSigma(value, units);
}

void setDistUNCRLabel(LSADist* lsaDist, const char *label)
{
    lsaDist->setUncrLabel(label);
}

void setDistVSCALabel(LSADist* lsaDist, const char *label)
{
    lsaDist->setVSCALabel(label);
}

void setDistVSCAValue(LSADist* lsaDist, double value)
{
    lsaDist->setVSCAValue(value);
}

void getLSADistString(LSADist* lsaDist, char *buffer)
{
	strcpy(buffer,lsaDist->getSingleLSAString().c_str()); 
}
