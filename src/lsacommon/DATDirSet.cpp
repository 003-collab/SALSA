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
/// @file DATDirSet.cpp  Include file for class DATDirSet, data for the *.dat file
///                      DIRSET record, direction (angle) measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "DATDirSet.hpp"
#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATDirSet::DocString = string("DIRSET group labFrom [ac LUNIT] [CORR [htf htfs [LUNIT]] [GEOID]]");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATDirSet::fromString(const string& line)
{
   int n;
   vector<string> F;
   vector<double> corrections;
   static const string M("M"), RAD("RAD");

   string str(line);                // copy const line
   stripTrailing(str,"\n");
   stripTrailing(str,"\r");
   stripTrailing(str," ");
   stripLeading(str," ");

   if(str.empty()) return false;
   if(str[0] == '#') return false;

   // check record type
   //DIRSET group labFrom [ac LUNIT] [CORR [hta htas [LUNIT]] [GEOID]]
   if(str.substr(0,7) != string("DIRSET ")) return false;

   // strip off and save optional "CORR ..." at end of str
   string corrstr(str);
   string::size_type pos = corrstr.rfind("CORR");     // find last "CORR"
   if(pos == string::npos)
      corrstr = string();
   else {
      corrstr.erase(0,pos-1);                // reduce to just CORR ... EOL
      str.erase(pos,string::npos);           // remove from end of line
      stripTrailing(str," ");
      // parse corrstr below after parsing str
   }

   // split into fields
   F = splitWithDoubleQuotes(str,' ');
   n = F.size();

   //DIRSET group labFrom ac LUNIT             5
   if(n == 5 && isLinearUnit(F[4])) {
      *this = DATDirSet(F[1],F[2]);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[3]),F[4]));
   }

   //DIRSET group labFrom                      3
   else if(n == 3) {
      *this = DATDirSet(F[1],F[2]);
   }

   else return false;

   // parse the CORR string
   //CORR HtF HtFs LUNIT GEOID
   if(!corrstr.empty()) {
      double HtF(0.0), HtFs(0.0);
      F = split(corrstr,' ');        // F[0] must == "CORR"
      n = F.size();

      //CORR HtF HtFs LUNIT GEOID
      if(n == 5 && isLinearUnit(F[3]) && F[4]==string("GEOID")) {
         HtF = convertTo(M,asDouble(F[1]),F[3]);
         HtFs = convertTo(M,asDouble(F[2]),F[3]);
         setCorrections(HtF, HtFs, true);
      }
      //CORR HtF LUNIT GEOID
      else if(n == 4 && isLinearUnit(F[2]) && F[3]==string("GEOID")) {
         HtF = convertTo(M,asDouble(F[1]),F[2]);
         setCorrections(HtF, HtFs, true);
      }
      //CORR HtF HtFs LUNIT
      else if(n == 4 && isLinearUnit(F[3])) {
         HtF = convertTo(M,asDouble(F[1]),F[3]);
         HtFs = convertTo(M,asDouble(F[2]),F[3]);
         setCorrections(HtF, HtFs, false);
      }
      //CORR HtF LUNIT
      else if(n == 3 && isLinearUnit(F[2])) {
         HtF = convertTo(M,asDouble(F[1]),F[2]);
         setCorrections(HtF, HtFs, false);
      }
      //CORR GEOID
      else if(n == 2 && F[1]==string("GEOID")) {
         setCorrections(HtF, HtFs, true);
      }
      //CORR HtF
      else if(n == 2) {
         HtF = asDouble(F[1]);
         setCorrections(HtF, HtFs, false);
      }
      else
         return false;
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) for the DAT file.
// return string, a single line for a DAT file
string DATDirSet::asDATString(void) const
{
   //DIRSET group labFrom [ac LUNIT] [CORR [htf htfs [LUNIT]] [GEOID]]
   ostringstream oss;
   oss << "DIRSET " << addQuotes(DSGroup) << " " << addQuotes(From);

   if(hasFcsig) oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M)) << " " << Fcentersig << " M";

   if(hasHtFrom || isGeoid) oss << " CORR";
   if(hasHtFrom) oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M)) << " " << HtFrom << " " << HtFromSigma << " M";
   if(isGeoid) oss << " GEOID";

   //DIRSET is not tagged if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING) << datscale;

   return oss.str();
}

//------------------------------------------------------------------------------------
// Write the object as a 1-line readable string
// param prec integer number of digits precision (default 8)
// param width integer width (default 13)
// return string version of the object
string DATDirSet::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << DSGroup << " " << From << fixed << setprecision(prec);

   if(hasFcsig) oss << ", F-cent " << fixed << setprecision(3) << Fcentersig << " m";

   if(hasHtFrom || isGeoid)
      oss << fixed << setprecision(3)
         << " HtF " << (hasHtFrom ?  HtFrom : 0.0)
         << " HtFs " << (hasHtFrom ?  HtFromSigma : 0.0) << " M"
         << " " <<  (isGeoid ? "":"no ") << "geoid";

   if(!dattag.empty())
      oss << " [ignored:tag=" << fixed << setprecision(4) << dattag << "]";
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(4) << datscale;

   return oss.str();
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
