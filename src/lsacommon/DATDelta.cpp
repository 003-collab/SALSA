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
/// @file DATDelta.cpp  Class DATDelta, data for the *.dat file record DEL,
///                     3-D XYZ coordinate difference measurement

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "DATDelta.hpp"
#include "lsaUtils.hpp"
#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATDelta::DocString = string("DEL labFr labTo dX dY dZ covxx xy xz yy yz zz [asig [PPM]] [fcsig tcsig] LUNIT [CORR htf htt htfs htts [LUNIT]]");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATDelta::fromString(const string& line)
{
   int n;
   double fact,fact2;
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
   //DEL labFr labTo dX dY dZ covxx xy xz yy yz zz [asig [PPM]] [FCsig TCsig] LUNIT
   if(str.substr(0,4) != string("DEL ")) return false;

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

   //DEL labFr labTo dX dY dZ covxx xy xz yy yz zz asig PPM FCsig TCsig LUNIT 17
   if(n == 17 && F[13]==PPM && isLinearUnit(F[16])) {
      fact = convertTo(M,1.0,F[16]);
      fact2 = fact*fact;
      *this = DATDelta(F[1], F[2],
               fact*asDouble(F[3]),fact*asDouble(F[4]),fact*asDouble(F[5]),
               fact2*asDouble(F[6]), fact2*asDouble(F[7]), fact2*asDouble(F[8]),
               fact2*asDouble(F[9]), fact2*asDouble(F[10]), fact2*asDouble(F[11]));
      // add additional sigma, in PPM
      setAddSigma(asDouble(F[12]), true);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[14]),F[16]),    // From Cent Err
                            convertTo(M,asDouble(F[15]),F[16]));   // To Cent Err
   }

   //DEL labFr labTo dX dY dZ covxx xy xz yy yz zz asig     FCsig TCsig LUNIT 16
   else if(n == 16 && isLinearUnit(F[15])) {
      fact = convertTo(M,1.0,F[15]);
      fact2 = fact*fact;
      *this = DATDelta(F[1], F[2],
         fact*asDouble(F[3]),fact*asDouble(F[4]),fact*asDouble(F[5]),
         fact2*asDouble(F[6]), fact2*asDouble(F[7]), fact2*asDouble(F[8]),
         fact2*asDouble(F[9]), fact2*asDouble(F[10]), fact2*asDouble(F[11]));
      // add additional sigma, in LUNITs
      setAddSigma(convertTo(M,asDouble(F[12]),F[15]), false);
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[13]),F[15]),    // From Cent Err
                        convertTo(M,asDouble(F[14]),F[15]));   // To Cent Err
   }

   // NB this must precede the next - they differ only in the PPM
   //DEL labFr labTo dX dY dZ covxx xy xz yy yz zz asig PPM LUNIT             15
   else if(n == 15 && F[13]==PPM && isLinearUnit(F[14])) {
      fact = convertTo(M,1.0,F[14]);
      fact2 = fact*fact;
      *this = DATDelta(F[1], F[2],
         fact*asDouble(F[3]),fact*asDouble(F[4]),fact*asDouble(F[5]),
         fact2*asDouble(F[6]), fact2*asDouble(F[7]), fact2*asDouble(F[8]),
         fact2*asDouble(F[9]), fact2*asDouble(F[10]), fact2*asDouble(F[11]));
      // add additional sigma, in LUNITs
      setAddSigma(convertTo(M,asDouble(F[12]),F[14]), true);
   }

   //DEL labFr labTo dX dY dZ covxx xy xz yy yz zz          FCsig TCsig LUNIT 15
   else if(n == 15 && isLinearUnit(F[14])) {
      fact = convertTo(M,1.0,F[14]);
      fact2 = fact*fact;
      *this = DATDelta(F[1], F[2],
         fact*asDouble(F[3]),fact*asDouble(F[4]),fact*asDouble(F[5]),
         fact2*asDouble(F[6]), fact2*asDouble(F[7]), fact2*asDouble(F[8]),
         fact2*asDouble(F[9]), fact2*asDouble(F[10]), fact2*asDouble(F[11]));
      // add centering errors
      setCenterSigmas(convertTo(M,asDouble(F[12]),F[14]),    // From Cent Err
                        convertTo(M,asDouble(F[13]),F[14]));   // To Cent Err
   }

   //DEL labFr labTo dX dY dZ covxx xy xz yy yz zz asig     LUNIT             14
   else if(n == 14 && isLinearUnit(F[13])) {
      fact = convertTo(M,1.0,F[13]);
      fact2 = fact*fact;
      *this = DATDelta(F[1], F[2],
         fact*asDouble(F[3]),fact*asDouble(F[4]),fact*asDouble(F[5]),
         fact2*asDouble(F[6]), fact2*asDouble(F[7]), fact2*asDouble(F[8]),
         fact2*asDouble(F[9]), fact2*asDouble(F[10]), fact2*asDouble(F[11]));
      // add additional sigma, in LUNITs
      setAddSigma(convertTo(M,asDouble(F[12]),F[13]), false);
   }

   //DEL labFr labTo dX dY dZ covxx xy xz yy yz zz          LUNIT             13
   else if(n == 13 && isLinearUnit(F[12])) {
      fact = convertTo(M,1.0,F[12]);
      fact2 = fact*fact;
      *this = DATDelta(F[1], F[2],
         fact*asDouble(F[3]),fact*asDouble(F[4]),fact*asDouble(F[5]),
         fact2*asDouble(F[6]), fact2*asDouble(F[7]), fact2*asDouble(F[8]),
         fact2*asDouble(F[9]), fact2*asDouble(F[10]), fact2*asDouble(F[11]));
   }

   else return false;

   // parse the CORR string
   //CORR HtF HtT HtFs HtTs [LUNIT]
   if(!corrstr.empty()) {
      double HtF(0.0),HtT(0.0), HtFs(0.0), HtTs(0.0);
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
string DATDelta::asDATString(void) const
{
   // DEL labFr labTo dX dY dZ covxx xy xz yy yz zz [asig [PPM]] [FCsig TCsig] LUNIT [CORR HtF HtT HtFs HtTs [LUNIT]]
   ostringstream oss;
   oss << "DEL " << addQuotes(From) << " " << addQuotes(To) << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M))
      << " " << dx << " " << dy << " " << dz << scientific << setprecision(lsa::NUM_DECIMALS_COVARIANCE)
      << " " << covxx << " " << covxy << " " << covxz
      << " " << covyy << " " << covyz << " " << covzz;

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
string DATDelta::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << From << "-" << To << fixed << setprecision(prec)
      << " " << dx << " " << dy << " " << dz
      << scientific << setprecision(prec)
      << " " << covxx << " " << covxy << " " << covxz
      << " " << covyy << " " << covyz << " " << covzz;      // TD units?

   if(hasAddsig)
      oss << ", add-sig " << fixed << addsig << (isPPM ? " PPM" : " m");

   if(hasFcsig || hasTcsig)
      oss << ", F,T-center-sig " << fixed << Fcentersig << " " << Tcentersig << " m";

   if(hasHtFrom || hasHtTo)
      oss << fixed << setprecision(4)
         << " HtF " << (hasHtFrom ?  HtFrom : 0.0)
         << " HtT " << (hasHtTo ?  HtTo : 0.0)
         << " HtFs " << (hasHtTo ?  HtFromSigma : 0.0)
         << " HtTs " << (hasHtTo ?  HtToSigma : 0.0) << " m";

   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(4) << datscale;

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
// write the object as a 3-line string of the form
//
// \verbatim
//     Delta labFrom-labTo  X-comp     covXX  covXY  covXZ
//                          Y-comp            covYY  covYZ
//                          Z-comp                   covZZ
// \endverbatim
// param msg string to place at first of string
// param prec integer number of digits precision (default 2)
// param width integer width (default 7)
// return string version of the object
string DATDelta::asString3(const string msg, const int prec, const int width) const
{
   int ncov;
   string label,spacer;
   ostringstream oss;

   // first line (X)
   oss << msg << " " << From << "-" << To;
   label = oss.str(); oss.str("");
   oss << scientific << setprecision(3) << -::fabs(covxx);
   spacer = oss.str(); oss.str("");
   ncov = spacer.size();
   spacer = StringUtils::rightJustify(" ",ncov);
   oss << label << " X" << fixed << setprecision(prec)
      << " " << setw(width) << dx << scientific << setprecision(3)
      << "  " << setw(ncov) << covxx
      << " " << setw(ncov) << covxy << " " << setw(ncov) << covxz;

   if(hasAddsig)
      oss << " additional-sig " << fixed << addsig << (isPPM ? " PPM" : " m");
   if(!dattag.empty()) oss << " tag=" << dattag;
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(4) << datscale;
   oss << "\n";

   // second line (Y)
   label = StringUtils::rightJustify(" ",label.size()+1);
   oss << label << " Y" << fixed << setprecision(prec)
      << " " << setw(width) << dy << scientific << setprecision(3)
      << "  " << spacer << " " << setw(ncov) << covyy
      << " " << setw(ncov) << covyz;

   if(hasFcsig)
      oss << " F-cent " << fixed << Fcentersig << " m";
   if(hasHtFrom)
      oss << " HtFrom " << fixed << setprecision(4) << HtFrom << " m" << "\n"
          << " HtFromSigma " << fixed << setprecision(4) << HtFromSigma << " m";
   oss << "\n";

   // third line (Z)
   oss << label << " Z" << fixed << setprecision(prec)
      << " " << setw(width) << dz << scientific << setprecision(3)
      << "  " << spacer << " " << spacer << " " << setw(ncov) << covzz;

   if(hasTcsig)
      oss << " T-cent " << fixed << Tcentersig << " m";
   if(hasHtTo)
      oss << " HtTo " << fixed << setprecision(4) << HtTo << " m" << "\n"
          << " HtToSigma " << fixed << setprecision(4) << HtToSigma << " m";

   return oss.str();
}

// this ends the base class interface

//------------------------------------------------------------------------------------
// Return the measurement covariance matrix defined just by the UT components in
// the constructor; see fullMeasurementCovariance() for this much + additions.
// return 3x3 measurement covariance matrix
Matrix<double> DATDelta::measurementCovariance(void)
{
   Matrix<double> mcov(3,3,0.0);
   mcov(0,0) = covxx;
   mcov(0,1) = mcov(1,0) = covxy;
   mcov(0,2) = mcov(2,0) = covxz;
   mcov(1,1) = covyy;
   mcov(1,2) = mcov(2,1) = covyz;
   mcov(2,2) = covzz;
   return mcov;
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
