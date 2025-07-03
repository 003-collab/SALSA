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
/// @file LSAComment.cpp  Class LSAComment, TODO 

#include <string>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"

#include "lsaUtils.hpp"
#include "LSAComment.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAComment::NUM_UI_DISPLAY_COLUMNS = 1;


LSAComment::LSAComment(std::string line) : LSARecord(LSAType::COMMENT)
{
    // Notes:
    // 1) LSAFile::readAndParseLSAFile(...) typically creates LSAComments where lineContents
    // begins with a #.  ie, whatever is passed to LSAComment::fromString(...) will already
    // have a # appended to it.
    // 2) HOWEVER, blank lines in lsa comments are stored as comments WITHOUT the # sign
    // 3) For convenience and consistency with legacy code, we append a # to the value passed
    // to this constructor EXCEPT when the string passed to this method is empty.

    if (!line.empty())
        lineContents = "#" +line;
}

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAComment::fromString(const string& line)
{
    lineContents = line;

    return;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) for the LSA file.
// return string, a single line for a LSA file
void LSAComment::writeLSAString(std::ostream& out) const
{
    out << lineContents << "\n";
    return;
}

/// Get a string to display in a given column in the Project Navigator
/// @param the column number to display
/// @return the string to display in the UI
std::string LSAComment::getUIColumnData(int columnNumber) const
{
    std::string retVal = "";

    switch (columnNumber)
    {
    case 0:

        retVal = getUIDetails();
        break;
    }

    return retVal;
}

void LSAComment::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    return;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAComment::getUIName() const
{
    return "";
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAComment::getUIDetails() const
{
    std::string retVal = lineContents;
    if (!retVal.empty())
        retVal = retVal.substr(1); // strip leading #

    return retVal;
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
