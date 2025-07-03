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
/// @file DATHAngle.hpp  Include file for class DATHAngle, data for the *.dat file
///                     record HAN horizontal angle

#ifndef LSA_DAT_HORIZONTAL_ANGLE_DATA_INCLUDE
#define LSA_DAT_HORIZONTAL_ANGLE_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include <math.h>
// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"
#include "lsaUtils.hpp"

/// Class DATHAngle encapsulates the data for a horizontal angle measurement,
/// including the labels "From" "At" and "To" which define the sense and orientation
/// of the angle, the measurement itself and its uncrtainty sigma, plus an optional
/// second (additive) sigma value, plus corrections.
class DATHAngle : public DATrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   // labels
   std::string At;      ///< string label for the "At" Point
   std::string From;    ///< string label for the "From" Point
   std::string To;      ///< string label for the "To" Point

   // angle and sigma
   double value;        ///< angle measurement in radians
   double sigma;        ///< angle sigma in radians

   // additional sigma
   double addsig;       ///< additional sigma in radians

   // centering errors
   double Fcentersig;   ///< centering error in meters at From Point
   double Acentersig;   ///< centering error in meters at At Point
   double Tcentersig;   ///< centering error in meters at To Point

   // corrections
   double HtFrom;       ///< height of instrument (at From) in meters
   double HtTo;         ///< height of target (at To) in meters
   double HtFromSigma;  ///< instrument height uncertainty in meters
   double HtToSigma;    ///< instrument height uncertainty in meters
   bool isGeoid;        ///< if true, apply geoid corrections

   // flags
   bool hasAddsig;      ///< true if addsig has been set
   bool hasFcsig;       ///< true if Fcentersig has been set
   bool hasAcsig;       ///< true if Acentersig has been set
   bool hasTcsig;       ///< true if Tcentersig has been set
   bool hasHtFrom;      ///< true if HtFrom has been set
   bool hasHtTo;        ///< true if HtTo has been set

   // member functions -------------------------------------------------

   /// empty constructor
   DATHAngle(void) { }

   /// constructor given labels and angle in radians
   /// @param f string label of the "From" Point
   /// @param a string label of the "At" Point
   /// @param t string label of the "To" Point
   /// @param angle double value of the angle in radians
   /// @param sig double uncrtainty of the measurment in radians
   DATHAngle(std::string f, std::string a, std::string t, double angle, double sig)
      : From(f), At(a), To(t), value(angle), sigma(sig),
        addsig(0.0), Fcentersig(0.0), Tcentersig(0.0),
        hasAddsig(false), hasFcsig(false), hasAcsig(false), hasTcsig(false),
        HtFrom(0.0), HtTo(0.0), HtFromSigma(0.0), HtToSigma(0.0), hasHtFrom(false), hasHtTo(false), isGeoid(false)
      { type = DATtype::HAN; }

   /// destructor
   virtual ~DATHAngle() { }

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
   virtual bool isConfig(void) const { return false; }

   /// Consistent interface for all DAT records - not a position record
   virtual bool isPosition(void) const { return false; }

   /// Consistent interface for all DAT records - is a measurement record
   virtual bool isMeasurement(void) const { return true; }

   /// Consistent interface for all DAT records - is an angle measurement
   virtual bool isAngle(void) const { return true; }

   /// Consistent interface for all DAT records - is not a linear measurement
   virtual bool isLength(void) const { return false; }

   // this ends the base class interface

   /// @return a 'label' = From-At-To
   std::string label(void) const { return (From+"-"+At+"-"+To); }

   /// Define the "additional sigma"
   /// @param asig double uncrtainty of measurment in radians
   void setAddSigma(double asig) throw()
   {
      addsig = asig;
      if(::fabs(addsig) > 1.e-15) hasAddsig = true;
   }

   /// Define the "centering error sigma", for From, At, and To Points.
   /// @param fcsig centering error in meters at From Point
   /// @param acsig centering error in meters at At Point
   /// @param tcsig centering error in meters at To Point
   void setCenterSigmas(double fcsig, double acsig, double tcsig) throw()
   {
      Fcentersig = fcsig;
      Acentersig = acsig;
      Tcentersig = tcsig;
      if(::fabs(Fcentersig) > 1.e-15) hasFcsig = true;
      if(::fabs(Acentersig) > 1.e-15) hasAcsig = true;
      if(::fabs(Tcentersig) > 1.e-15) hasTcsig = true;
   }

   /// Define the corrections HtFrom, HtTo, and target height difference
   /// @param htf height at From Point in meters
   /// @param htt height at To Point in meters
   /// @param htFs height uncertainty at From point in meters
   /// @param htTs height uncertainty at To point in meters
   /// @param isgeoid if true apply geoid corrections
   void setCorrections(double htf, double htt, double htFs, double htTs, bool isgeoid) throw()
   {
      HtFrom = htf;
      HtTo = htt;
      HtFromSigma = htFs;
      HtToSigma = htTs;
      if(::fabs(HtFrom) > 1.e-15 || ::fabs(HtFromSigma) > 1.e-15) hasHtFrom = true;
      if(::fabs(HtTo) > 1.e-15 || ::fabs(HtToSigma) > 1.e-15) hasHtTo = true;
      isGeoid = isgeoid;
   }

}; // end class DATHAngle

#endif   // LSA_DAT_HORIZONTAL_ANGLE_DATA_INCLUDE
