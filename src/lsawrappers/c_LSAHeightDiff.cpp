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
#include <c_LSAHeightDiff.hpp>

LSAHeightDiff* new_LSAHeightDiff(const char *fromLabel, const char *toLabel, double hDif, double sigma, const char *linUnits)
{
    return new LSAHeightDiff(fromLabel, toLabel, hDif, sigma, linUnits);
}

void delete_LSAHeightDiff(LSAHeightDiff *lsaHeightDiff)
{
    delete lsaHeightDiff;
}

void setHDifUNCRLabel(LSAHeightDiff *lsaHeightDiff, const char *label)
{
    lsaHeightDiff->setUncrLabel(label);
}

void setHDifRefraction(LSAHeightDiff *lsaHeightDiff, double value)
{
    lsaHeightDiff->setRefract(value);
}

void setHDifIsReduced(LSAHeightDiff *lsaHeightDiff, bool isReduced)
{
    lsaHeightDiff->setIsReduced(isReduced);
}

void setHDifVSCALabel(LSAHeightDiff *lsaHeightDiff, const char *label)
{
    lsaHeightDiff->setVSCALabel(label);
}

void setHDifVSCAValue(LSAHeightDiff *lsaHeightDiff, double value)
{
    lsaHeightDiff->setVSCAValue(value);
}

void setHDifCurvCorr(LSAHeightDiff *lsaHeightDiff, bool curvCorr)
{
    lsaHeightDiff->setCurvCorr(curvCorr);
}

//This method is NOT exported (see c_LSAHeightDiff.hpp)
void setHDifOHC(LSAHeightDiff *lsaHeightDiff, bool ohcCorr)
{
    lsaHeightDiff->setOHC(ohcCorr);
}

void getHDifString(LSAHeightDiff *lsaHeightDiff, char *buffer)
{
    strcpy(buffer,lsaHeightDiff->getSingleLSAString().c_str());
}
