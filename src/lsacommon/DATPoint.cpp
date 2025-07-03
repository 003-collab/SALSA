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
/// @file DATPoint.cpp  Class DATPoint, data for the *.dat file record POS
///                     3-D XYZ coordinate position

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"
#include "Position.hpp"
#include "SunEarthSatGeometry.hpp"

#include "lsaUtils.hpp"
#include "DATPoint.hpp"
//#include "logstream.hpp"

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
// TODO
// Check covariance matrices for positive definite

//------------------------------------------------------------------------------------
const string DATPoint::DocString = string("POS label X Y Z covxx xy xz yy yz zz LUNIT [FIX|ADJ|EST] [CONS c[c]] # no corrections; constraints c one[two] of NEU");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATPoint::fromString(const string& line)
{
   int n;
   double fact,fact2;
   vector<string> F;
   static const string M("M"), RAD("RAD"), FIX("FIX");

   string str(line);                // copy const line
   stripTrailing(str,"\n");
   stripTrailing(str,"\r");
   stripTrailing(str," ");
   stripLeading(str," ");

   if(str.empty()) return false;
   if(str[0] == '#') return false;

   // check record type
   //POS label X Y Z covxx xy xz yy yz zz LUNIT [FIX] [CONS c[c]]
   if(str.substr(0,4) != string("POS ")) return false;

   // strip off and save optional "CONS ..." at end of str
   string constr(str);
   string::size_type pos = constr.rfind("CONS");      // find last "CONS"
   if(pos == string::npos)
      constr = string();
   else {
      constr.erase(0,pos-1);                 // reduce to just CONS ... EOL
      str.erase(pos,string::npos);           // remove from end of line
      stripTrailing(str," ");
      // parse constr below after parsing str
   }

   // strip off fix type uefa
   Fixtype ftype;
   pos = str.rfind(" ");
   string fixstr(str.substr(pos));
   stripLeading(fixstr," ");
   if(fixstr == string("FIX") || fixstr == string("EST") || fixstr == string("ADJ")) {
      if(fixstr == string("FIX")) ftype = Fixtype::Fixed;
      else if(fixstr == string("EST")) ftype = Fixtype::Estimated;
      else if(fixstr == string("ADJ")) ftype = Fixtype::Adjusted;
      // remove it
      str.erase(pos,string::npos);
      stripTrailing(str," ");
   }
   else ftype = Fixtype::Estimated;                               // the default is estimate

   // split into fields
   F = splitWithDoubleQuotes(str,' ');
   n = F.size();

   //POS label X Y Z covxx xy xz yy yz zz LUNIT                             12
   if(n == 12 && isLinearUnit(F[11]) && (F[5]!=string("N") && F[5]!=string("S"))) {
      // change units
      fact = convertTo(M, 1.0, F[11]);
      fact2 = fact*fact;
      *this = DATPoint(F[1],
         fact*asDouble(F[2]),fact*asDouble(F[3]),fact*asDouble(F[4]),
         fact2*asDouble(F[5]), fact2*asDouble(F[6]), fact2*asDouble(F[7]),
         fact2*asDouble(F[8]), fact2*asDouble(F[9]), fact2*asDouble(F[10]));
   }

   //POS label X Y Z LUNIT                                                   6
   else if(n == 6 && isLinearUnit(F[5])) {
      if(ftype == Fixtype::Adjusted) {
         //LOG(WARNING) << " In DAT file, POS ADJ requires covariance";
         return false;
      }
      // change units
      fact = convertTo(M, 1.0, F[5]);
      fact2 = fact*fact;
      *this = DATPoint(F[1],
         fact*asDouble(F[2]),fact*asDouble(F[3]),fact*asDouble(F[4]),
         0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
   }

   // the rest are undocumented
   //POS label D M S N|S D M S E|W Ht LUNIT covNN NE NU EE EU UU NEU LUNIT  20
   else if(n == 20 && (F[5]==string("N") || F[5]==string("S")) &&
                      (F[9]==string("E") || F[9]==string("W")) &&
                      isLinearUnit(F[11]) && F[18]==string("NEU") &&
                      isLinearUnit(F[19])) {
      // get lat lon ht in deg,deg,m
      double lat(DMSToDeg(F[2],F[3],F[4]));
      if(F[5]=="S") lat = -lat;
      double lon(DMSToDeg(F[6],F[7],F[8]));
      if(F[9]=="W") lon = -lon;
      double ht(convertTo(M,asDouble(F[10]),F[11]));
      // convert to XYZ
      Position Pos;
      Pos.setGeodetic(lat, lon, ht);
      Pos.transformTo(Position::Cartesian);
      // get covariance in NEU
      Matrix<double> CovNEU(3,3),Rot,CovXYZ;
      // get covariance units
      fact = convertTo(M, 1.0, F[19]);
      fact2 = fact*fact;
      CovNEU(0,0) = fact2*asDouble(F[12]);
      CovNEU(0,1) = CovNEU(1,0) = fact2*asDouble(F[13]);
      CovNEU(0,2) = CovNEU(2,0) = fact2*asDouble(F[14]),
      CovNEU(1,1) = fact2*asDouble(F[15]);
      CovNEU(1,2) = CovNEU(2,1) = fact2*asDouble(F[16]);
      CovNEU(2,2) = fact2*asDouble(F[17]);
      // get rotation XYZ->NEU
      Rot = northEastUp(Pos);
      // rotate covariance into XYZ
      CovXYZ = transpose(Rot)*CovNEU*Rot;

      *this = DATPoint(F[1],Pos.X(),Pos.Y(),Pos.Z(),
         CovXYZ(0,0),CovXYZ(0,1),CovXYZ(0,2),CovXYZ(1,1),CovXYZ(1,2),CovXYZ(2,2));
   }

   //POS label D M S N|S D M S E|W Ht LUNIT covxx xy xz yy yz zz     LUNIT  19
   else if(n == 19 && (F[5]==string("N") || F[5]==string("S")) &&
                      (F[9]==string("E") || F[9]==string("W")) &&
                      isLinearUnit(F[11]) && isLinearUnit(F[18])) {
      // get lat lon ht in deg,deg,m
      double lat(DMSToDeg(F[2],F[3],F[4]));
      if(F[5]=="S") lat = -lat;
      double lon(DMSToDeg(F[6],F[7],F[8]));
      if(F[9]=="W") lon = -lon;
      double ht(convertTo(M,asDouble(F[10]),F[11]));
      // convert to XYZ
      Position Pos;
      Pos.setGeodetic(lat, lon, ht);
      Pos.transformTo(Position::Cartesian);
      // get covariance units
      fact = convertTo(M, 1.0, F[18]);
      fact2 = fact*fact;
      *this = DATPoint(F[1],Pos.X(),Pos.Y(),Pos.Z(),
         fact2*asDouble(F[12]), fact2*asDouble(F[13]), fact2*asDouble(F[14]),
         fact2*asDouble(F[15]), fact2*asDouble(F[16]), fact2*asDouble(F[17]));
   }

   //POS label D M S N|S D M S E|W Ht LUNIT                                 12
   else if(n == 12 && (F[5]==string("N") || F[5]==string("S")) &&
                      (F[9]==string("E") || F[9]==string("W")) &&
                      isLinearUnit(F[11])) {
      if(ftype == Fixtype::Adjusted) {
         //LOG(WARNING) << " In DAT file, POS ADJ requires covariance";
         return false;
      }
      // get lat lon ht in deg,deg,m
      double lat(DMSToDeg(F[2],F[3],F[4]));
      if(F[5]=="S") lat = -lat;
      double lon(DMSToDeg(F[6],F[7],F[8]));
      if(F[9]=="W") lon = -lon;
      double ht(convertTo(M,asDouble(F[10]),F[11]));
      // convert to XYZ
      Position Pos;
      Pos.setGeodetic(lat, lon, ht);
      Pos.transformTo(Position::Cartesian);
      *this = DATPoint(F[1],Pos.X(),Pos.Y(),Pos.Z(), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
   }

   else return false;

   setFixType(ftype);

   // parse the CONS string
   //CONS c[c]  one[two] axes belonging to 'XYZ' or 'NEU' - not mixed
   if(!constr.empty()) {
      F = split(constr,' ');              // F[0] must == "CONS"
      n = F.size();

      //CONS c[c]
      if(n != 2) return false;            // CONS c[c] has wrong number of fields

      // how many?
      n = F[1].size();
      if(n != 1 && n != 2) return false;  // wrong number of axes

      F[1] = upperCase(F[1]);
      pos = F[1].find_first_not_of("NEU");
      if(pos != string::npos) {                 // not a subset of NEU
         pos = F[1].find_first_not_of("XYZ");
         if(pos != string::npos)                // not a subset of XYZ
            return false;
      }

      // if the 2 are identical, throw out one
      if(n == 2 && F[1][0] == F[1][1]) F[1] = F[1].substr(0,1);

      // ok
      setConstraintAxis(F[1]);
   }

   // success
   return true;
}

//------------------------------------------------------------------------------------
// Output as a string (one line) for the DAT file.
// return string, a single line for a DAT file
string DATPoint::asDATString(void) const
{
   //POS label X Y Z covxx xy xz yy yz zz LUNIT [ADJ|FIX|EST]
   //POS label X Y Z covxx xy xz yy yz zz LUNIT [ADJ|FIX|EST] [CONS c[c]]
   //POS label X Y Z LUNIT [FIX|EST]
   //POS label X Y Z LUNIT [FIX|EST] [CONS c[c]]
   ostringstream oss;
   oss << "POS " << addQuotes(label) << fixed << setprecision(getPrecisionForUnits(lsa::UNITS_M, true))
      << " " << x << " " << y << " " << z
      << scientific << setprecision(lsa::NUM_DECIMALS_COVARIANCE) << " " << covxx << " " << covxy << " " << covxz
      << " " << covyy << " " << covyz << " " << covzz << " M"
      << " " << (fixtype==Fixtype::Unknown ? "UNK" :
                (fixtype==Fixtype::Estimated ? "EST" :
                (fixtype==Fixtype::Fixed ? "FIX" :
                (fixtype==Fixtype::Adjusted ? "ADJ" : "ERROR"))));

   // add constraints
   if(!constraint.empty())
      oss << " CONS " << constraint;

   // scale
   if(datscale != 1.0) oss << " scale=" << fixed << setprecision(lsa::NUM_DECIMALS_VARIANCE_SCALING) << datscale;
   if(!dattag.empty()) oss << " tag=" << dattag;

   return oss.str();
}

//------------------------------------------------------------------------------------
string DATPoint::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << label << fixed << setprecision(prec)
      << " " << setw(width) << x
      << " " << setw(width) << y
      << " " << setw(width) << z
      << " " << (fixtype==Fixtype::Unknown ? "Unknown" :
                (fixtype==Fixtype::Estimated ? "Estimate" :
                (fixtype==Fixtype::Fixed ? "Fix" :
                (fixtype==Fixtype::Adjusted ? "Adjust" : "ERROR"))));

   if(!constraint.empty()) oss << " constrain " << constraint;

   if(fixtype == Fixtype::Adjusted) oss << " with cov(XYZ,UT):" << scientific
      << setprecision(2) << " " << covxx << " " << covxy << " " << covxz
      << " " << covyy << " " << covyz << " " << covzz << " M";
      
   return oss.str();
}

//------------------------------------------------------------------------------------
// Define the constraint axis (or axes)
// param axes string giving the axes of constraint, e.g. "N" | "Y" | "YZ" | "NE"
void DATPoint::setConstraintAxis(string axes) throw(Exception)
{
   stripLeading(axes," ");
   stripTrailing(axes," ");
   if(axes.size() == 0) { constraint = ""; return; }
   if((axes.find_first_not_of("NEU") != string::npos &&
       axes.find_first_not_of("XYZ") != string::npos) || axes.size() > 2)
      GNSSTK_THROW(Exception("Invalid constraint axes: " + axes));

   constraint = axes;
}

//------------------------------------------------------------------------------------
// Compute the measurement covariance matrix
// return 3x3 measurement covariance matrix
Matrix<double> DATPoint::measurementCovariance(void)
{
try {
   Matrix<double> mcov(3,3,0.0);
   mcov(0,0) = covxx;
   mcov(0,1) = mcov(1,0) = covxy;
   mcov(0,2) = mcov(2,0) = covxz;
   mcov(1,1) = covyy;
   mcov(1,2) = mcov(2,1) = covyz;
   mcov(2,2) = covzz;

   if(datscale != 1.0) mcov *= datscale;

   return mcov;
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

// this ends the base class interface

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
