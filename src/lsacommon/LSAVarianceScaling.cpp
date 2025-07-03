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
#include "LSAVarianceScaling.hpp"

#include "logstream.hpp"         // TEMP

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAVarScaling::NUM_UI_DISPLAY_COLUMNS = 3;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAVarScaling::fromString(const string& line)
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

    if (n < 3 || n > 4)
    {
        addNumFieldsWarning(LSAType::VSCA, label);
        return;
    }

    try
    {
        label = F[1];
        varFactor = validateDoubleMin(F[2], LSAFieldType::VAR_FACTOR, 0.0, true);

        if (n == 4)
        {
            validateString(F[3], LSAFieldType::VAR_FACTOR, "toparent");
            if (gnsstk::StringUtils::upperCase(F[3]) == "TOPARENT")
            {
                isAppliedToParent = true;
            }
        }
    }
    catch(...)
    {
        goodParse = false; // scj: push warning here
    }

    return;
}

//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAVarScaling::writeLSAString(std::ostream & out) const
{
    out << std::fixed << std::setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING);

    if (isCommented)
    {
        out << "#";
    }

    out << "VSCA " << addQuotes(label) << " " << varFactor;

    if (isAppliedToParent)
    {
        out << " " << "TOPARENT";
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
std::string LSAVarScaling::getUIColumnData(int columnNumber) const
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

void LSAVarScaling::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    return;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAVarScaling::getUIName() const
{
    std::ostringstream oss;
    oss << label << " " << varFactor;
    return oss.str();
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAVarScaling::getUIDetails() const
{
    std::ostringstream oss;
    oss << varFactor;
    return oss.str();
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
