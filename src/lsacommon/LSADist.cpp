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
/// @file LSADist.cpp  Class LSADist, data for the *.LSA file record DIS,
///                    distance measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "LSADist.hpp"
//#include "logstream.hpp"      // TEMP

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSADist::NUM_UI_DISPLAY_COLUMNS = 5;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSADist::fromString(const string& line)
{    
    if (line.empty()) return;

    modifiers.clearModifiers();

    int n;
    vector<string> F;

    string str(line);                // copy const line
    stripTrailing(str,"\n");
    stripTrailing(str,"\r");
    stripTrailing(str," ");
    stripLeading(str," ");
    
    // Steal the General Notes before continuing.
    textNotes = QString::fromStdString(extractXMLFromString(str, RecordTags::TextNotes, true));
    F = splitWithDoubleQuotes(str,' ');
    n = F.size();

    if (n < 6)
    {
        std::string fromTo = From + ", " + To;
        addNumFieldsWarning(LSAType::DIST, fromTo);

        return;
    }

    // Required Fields
    From = F[1];
    To = F[2];
    distance = validateDoubleMin(F[3], LSAFieldType::DISTANCE, 0.0);
    sigma =    validateDoubleMin(F[4], LSAFieldType::SIGMA, 0.0);
    linUnits = validateString(F[5], LSAFieldType::UNITS, "cm,km,m,ft");

    int numRequiredParms = 6; // Specific number of required parms for DIST record
    if(n == numRequiredParms)
    {
        // No optional parameters to parse
        return;
    }

    // push the remaining parms onto a deque and parse them
    std::deque<std::string> optionalParms;
    int refractValueIndex = -1;
    for (size_t i = numRequiredParms; i < F.size(); ++i)
    {
        //silently remove REFRACT modifier (removed from Record Editor in 1.0.0, removed from backend in 1.1.0)
        if(F[i]==string("REFRACT"))
        {
            refractValueIndex = i+1;
            continue;
        }
        if(i == refractValueIndex)
            continue;

        optionalParms.push_back(F[i]);
    }

    std::string fromTo = From + ", " + To;
    modifiers.parseOptionalParms(optionalParms, LSAType::DIST, fromTo, &parseWarnings);

    return;
}

//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSADist::writeLSAString(std::ostream & out) const
{
    if (isCommented)
    {
        out << "#";
    }

    // label
    out << "DIST " << addQuotes(From) << " " << addQuotes(To);

    // distance
    int linearPrecision = grabNumDigits(getRecType(), linUnits);
    out << std::fixed << std::setprecision(linearPrecision);
    out << " " << distance;

    // sigma
    // Previous call before attempting to resolve Issues #31 and 118 was
    // to go with default value of 11. Sticking with same precision as distance
    // for now.
    out << " " << sigma << " " << linUnits;

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
std::string LSADist::getUIColumnData(int columnNumber) const
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

void LSADist::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    modifiers.modifyLabel(lsaType, oldLabel, newLabel);

    return;
}

void LSADist::renamePosition(std::string oldLabel, std::string newLabel)
{
    if (From == oldLabel) From = newLabel;
    if (To   == oldLabel) To   = newLabel;
}

std::vector<std::string> LSADist::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(From);
    returnValues.push_back(To);

    return returnValues;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSADist::getUIName() const
{
    std::ostringstream oss;
    oss << From << ", " << To;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSADist::getUIDetails() const
{
    std::ostringstream oss;

    oss << "stdev: " << std::fixed << std::setprecision(3) << std::setw(5) << sigma;

    oss << " dist: " << distance << " " << linUnits;
    return oss.str();
}

bool LSADist::referencesModifier(LSAType modifierType, std::string label) const
{
    if      (modifierType == LSAType::VSCA)  return (modifiers.getVSCALabel() == label);
    else if (modifierType == LSAType::UNCR) return (modifiers.getUncrLabel() == label);
    else if (modifierType == LSAType::HGHT)
    {
       return ( modifiers.getHeightFromLabel() == label || modifiers.getHeightToLabel() == label);
    }

    return false;
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
