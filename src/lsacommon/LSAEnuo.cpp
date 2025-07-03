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
#include <string>
#include <ostream>

#include "math.h"

#include "Matrix.hpp"
#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "LSAEnuo.hpp"
#include "lsaUtils.hpp"
#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAEnuo::NUM_UI_DISPLAY_COLUMNS = 4;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAEnuo::fromString(const string& line)
{
    if (line.empty()) return;

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
    if (n != 10)
    {
        addNumFieldsWarning(LSAType::ENUO, To);
        return;
    }

    // Required Fields
    From = F[1];
    To = F[2];

    // coordinate differences
    de = validateDouble(F[3], LSAFieldType::DELTA_E);
    dn = validateDouble(F[4], LSAFieldType::DELTA_E);
    du = validateDouble(F[5], LSAFieldType::DELTA_E);
    linUnits = validateString(F[6], LSAFieldType::UNITS, "m,km,ft");

    // components of the UT portion of the covariances matrix.
    se = validateDoubleMin(F[7], LSAFieldType::SIGMA, 0.0);
    sn = validateDoubleMin(F[8], LSAFieldType::SIGMA, 0.0);
    su = validateDoubleMin(F[9], LSAFieldType::SIGMA, 0.0);

    return;
}


//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAEnuo::writeLSAString(std::ostream & out) const
{
    std::string comment = isCommented ? "#" : "";

    // label
    out << comment << "ENUO " << addQuotes(From) << " " << addQuotes(To);

    int linearPrecision = grabNumDigits(getRecType(), linUnits);
    out << std::fixed << std::setprecision(linearPrecision);

    // de dn du units
    out << " " << de << " " << dn << " " << du << " " << linUnits;

    // se sn su
    out << " " << se << " " << sn << " " << su;
    
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
std::string LSAEnuo::getUIColumnData(int columnNumber) const
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
    }

    return retVal;
}

void LSAEnuo::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    return;
}

void LSAEnuo::renamePosition(std::string oldLabel, std::string newLabel)
{
    if (From == oldLabel)  From  = newLabel;
    if (To == oldLabel) To = newLabel;
}

/*
std::vector<std::string> LSAEnuo::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(To);

    return returnValues;
}
*/
void LSAEnuo::validatePositions(std::set<std::string> allPositionsIncludingAutogen, std::list<std::string> derivedPoints)
{
    // Check that all input point exists
    if (allPositionsIncludingAutogen.find(From) == allPositionsIncludingAutogen.end() &&
        std::find(derivedPoints.begin(), derivedPoints.end(), From) == derivedPoints.end())
    {
        std::string message = "Input position " + From + " does not exist.";
        warningMessages_generated.insert(message);
    }

    // Check that the output point does not duplicate anything else in the project
    int ctr=0;
    std::list<std::string>::iterator itr;
    for(itr=derivedPoints.begin();itr!=derivedPoints.end();itr++)
    {
        if(*itr == To)
            ctr++;
    }
    if (allPositionsIncludingAutogen.find(To) != allPositionsIncludingAutogen.end() ||
        ctr > 1)
    {
        std::string message = "New point label  " + To + " already exists in project.";
        warningMessages_generated.insert(message);
    }

    //check that component point label is not equal to the derived point label
    if(From == To)
    {
        std::string message = "New point label " + To + " matches the input position " + From + ".";
        warningMessages_generated.insert(message);
    }
}

//------------------------------------------------------------------------------------
// This record type doesn't have an asDATString function for when writing to the dat file. This
// function serves that purpose
// @return The string representation of the record without the Text Notes, for writing to the dat file.
string LSAEnuo::getSingleDATString() const
{
    std::string singleString = getSingleLSAString();
    extractXMLFromString(singleString, RecordTags::TextNotes, true);
    return singleString;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAEnuo::getUIName() const
{
    std::ostringstream oss;
    oss << To;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAEnuo::getUIDetails() const
{
    std::ostringstream oss;

    // from label
    oss << From << " ";

    // de dn du units
    oss << std::fixed << std::setprecision(3) ;
    oss << setw(7) << de << " " << dn << " " << du << " " << linUnits;

    // sigmas
    oss << std::fixed << std::setprecision(3) ;
    oss << setw(7) << se << " " << sn << " " << su;
    return oss.str();
}

// this ends the base class interface




//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
