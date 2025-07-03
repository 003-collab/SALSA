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
/// @file LSAHeight.cpp for class LSAHeight

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "LSAHeight.hpp"

#include "logstream.hpp"         // TEMP

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAHeight::NUM_UI_DISPLAY_COLUMNS = 4;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAHeight::fromString(const string& line)
{
    if (line.empty()) return;

    int n;
    vector<string> F;
    static const string M("M"), RAD("RAD"), FIX("FIX"), NONE("NONE");
    bool goodParse = true;

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


   if (n == 6)
    {
       label = F[1];
        value = validateDouble(F[2], LSAFieldType::HEIGHT);
        units = validateString(F[3], LSAFieldType::HEIGHT_UNITS, "cm,m,ft");
        sigmaValue = validateDouble(F[4], LSAFieldType::SIGMA);
        sigmaUnits = validateString(F[5], LSAFieldType::SIGMA_UNITS, "cm,m,ft");
    }
   else if (n == 4)
    {
       label = F[1];
        value = validateDouble(F[2], LSAFieldType::HEIGHT);
        units = validateString(F[3], LSAFieldType::HEIGHT_UNITS, "cm,m,ft");
        sigmaValue = validateDouble("0.0", LSAFieldType::SIGMA);
        sigmaUnits = validateString("m", LSAFieldType::SIGMA_UNITS, "cm,m,ft");
    }
    else
    {
        addNumFieldsWarning(LSAType::HGHT, label);
    }

    return;
}

//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAHeight::writeLSAString(std::ostream & out) const
{
    if (isCommented)
    {
        out << "#";
    }

    int heightPrecision = grabNumDigits(getRecType(), units);
    out << std::fixed << std::setprecision(heightPrecision);
    out << "HGHT " << addQuotes(label) << " " << value
        << " " << units << " " << sigmaValue
        << " " << sigmaUnits;
    
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
std::string LSAHeight::getUIColumnData(int columnNumber) const
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
        retVal = "";
        break;
    case 3:
        retVal = getUIDetails();
        break;
    }

    return retVal;
}

void LSAHeight::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    return;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAHeight::getUIName() const
{
    std::ostringstream oss;
    oss << label;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAHeight::getUIDetails() const
{
    std::ostringstream oss;
    oss << value << " " << units << " " << sigmaValue << " " << sigmaUnits;
    return oss.str();
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
