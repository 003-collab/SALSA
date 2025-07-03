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
/// @file LSADelta.cpp  Class LSADelta, data for the *.LSA file record DEL,
///                     3-D XYZ coordinate difference measurement

#include <string>
#include <ostream>

#include "math.h"

#include "Matrix.hpp"
#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "LSADelta.hpp"
#include "lsaUtils.hpp"
#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSADelta::NUM_UI_DISPLAY_COLUMNS = 5;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSADelta::fromString(const string& line)
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

    // Make sure we have enough required fields to parse
    if (n < 13)
    {
        std::string fromTo = From + ", " + To;
        addNumFieldsWarning(LSAType::DXYZ, fromTo);
        return;
    }

    // Required Fields
    From = F[1];
    To = F[2];

    // coordinate differences
    dx = validateDouble(F[3], LSAFieldType::DELTA_X);
    dy = validateDouble(F[4], LSAFieldType::DELTA_Y);
    dz = validateDouble(F[5], LSAFieldType::DELTA_Z);
    linUnits = validateString(F[6], LSAFieldType::UNITS, "m,km,ft");

    // components of the UT portion of the covariances matrix.
    covxx = validateDouble(F[7], LSAFieldType::COVARIANCE);
    covxy = validateDouble(F[8], LSAFieldType::COVARIANCE);
    covxz = validateDouble(F[9], LSAFieldType::COVARIANCE);
    covyy = validateDouble(F[10], LSAFieldType::COVARIANCE);
    covyz = validateDouble(F[11], LSAFieldType::COVARIANCE);
    covzz = validateDouble(F[12], LSAFieldType::COVARIANCE);

    int numRequiredParms = 13; // Specific number of parms for DXYZ record
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
    bool goodParse = modifiers.parseOptionalParms(optionalParms, LSAType::DXYZ, fromTo, &parseWarnings);

    return;
}


//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSADelta::writeLSAString(std::ostream & out) const
{
    std::string comment = isCommented ? "#" : "";

    // label
    out << comment << "DXYZ " << addQuotes(From) << " " << addQuotes(To);

    // dx dy dz units
    int linearPrecision = grabNumDigits(getRecType(), linUnits);
    out << std::fixed << std::setprecision(linearPrecision);
    out << " " << dx << " " << dy << " " << dz << " " << linUnits;

    // covariance
    out << " ...\n";
    int w = 18;
    out << std::scientific << setprecision(lsa::NUM_DECIMALS_COVARIANCE);
    out << comment << setw(w) << covxx << " " << setw(w) << covxy << " " << setw(w) << covxz << " ...\n";
    out << comment << setw(w) << ""    << " " << setw(w) << covyy << " " << setw(w) << covyz << " ...\n";
    out << comment << setw(w) << ""    << " " << setw(w) << ""    << " " << setw(w) << covzz;

    
    std::string modString = modifiers.getLSAString();
    if (!modString.empty())
    {
        out << " ...\n";
        out << comment << modString;
    }
    
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
std::string LSADelta::getUIColumnData(int columnNumber) const
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

void LSADelta::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    modifiers.modifyLabel(lsaType, oldLabel, newLabel);

    return;
}

void LSADelta::renamePosition(std::string oldLabel, std::string newLabel)
{
    if (From == oldLabel) From = newLabel;
    if (To   == oldLabel) To   = newLabel;
}

std::vector<std::string> LSADelta::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(From);
    returnValues.push_back(To);

    return returnValues;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSADelta::getUIName() const
{
    std::ostringstream oss;
    oss << From << ", " << To;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSADelta::getUIDetails() const
{
    std::ostringstream oss;
    // dx dy dz units
    double slantDistance = std::sqrt(dx*dx + dy*dy + dz*dz);
    oss << std::fixed << std::setprecision(3) ;
    oss << setw(7) << slantDistance << " " << linUnits;
    return oss.str();
}

bool LSADelta::referencesModifier(LSAType modifierType, std::string label) const
{
    if      (modifierType == LSAType::VSCA)  return (modifiers.getVSCALabel() == label);
    else if (modifierType == LSAType::UNCR) return (modifiers.getUncrLabel() == label);
    else if (modifierType == LSAType::HGHT)
    {
       return ( modifiers.getHeightFromLabel() == label || modifiers.getHeightToLabel() == label);
    }

    return false;
}

bool LSADelta::hasValidCovariance() const
{
    bool covIsPositiveSemiDefinite = isPositiveSemiDefinite(covxx, covxy, covxz, covyy, covyz, covzz);

    if (covIsPositiveSemiDefinite)
    {
        return true;
    }

    return false;
}

// this ends the base class interface




//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
