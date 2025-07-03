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
#ifndef IOBUTILS_HPP
#define IOBUTILS_HPP

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <map>//for parseCOVCORRRecords

#include "Exception.hpp"
#include "IOBVSCA.hpp"//for parseCOVCORRRecords
bool IOBRecordParser(const std::vector<int> fieldwidths,
                     const std::string& line, std::vector<std::string>& F,
                     const std::set<int> station_name_indexes);

std::string prettifyCovMat(const std::string cov_mat, bool isCommented);

std::string parseELEMRecords(const std::string mat_type, const std::string mat_form, const int numELEMs, const std::string ELEMrecords[4]);
std::string parseCOVCORRRecords(const std::string& lastCrecord, const int cCounter, bool applyVSCA, int vscaTagNum,
                                std::map<std::string,IOBVSCA*> &VSCARecords, bool &unsupportedCOVScaling);
std::set<std::string> validLinearUnit();

class IOBException: public std::exception {
private:
public:
    std::string record;
    IOBException(const std::string& rec) : record(rec) {}
    ~IOBException() throw() { }
    virtual const char* what() const throw() {
        return record.c_str();
    }
};

#endif // IOBUTILS_HPP
