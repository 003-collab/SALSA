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
#include <c_LSAHeight.hpp>
#include <string.h>//why was this needed here, but not for other wrappers?

LSAHeight* new_LSAHeight(const char *label, double height, const char *linUnits, double sigma, const char *sigmaUnits)
{
    return new LSAHeight(label, height, linUnits, sigma, sigmaUnits);
}

void delete_LSAHeight(LSAHeight *lsaHeight) 
{ 
	delete lsaHeight; 
}

void getLSAHeightString(LSAHeight* lsaHeight, char *buffer)
{
	strcpy(buffer,lsaHeight->getSingleLSAString().c_str()); 
}
