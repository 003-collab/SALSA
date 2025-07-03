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
/// @file DATConfig.cpp  Include file for class DATConfig, configuration input
///                      in *.dat files, including title, precision, dimension,
///                      and convergence limits

#include <string>
#include <ostream>

#include "StringUtils.hpp"
#include "stl_helpers.hpp"

#include "lsaUtils.hpp"
#include "DATConfig.hpp"

#include "logstream.hpp"         // TEMP

using namespace std;
using namespace gnsstk;
using namespace gnsstk::StringUtils;

//------------------------------------------------------------------------------------
const string DATConfig::DocString = string("TITLE title # quotes optional\n"
   "PREC  eps UNIT [eps UNIT] # output precision; linear and/or angular units\n"
   "DIM 2|3  # dimension of the problem; 2D is XY only, i.e. Z's ignored\n"
   "CONV [n ITER] [d CONV] # convergence criteria; either or both\n"
   "OUT [NOAPV]            # output NOAPV = do not scale covariance with APV\n"
   "EXTRELVECT [YES] [NO]  # determine if external reliability is calculated or not\n"
   "GEOIDFILE filename # Filename for the gridded EGM08 geoid file\n"
   "INTERPOLATION method # Interpolation method for calculating geoid values (bicubic,bilinear)");

//------------------------------------------------------------------------------------
// Parse a string from a single line in the DAT file.
// param line single line read from DAT file
// return true if successful
bool DATConfig::fromString(const string& in_line)
{
   string line(in_line);
   stripTrailing(line,"\n");
   stripTrailing(line,"\r");
   stripTrailing(line," ");
   stripLeading(line," ");

   if(line.empty()) return false;
   if(line[0] == '#') return false;
   vector<string> F = splitWithDoubleQuotes(line,' ');
   unsigned int n = F.size();
   if(n < 2) return false;

   // TITLE title
   if(F[0] == string("TITLE")) {
      title = line;
      title = title.substr(5);
      stripTrailing(title," ");
      stripLeading(title," ");
   }

   // PREC  eps UNIT [eps UNIT]
   else if(F[0] == string("PREC")) {
      try {
         for(unsigned int i=1; i<n-1; i+=2) {
            string up(upperCase(F[i+1]));
            if(isLinearUnit(F[i+1]) || isAngularUnit(F[i+1])) {
               setPrecision(asDouble(F[i]),F[i+1]);
            }
            else if(isLike(up,"LLH")) {
               change(up,"LLH","");
               setPrecision(asDouble(F[i]),up,true);
            }
            else return false;
         }
      }
      catch(Exception& e) { return false; }

/*    if(n == 5 && ((isLinearUnit(F[2]) && isAngularUnit(F[4])) ||
                    (isAngularUnit(F[2]) && isLinearUnit(F[4]))))
      {
         try {
            setPrecision(asDouble(F[1]),F[2]);
            setPrecision(asDouble(F[3]),F[4]);
         }
         catch(Exception& e) { return false; }
      }
      else if(n == 3 && (isAngularUnit(F[2]) || isLinearUnit(F[2]))) {
         try {
            setPrecision(asDouble(F[1]),F[2]);
         }
         catch(Exception& e) { return false; }
      }
      else return false; */
   }

   // DIM dim  dimension of the problem
   else if(F[0] == string("DIM")) {
      // DIM dim  dimension of the problem
      if(F[1] == "2") dim = 2;
      else if(F[1] == "3")  dim = 3;
      else return false;
   }

   // CONV <n> iterations  <d> convergence
   else if(F[0] == string("CONV")) {
      for(unsigned int i=1; i<n; i++) lowerCase(F[i]);

      if(n == 5 && isLike(F[2],"conv*") && isLike(F[4],"iter*")) {
         maxiterations = asInt(F[3]);
         convergence = asDouble(F[1]);
      }
      else if(n == 5 && isLike(F[2],"iter*") && isLike(F[4],"conv*")) {
         maxiterations = asInt(F[1]);
         convergence = asDouble(F[3]);
      }
      else if(n == 3 && isLike(F[2],"iter*")) {
         maxiterations = asInt(F[1]);
      }
      else if(n == 3 && isLike(F[2],"conv*")) {
         convergence = asDouble(F[1]);
      }
      else return false;
   }

   // OUT [NOAPV]            # output NOAPV = do not scale covariance with APV
   else if(F[0] == string("OUT")) {
      for(unsigned int i=1; i<n; i++) {
         lowerCase(F[i]);
         if(F[i] == string("noapv")) {
            applyAPV = false;
         }
         else if(F[i] == string("apquit")) {
            apQuit = true;
         }
         //else if(...
         else return false;
      }
   }

   // NO EXTRELVECT
   else if(F[0] == string("EXTRELVECT")) {
      for(unsigned int i=1; i<n; i++) {
         lowerCase(F[i]);
         if(F[i] == string("no")) {
            noExtRelVect = true;
         }
         else if(F[i]== string("yes")) {
            noExtRelVect = false;
         }
         else return false;
      }
   }

   // GEOIDFILE filename
   else if(F[0] == string("GEOIDFILE")) {
      geoidfile = F[1];
   }
   
   // INTERPOLATION method
   else if(F[0] == string("INTERPOLATION")) {
       geoidinterp = F[1];
   }

   // CONFIDENCE
   else if(F[0] == string("CONFIDENCE")) {
      confidence = asDouble(F[1]);
   }

   // comments
   else if(F[0] == string("COMMENT")) {
      stripTrailing(line," ");
      stripLeading(line," ");
      stripTrailing(line,"\"");
      stripLeading(line,"\"");
      comments.push_back(line);
   }

   // hash
   else if(F[0] == string("HASH")) {
      hashstring = F[1];
   }

   else  
      return false;

   return true;
}

//------------------------------------------------------------------------------------
// Output as a string (several lines) for the DAT file.
// return string, several lines for a DAT file
string DATConfig::asDATString(void) const
{
   // TITLE title
   ostringstream oss;
   oss << "TITLE " << title << endl;
   oss << "PREC " << linprec << " M " << angprecM << " RAD "
               << angprecP << " LLHRAD" << endl;
   oss << "DIM " << dim << endl;
   oss << "CONV " << maxiterations << " Iterations "
         << scientific << setprecision(3) << convergence << " Convergence.";
   if(!applyAPV) oss << endl << "OUT NOAPV";
   if(apQuit) oss << endl << "OUT APQUIT";
   if(noExtRelVect)
   {
      oss << endl << "EXTRELVECT NO";
   }
   else
   {
      oss << endl << "EXTRELVECT YES";
   }
   if(!geoidfile.empty()) oss << endl << "GEOIDFILE " << geoidfile;
   if(!geoidinterp.empty()) oss << endl << "INTERPOLATION " << geoidinterp;
   if(confidence != 0.0) oss << endl << "CONFIDENCE "
      << fixed << setprecision(2) << confidence;
   if(comments.size() > 0)
      for(int i=0; i<comments.size(); i++)
         oss << endl << comments[i];
   if(!hashstring.empty()) oss << endl << "HASH " << hashstring;
   return oss.str();
}

//------------------------------------------------------------------------------------
string DATConfig::asString(const int prec, const int width) const
{
   ostringstream oss;
   oss << "Title \"" << title << "\"";
   oss << "; Prec " << linprec << " lin, " << angprecM << " ang(meas), and "
            << angprecP << " ang(pos)";
   oss << "; dim " << dim;
   oss << "; converge: " << maxiterations << " iterations, "
         << scientific << setprecision(3) << convergence << " convergence.";
   if(confidence != 0.0) oss << " Chi squ confidence = " << fixed << setprecision(2)
         << confidence;
   if(!applyAPV) oss << " APV is not applied to covariance.";
   if(apQuit) oss << endl << " Quit after a priori computations.";
   if(noExtRelVect)
   {
      oss << endl << "No external reliability calculations were performed.";
   }
   else
   {
      oss << endl << "External reliability calculations were performed.";
   }
   if(!geoidfile.empty()) oss << endl << " Geoid file: " << geoidfile;
   if(!geoidinterp.empty()) oss << endl << " Geoid interpolation method: "
      << geoidinterp;
   if(!hashstring.empty()) oss << endl << " HASH is " << hashstring;
   return oss.str();
}

//------------------------------------------------------------------------------------
// Define the value of last digit in the output for linear or angular quanities.
// param value smallest value in UNIT units printed in the output, e.g. 0.1
// param UNIT units of value, e.g. M, DEG, SOA, FT, etc.
// param forllh if true, applies to position (LLH) data, i.e. set angprecP
void DATConfig::setPrecision(double value, string UNIT, bool forllh) throw(Exception)
{
try {
   if(isLinearUnit(UNIT)) {
      value = convertTo("M",value,UNIT);
      if(value > 1.0)
         linprec = 1;
      else
         linprec = int(-log10(value)+0.5);
   }
   else if(isAngularUnit(UNIT) && forllh) {
      value = convertTo("SOA",value,UNIT);
      if(value > 1.0)
         angprecM = 1;
      else
         angprecM = int(-log10(value)+0.5);
   }
   else if(isAngularUnit(UNIT)) {
      value = convertTo("SOA",value,UNIT);
      if(value > 1.0)
         angprecP = 1;
      else
         angprecP = int(-log10(value)+0.5);
   }
   else
      GNSSTK_THROW(Exception(string("Invalid units: ") + UNIT));
}
catch(Exception& e) { GNSSTK_RETHROW(e); }
}

//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
