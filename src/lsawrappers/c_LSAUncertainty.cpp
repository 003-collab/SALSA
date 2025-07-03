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
#include <c_LSAUncertainty.hpp>
#include <string.h>
LSAUncertainty* new_LSAUncr(const char *label)
{
    return new LSAUncertainty(label);
}

void delete_LSAUncr(LSAUncertainty *Uncertainty)
{ 
    delete Uncertainty;
}

void setUncrSigma(LSAUncertainty* uncertainty, double value, const char *sunits)
{
    uncertainty->Sigma = value;
    uncertainty->sigmaUnits = sunits;
    uncertainty->hasAddSigma = true;
}

void setUncrPPM(LSAUncertainty* uncertainty, double value)
{
    uncertainty->PPM = value;
    uncertainty->hasPPM = true;
}

void setUncrAtCenterError(LSAUncertainty* uncertainty, double value, const char *lunits)
{
    uncertainty->AtCenter = value;
    uncertainty->linUnits = lunits;
    uncertainty->hasAtCenter = true;
}

void setUncrFromCenterError(LSAUncertainty* uncertainty, double value, const char *lunits)
{
    uncertainty->FromCenter = value;
    uncertainty->linUnits = lunits;
    uncertainty->hasFromCenter = true;
}

void setUncrToCenterError(LSAUncertainty* uncertainty, double value, const char *lunits)
{
    uncertainty->ToCenter = value;
    uncertainty->linUnits = lunits;
    uncertainty->hasToCenter = true;
}

void getLSAUncrString(LSAUncertainty* uncertainty, char * buffer)
{
    strcpy(buffer,uncertainty->getSingleLSAString().c_str());
}

