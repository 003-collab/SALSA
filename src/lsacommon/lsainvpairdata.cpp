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
#include <QDir>

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <iostream>

#include "lsaUtils.hpp"
#include "lsainvpairdata.h"

#include "StringUtils.hpp"

// Test comment


LSAInvPairData::LSAInvPairData(const QString &dirPath, const QString &projName)
    : directory(dirPath), projectName(projName)
{
    fullFileInfo = QFileInfo(QDir(directory), projectName + "Inverses.cfg");
    performParse();
}

void LSAInvPairData::writeConfig()
{
    // Avoid overwriting and giving new time stamp if there have been no modifications
    // to station pair data from original config file.
    if(!dataModified)
    {
        return;
    }

    std::string fileName = giveFullFileString();

    std::ofstream outFile(fileName);
    if(!outFile.is_open())
    {
        throw std::runtime_error("Error - could not open " + fileName + " for editing.");
    }

    for(const auto &pairElem : stationPairList)
    {
        outFile << addQuotes(pairElem.first) << " " << addQuotes(pairElem.second) << std::endl;
    }

    outFile.close();

    dataModified = false;
}

void LSAInvPairData::addPair(const LSAInvPairData::fromToPair &pairToAdd)
{
    //addTop ? stationPairList.insert(stationPairList.begin(), pairToAdd) : stationPairList.push_back(pairToAdd);
    stationPairList.push_back(pairToAdd);

    std::map<fromToPair, unsigned int>::iterator pairCounterIter = stationPairCounter.find(pairToAdd);

    if(pairCounterIter == stationPairCounter.end())
    {
        stationPairCounter[pairToAdd] = 1;
    }

    else
    {
        pairCounterIter->second++;
    }

    dataModified = true;
}

bool LSAInvPairData::deletePair(unsigned int deleteIndex)
{
    unsigned int listSize = static_cast<unsigned int>(stationPairList.size());

    if(deleteIndex >= listSize)
    {
        return false;
    }

    fromToPair pairToDelete = stationPairList[deleteIndex];
    std::map<fromToPair, unsigned int>::iterator pairCounterIter = stationPairCounter.find(pairToDelete);

    // Return statements indicate something was wrong in setting the data. If at this point the
    // pair to delete exists in LSAInvPairData::stationPairList but not LSAInvPairData::stationPairCounter
    // may want to handle differently via exception or something else. For now just return false.
    if(pairCounterIter != stationPairCounter.end())
    {
        if(pairCounterIter->second != 0)
        {
            pairCounterIter->second -= 1;
        }

        else
        {
            return  false;
        }
    }

    else
    {
        return false;
    }

    stationPairList.erase(stationPairList.begin() + deleteIndex);
    dataModified = true;
    return true;
}

bool LSAInvPairData::isDuplicate(const LSAInvPairData::fromToPair &provPair) const
{
    std::map<fromToPair, unsigned int>::const_iterator pairCounterIter = stationPairCounter.find(provPair);

    if(pairCounterIter == stationPairCounter.end())
    {
        return false;
    }

    else if(pairCounterIter->second <= 1)
    {
        return false;
    }

    else
    {
        return true;
    }
}

std::vector<LSAInvPairData::fromToPair> LSAInvPairData::giveDuplicateList() const
{
    std::vector<fromToPair> duplicateStations;

    for(const auto &pairCounterElem : stationPairCounter)
    {
        if(pairCounterElem.second > 1)
        {
            duplicateStations.push_back(pairCounterElem.first);
        }
    }

    return duplicateStations;
}

std::string LSAInvPairData::giveFullFileString() const
{
    return fullFileInfo.absoluteFilePath().toStdString();
}

void LSAInvPairData::clear()
{
    stationPairList.clear();
    stationPairCounter.clear();
    dataModified = false;
}

void LSAInvPairData::performParse()
{
    std::string fullFileName = giveFullFileString();

    std::ifstream inputFile(fullFileName);

    // May want to include option to throw warning in case
    // it's expected that file exists.
    if(!inputFile.is_open())
    {
        return;
    }

    std::string lineToParse;
    std::vector<std::string> parsedResults;
    fromToPair currentPair;
    unsigned int parsedSize;

    while(!inputFile.eof())
    {
        std::getline(inputFile, lineToParse);
        parsedResults = gnsstk::StringUtils::splitWithDoubleQuotes(lineToParse);

        parsedSize = static_cast<unsigned int>(parsedResults.size());
        switch (parsedSize) {
        case 0:
            currentPair.first = "";
            currentPair.second = "";
            break;
        case 1:
            currentPair.first = parsedResults[0];
            currentPair.second = "";
            break;
        default:
            currentPair.first = parsedResults[0];
            currentPair.second = parsedResults[1];
            // Fix for Issue #176
            gnsstk::StringUtils::stripTrailing(currentPair.second, '\r');
            break;
        }

        stationPairList.push_back(currentPair);
    }

    inputFile.close();

    std::vector<fromToPair>::const_reverse_iterator lastPlaceIter = stationPairList.rbegin();
    if(lastPlaceIter->first.empty() && lastPlaceIter->second.empty())
    {
        stationPairList.pop_back();
    }

    for(const auto &stationPair : stationPairList)
    {
        unsigned int count = static_cast<unsigned int>(std::count(stationPairList.begin(), stationPairList.end(), stationPair));
        stationPairCounter[stationPair] = count;
    }
}
