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
/// @file IOBDXYZ.cpp  Class IOBDXYZ, data for the *.iob file record DXYZ
///                     3-D XYZ coordinate difference

#include <string>
#include <ostream>
#include <exception>
#include <cmath>//fabs

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "logstream.hpp"

#include "lsaUtils.hpp"
#include "IOBDXYZ.hpp"
#include "iobUtils.hpp"
#include "IOBHGHT.hpp"

#include "iobconverter.hpp"//for GlobalData

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string IOBDXYZ::DocString = std::string("Columns Description\n"
"002-005 DXYZ\n"
"010 (blank)\n"
"011-022 Name of from-station\n"
"024-035 Name of to-station\n"
"037-049 X-coordinate difference\n"
"051-063 Y-coordinate difference\n"
"065-077 Z-coordinate difference\n"
"079-080 Linear unit name\n\n"
"If column 010 has an *,\n\n"
"002-005 DXYZ\n"
"010 * (asterisk)\n"
"011-041 Name of from-station\n"
"043-073 Name of to-station\n"
"075-087 X-coordinate difference\n"
"089-101 Y-coordinate difference\n"
"103-115 Z-coordinate difference\n"
"117-118 Linear unit name\n");

const int IOBDXYZ::DXYZ_widths[] = {-1,4,-5,12,-1,12,-1,13,-1,13,-1,13,-1,2};

//------------------------------------------------------------------------------------
// Parse a string from a single line in the IOB file.
// param DXYZrecord single line containing DXYZ record from IOB file
// param SIGM_label label from last 3DD record
// param Crecord single line containing COV/CORR record from IOB file
// param cCounter number used to generate unique COVM tag
// param ELEMrecords vector of lines containing ELEM records from IOB file
// param applyVSCA bool on whether or not to apply VSCA
// param VSCAtagnim int for unique VSCA tag
// return true if successful
bool IOBDXYZ::fromString(const std::string& DXYZrecord, const std::string& SIGM_label,
                         const std::string& crec, const std::string cmat, const bool applyVSCA,
                         const int VSCAtagnum)
{
   vector<string> F;
   std::vector<int> fieldwidths (DXYZ_widths, DXYZ_widths + sizeof(DXYZ_widths)/sizeof(int));
   std::vector<string> HT_parts;
   std::set<int> station_name_indexes;

   station_name_indexes.insert(3);
   station_name_indexes.insert(5);
   try
   {
       if(!IOBRecordParser(fieldwidths,DXYZrecord,F,station_name_indexes))
           return false;

       //handle missing units
       if(F[6].empty()) F[6] = string("m");
   }
   catch (...)
   {
       throw(IOBException(DXYZrecord));
   }

   std::string modifiers;

   if(SIGM_label.empty())
     modifiers = std::string("");
   else
     modifiers = "UNCR " + addQuotes(SIGM_label);

   if(applyVSCA)
   {
       if(!crec.empty())//VSCA COV#
       {
          std::vector<string> parts = splitWithDoubleQuotes(crec,' ');
          if(SIGM_label.empty())
             modifiers = "VSCA " + parts[1];
          else
             modifiers += " VSCA " + parts[1];
       }
       else
       {
           if(!modifiers.empty())
             modifiers += std::string(" ");
           modifiers += std::string("VSCA VSCA") + gnsstk::StringUtils::asString(VSCAtagnum);
       }
   }
   else if(!crec.empty())//VSCA VALUE ###
   {
       if(SIGM_label.empty())
          modifiers = crec;
       else
          modifiers += " " + crec;
   }

   //convert vector of strings into the IOB class
   *this = IOBDXYZ(F[1],F[2],F[3],F[4],F[5],F[6],cmat,crec,modifiers);

   return true;
}
//------------------------------------------------------------------------------------
std::string IOBDXYZ::asLSAString(void) const
{
//## DXYZ <from_sta> <to_sta> <dX> <dY> <dZ> <m|km|ft> <cxx> <cxy> <cxz> <cyy> <cyz> <czz> [hfrom hto cm|m|ft] [optional tags]

   std::ostringstream oss;
   std::string unitstr;
   std::set<std::string> LUNIT = validLinearUnit();
   GlobalData& GD=GlobalData::Instance();

   unitstr = lowerCase(units);

   if(isCommented)
       oss << "#";

   oss << "DXYZ " << addQuotes(from) << " " << addQuotes(to) << " " << dx << " " << dy << " " << dz << " "
                 << unitstr << " " << prettifyCovMat(cov_mat,isCommented);

   if(!htfrom_tag.empty() || !htto_tag.empty())
   {
       oss << setw(5) << " ...\n";
       if(isCommented)
           oss << "#";
       if(!htfrom_tag.empty())
           oss << " HFROM " << addQuotes(htfrom_tag);
       if(!htto_tag.empty())
           oss << " HTO " << addQuotes(htto_tag);
   }

   if(isCommented)
   {
       if(!tags.empty())
          oss << " ...\n" << "#" << tags;
       if(cov_rec != string("Undef") && !cov_rec.empty() && (cov_rec.find("VALUE")==string::npos))
          oss << "\n" << "#" << cov_rec;
   }
   else
   {
        if(!tags.empty())
           oss << " ...\n" << tags;
        if(cov_rec != string("Undef") && !cov_rec.empty() && (cov_rec.find("VALUE")==string::npos))
           oss << "\n" << cov_rec;
   }
   //check that the units are supported
   if(LUNIT.find(unitstr) == LUNIT.end())
   {
       pLOGstrm = &GD.ofwarn;
       LOG(INFO) << "Warning - invalid units in record: " << oss.str() << ".";
       pLOGstrm = &GD.oflog;
       GD.invalidUnits = true;
   }

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
