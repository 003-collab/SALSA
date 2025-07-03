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
/// @file DATHeight.cpp  Include file for class DATHeight, data for the *.dat file
///                    HGT record, orthometric or geodetic height measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "DATHeight.hpp"
//#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATHeight::DocString = string("HGT labFrom labTo height sig [asig [PPM]] LUNIT [CORR [refract] [CURV] [OHC] [GEOID]]");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATHeight::fromString(const string& line)
{
   int n;
   vector<string> F;
   vector<double> corrections;
   static const string M("M"), PPM("PPM");

   string str(line);                // copy const line
   stripTrailing(str,"\n");
   stripTrailing(str,"\r");
   stripTrailing(str," ");
   stripLeading(str," ");

   if(str.empty()) return false;
   if(str[0] == '#') return false;

   // check record type
   // HGT labFrom labTo height sig [asig [PPM]] LUNIT
   if(str.substr(0,4) != string("HGT ")) return false;

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

   //HGT labFrom labTo height sig asig PPM LUNIT  8 fields
   if(n == 8 && F[6]==PPM && isLinearUnit(F[7])) {
      value = convertTo(M,asDouble(F[3]),F[7]);
      sigma = convertTo(M,asDouble(F[4]),F[7]);
      *this = DATHeight(F[1], F[2], value, sigma);
      // add additional sigma, in PPM
      setAddSigma(asDouble(F[5]), true);
   }

   //HGT labFrom labTo height sig asig     LUNIT   7
   else if(n == 7 && isLinearUnit(F[6])) {
      value = convertTo(M,asDouble(F[3]),F[6]);
      sigma = convertTo(M,asDouble(F[4]),F[6]);
      *this = DATHeight(F[1], F[2], value, sigma);
      // add additional sigma, in LUNITs
      setAddSigma(convertTo(M,asDouble(F[5]),F[6]), false);
   }

   // NB this must precede the next one - they differ only in PPM
   //HGT labFrom labTo height sig asig PPM             LUNIT   8
   else if(n == 8 && F[6]==PPM && isLinearUnit(F[7])) {
      value = convertTo(M,asDouble(F[3]),F[7]);
      sigma = convertTo(M,asDouble(F[4]),F[7]);
      *this = DATHeight(F[1], F[2], value, sigma);
      // add additional sigma, in PPM
      setAddSigma(asDouble(F[5]), true);
   }

   //HGT labFrom labTo height sig          LUNIT   6
   else if(n == 6 && isLinearUnit(F[5])) {
      value = convertTo(M,asDouble(F[3]),F[5]);
      sigma = convertTo(M,asDouble(F[4]),F[5]);
      *this = DATHeight(F[1], F[2], value, sigma);
   }

   //HGT labFrom labTo height sig asig                 LUNIT   7
   else if(n == 7 && isLinearUnit(F[6])) {
      value = convertTo(M,asDouble(F[3]),F[6]);
      sigma = convertTo(M,asDouble(F[4]),F[6]);
      *this = DATHeight(F[1], F[2], value, sigma);
      // add additional sigma, in LUNITs
      setAddSigma(convertTo(M,asDouble(F[5]),F[6]), false);
   }

   //HGT labFrom labTo height sig                      LUNIT   6
   else if(n == 6 && isLinearUnit(F[5])) {
      value = convertTo(M,asDouble(F[3]),F[5]);
      sigma = convertTo(M,asDouble(F[4]),F[5]);
      *this = DATHeight(F[1], F[2], value, sigma);
   }

   else return false;

   // parse the CORR string
   //CORR [refract] [CURV] [OHC] [GEOID]
   if(!corrstr.empty()) {
      F = split(corrstr,' ');        // F[0] must == "CORR"
      n = F.size();
      if(n < 2 || n > 5) return false;

      double refract(0.0);
      bool curv(false),ohc(false),geo(false);

      // First entry can be [refract] [CURV], [OHC] or [GEOID]
      if(F[1] == string("CURV")) curv=true;
      else if(F[1] == string("OHC")) ohc=true;
      else if(F[1] == string("GEOID")) geo=true;
      // TODO Better error handling if F[1] is not a double, nor CURV/OHC/GEOID
      else refract = asDouble(F[1]);
      
      for(int i=2; i<n; i++)         // scan other entries for CURV,OHC,GEOID flags
         if(F[i] == string("CURV")) curv=true;
         else if(F[i] == string("OHC")) ohc=true;
         else if(F[i] == string("GEOID")) geo=true;
         else return false;

      setCorrections(refract,curv,ohc,geo);
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) for the DAT file.
// return string, a single line for a DAT file
string DATHeight::asDATString(void) const
{
   //HGT labFrom labTo height sig [asig [PPM]] [fcsig tcsig ] LUNIT [CORR refract [CURV] [OHC] [GEOID]]
   ostringstream oss;
   oss << "HGT " << addQuotes(From) << " " << addQuotes(To) << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
      << " " << value << scientific << setprecision(lsa::NUM_DECIMALS_COVARIANCE) << " " << sigma;

   if(hasAddsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M)) << " " << addsig << (isPPM ? " PPM":"");

   oss << " M";

   if(hasRefract || isCurvature || isOHC || isGeoid)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
         << " CORR " << (hasRefract ? refract : 0.0)
         << (isCurvature ? " CURV":"")
         << (isOHC ? " OHC":"")
         << (isGeoid ? " GEOID":"");

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING) << datscale;

   return oss.str();
}

//------------------------------------------------------------------------------------
// Write the object as a 1-line readable string
// param prec integer number of digits precision (default 3)
// param width integer width (default 8)
// return string version of the object
string DATHeight::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << From << "-" << To << fixed << setprecision(prec)
      << " is " << value
      << " with sig " << scientific << setprecision(3) << sigma;

   if(hasAddsig)
      oss << ", add-sig " << fixed << addsig << (isPPM ? " PPM" : " m");

   if(hasRefract || isCurvature || isOHC || isGeoid)
      oss << fixed << setprecision(4)
         << ", refr " << (hasRefract ?  refract : 0.0)
         << ", " << (isCurvature ? "":"do not ") << "apply curvature"
         << ", " << (isOHC ? "":"do not ") << "apply ohc"
         << ", " <<  (isGeoid ? "":"do not ") << "apply geoid";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(4) << datscale;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

