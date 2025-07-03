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
/// @file LSAEHeight.cpp  Class LSAEHeight, data for the *.LSA file
///                    EHT record, orthometric height measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "logstream.hpp"      // TEMP
#include "lsaUtils.hpp"
#include "LSAHDir.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAHDir::NUM_UI_DISPLAY_COLUMNS = 5;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAHDir::fromString(const string& line)
{
    if (line.empty()) return;

    modifiers.clearModifiers();

    int n;
    vector<string> F;

    string str(line);     //copy const line
    stripTrailing(str,"\n");
    stripTrailing(str,"\r");
    stripTrailing(str," ");
    stripLeading(str," ");
    
    // Steal the General Notes before continuing.
    textNotes = QString::fromStdString(extractXMLFromString(str, RecordTags::TextNotes, true));
    // split into fields
    F = splitWithDoubleQuotes(str,' ');
    n = F.size();

    int numRequiredParms;

    if(n > 5 && isAngularUnit(F[5]))
    {
        dirGroupLabel = F[1];
        toLabel = F[2];
        usesDecDeg = true;
        angleDecDeg = validateDoubleMinMax(F[3], LSAFieldType::ANGLE_DEC_DEG, -360.0, 360.0, true, true);

        sigma = validateDoubleMin(F[4], LSAFieldType::SIGMA, 0);
        sigmaUnits = validateString(F[5], LSAFieldType::SIGMA_UNITS, "rad,deg,soa");

        numRequiredParms = 6;
    }
    else if(n > 8 && "DMS" == F[6] && isAngularUnit(F[8]))
    {
        dirGroupLabel = F[1];
        toLabel = F[2];
        usesDecDeg = false;
        angleDeg = validateIntBool(angleDMSIsNeg, F[3], LSAFieldType::ANGLE_DEG,-359, 359);
        angleMin = validateIntMinMax(F[4], LSAFieldType::ANGLE_MIN, 0, 59);
        angleSec = validateDoubleMinMax(F[5], LSAFieldType::ANGLE_SEC, 0.0, 60.0, false, true);

        sigma = validateDoubleMin(F[7], LSAFieldType::SIGMA, 0);
        sigmaUnits = validateString(F[8], LSAFieldType::SIGMA_UNITS, "rad,deg,soa");

        numRequiredParms = 9;
    }
    else
    {
        addNumFieldsWarning(LSAType::HDIR, toLabel);
        return;
    }

    if(n == numRequiredParms)
    {
        // No optional parameters to parse
        return;
    }

    // push the remaining parms onto a deque and parse them
    std::deque<std::string> optionalParms;
    for (size_t i = numRequiredParms; i < F.size(); ++i)
    {
        optionalParms.push_back(F[i]);
    }

    modifiers.parseOptionalParms(optionalParms, LSAType::HDIR, toLabel, &parseWarnings);

    return;
}
//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAHDir::writeLSAString(std::ostream & out) const
{
    if (isCommented)
    {
        out << "#";
    }

    out << "HDIR " << addQuotes(dirGroupLabel) << " " <<  addQuotes(toLabel);

    // angle

    int angularPrecision;
    if (usesDecDeg)
    {
        angularPrecision = grabNumDigits(getRecType(), lsa::UNITS_DEG);
        out << std::fixed << std::setprecision(angularPrecision);
        out << " " << angleDecDeg;
    }
    else
    {
        angularPrecision = grabNumDigits(getRecType(), lsa::UNITS_SOA);
        out << std::fixed << std::setprecision(angularPrecision);
        std::string sign("");
        if(angleDMSIsNeg)
             sign = "-";
        out << " " << sign << angleDeg << " " << angleMin << " "
            << angleSec << " DMS";
    }

    // sigma
    int sigmaPrecision = grabNumDigits(getRecType(), sigmaUnits);
    out << std::setprecision(sigmaPrecision) << " " << sigma << " " << sigmaUnits;

    // modifiers
    out << modifiers.getLSAString();
    
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
std::string LSAHDir::getUIColumnData(int columnNumber) const
{
    std::string retVal = "";

    switch (columnNumber)
    {
    case 0:
        retVal = asTypeString();
        break;
    case 1:
        retVal = getUIName();
        break;
    case 2:
        retVal = ""; // we don't display scale factors for individual HDIR records
        break;
    case 3:
        retVal = getUIDetails();
        break;
    case 4:
        retVal = modifiers.getUIDetails();
        break;
    }

    return retVal;
}

void LSAHDir::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    modifiers.modifyLabel(lsaType, oldLabel, newLabel);

    return;
}

void LSAHDir::renamePosition(std::string oldLabel, std::string newLabel)
{
    if (toLabel == oldLabel) toLabel = newLabel;
}

std::vector<std::string> LSAHDir::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(toLabel);

    return returnValues;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
// NOTE: This code is never actually used.  Column 1 in the tree view is overwritten by code
// called in GuiModel::synchItemToRecord(QModelIndex index).
std::string LSAHDir::getUIName() const
{
    std::ostringstream oss;
    oss << dirGroupLabel << ", " << toLabel;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAHDir::getUIDetails() const
{
    std::ostringstream oss;
    oss << std::fixed;
    oss << "stdev: " << std::fixed << std::setprecision(1) << std::setw(4) << sigma;

    oss << " angle: ";

    if (usesDecDeg)
    {
        oss << std::setprecision(4) << setw(9) << angleDecDeg;
    }
    else
    {
        std::string sign;
        if(angleDMSIsNeg)
             sign = "-";
        oss << std::setw(3) << sign << angleDeg << " " << std::setw(3) << angleMin << " "
            << std::setprecision(1) << std::setw(5) << angleSec;
    }

    return oss.str();
}

bool LSAHDir::referencesModifier(LSAType modifierType, std::string label) const
{
    if      (modifierType == LSAType::VSCA)  return (modifiers.getVSCALabel() == label);
    else if (modifierType == LSAType::UNCR) return (modifiers.getUncrLabel() == label);
    else if (modifierType == LSAType::HGHT)
    {
       return ( modifiers.getHeightFromLabel() == label || modifiers.getHeightToLabel() == label);
    }

    return false;
}
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

