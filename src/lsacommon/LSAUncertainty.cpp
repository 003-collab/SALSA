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
/// @file LSASample.cpp  Include file for class LSASample, configuration input
///                      in *.LSA files, including title, precision, dimension,
///                      and convergence limits

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "LSAConstants.hpp"
#include "lsaUtils.hpp"
#include "LSAUncertainty.hpp"

#include "logstream.hpp"         // TEMP

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAUncertainty::NUM_UI_DISPLAY_COLUMNS = 1;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAUncertainty::fromString(const string& line)
{
    if (line.empty()) return;

    int n;
    vector<string> F;
    static const string M("M"), RAD("RAD"), FIX("FIX"), NONE("NONE");

    string str(line);                // copy const line
    stripTrailing(str,"\n");
    stripTrailing(str,"\r");
    stripTrailing(str," ");
    stripLeading(str," ");
    
    // Steal the General Notes before continuing.
    textNotes = QString::fromStdString(extractXMLFromString(str, RecordTags::TextNotes, true));
    // split into fields
    F = splitWithDoubleQuotes(str,' ');
    n = F.size();

    bool goodParse = true;
    if(n == 9 && isLinearUnit(F[8]) )
    {
        label = F[1];

        if (F[2] != "[]")
        {
            hasAddSigma = true;
            Sigma = validateDoubleMin(F[2], LSAFieldType::ADD_SIGMA, 0.0);
        }

        if (F[3] != "[]")
        {
            sigmaUnits = validateString(F[3],LSAFieldType::SIGMA_UNITS,"cm,m,ft,rad,deg,soa");
        }

        if (F[4] != "[]")
        {
            hasPPM = true;
            PPM = validateDoubleMin(F[4], LSAFieldType::PPM, 0.0);
        }

        if (F[5] != "[]")
        {
            hasAtCenter = true;
            AtCenter = validateDoubleMin(F[5], LSAFieldType::CENTER_AT, 0.0);
        }

        if (F[6] != "[]")
        {
            hasFromCenter = true;
            FromCenter = validateDoubleMin(F[6], LSAFieldType::CENTER_FROM, 0.0);
        }

        if (F[7] != "[]")
        {
            hasToCenter = true;
            ToCenter = validateDoubleMin(F[7], LSAFieldType::CENTER_TO, 0.0);
        }

        linUnits = validateString(F[8], LSAFieldType::UNITS, "cm,m,ft");
    }
    else
    {
        addNumFieldsWarning(LSAType::UNCR, label);
    }

    return;
}

//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAUncertainty::writeLSAString(std::ostream & out) const
{
    if (isCommented)
    {
        out << "#";
    }

    out << "UNCR " << addQuotes(label);
    out << std::fixed;

    if (hasAddSigma)
    {
        int sigmaPrecision = grabNumDigits(getRecType(), sigmaUnits);
        out << std::setprecision(sigmaPrecision) << " " << Sigma << " " << lowerCase(sigmaUnits);
    }
    else
    {
        out << " [] " << lowerCase(sigmaUnits);
    }

    if (hasPPM)
    {
        out << std::setprecision(lsa::NUM_DECIMALS_UNCR_PPM) << " " << PPM;
    }
    else
    {
        out << " []";
    }

    int centErrorPrecision = grabNumDigits(getRecType(), linUnits);
    if (hasAtCenter)
    {
        out << std::setprecision(centErrorPrecision) << " " << AtCenter;
    }
    else
    {
        out << " []";
    }

    if (hasFromCenter)
    {
        out << std::setprecision(centErrorPrecision) << " " << FromCenter;
    }
    else
    {
        out << " []";
    }

    if (hasToCenter)
    {
        out << std::setprecision(centErrorPrecision) << " " << ToCenter;
    }
    else
    {
        out << " []";
    }

    out << " " << lowerCase(linUnits);
    
    if (!textNotes.isEmpty())
    {
        out << " ...\n";
        // put the text notes at the end of the record.
        out << RecordTags::TextNotes.first + textNotes.toStdString() + RecordTags::TextNotes.second;
    }

    out << "\n";

    return;
}

// Get a string to display in a given column in the Project Navigator
// @param the column number to display
// @return the string to display in the UI
std::string LSAUncertainty::getUIColumnData(int columnNumber) const
{
    std::string retVal = "";

    switch (columnNumber)
    {
    case 0:
        std::ostringstream oss;
        oss << asTypeString() << " " << getUIName() << " " << getUIDetails();
        retVal = oss.str();
        break;

    }

    return retVal;
}

void LSAUncertainty::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    return;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAUncertainty::getUIName() const
{
    std::ostringstream oss;
    oss << label;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAUncertainty::getUIDetails() const
{
    std::ostringstream oss;
    oss << "at: " << AtCenter<< " from: " << FromCenter << " to: " << ToCenter << " " << linUnits << " ppm: " << PPM << " sigma: " << Sigma << " " << sigmaUnits;
    return oss.str();
}

std::string LSAUncertainty::getUIDetailsBrief() const
{
    std::ostringstream oss;

    if (hasAddSigma)   oss << "sig: "  << Sigma << " ";
    if (hasPPM)        oss << "ppm: "  << PPM   << " ";
    if (hasAtCenter)   oss << "at: "   << AtCenter << " ";
    if (hasFromCenter) oss << "from: " << FromCenter << " ";
    if (hasToCenter)   oss << "to: "   << ToCenter<< " ";
    if (hasAtCenter || hasFromCenter || hasAtCenter ) oss << linUnits;

    return oss.str();
}

bool LSAUncertainty::isSigmaApplicable(const LSAType &verifyType) const
{
    return (verifyType == LSAType::Types::AZIM ||
            verifyType == LSAType::Types::DGRP ||
            verifyType == LSAType::Types::DIST ||
            verifyType == LSAType::Types::DXYZ ||
            verifyType == LSAType::Types::HANG ||
            verifyType == LSAType::Types::HDIF ||
            verifyType == LSAType::Types::VANG ||
            verifyType == LSAType::Types::ZANG);
}

bool LSAUncertainty::isPPMApplicable(const LSAType &verifyType) const
{
    return (verifyType == LSAType::Types::DIST ||
            verifyType == LSAType::Types::DXYZ ||
            verifyType == LSAType::Types::HDIF);
}

bool LSAUncertainty::isAtCenterApplicable(const LSAType &verifyType) const
{
    return (verifyType == LSAType::Types::HANG);
}

bool LSAUncertainty::isFromCenterApplicable(const LSAType &verifyType) const
{
    return (verifyType == LSAType::Types::AZIM ||
            verifyType == LSAType::Types::DGRP ||
            verifyType == LSAType::Types::DIST ||
            verifyType == LSAType::Types::DXYZ ||
            verifyType == LSAType::Types::HANG ||
            verifyType == LSAType::Types::VANG ||
            verifyType == LSAType::Types::ZANG);
}

bool LSAUncertainty::isToCenterApplicable(const LSAType &verifyType) const
{
    return (verifyType == LSAType::Types::AZIM ||
            verifyType == LSAType::Types::DGRP ||
            verifyType == LSAType::Types::DIST ||
            verifyType == LSAType::Types::DXYZ ||
            verifyType == LSAType::Types::HANG ||
            verifyType == LSAType::Types::VANG ||
            verifyType == LSAType::Types::ZANG);
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
