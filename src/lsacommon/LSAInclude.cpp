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
/// @file LSAPoint.cpp  Class LSAPoint, data for the *.LSA file record POS
///                     3-D XYZ coordinate position

#include <string>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"

#include "lsaUtils.hpp"
#include "LSAInclude.hpp"
#include "expandpath.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAInclude::NUM_UI_DISPLAY_COLUMNS = 5;

/// destructor
LSAInclude::~LSAInclude()
{
    for (std::list<LSARecord*>::iterator iter = childRecords.begin(); iter != childRecords.end(); ++iter )
    {
        delete *iter;
    }
}

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAInclude::fromString(const string& line)
{
    if (line.empty()) return;

    modifiers.clearModifiers();

    int n;
    std::vector<std::string> F;

    std::string str(line);                // copy const line
    gnsstk::StringUtils::stripTrailing(str,"\n");
    gnsstk::StringUtils::stripTrailing(str,"\r");
    gnsstk::StringUtils::stripTrailing(str," ");
    gnsstk::StringUtils::stripLeading(str," ");

    // Steal the General Notes before continuing.
    textNotes = QString::fromStdString(extractXMLFromString(str, RecordTags::TextNotes, true));
    // split into fields
    F = splitWithDoubleQuotes(str,' ');
    n = F.size();

    if (n < 2)
    {
        return;
    }

    lsaPath = F[1];

    if (n >2)
    {
        // push the remaining parms onto a deque and parse them
        std::deque<std::string> optionalParms;
        for (size_t i = 2; i < n; ++i )
        {
            optionalParms.push_back(F.at(i));
        }
        modifiers.parseOptionalParms(optionalParms, LSAType::INCLUDE, lsaPath, &parseWarnings);
    }

    return;
}


bool LSAInclude::parseModifiers(std::vector<std::string> modifierStrings)
{
    modifiers.clearModifiers();
    // push the remaining parms onto a deque and parse them
    std::deque<std::string> optionalParms;
    for (size_t i = 0; i < modifierStrings.size(); ++i)
    {
        optionalParms.push_back(modifierStrings[i]);
    }

    return modifiers.parseOptionalParms(optionalParms, LSAType::INCLUDE, lsaPath, &parseWarnings);
}

//------------------------------------------------------------------------------------
// Output as a string (one line) for the LSA file.
// return string, a single line for a LSA file
void LSAInclude::writeLSAString(std::ostream& out) const
{
    if (isCommented)
    {
        out << "#";
    }

   std::string modifierString = modifiers.getLSAString();

   if (lsaPath.find(" ") != std::string::npos)
   {
       out << "--include " << "\"" << lsaPath << "\"";
   }
   else
   {
       out << "--include " << lsaPath;

   }

   if (!modifierString.empty())
   {
       out << modifierString;
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
std::string LSAInclude::getUIColumnData(int columnNumber) const
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

void LSAInclude::renameModifierLabel(LSAType lsaType, std::string oldLabel, std::string newLabel)
{
    modifiers.modifyLabel(lsaType, oldLabel, newLabel);

    return;
}

bool LSAInclude::referencesModifier(LSAType modifierType, std::string label) const
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
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAInclude::getUIName() const
{
    return lsaPath;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAInclude::getUIDetails() const
{
    return "";
}

// this ends the base class interface


bool LSAInclude::insertNewRecord(int position, LSARecord* newRecord)
{
    if (position > childRecords.size() )
    {
        return false;
    }

    std::list<LSARecord*>::iterator it = childRecords.begin();
    std::advance(it, position);
    childRecords.insert(it, newRecord);

    return true;
}

LSARecord* LSAInclude::getChildRecord(int position)
{
    std::list<LSARecord*>::iterator it = childRecords.begin();
    std::advance(it, position);

    return *it;
}

void LSAInclude::deleteChildRecord(int position)
{
    std::list<LSARecord*>::iterator it = childRecords.begin();
    std::advance(it, position);

    childRecords.erase(it);

    return;
}



//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
