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
#ifndef LSAINVPAIRDATA_H
#define LSAINVPAIRDATA_H

#include <vector>
#include <string>
#include <utility>
#include <map>

#include <QString>
#include <QFileInfo>

class LSAInvPairData
{
public:
    // Have the pair.first element be the from station
    // pair.second element be the to station. Shorthand
    // to save typing
    typedef std::pair<std::string, std::string> fromToPair;

    // Empty Constructor
    LSAInvPairData() {}

    // Constructor set the projectDir and projectName data members
    // and call performParse()
    LSAInvPairData(const QString &dirPath, const QString &projName);

    // Empty for now
    ~LSAInvPairData() {}

    // Method will rewrite the input config file conditioned that dataModified != false
    void writeConfig();

    // Adds inverse pair to list.
    void addPair(const fromToPair &pairToAdd);

    // Deletes pair from list. Work with index instead in case user wants to keep other duplicates
    // around
    bool deletePair(unsigned int deleteIndex);

    // Checks to see if parameter already exists in stationPairList. Return true if it does
    // false otherwise
    bool isDuplicate(const fromToPair &provPair) const;

    // Returns a list of all duplicate station pairs
    std::vector<fromToPair> giveDuplicateList() const;

    // Returns the full name of the inv config file.
    std::string giveFullFileString() const;

    // Returns copy of LSAInvPairData::stationPairList
    std::vector<fromToPair> giveStationPairList() const {return  stationPairList;}

    // Returns value of LSAInvPairData::dataModified
    bool getDataModified() const {return dataModified;}

    void clear();

private:
    // Container with a list of all the station pair points.
    std::vector<fromToPair> stationPairList;

    // Container which counts all occurrances of a station pair
    std::map<fromToPair, unsigned int> stationPairCounter;

    // Boolean that keeps track of whether data in stationPairList has been modified
    // from the original config file. If data has been added or deleted without those changes
    // being reflected to input config file then this should have a value of true.
    // If the two are in sync this should have a value of false.
    bool dataModified = false;

    QString directory;
    QString projectName;
    QFileInfo fullFileInfo;

    // Method that will parse the data from the input config file and fill in the data for stationPairList
    void performParse();
};

#endif // LSAINVPAIRDATA_H
