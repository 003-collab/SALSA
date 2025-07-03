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
/// @file DATDir.cpp  Include file for class DATDir, data for the *.dat file
///                    DIR record, direction (angle) measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "DATDir.hpp"
#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATDir::DocString = string("DIR group labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [tc LUNIT] [CORR htt htts [LUNIT]]");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATDir::fromString(const string& line)
{
   int n;
   vector<string> F;
   vector<double> corrections;
   static const string M("M"), DMS("DMS"), RAD("RAD"), DEG("DEG");

   string str(line);                // copy const line
   stripTrailing(str,"\n");
   stripTrailing(str,"\r");
   strip(str);

   if(str.empty()) return false;
   if(str[0] == '#') return false;

   // check record type
   //DIR group labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [tc LUNIT] [CORR htt [LUNIT]]
   if(str.substr(0,4) != string("DIR ")) return false;

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
   //LOG(INFO) << "Parse DAT record >" << str << "< with n=" << n;

   //DIR group labTo angle AUNIT sig AUNIT tc LUNIT                 9
   if(n == 9 && isAngularUnit(F[4]) && isAngularUnit(F[6])
                                            && isLinearUnit(F[8]))
   {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[6]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[7]),F[8]));
   }

   //DIR group labTo angle sig asig AUNIT tc LUNIT                  9
   else if(n == 9 && isAngularUnit(F[6])) {
      value = convertTo(RAD,asDouble(F[3]),F[6]);
      sigma = convertTo(RAD,asDouble(F[4]),F[6]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[5]),F[6]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[7]),F[8]));
   }

   //DIR group labTo angle AUNIT sig asig AUNIT                     8
   else if(n == 8 && isAngularUnit(F[4]) && isAngularUnit(F[7])) {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[7]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[6]),F[7]));
   }

   //DIR group labTo angle sig AUNIT tc LUNIT                       8
   else if(n == 8 && isAngularUnit(F[5])) {
      value = convertTo(RAD,asDouble(F[3]),F[5]);
      sigma = convertTo(RAD,asDouble(F[4]),F[5]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[6]),F[7]));
   }

   //DIR group labTo angle AUNIT sig AUNIT                          7
   else if(n == 7 && isAngularUnit(F[4]) && isAngularUnit(F[6])) {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[6]);
      *this = DATDir(F[1],F[2],value,sigma);
   }

   //DIR group labTo angle sig asig AUNIT                           7
   else if(n == 7 && isAngularUnit(F[6])) {
      value = convertTo(RAD,asDouble(F[3]),F[6]);
      sigma = convertTo(RAD,asDouble(F[4]),F[6]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[5]),F[6]));
   }

   //DIR group labTo angle sig AUNIT                               6
   else if(n == 6 && isAngularUnit(F[5])) {
      value = convertTo(RAD,asDouble(F[3]),F[5]);
      sigma = convertTo(RAD,asDouble(F[4]),F[5]);
      *this = DATDir(F[1],F[2],value,sigma);
   }

   //DIR group labTo angle AUNIT sig asig AUNIT tc LUNIT           10
   else if(n == 10 && isAngularUnit(F[4]) && isAngularUnit(F[7])) {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[7]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[6]),F[7]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[8]),F[9]));
   }

   // DMS

   //DIR group labTo angle min sec DMS sig asig AUNIT tc LUNIT     12
   else if(n == 12 && F[6] == DMS && isAngularUnit(F[9]) && isLinearUnit(F[11])) {
       bool is_neg;
       if(F[3].at(0) == '-')
           is_neg = true;
       else
           is_neg = false;

      value = DMSToDeg(is_neg,abs(asInt(F[3])),asInt(F[4]),asDouble(F[5]));
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD,asDouble(F[7]),F[9]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[8]),F[9]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[10]),F[11]));
   }

   //DIR group labTo angle min sec DMS sig AUNIT tc LUNIT          11
   else if(n == 11 && F[6] == DMS && isAngularUnit(F[8]) && isLinearUnit(F[10])) {
      bool is_neg;
      if(F[3].at(0) == '-')
         is_neg = true;
      else
         is_neg = false;

      value = DMSToDeg(is_neg, abs(asInt(F[3])),asInt(F[4]),asDouble(F[5]));
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD,asDouble(F[7]),F[8]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[9]),F[10]));
   }

   //DIR group labTo angle min sec DMS sig asig AUNIT              10
   else if(n == 10 && F[6] == DMS && isAngularUnit(F[9])) {
      bool is_neg;
      if(F[3].at(0) == '-')
         is_neg = true;
      else
         is_neg = false;

      value = DMSToDeg(is_neg, abs(asInt(F[3])),asInt(F[4]),asDouble(F[5]));
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD,asDouble(F[7]),F[9]);
      *this = DATDir(F[1],F[2],value,sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[8]),F[9]));
   }

   //DIR group labTo angle min sec DMS sig AUNIT                   9
   else if(n == 9 && F[6] == DMS && isAngularUnit(F[8])) {
      bool is_neg;
      if(F[3].at(0) == '-')
         is_neg = true;
      else
         is_neg = false;

      value = DMSToDeg(is_neg, abs(asInt(F[3])),asInt(F[4]),asDouble(F[5]));
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD,asDouble(F[7]),F[8]);
      *this = DATDir(F[1],F[2],value,sigma);
   }

   else return false;

   // parse the CORR string
   //CORR HtT HtTs [LUNIT]
   if(!corrstr.empty()) {
      double HtT(0.0),HtTs(0.0);
      F = split(corrstr,' ');        // F[0] must == "CORR"
      n = F.size();


      //CORR HtT HtTs LUNIT
      if(n == 4 && isLinearUnit(F[3])) {
         HtT = convertTo(M,asDouble(F[1]),F[3]);
         HtTs = convertTo(M,asDouble(F[2]),F[3]);
         setCorrections(HtT,HtTs);
      }
      //CORR HtT LUNIT
      else if(n == 3 && isLinearUnit(F[2])) {
         HtT = convertTo(M,asDouble(F[1]),F[2]);
         setCorrections(HtT,HtTs);
      }
      //CORR HtT
      else if(n == 2) {
         HtT = asDouble(F[1]);
         setCorrections(HtT,HtTs);
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
string DATDir::asDATString(void) const
{
   //DIR group labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [tc LUNIT] [CORR htt htts [LUNIT]] tag=dattag
   ostringstream oss;
   oss << "DIR " << addQuotes(DSGroup) << " " << addQuotes(To)
      << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_RAD)) << " " << value << " RAD"
      << setprecision(getPrecisionForUnits(lsa::UNITS_SOA)) << " " << sigma*RAD_TO_SOA;

   if(hasAddsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_SOA)) << " " << addsig*RAD_TO_SOA;
   
   oss << " SOA";

   if(hasTcsig) oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M)) << " " << Tcentersig << " M";

   if(hasHtTo) oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M)) << " CORR " << HtTo << " " << HtToSigma << " M";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING) << datscale;

   return oss.str();
}

//------------------------------------------------------------------------------------
// Write the object as a 1-line readable string
// param prec integer number of digits precision (default 8)
// param width integer width (default 13)
// return string version of the object
string DATDir::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << DSGroup << " " << To << fixed << setprecision(prec) << " " << value
      << " = " << angleAsDMSstring(value) << " DMS, sig "
      << scientific << setprecision(3) << sigma << " rad = "
      << fixed << setprecision(2) << RAD_TO_SOA*sigma << " SOA";

   if(hasAddsig) oss << ", add-sig "
      << fixed << setprecision(prec) << addsig*RAD_TO_SOA << " SOA";

   if(hasTcsig)
      oss << ", T-cent " << fixed << setprecision(3) << Tcentersig << " m";

   if(hasHtTo)
      oss << fixed << setprecision(3) << " HtT " << HtTo << " HtTs " << HtToSigma << " m";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0)
      oss << " [ignored:scale=" << fixed << setprecision(4) << datscale << "]";

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
