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
#include <c_LSAHDir.hpp>
#include <string.h>

LSAHDir* new_LSAHDir()
{
    return new LSAHDir();
}

void delete_LSAHDir(LSAHDir *lsaHDir)
{ 
    delete lsaHDir;
}

void setHDirRequiredValues_DecDeg(LSAHDir *lsaHDir, const char *dgrpLabel, const char *toStation, double angle, double sig, const char *units)
{
    lsaHDir->dirGroupLabel = dgrpLabel;
    lsaHDir->toLabel       = toStation;
    lsaHDir->angleDecDeg   = angle;
    lsaHDir->usesDecDeg    = true;
    lsaHDir->sigma         = sig;
    lsaHDir->sigmaUnits    = units;
}

void setHDirRequiredValues_DMS(LSAHDir *lsaHDir, const char *dgrpLabel, const char *toStation, int degrees, int minutes, double seconds, double sig, const char *units)
{
    lsaHDir->dirGroupLabel = dgrpLabel;
    lsaHDir->toLabel       = toStation;
    lsaHDir->angleDeg   = degrees;
    lsaHDir->angleMin   = minutes;
    lsaHDir->angleSec   = seconds;
    lsaHDir->usesDecDeg = false;
    lsaHDir->sigma      = sig;
    lsaHDir->sigmaUnits = units;
}

void setHDirHeightToLabel(LSAHDir* lsaHDir, const char *label)
{
    lsaHDir->setHeightToLabel(label);
}

void setHDirHeightToValue(LSAHDir* lsaHDir, double value, const char *units)
{
    lsaHDir->setHeightToValue(value, units);
}

void setHDirHeightToSigma(LSAHDir* lsaHDir, double value, const char *units)
{
    lsaHDir->setHeightToSigma(value, units);
}

void getHDirString(LSAHDir* lsaHDir, char *buffer)
{
    strcpy(buffer,lsaHDir->getSingleLSAString().c_str());
}
