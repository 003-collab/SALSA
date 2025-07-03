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
#include <c_LSAPosC.hpp>

LSAPosC* new_LSAPosC(const char *label, double x, double y, double z, const char* units)
{
    formatExponent();
    return new LSAPosC(label, x, y, z, units);
}

void delete_LSAPosC(LSAPosC *lsaPosC)
{ 
    delete lsaPosC;
}

void setPosCCovariance(LSAPosC *lsaPosC, double cxx, double cxy, double cxz,
                       double cyy, double cyz, double czz)
{
    lsaPosC->setCovariance(cxx, cxy, cxz, cyy, cyz, czz);
}

void setPosCVSCALabel(LSAPosC* lsaPosC, const char *label)
{
    lsaPosC->setVSCALabel(label);
}

void setPosCVSCAValue(LSAPosC* lsaPosC, double value)
{
    lsaPosC->setVSCAValue(value);
}

void getLSAPosCString(LSAPosC* lsaPosC, char *buffer)
{
    strcpy(buffer,lsaPosC->getSingleLSAString().c_str());
}
