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
/// @file DATVAngle.cpp  Class DATVAngle, data for the *.dat file record VAN
///                      vertical angle measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "DATVAngle.hpp"
#include "lsaUtils.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATVAngle::DocString = string("VAN labF labT angle [min sec DMS|AUNIT] sig [asig] AUNIT [fc tc LUNIT] [CORR htf htt htfs htts [LUNIT] refract [GEOID]]");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATVAngle::fromString(const string& line)
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
   // VAN labFrom labTo angle [min sec DMS|AUNIT] sig [asig] AUNIT [FC TC LUNIT]
   if(str.substr(0,4) != string("VAN ")) return false;

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

   // VAN labFrom labTo angle min sec DMS sig asig AUNIT ACsig TCsig LUNIT 13 flds
   if(n == 13 && F[6]==DMS && isAngularUnit(F[9]) && isLinearUnit(F[12])) {
      value = DMSToDeg(F[3],F[4],F[5]);
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD, asDouble(F[7]), F[9]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[6]),F[9]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[10]),F[12]),    // From Cent Err
                        convertTo(M,asDouble(F[11]),F[12]));   // To Cent Err
   }

   // VAN labFrom labTo angle min sec DMS sig      AUNIT ACsig TCsig LUNIT 12
   else if(n == 12 && F[6]==DMS && isAngularUnit(F[8]) && isLinearUnit(F[11])) {
      value = DMSToDeg(F[3],F[4],F[5]);
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD, asDouble(F[7]), F[8]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[ 9]),F[11]),    // From Cent Err
                        convertTo(M,asDouble(F[10]),F[11]));   // To Cent Err
   }

   // VAN labFrom labTo angle min sec DMS sig asig AUNIT                   10
   else if(n == 10 && F[6]==DMS && isAngularUnit(F[9])) {
      value = DMSToDeg(F[3],F[4],F[5]);
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD, asDouble(F[7]), F[9]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[8]),F[9]));
   }

   // VAN labFrom labTo angle min sec DMS sig      AUNIT                    9
   else if(n == 9 && F[6]==DMS && isAngularUnit(F[8])) {
      value = DMSToDeg(F[3],F[4],F[5]);
      value = convertTo(RAD, value, DEG);
      sigma = convertTo(RAD, asDouble(F[7]), F[8]);
      *this = DATVAngle(F[1], F[2], value, sigma);
   }

   // not DMS

   // VAN labFrom labTo angle             sig asig AUNIT ACsig TCsig LUNIT 10
   else if(n == 10 && isAngularUnit(F[6]) && isLinearUnit(F[9])) {
      value = convertTo(RAD,asDouble(F[3]),F[6]);
      sigma = convertTo(RAD,asDouble(F[4]),F[6]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[5]),F[6]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[7]),F[9]),   // From Cent Err
                        convertTo(M,asDouble(F[8]),F[9]));  // To Cent Err
   }

   // VAN labFrom labTo angle             sig      AUNIT ACsig TCsig LUNIT  9
   else if(n == 9 && isAngularUnit(F[5]) && isLinearUnit(F[8])) {
      value = convertTo(RAD,asDouble(F[3]),F[5]);
      sigma = convertTo(RAD,asDouble(F[4]),F[5]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[6]),F[8]),   // From Cent Err
                        convertTo(M,asDouble(F[7]),F[8]));  // To Cent Err
   }

   // VAN labFrom labTo angle AUNIT       sig      AUNIT                    7
   else if(n == 7 && isAngularUnit(F[4]) && isAngularUnit(F[6])) {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[6]);
      *this = DATVAngle(F[1], F[2], value, sigma);
   }

   // VAN labFrom labTo angle             sig asig AUNIT                    7
   else if(n == 7 && isAngularUnit(F[6])) {
      value = convertTo(RAD,asDouble(F[3]),F[6]);
      sigma = convertTo(RAD,asDouble(F[4]),F[6]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[5]),F[6]));
   }

   // VAN labFrom labTo angle             sig      AUNIT                    6
   else if(n == 6 && isAngularUnit(F[5])) {
      value = convertTo(RAD,asDouble(F[3]),F[5]);
      sigma = convertTo(RAD,asDouble(F[4]),F[5]);
      *this = DATVAngle(F[1], F[2], value, sigma);
   }

   // VAN labFrom labTo angle AUNIT       sig asig AUNIT ACsig TCsig LUNIT 11
   else if(n == 11 && isAngularUnit(F[4]) && isAngularUnit(F[7])
                   && isLinearUnit(F[10]))
   {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[7]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[6]),F[7]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[8]),F[10]),   // From Cent Err
                        convertTo(M,asDouble(F[9]),F[10]));  // To Cent Err
   }

   // VAN labFrom labTo angle AUNIT       sig      AUNIT ACsig TCsig LUNIT  8
   else if(n == 8 && isAngularUnit(F[4]) && isAngularUnit(F[6])
                  && isLinearUnit(F[9]))
   {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[6]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[7]),F[9]),   // From Cent Err
                        convertTo(M,asDouble(F[8]),F[9]));  // To Cent Err
   }

   // VAN labFrom labTo angle AUNIT       sig asig AUNIT                    8
   else if(n == 8 && isAngularUnit(F[4]) && isAngularUnit(F[7])) {
      value = convertTo(RAD,asDouble(F[3]),F[4]);
      sigma = convertTo(RAD,asDouble(F[5]),F[7]);
      *this = DATVAngle(F[1], F[2], value, sigma);
      // add additional sigma
      setAddSigma(convertTo(RAD,asDouble(F[6]),F[7]));
   }

   else return false;

   // parse the CORR string
   //CORR htf htt htfs htts [LUNIT] refract [GEOID]
   if(!corrstr.empty()) {
      bool isgeoid(false);
      double HtF(0.0),HtT(0.0),HtFs(0.0), HtTs(0.0),refract(0.0);

      // first find GEOID
      pos = corrstr.rfind("GEOID");    // find last "GEOID"
      if(pos != string::npos) {
         isgeoid = true;
         corrstr.erase(pos);           // remove GEOID
         stripTrailing(corrstr," ");
      }

      F = split(corrstr,' ');          // F[0] must == "CORR"
      n = F.size();

      //CORR htf htt htfs htts LUNIT refract
     if(n == 7 && isLinearUnit(F[5])) {
         HtF = convertTo(M,asDouble(F[1]),F[5]);
         HtT = convertTo(M,asDouble(F[2]),F[5]);
         HtFs = convertTo(M,asDouble(F[3]),F[5]);
         HtTs = convertTo(M,asDouble(F[4]),F[5]);
         refract = asDouble(F[6]);
      }
      //CORR htf htt LUNIT refract
      else if(n == 5 && isLinearUnit(F[3])) {
         HtF = convertTo(M,asDouble(F[1]),F[3]);
         HtT = convertTo(M,asDouble(F[2]),F[3]);
         refract = asDouble(F[4]);
      }
      //CORR htf htt refract
      else if(n == 4) {
         HtF = asDouble(F[1]);
         HtT = asDouble(F[2]);
         refract = asDouble(F[3]);
      }
      else
         return false;

      setCorrections(HtF, HtT, HtFs, HtTs, refract, isgeoid);
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) for the DAT file.
// return string, a single line for a DAT file
string DATVAngle::asDATString(void) const
{
   //VAN labF labT angle [min sec DMS|AUNIT] sig [asig] AUNIT [fc tc LUNIT] [CORR htf htt htfs htts [LUNIT] refract [GEOID]]
   ostringstream oss;
   oss << "VAN " << addQuotes(From) << " " << addQuotes(To)
      << fixed << " " << angleAsDMSstring(value) << " DMS"
      << setprecision(getPrecisionForUnits(lsa::UNITS_SOA)) << " " << sigma*RAD_TO_SOA;

   if(hasAddsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_SOA)) << " " << addsig*RAD_TO_SOA;

   oss << " SOA";

   if(hasFcsig || hasTcsig)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
         << " " << (hasFcsig ?  Fcentersig : 0.0)
         << " " << (hasTcsig ?  Tcentersig : 0.0) << " M";

   if(hasHtFrom || hasHtTo || hasRefract || isGeoid)
      oss << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M)) << " CORR"
         << " " << (hasHtFrom ? HtFrom : 0.0)
         << " " << (hasHtTo ? HtTo : 0.0)
         << " " << (hasHtFrom ? HtFromSigma : 0.0)
         << " " << (hasHtTo ? HtToSigma : 0.0) << " M "
         << " " << (hasRefract ? refract : 0.0)
         << (isGeoid ? " GEOID" : "");

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING) << datscale;

   return oss.str();
}

//------------------------------------------------------------------------------------
// return readable string
string DATVAngle::asString(const int prec, const int wid) const
{
   ostringstream oss;
   int precsoa(prec-5);
   if(precsoa <= 0) precsoa = 1;

   oss << From << "-" << To
      << fixed << setprecision(prec)
      << " is " << value << " RAD = " << angleAsDMSstring(value) << " DMS"
      << " sig " << setprecision(precsoa) << sigma*RAD_TO_SOA << " SOA";

   if(hasAddsig) oss << " add-sigma "
      << setprecision(precsoa) << addsig*RAD_TO_SOA << " SOA";

   if(hasFcsig || hasTcsig)
         oss << ", F,T-cent " << fixed << setprecision(3)
            << " " << Fcentersig << " " << Tcentersig << " m";

   if(hasHtFrom || hasHtTo || hasRefract || isGeoid)
      oss << fixed << setprecision(3)
         << " HtF " << (hasHtFrom ? HtFrom : 0.0)
         << " HtT " << (hasHtTo ? HtTo : 0.0)
         << " HtFs " << (hasHtFrom ? HtFromSigma : 0.0)
         << " HtTs " << (hasHtTo ? HtToSigma : 0.0) << " M"
         << " refr " << (hasRefract ? refract : 0.0)
         << " " <<  (isGeoid ? "":"no-") << "geoid";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(4) << datscale;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
