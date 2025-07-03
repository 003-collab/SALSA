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
/// @file LSAConfig.cpp  Include file for class LSAConfig, configuration input
///                      in *.LSA files, including title, precision, dimension,
///                      and convergence limits

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "expandpath.hpp"
#include "LSAConfig.hpp"

#include "logstream.hpp"         // TEMP

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAConfig::NUM_UI_DISPLAY_COLUMNS = 3;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAConfig::fromString(const string& line)
{
    if (line.empty()) return;


    int n;
    double fact,fact2;
    vector<string> F;

    string str(line);                // copy const line
    stripTrailing(str,"\n");
    stripTrailing(str,"\r");
    stripTrailing(str," ");
    stripLeading(str," ");


    if(str.empty()) return; // scj: push warning here
    if(str[0] == '#') return;// scj: push warning here

    // check record type
    //POS label X Y Z covxx xy xz yy yz zz LUNIT [FIX] [CONS c[c]]
    if(str.substr(0,2) != string("--")) return; // scj: push warning here

    // split into fields
    F = splitWithDoubleQuotes(str);
    n = F.size();

    bool goodParse = false;

    if (n == 2)
    {
        configType = F[0];
        configValue = F[1];
        goodParse = true;
    }

    return;
}

//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAConfig::writeLSAString(std::ostream & out) const
{
    if (isCommented)
    {
        out << "#";
    }

    out << configType << " " << configValue << "\n";

   return;
}

/// Get a string to display in a given column in the Project Navigator
/// @param the column number to display
/// @return the string to display in the UI
std::string LSAConfig::getUIColumnData(int columnNumber) const
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
        retVal = getUIDetails();
        break;
    }

    return retVal;
}

void LSAConfig::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    return;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAConfig::getUIName() const
{
    return configType;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAConfig::getUIDetails() const
{
    return configValue;
}

std::string LSAConfig::asDATString() const
{
    std::ostringstream oss;
    std::string geoidFilePath;
    std::string configTypeCopy1(configType.substr(2));
    std::string configTypeCopy2(configType);

    oss << gnsstk::StringUtils::upperCase(configTypeCopy1); // trim the leading "--" from the front of the configType

    if (gnsstk::StringUtils::upperCase(configTypeCopy2) == "--GEOIDFILE")
    {
        // TODO: find a better way to handle paths, probably by replacing tokens during install
        geoidFilePath = getExecutablePath() + "/../data/geoid/" + configValue;
        oss << " " << quoteIfSpaces(geoidFilePath) << "\n";
    }
    else
    {
        oss << " " << configValue << "\n";
    }

    return oss.str();
}


//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
