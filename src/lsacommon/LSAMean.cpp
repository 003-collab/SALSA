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

#include "LSAMean.hpp"
#include "lsaUtils.hpp"
#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

// CONSTANTS
const int LSAMean::NUM_UI_DISPLAY_COLUMNS = 4;

//------------------------------------------------------------------------------------
// Parse a string from a single line in the LSA file.
// param line single line read from LSA file
// return true if successful
void LSAMean::fromString(const string& line)
{
    if (line.empty()) return;

    int numWords;
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
    numWords = F.size();

    // Make sure we have enough required fields to parse
    if (numWords < 5)
    {
        addNumFieldsWarning(LSAType::MEAN, label);
        return;
    }

    // new point label
    label = F[1];

    // extra std dev, the variance is added uniformly to each new covariance element
    Sigma = validateDoubleMin(F[2], LSAFieldType::SIGMA, 0.0);
    linUnits = validateString(F[3], LSAFieldType::SIGMA_UNITS, "cm,m,ft");

    // points
    int imin = 4;
    for (int i = imin; i < numWords; ++i)
    {
        points.push_back(F[i]);
    }

    return;
}

//------------------------------------------------------------------------------------
// Output as a string for the LSA file.
// param out, an ostream reference to the output filestream or stringstream
void LSAMean::writeLSAString(std::ostream & out) const
{
    std::string comment = isCommented ? "#" : "";

    // label
    out << comment << "MEAN " << addQuotes(label) << " ";

    //extrasigma and units
    int sigmaPrecision = grabNumDigits(getRecType(), linUnits);
    out << std::fixed << std::setprecision(sigmaPrecision);
    out << Sigma <<" "<< lowerCase(linUnits)<<" ";

    // points
    for (int i = 0; i < points.size(); ++i)
    {
        out << addQuotes(points[i]) << " ";
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
std::string LSAMean::getUIColumnData(int columnNumber) const
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
//        break;
    case 3:
        retVal = getUIDetails();
        break;
    }

    return retVal;
}

bool LSAMean::referencesPosition(const std::string& position) const
{
    bool positionFound = false;
    for (int i = 0; i < points.size(); ++i)
    {
        if( position == points[i])
        {
            positionFound = true;
            break;
        }
    }

    return positionFound;
}
/*
std::vector<std::string> LSAMean::getReferencedPositions() const
{
    std::vector<std::string> returnValues;
    returnValues.push_back(label);

    return returnValues;
}
*/
void LSAMean::renamePosition(std::string oldLabel, std::string newLabel)
{
    if (label == oldLabel)
        label = newLabel;

    for (int i = 0; i < points.size(); ++i)
    {
        if( oldLabel == points[i])
        {
            points[i] = newLabel;
        }
    }

    return;
}

void LSAMean::validatePositions(std::set<std::string> allPositionsIncludingAutogen, std::list<std::string> derivedPoints)
{
    // Check that the list of points to compute the mean is non-empty
    if(points.size() == 0)
    {
        std::string message = "No input positions provided.";
        warningMessages_generated.insert(message);
    }

    // Check that all input points exist
    for (size_t i = 0; i < points.size(); ++i)
    {
        std::string input = points.at(i);
        if (allPositionsIncludingAutogen.find(input) == allPositionsIncludingAutogen.end() &&
            std::find(derivedPoints.begin(), derivedPoints.end(), input) == derivedPoints.end())
        {
            std::string message = "Input position " + input + " does not exist.";
            warningMessages_generated.insert(message);
        }
    }

    // Check that the output point does not duplicate anything else in the project
    int ctr=0;
    std::list<std::string>::iterator itr;
    for(itr=derivedPoints.begin();itr!=derivedPoints.end();itr++)
    {
        if(*itr == label)
            ctr++;
    }
    if (allPositionsIncludingAutogen.find(label) != allPositionsIncludingAutogen.end() ||
        ctr > 1)
    {
        std::string message = "New point label " + label + " already exists in project.";
        warningMessages_generated.insert(message);
    }

    // Check that none of the input points are included multiple times
    for (size_t i = 0; i < points.size(); ++i)
    {
        std::string pointToCheck = points.at(i);
        for (size_t j = i + 1; j < points.size(); ++j)
        {
            std::string otherPoint = points.at(j);
            if ( pointToCheck == otherPoint )
            {
                std::string message = "Duplicate Input position: " + pointToCheck;
                warningMessages_generated.insert(message);
            }
        }
    }

    //check that none of the input points matches the new label
    for (size_t i = 0; i < points.size(); ++i)
    {
        std::string pointToCheck = points.at(i);
        if(pointToCheck == label)
        {
            std::string message = "New point label " + label + " matches the input position " + pointToCheck + ".";
            warningMessages_generated.insert(message);
            break;
        }
    }

    return;
}

//------------------------------------------------------------------------------------
// This record type doesn't have an asDATString function for when writing to the dat file. This
// function serves that purpose
// @return The string representation of the record without the Text Notes, for writing to the dat file.
string LSAMean::getSingleDATString() const
{
    std::string singleString = getSingleLSAString();
    extractXMLFromString(singleString, RecordTags::TextNotes, true);
    return singleString;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAMean::getUIName() const
{
    return label;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) to display in the UI. Pure virtual
// @return a string to display in the UI
std::string LSAMean::getUIDetails() const
{
    int numPoints = points.size();

    std::ostringstream oss;
    for (int i = 0; i < numPoints; ++i)
    {
        oss << points[i];
        if (i < numPoints-1)
            oss << ", ";
    }
    oss << " sig: "  << Sigma;

    return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
