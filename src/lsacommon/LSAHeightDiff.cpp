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
#include "LSAHeightDiff.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAHeightDiff::NUM_UI_DISPLAY_COLUMNS = 5;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAHeightDiff::fromString(const string& line)
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
    // split into fields
    F = splitWithDoubleQuotes(str,' ');
    n = F.size();

    if (n < 6)
    {
        std::string fromTo = From + ", " + To;
        addNumFieldsWarning(LSAType::HDIF, fromTo);
        return;
    }

    // Required Fields
    From = F[1];
    To = F[2];
    heightDiff = validateDouble(F[3], LSAFieldType::HEIGHT_DIFF);
    sigma = validateDoubleMin(F[4], LSAFieldType::SIGMA, 0.0);
    linUnits = validateString(F[5], LSAFieldType::HEIGHT_UNITS, "cm,m,ft");

    int numRequiredParms = 6; // Specific number of required parms for DIST record
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


    std::string fromTo = From + ", " + To;
    modifiers.parseOptionalParms(optionalParms, LSAType::HDIF, fromTo, &parseWarnings);

    return;
}
//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAHeightDiff::writeLSAString(std::ostream & out) const
{
    if (isCommented)
    {
        out << "#";
    }

    // label
    out << "HDIF " << addQuotes(From) << " " << addQuotes(To);

    // distance
    int heightPrecision = grabNumDigits(getRecType(), linUnits);
    out << std::fixed << std::setprecision(heightPrecision);
    out << " " << heightDiff;

    // sigma
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
std::string LSAHeightDiff::getUIColumnData(int columnNumber) const
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

void LSAHeightDiff::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    modifiers.modifyLabel(lsaType, oldLabel, newLabel);

    return;
}

void LSAHeightDiff::renamePosition(std::string oldLabel, std::string newLabel)
{
    if (From == oldLabel) From = newLabel;
    if (To   == oldLabel) To   = newLabel;
}

std::vector<std::string> LSAHeightDiff::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(From);
    returnValues.push_back(To);

    return returnValues;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAHeightDiff::getUIName() const
{
    std::ostringstream oss;
    oss << From << ", " << To;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAHeightDiff::getUIDetails() const
{
    std::ostringstream oss;
    oss << "stdev: " << std::fixed << std::setprecision(3) << std::setw(5) << sigma;

    oss << " diff: " << heightDiff << " " << linUnits;

    return oss.str();
}

bool LSAHeightDiff::referencesModifier(LSAType modifierType, std::string label) const
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

