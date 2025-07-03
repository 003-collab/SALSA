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
/// @file DATConfig.hpp  Include file for class DATConfig, configuration input
///                      in *.dat files, including title, precision, dimension,
///                      and convergence limits

#ifndef LSA_DAT_CONFIG_DATA_INCLUDE
#define LSA_DAT_CONFIG_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"

/// Class DATConfig encapsulates data for a title, precision of the output for both
/// linear and angular data, the dimension of the problem, and convergence criteria,
/// both a maximum iteration count and a fractional convergence criterion.
class DATConfig : public DATrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   std::string title;      ///< the title
   int linprec;            ///< precision for linear (distance) values
   int angprecM;           ///< precision for angular measurements in SOA
   int angprecP;           ///< precision for angles in positions (LLH) in SOA
   int dim;                ///< the dimension of the problem (2 or 3)
   int maxiterations;      ///< maximum number of iterations
   double convergence;     ///< convergence criterion
   double confidence;      ///< confidence for Chi squared test (0 < c < 1)
   bool applyAPV;          ///< apply APV to output covariance, default T
   bool apQuit;            ///< quit after ComputeAPriori(), default F
   bool noExtRelVect;      ///< do not calculate external reliability, default F
   std::string geoidfile;  ///< filename for gridded EGM08 geoid file
   std::string geoidinterp;///< Interpolation method to be used for geoid values
   std::vector<std::string> comments;  ///< all comment records
   std::string hashstring; ///< Hash string

   // member functions -------------------------------------------------

   /// empty constructor. Default values are all zero.
   DATConfig() : title(std::string("")), linprec(0), angprecM(0), angprecP(0),
                 dim(0), maxiterations(0), convergence(0.0), confidence(0.0),
                 applyAPV(true), apQuit(false), noExtRelVect(false),
                 geoidfile(std::string()), geoidinterp(std::string()),
                 hashstring(std::string())
      { type = DATtype::CONFIG; }

   /// destructor
   virtual ~DATConfig() { }

   /// Parse a string from a single line in the DAT file.
   /// @param line single line read from DAT file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// Output as a string (one line) for the DAT file.
   /// @return string, a single line for a DAT file
   virtual std::string asDATString(void) const;

   /// write the object as a 1-line string
   /// @param prec integer number of digits precision (default 3)
   /// @param width integer width (default 8)
   /// @return string version of the object
   virtual std::string asString(const int prec=3, const int width=8) const;

   /// Consistent interface for all DAT records - not a config record
   virtual bool isConfig(void) const { return true; }

   /// Consistent interface for all DAT records - is a position record
   virtual bool isPosition(void) const { return false; }

   /// Consistent interface for all DAT records - is not a measurement record
   virtual bool isMeasurement(void) const { return false; }

   /// Consistent interface for all DAT records - is not an angle measurement
   virtual bool isAngle(void) const { return false; }

   /// Consistent interface for all DAT records - is not a linear measurement
   virtual bool isLength(void) const { return false; }

   // this ends the base class interface

   /// Define the value of last digit in the output for linear or angular quanities.
   /// @param value smallest value in UNIT units printed in the output, e.g. 0.1
   /// @param UNIT units of value, e.g. M, DEG, SOA, FT, etc.
   /// @param forllh if true, applies to position (LLH) data, i.e. set angprecP
   void setPrecision(double value, std::string UNIT, bool forllh=false)
      throw(gnsstk::Exception);

}; // end class DATConfig

#endif   // LSA_DAT_CONFIG_DATA_INCLUDE

