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
/// @file DATHAngle.cpp  Class DATHAngle, data for the *.dat file
///                      record HAN horizontal angle

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "DATHAngle.hpp"
#include "lsaUtils.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATHAngle::DocString = string("HAN labF labA labT angle [min sec DMS|AUNIT] sig [asig] AUNIT [fc ac tc LUNIT] [CORR htf htt htfs htts [LUNIT] [GEOID]]");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATHAngle::fromString(const string& line)
{
   int n;
   vector<string> F;
   vector<double> corrections;
   static const string M("M"), DMS("DMS"), RAD("RAD"), DEG("DEG");

   string str(line);                // copy const line
   stripTrailing(str,"\n");
   stripTrailing(str,"\r");
   stripTrailing(str," ");
   stripLeading(str," ");

   if(str.empty()) return false;
   if(str[0] == '#') return false;

   // check record type
   //HAN labF labA labT angle [min sec DMS|AUNIT] sig [asig] AUNIT [FC AC TC LUNIT]
   if(str.substr(0,4) != string("HAN ")) return false;

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

   //HAN labF labA labT angle min sec DMS sig asig     AUNIT FC AC TC LUNIT  15
   if(n == 15 && F[7]==DMS && isAngularUnit(F[10]) && isLinearUnit(F[14]))
   {
      bool is_neg;
      if(F[4].at(0) == '-')
         is_neg = true;
      else
         is_neg = false;
      value = DMSToDeg(is_neg,abs(asInt(F[4])),asInt(F[5]),asDouble(F[6]));
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD, asDouble(F[8]), F[10]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
      // add additional sigma, in AUNITs -> RAD
      setAddSigma(convertTo(RAD,asDouble(F[9]),F[10]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[11]),F[14]),    // From Cent Err
                      convertTo(M,asDouble(F[12]),F[14]),    // At Cent Err
                      convertTo(M,asDouble(F[13]),F[14]));   // To Cent Err
   }
   //HAN labF labA labT angle min sec DMS sig          AUNIT FC AC TC LUNIT  14
   else if(n == 14 && F[7]==DMS && isAngularUnit(F[9]) && isLinearUnit(F[13])) {
      bool is_neg;
      if(F[4].at(0) == '-')
         is_neg = true;
      else
         is_neg = false;
      value = DMSToDeg(is_neg,abs(asInt(F[4])),asInt(F[5]),asDouble(F[6]));
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD, asDouble(F[8]), F[9]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[10]),F[13]),     // From Cent Err
                      convertTo(M,asDouble(F[11]),F[13]),     // At Cent Err
                      convertTo(M,asDouble(F[12]),F[13]));    // To Cent Err
   }
   //HAN labF labA labT angle min sec DMS sig          AUNIT                 10
   else if(n == 10 && F[7]==DMS && isAngularUnit(F[9])) {
      bool is_neg;
      if(F[4].at(0) == '-')
         is_neg = true;
      else
         is_neg = false;
      value = DMSToDeg(is_neg, abs(asInt(F[4])),asInt(F[5]),asDouble(F[6]));
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD, asDouble(F[8]), F[9]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
   }

   // not DMS

   //HAN labF labA labT angle             sig asig     AUNIT FC AC TC LUNIT  12
   else if(n == 12 && isAngularUnit(F[7]) && isLinearUnit(F[11])) {
      value = convertTo(RAD, asDouble(F[4]), F[7]);
      sigma = convertTo(RAD, asDouble(F[5]), F[7]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
      // add additional sigma, in AUNITs -> RAD
      setAddSigma(convertTo(RAD,asDouble(F[6]),F[7]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[ 8]),F[11]),    // From Cent Err
                      convertTo(M,asDouble(F[ 9]),F[11]),    // At Cent Err
                      convertTo(M,asDouble(F[10]),F[11]));   // To Cent Err
   }
   //HAN labF labA labT angle             sig          AUNIT FC AC TC LUNIT  11
   else if(n == 11 && isAngularUnit(F[6]) && isLinearUnit(F[10])) {
      value = convertTo(RAD, asDouble(F[4]), F[6]);
      sigma = convertTo(RAD, asDouble(F[5]), F[6]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[ 7]),F[10]),    // From Cent Err
                      convertTo(M,asDouble(F[ 8]),F[10]),    // At Cent Err
                      convertTo(M,asDouble(F[ 9]),F[10]));   // To Cent Err
   }
   //HAN labF labA labT angle             sig          AUNIT                  7
   else if(n == 7 && isAngularUnit(F[6])) {
      value = convertTo(RAD, asDouble(F[4]), F[6]);
      sigma = convertTo(RAD, asDouble(F[5]), F[6]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
   }
   //HAN labF labA labT angle AUNIT       sig asig     AUNIT FC AC TC LUNIT  13
   else if(n == 13 && isAngularUnit(F[5]) && isAngularUnit(F[8])
                   && isLinearUnit(F[12]))
   {
      value = convertTo(RAD, asDouble(F[4]), F[5]);
      sigma = convertTo(RAD, asDouble(F[6]), F[8]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
      // add additional sigma, in AUNITs -> RAD
      setAddSigma(convertTo(RAD,asDouble(F[7]),F[8]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[ 9]),F[12]),    // From Cent Err
                      convertTo(M,asDouble(F[10]),F[12]),    // At Cent Err
                      convertTo(M,asDouble(F[11]),F[12]));   // To Cent Err
   }
   //HAN labF labA labT angle AUNIT       sig          AUNIT FC AC TC LUNIT  12
   else if(n == 12 && isAngularUnit(F[5]) && isAngularUnit(F[7])
                   && isLinearUnit(F[10]))
   {
      value = convertTo(RAD, asDouble(F[4]), F[5]);
      sigma = convertTo(RAD, asDouble(F[6]), F[7]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[ 8]),F[11]),    // From Cent Err
                      convertTo(M,asDouble(F[ 9]),F[11]),    // At Cent Err
                      convertTo(M,asDouble(F[10]),F[11]));   // To Cent Err
   }
   //HAN labF labA labT angle AUNIT       sig          AUNIT                  8
   else if(n == 8 && isAngularUnit(F[5]) && isAngularUnit(F[7])) {
      value = convertTo(RAD, asDouble(F[4]), F[5]);
      sigma = convertTo(RAD, asDouble(F[6]), F[7]);
      *this = DATHAngle(F[1],F[2],F[3], value, sigma);
   }

   else return false;

   // parse the CORR string
   //CORR HtF HtT HtFs HtTs [LUNIT] [GEOID]
   if(!corrstr.empty()) {
      // first find GEOID
      bool geo(false);
      pos = corrstr.rfind("GEOID");     // find last "GEOID"
      if(pos != string::npos) {
         geo = true;
         corrstr.erase(pos);            // remove GEOID
         stripTrailing(corrstr," ");
      }

      double HtF(0.0), HtT(0.0), HtFs(0.0), HtTs(0.0);
      F = split(corrstr,' ');        // F[0] must == "CORR"
      n = F.size();


      //CORR HtF HtT HtFs HtTs LUNIT
      if(n == 6 && isLinearUnit(F[5])) {
         HtF = convertTo(M,asDouble(F[1]),F[5]);
         HtT = convertTo(M,asDouble(F[2]),F[5]);
         HtFs = convertTo(M,asDouble(F[3]),F[5]);
         HtTs = convertTo(M,asDouble(F[4]),F[5]);
         setCorrections(HtF, HtT, HtFs, HtTs, geo);
      }
      //CORR HtF HtT LUNIT
      else if(n == 4 && isLinearUnit(F[3])) {
         HtF = convertTo(M,asDouble(F[1]),F[3]);
         HtT = convertTo(M,asDouble(F[2]),F[3]);
         setCorrections(HtF, HtT, HtFs, HtTs, geo);
      }
      //CORR HtF HtT
      else if(n == 3) {
         HtF = asDouble(F[1]);
         HtT = asDouble(F[2]);
         setCorrections(HtF, HtT,HtFs, HtTs, geo);
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
string DATHAngle::asDATString(void) const
{
   //HAN labF labA labT angle [min sec DMS|AUNIT] sig [asig] AUNIT [FC AC TC LUNIT] [CORR HtF HtT HtFs HtTs [LUNIT] [GEOID]]
   ostringstream oss;
   oss << "HAN " << addQuotes(From) << " " << addQuotes(At) << " " << addQuotes(To)
      << fixed << " " << angleAsDMSstring(value) << " DMS"
      << setprecision(getPrecisionForUnits(lsa::UNITS_SOA)) << " " << sigma*RAD_TO_SOA;

   if(hasAddsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_SOA)) << " " << addsig*RAD_TO_SOA;
   
   oss << " SOA";

   if(hasFcsig || hasTcsig || hasAcsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
         << " " << (hasFcsig ?  Fcentersig : 0.0)
         << " " << (hasAcsig ?  Acentersig : 0.0)
         << " " << (hasTcsig ?  Tcentersig : 0.0) << " M";

   if(hasHtFrom || hasHtTo || isGeoid)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
         << " CORR " << (hasHtFrom ?  HtFrom : 0.0)
         << " " << (hasHtTo ?  HtTo : 0.0)
         << " " << (hasHtFrom ?  HtFromSigma : 0.0)
         << " " << (hasHtTo ?  HtToSigma : 0.0) << " M"
         << (isGeoid ? " GEOID":"");

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING) << datscale;

   return oss.str();
}

//------------------------------------------------------------------------------------
// return readable string
string DATHAngle::asString(const int prec, const int wid) const
{
   ostringstream oss;
   int precsoa(prec-5);
   if(precsoa <= 0) precsoa = 1;
   oss << "(f-a-t) " << From << "-" << At << "-" << To
      << fixed << setprecision(prec)
      << " " << value << " RAD = " << angleAsDMSstring(value) << " DMS"
      << " sig " << setprecision(precsoa) << sigma*RAD_TO_SOA << " SOA";

   if(hasAddsig)
      oss << " add-sigma " << setprecision(precsoa) << addsig*RAD_TO_SOA << " SOA";

   if(hasFcsig || hasTcsig || hasAcsig)
         oss << ", F,A,T-cent " << fixed << setprecision(3) << Fcentersig
            << " " << Acentersig << " " << Tcentersig << " M";

   if(hasHtFrom || hasHtTo || isGeoid)
      oss << fixed << setprecision(3)
         << " HtF " << (hasHtFrom ?  HtFrom : 0.0)
         << " HtT " << (hasHtTo ?  HtTo : 0.0)
         << " HtFs " << (hasHtFrom ?  HtFromSigma : 0.0)
         << " HtTs " << (hasHtTo ?  HtToSigma : 0.0) << " M"
         << " " <<  (isGeoid ? "":"no-") << "geoid";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(4) << datscale;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
