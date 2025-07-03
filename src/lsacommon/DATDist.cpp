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
/// @file DATDist.cpp  Class DATDist, data for the *.dat file record DIS,
///                    distance measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "DATDist.hpp"
//#include "logstream.hpp"      // TEMP

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATDist::DocString = string("DIS labFrom labTo distance sig [asig [PPM]] [fcsig tcsig] LUNIT [CORR htf htt htfs htts [LUNIT]]");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATDist::fromString(const string& line)
{
   int n;
   string::size_type pos;
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
   //DIS labFrom labTo distance sig [asig [PPM]] [FCsig TCsig ] LUNIT
   if(str.substr(0,4) != string("DIS ")) return false;

   // strip off and save optional "CORR ..." at end of str
   string corrstr(str);
   pos = corrstr.rfind("CORR");     // find last "CORR"
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

   //DIS labFrom labTo distance sig asig PPM FCsig TCsig LUNIT  10 fields
   if(n == 10 && F[6]==PPM && isLinearUnit(F[9])) {
      value = convertTo(M,asDouble(F[3]),F[9]);
      sigma = convertTo(M,asDouble(F[4]),F[9]);
      *this = DATDist(F[1],F[2],value,sigma);
      // add additional sigma, in PPM
      setAddSigma(asDouble(F[5]), true);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[7]),F[9]),   // From Cent Err
                      convertTo(M,asDouble(F[8]),F[9]));  // To Cent Err
   }
   //DIS labFrom labTo distance sig asig     FCsig TCsig LUNIT   9
   else if(n == 9 && isLinearUnit(F[8])) {
      value = convertTo(M,asDouble(F[3]),F[8]);
      sigma = convertTo(M,asDouble(F[4]),F[8]);
      *this = DATDist(F[1],F[2],value,sigma);
      // add additional sigma, in LUNITs
      setAddSigma(convertTo(M,asDouble(F[5]),F[8]), false);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[6]),F[8]),   // From Cent Err
                      convertTo(M,asDouble(F[7]),F[8]));  // To Cent Err
   }
   //DIS labFrom labTo distance sig asig PPM             LUNIT   8
   // NB this must precede the next - they differ only in PPM
   else if(n == 8 && F[6]==PPM && isLinearUnit(F[7])) {
      value = convertTo(M,asDouble(F[3]),F[7]);
      sigma = convertTo(M,asDouble(F[4]),F[7]);
      *this = DATDist(F[1],F[2],value,sigma);
      // add additional sigma, in PPM
      setAddSigma(asDouble(F[5]), true);
   }
   //DIS labFrom labTo distance sig          FCsig TCsig LUNIT   8
   else if(n == 8 && isLinearUnit(F[7])) {
      value = convertTo(M,asDouble(F[3]),F[7]);
      sigma = convertTo(M,asDouble(F[4]),F[7]);
      *this = DATDist(F[1],F[2],value,sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[5]),F[7]),   // From Cent Err
                      convertTo(M,asDouble(F[6]),F[7]));  // To Cent Err
   }
   //DIS labFrom labTo distance sig asig                 LUNIT   7
   else if(n == 7 && isLinearUnit(F[6])) {
      value = convertTo(M,asDouble(F[3]),F[6]);
      sigma = convertTo(M,asDouble(F[4]),F[6]);
      *this = DATDist(F[1],F[2],value,sigma);
      // add additional sigma, in LUNITs
      setAddSigma(convertTo(M,asDouble(F[5]),F[6]), false);
   }
   //DIS labFrom labTo distance sig                      LUNIT   6
   else if(n == 6 && isLinearUnit(F[5])) {
      value = convertTo(M,asDouble(F[3]),F[5]);
      sigma = convertTo(M,asDouble(F[4]),F[5]);
      *this = DATDist(F[1],F[2],value,sigma);
   }
   else return false;

   // parse the CORR string
   //CORR HtF HtT HtFs HtTs [LUNIT]
   if(!corrstr.empty()) {
      double HtF(0.0),HtT(0.0),HtFs(0.0),HtTs(0.0);
      F = split(corrstr,' ');        // F[0] must == "CORR"
      n = F.size();

      //CORR HtF HtT HtFs HtTs LUNIT
      if(n == 6 && isLinearUnit(F[5])) {
         HtF = convertTo(M,asDouble(F[1]),F[5]);
         HtT = convertTo(M,asDouble(F[2]),F[5]);
         HtFs = convertTo(M,asDouble(F[3]),F[5]);
         HtTs = convertTo(M,asDouble(F[4]),F[5]);
         setCorrections(HtF, HtT, HtFs, HtTs);
      }
      //CORR HtF HtT LUNIT
      else if(n == 4 && isLinearUnit(F[3])) {
         HtF = convertTo(M,asDouble(F[1]),F[3]);
         HtT = convertTo(M,asDouble(F[2]),F[3]);
         setCorrections(HtF, HtT, HtFs, HtTs);
      }
      //CORR HtF HtT
      else if(n == 3) {
         HtF = asDouble(F[1]);
         HtT = asDouble(F[2]);
         setCorrections(HtF, HtT, HtFs, HtTs);
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
string DATDist::asDATString(void) const
{
   //DIS labFrom labTo distance sig [asig [PPM]] [fcsig tcsig] LUNIT [CORR htf htt htfs htts[LUNIT]]
   ostringstream oss;
   oss << "DIS " << addQuotes(From) << " " << addQuotes(To) << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
      << " " << value << scientific
      << " " << sigma;

   if(hasAddsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M)) << " " << addsig << (isPPM ? " PPM":"");

   if(hasFcsig || hasTcsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
         << " " << (hasFcsig ?  Fcentersig : 0.0)
         << " " << (hasTcsig ?  Tcentersig : 0.0);

   oss << " M";

   if(hasHtFrom || hasHtTo)
         oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
            << " CORR " << (hasHtFrom ?  HtFrom : 0.0)
            << " " << (hasHtTo ?  HtTo : 0.0) << " " << (hasHtFrom ?  HtFromSigma : 0.0) << " " << (hasHtTo ?  HtToSigma : 0.0) << " M";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING) << datscale;

   return oss.str();
}

//------------------------------------------------------------------------------------
// Write the object as a 1-line readable string
// param prec integer number of digits precision (default 3)
// param width integer width (default 8)
// return string version of the object
string DATDist::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << From << "-" << To << fixed << setprecision(prec)
      << " is " << value
      << " with sig " << scientific << setprecision(3) << sigma;

   if(hasAddsig)
      oss << ", add-sig " << fixed << addsig << (isPPM ? " PPM" : " m");

   if(hasFcsig || hasTcsig)
      oss << ", F,T-cent " << fixed << Fcentersig << " " << Tcentersig << " m";

   if(hasHtFrom || hasHtTo)
      oss << fixed << setprecision(3)
         << " HtF " << (hasHtFrom ?  HtFrom : 0.0)
         << " HtT " << (hasHtTo ?  HtTo : 0.0)
         << " HtFs " << (hasHtFrom ?  HtFromSigma : 0.0)
         << " HtTs " << (hasHtTo ?  HtToSigma : 0.0) << " m";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(4) << datscale;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
