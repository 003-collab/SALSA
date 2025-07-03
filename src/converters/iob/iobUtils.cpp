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
/// @file IOBANGL.cpp  Class IOBANGL, data for the *.iob file record ANGL
/// horizontal angle measurement

#include <string>
#include <ostream>
#include <exception>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "iobUtils.hpp"

#include "iobconverter.hpp" // for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;


bool IOBRecordParser(const std::vector<int> fieldwidths,
                     const std::string& line, std::vector<std::string>& F,
                     const std::set<int> station_name_indexes)
{
    int ptr=0;
    string field, rectype;
    bool longtype = false;
    int longstationwidth = 31;
    int numfields = 0;

    for(int i=0;i<fieldwidths.size();i++)
        if(fieldwidths[i]>0) numfields++;

    string str(line);                // copy const line
    stripTrailing(str,"\n");
    stripTrailing(str,"\r");

    if(str.empty()) return false;
    // check record type
    rectype = str.substr(1,4);
    stripTrailing(rectype,' ');
    stripLeading(rectype,' ');

    // check if long or short record type
    if(str.substr(9,1) == string("*")) longtype = true;

    //split into fields
    for(int i=0;i<fieldwidths.size();i++) {
       if(ptr >= str.size()) {//deal with empty fields at end of line
           while(F.size()<numfields)
               F.push_back(std::string(""));
           break;
       }
       if(fieldwidths[i] < 0) {
           if((((rectype == string("AZIM") || rectype == string("GAZI") ||
               rectype == string("EHDF") || rectype == string("OHDF") ||
               rectype == string("VANG") || rectype == string("GVAN") ||
               rectype == string("ZANG") || rectype == string("GZAN") ||
               rectype == string("DIST")) && i==8) ||
               (rectype == string("DIR") && i==6)) && longtype)
               ptr += 1;
           else
               ptr += abs(fieldwidths[i]);
       }

       else {
          if((station_name_indexes.find(i) != station_name_indexes.end()) && longtype)
          {
              field = str.substr(ptr,longstationwidth);
              ptr += longstationwidth;
          }
          else
          {
             field = str.substr(ptr,fieldwidths[i]);
             ptr += fieldwidths[i];
          }
          field = stripTrailing(stripLeading(field," ")," ");
          F.push_back(field);
          field.clear();
       }
    }

    //do validation (addresses Bug #811)
    if(rectype == string("HI") || rectype == string("HT"))
    {
        bool validRecord = true;
        //HT_widths[] = {-1,4,-5,12,-1,10,-1,2};
        //check height value
        if( !isNumber(F[2]))
            validRecord = false;
        //check units
        else if(!isLinearUnit(F[3]) && !F[3].empty())//lack of units OK
            validRecord = false;
        //check that expected spaces are spaces
        else if( longtype && (str.size() > 41) && (str[41] != ' ') )
            validRecord = false;
        else if( longtype && (str.size() > 52) && (str[52] != ' ') )
            validRecord = false;
        else if( (str.size() > 22) && (str[22] != ' ') )
            validRecord = false;
        else if( (str.size() > 33) && (str[33] != ' ') )
            validRecord = false;

        if(!validRecord)
        {
            return false;
        }
    }

    return true;
}

std::string prettifyCovMat(const std::string cov_mat, bool isCommented)
{
    //pretty-up covariance matrix
    //assumes 6-element space-delimited array representing an upper-triangular matrix
    std::ostringstream pretty;
    std::vector<std::string> elems = splitWithDoubleQuotes(cov_mat, ' ');
    int colw=21;

    //handle extra-width cov matrix elements
    for(int i=0;i<elems.size();i++)
    {
        if(elems[i].size()>=colw)
            colw = elems[i].size() + 1;
    }

    if(isCommented)
    {
        pretty << " ...\n#"
               << left << setw(colw) << elems[0]
               << left << setw(colw) << elems[1]
               << left << setw(colw) << elems[2]
               << setw(5) << " ...\n#"
               << setw(colw) << " "
               << left << setw(colw) << elems[3]
               << left << setw(colw) << elems[4]
               << setw(5) << " ...\n#"
               << setw(2*colw) << " "
               << left << setw(colw) << elems[5];
    }
    else
    {
        pretty << " ...\n"
               << left << setw(colw) << elems[0]
               << left << setw(colw) << elems[1]
               << left << setw(colw) << elems[2]
               << setw(5) << " ...\n"
               << setw(colw) << " "
               << left << setw(colw) << elems[3]
               << left << setw(colw) << elems[4]
               << setw(5) << " ...\n"
               << setw(2*colw) << " "
               << left << setw(colw) << elems[5];
    }
    return pretty.str();
}

std::string parseELEMRecords(const std::string mat_type, const std::string mat_form, const int numELEMs, const std::string ELEMrecords[4])
{
    const int ELEM_widths[] = {-1,4,-1,23,-1,23,-1,23,-1,2};
    std::vector<std::string> E;
    std::string cmat, dummy;
    std::vector<int> efieldwidths (ELEM_widths, ELEM_widths + sizeof(ELEM_widths)/sizeof(int));
    std::set<int> station_name_indexes;

    for(int j=0;j<numELEMs;j++)
    {
       try
       {
           if(!IOBRecordParser(efieldwidths,ELEMrecords[j],E,station_name_indexes))
                throw(IOBException(ELEMrecords[j]));
           while(E.size()<(j+1)*5)//account for missing fields
           {
               if(E.size()==(j+1)*5-1)
                   E.push_back(std::string(""));
               else
                   E.push_back(std::string("0.0"));
           }
           if(mat_type == std::string("COV"))//COV matrix
           {
               if(mat_form == std::string("UPPR"))//expect 3 ELEMs
               {
                   if(j==0)
                       cmat = E[1] + " " + E[2] + " " + E[3] + " ";
                   else if(j==1)
                       cmat += E[6] + " " + E[7] + " ";
                   else
                       cmat += E[11];
               }
               else//DIAG - expect 1 ELEM
               {
                   cmat = E[1] + " 0.0 0.0 " + E[2] + " 0.0 " + E[3];
               }
           }
           else//CORR matrix.  Convert to COV matrix;
           {
               if(mat_form == std::string("UPPR"))//expect 4 ELEMs
               {
                   if(j==0)
                       dummy = gnsstk::StringUtils::asString(asDouble(E[1])*asDouble(E[2])*asDouble(E[3]));
                   else if(j==1)
                       dummy = gnsstk::StringUtils::asString(asDouble(E[6])*asDouble(E[7]));
                   else if(j==2)
                       dummy = gnsstk::StringUtils::asString(asDouble(E[11]));
                   else
                       //assumes upper triangular
                       //COV = diag(sigmas)*CORR*diag(sigmas) =
                       // |E[1]E[16]E[16]  E[2]E[16]E[17]  E[3]E[16]E[18]  |
                       // |     0          E[6]E[17]E[17]  E[7]E[17]E[18]  |
                       // |     0             0            E[11]E[18]E[18] |
                       // E[16],E[17],E[18] <-- sigmas
                       cmat = gnsstk::StringUtils::asString(asDouble(E[1])*asDouble(E[16])*asDouble(E[16])) + string(" ") +
                              gnsstk::StringUtils::asString(asDouble(E[2])*asDouble(E[16])*asDouble(E[17])) + string(" ") +
                              gnsstk::StringUtils::asString(asDouble(E[3])*asDouble(E[16])*asDouble(E[18])) + string(" ") +
                              gnsstk::StringUtils::asString(asDouble(E[6])*asDouble(E[17])*asDouble(E[17])) + string(" ") +
                              gnsstk::StringUtils::asString(asDouble(E[7])*asDouble(E[17])*asDouble(E[18])) + string(" ") +
                              gnsstk::StringUtils::asString(asDouble(E[11])*asDouble(E[18])*asDouble(E[18]));
               }
               else//DIAG - expect 2 ELEMs
               {
                   if(j==0)
                       dummy = gnsstk::StringUtils::asString(asDouble(E[1]));
                   else
                   //assumes upper triangular
                   //COV = diag(sigmas)*CORR*diag(sigmas) =
                   // |E[1]E[6]E[6]       0.0           0.0     |
                   // |     0        E[2]E[7]E[7]       0.0     |
                   // |     0             0        E[3]E[8]E[8] |
                   // E[6],E[7],E[8] <-- sigmas
                       cmat = gnsstk::StringUtils::asString(asDouble(E[1])*asDouble(E[6])*asDouble(E[6])) + string(" 0.0 0.0 ") +
                              gnsstk::StringUtils::asString(asDouble(E[2])*asDouble(E[7])*asDouble(E[7])) + string(" 0.0 ") +
                              gnsstk::StringUtils::asString(asDouble(E[3])*asDouble(E[8])*asDouble(E[8]));
               }
           }
        }
        catch(...)
        {
            throw(IOBException(ELEMrecords[j]));
        }
    }

    return cmat;
}

std::string parseCOVCORRRecords(const std::string& lastCrecord, const int cCounter, bool applyVSCA, int vscaTagNum, std::map<std::string,IOBVSCA*> &VSCARecords,
                                bool &unsupportedCOVScaling)
{
    const int COV_widths[] = {-1,4,-1,2,-1,4,-1,10,-1,10,-1,10,-1,10,-1,10,-1,10,-1,10,-1,2};
    std::vector<std::string> C;
    std::vector<int> cfieldwidths (COV_widths, COV_widths + sizeof(COV_widths)/sizeof(int));
    std::set<int> station_name_indexes;
    std::string crec;
    bool scaling = false;

    if(lastCrecord.empty())
        return string("");

    try
    {
        if(!IOBRecordParser(cfieldwidths,lastCrecord,C,station_name_indexes))
            throw IOBException(lastCrecord);

        //check if the COV record is actually doing anything or if it can be ignored
        //assumes UPPR for DXYZ records
        if(C.size()<4)
            return string("");

        //check for unsupported scaling elements in the COV/CORR record
        if((std::fabs(asDouble(C[3])) > 1E-9)       ||//add constant to entire matrix
           (std::fabs(asDouble(C[5])) > 1E-9)       ||//add constant to diagonal elements
           ((std::fabs(asDouble(C[6])-1.0) > 1E-9)  &&//scaling diagonal elements
            (std::fabs(asDouble(C[6])) > 1E-9))     ||//GeoLab convention of using zeros for unusused elements
           ((std::fabs(asDouble(C[7])-1.0) > 1E-9)  &&//PPM for diagonal elements
            (std::fabs(asDouble(C[7])) > 1E-9))     ||//GeoLab convention of using zeros for unusused elements
           (std::fabs(asDouble(C[8])) > 1E-9)       ||//add constant to Z/Up
           ((std::fabs(asDouble(C[9])-1.0) > 1E-9)  &&//scaling for Z/Up elements
            (std::fabs(asDouble(C[9])) > 1E-9))     )//GeoLab convention of using zeros for unusused elements
        {
            unsupportedCOVScaling = true;
        }

        //we are only supporting whole-matrix scaling
        if((std::fabs(asDouble(C[4])-1.0) < 1E-9) || //scaling by 1.0
           (std::fabs(asDouble(C[4])) < 1E-9)       )//GeoLab convention of using zeros for unusused elements
            return string("");

        //Nov 2015 - Decided to convert COVM records to VSCA records
        if(applyVSCA)
        {
            //handle case where both a VSCA and a COVM are applied to a record:
            //create a new VSCA record with the scale factor equal to the product of both
            double VScale=0.0;
            double CScale=0.0;
            //find the scaling value of the VSCA applied to the record this COVM modifies
            map<std::string,IOBVSCA*>::iterator it;

            //see if a SIGM record with this label has already been read
            for(it = VSCARecords.begin(); it != VSCARecords.end(); it++)
            {
                if(it->first == (std::string("VSCA") + gnsstk::StringUtils::asString(vscaTagNum)))
                {
                    VScale = gnsstk::StringUtils::asDouble(it->second->scale_factor);
                    break;
                }
            }

            CScale = gnsstk::StringUtils::asDouble(C[4]);

            crec = "VSCA COV" + gnsstk::StringUtils::asString(cCounter) + " " + gnsstk::StringUtils::asString(VScale*CScale);
        }
        else
            crec = "VSCA VALUE " + C[4];
    }
    catch(...)
    {
        throw(IOBException(lastCrecord));
    }

    return crec;
}

std::set<std::string> validLinearUnit()
{
    // from DATfile.hpp:# LUNIT means linear unit MM|M|CM|KM|FT
    std::string dumdum[] = {"mm","m","cm","km","ft"};
    const std::set<std::string> LUNIT(dumdum, dumdum + sizeof(dumdum) / sizeof(dumdum[0]));

    return LUNIT;
}


