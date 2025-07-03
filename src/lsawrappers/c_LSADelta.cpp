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
#include <c_LSADelta.hpp>

LSADelta* new_LSADelta(const char *fromLabel, const char *toLabel, double dx, double dy, double dz, const char *units,
                       double cxx, double cxy, double cxz,
                       double cyy, double cyz, double czz)
{
    formatExponent();
    return new LSADelta(fromLabel, toLabel, dx, dy, dz, units,
                        cxx, cxy, cxz, cyy, cyz, czz);
}

void delete_LSADelta(LSADelta *lsaDelta)
{ 
    delete lsaDelta;
}

void setDeltaHeightFromLabel(LSADelta* lsaDelta, const char *label)
{
    lsaDelta->setHeightFromLabel(label);
}

void setDeltaHeightFromValue(LSADelta* lsaDelta, double value, const char *units)
{
    lsaDelta->setHeightFromValue(value, units);
}

void setDeltaHeightFromSigma(LSADelta* lsaDelta, double value, const char *units)
{
    lsaDelta->setHeightFromSigma(value, units);
}

void setDeltaHeightToLabel(LSADelta* lsaDelta, const char *label)
{
    lsaDelta->setHeightToLabel(label);
}

void setDeltaHeightToValue(LSADelta* lsaDelta, double value, const char *units)
{
    lsaDelta->setHeightToValue(value, units);
}

void setDeltaHeightToSigma(LSADelta* lsaDelta, double value, const char *units)
{
    lsaDelta->setHeightToSigma(value, units);
}

void setDeltaUNCRLabel(LSADelta* lsaDelta, const char *label)
{
    lsaDelta->setUncrLabel(label);
}

void setDeltaVSCALabel(LSADelta* lsaDelta, const char *label)
{
    lsaDelta->setVSCALabel(label);
}

void setDeltaVSCAValue(LSADelta* lsaDelta, double value)
{
    lsaDelta->setVSCAValue(value);
}

void getLSADeltaString(LSADelta* lsaDelta, char *buffer)
{
    // configure windows to only use two digit exponents when possible
    #ifdef _WIN32
        #if (_MSC_VER < 1900)//Visual Studio 2015
            _set_output_format(_TWO_DIGIT_EXPONENT);
        #endif
    #endif

    strcpy(buffer,lsaDelta->getSingleLSAString().c_str());
}

void setConversionNotes(LSADelta* lsaDelta, const char *notes)
{
    lsaDelta->textNotes = QString::fromStdString(notes);
}
