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
/// @file LSAHAngle.cpp  Class LSAHAngle, data for the *.LSA file
///                      record HAN horizontal angle

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "LSAHAngle.hpp"
#include "lsaUtils.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAHAngle::NUM_UI_DISPLAY_COLUMNS = 5;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAHAngle::fromString(const string& line)
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

    if(n > 6 && isAngularUnit(F[6]))
    {
        From = F[1];
        At = F[2];
        To = F[3];
        usesDecDeg = true;
        angleDecDeg = validateDoubleMinMax(F[4], LSAFieldType::ANGLE_DEC_DEG, -360, 360, true, true);

        sigma = validateDoubleMin(F[5], LSAFieldType::SIGMA, 0);
        sigmaUnits = validateString(F[6], LSAFieldType::SIGMA_UNITS, "rad,deg,soa");

        numRequiredParms = 7;
    }
    else if( n > 9 && "DMS" == F[7] && isAngularUnit(F[9]))
    {
        From = F[1];
        At = F[2];
        To = F[3];
        usesDecDeg = false;
        angleDMSIsNeg = false;
        angleDeg = validateIntBool(angleDMSIsNeg, F[4], LSAFieldType::ANGLE_DEG, -359, 359);
        angleMin = validateIntMinMax(F[5], LSAFieldType::ANGLE_MIN, 0, 59);
        angleSec = validateDoubleMinMax(F[6], LSAFieldType::ANGLE_SEC, 0.0, 60.0, false, true);

        sigma = validateDoubleMin(F[8], LSAFieldType::SIGMA, 0);
        sigmaUnits = validateString(F[9], LSAFieldType::SIGMA_UNITS, "rad,deg,soa");

        numRequiredParms = 10;
    }
    else
    {
        std::string fromAtTo = From + ", " + To + ", " + At;
        addNumFieldsWarning(LSAType::HANG, fromAtTo);
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

    std::string fromAtTo = From + ", " + At + ", " + To;
    modifiers.parseOptionalParms(optionalParms, LSAType::HANG, fromAtTo, &parseWarnings);

    return;
}

//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAHAngle::writeLSAString(std::ostream & out) const
{
    if (isCommented)
    {
        out << "#";
    }

    out << "HANG " << addQuotes(From) << " " << addQuotes(At) << " " << addQuotes(To);

    // angle
    int anglePrecision;
    if (usesDecDeg)
    {
        anglePrecision = grabNumDigits(getRecType(), lsa::UNITS_DEG);
        out << std::fixed << std::setprecision(anglePrecision);
        out << " " << angleDecDeg;
    }
    else
    {
        anglePrecision = grabNumDigits(getRecType(), lsa::UNITS_SOA);
        out << std::fixed << std::setprecision(anglePrecision);
        std::string sign;
        if(angleDMSIsNeg)
             sign = "-";
        out << " " << sign << angleDeg << " " << angleMin << " " << angleSec << " DMS";
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
std::string LSAHAngle::getUIColumnData(int columnNumber) const
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
        // SCJ TODO: vsca refactoring needed
        // retVal = gnsstk::StringUtils::asString(groupVSCA * modifiers.getVSCAValue(), 3);
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

void LSAHAngle::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    modifiers.modifyLabel(lsaType, oldLabel, newLabel);

    return;
}

std::vector<std::string> LSAHAngle::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(From);
    returnValues.push_back(At);
    returnValues.push_back(To);

    return returnValues;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAHAngle::getUIName() const
{
    std::ostringstream oss;
    oss << From << ", " << At << ", " << To;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAHAngle::getUIDetails() const
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

bool LSAHAngle::referencesModifier(LSAType modifierType, std::string label) const
{
    if      (modifierType == LSAType::VSCA)  return (modifiers.getVSCALabel() == label);
    else if (modifierType == LSAType::UNCR) return (modifiers.getUncrLabel() == label);
    else if (modifierType == LSAType::HGHT)
    {
       return ( modifiers.getHeightFromLabel() == label || modifiers.getHeightToLabel() == label);
    }

    return false;
}

void LSAHAngle::renamePosition(std::string oldLabel, std::string newLabel)
{
    if (From == oldLabel) From = newLabel;
    if (At   == oldLabel) At   = newLabel;
    if (To   == oldLabel) To   = newLabel;
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
