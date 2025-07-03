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
/// @file DATDir.hpp  Include file for class DATDir, data for the *.dat file
///                    DIR record, direction (angle) measurement

#ifndef LSA_DAT_DIRECTION_DATA_INCLUDE
#define LSA_DAT_DIRECTION_DATA_INCLUDE

#include <cmath>
#include <string>
// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"

/// Class DATDir encapsulates the data from a DIR record in the *.dat file;
/// a direction measurement (angle), including the measurement sigma, and the label
/// "To" and a tag indicating to which DIRSET it belongs.
/// The measurement is the angle between a Point("At") in DIRSET to Point("To").
class DATDir : public DATrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   // labels
   std::string To;      ///< string label for the "To" Point
   std::string DSGroup; ///< string matching group in the appropriate DIRSET record

   // distance and sigma
   double value;        ///< coordinate distance (To - From) in meters
   double sigma;        ///< uncrtainty on the distance in meters

   // additional sigma
   double addsig;       ///< additional sigma in angular units

   // centering errors
   double Tcentersig;   ///< centering error in meters at To Point

   // corrections
   double HtTo;         ///< height of target (at To) in meters
   double HtToSigma;    ///< instrument height uncertainty in meters

   // flags
   bool hasAddsig;      ///< true if addsig has been set
   bool hasTcsig;       ///< true if Tcentersig has been set
   bool hasHtTo;        ///< true if HtTo has been set

   // member functions -------------------------------------------------

   /// empty constructor
   DATDir(void) { }

   /// destructor
   virtual ~DATDir() { }

   /// constructor, given the From label and the group, the distance and sigma
   /// @param group string group matching the DIRSET group
   /// @param to string label of the "To" point
   /// @param angle double distance measurement in radians
   /// @param sig sigma in radians
   DATDir(std::string group, std::string to, double angle, double sig)
      : DSGroup(group), To(to), value(angle), sigma(sig),
        addsig(0.0), Tcentersig(0.0), hasAddsig(false), hasTcsig(false),
        HtTo(0.0), HtToSigma(0.0), hasHtTo(false)
      { type = DATtype::DIR; }

   /// Parse a string from a single line in the DAT file.
   /// @param line single line read from DAT file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// Output as a string (one line) for the DAT file.
   /// @return string, a single line for a DAT file
   virtual std::string asDATString(void) const;

   /// write the object as a 1-line string
   /// @param prec integer number of digits precision (default 8)
   /// @param width integer width (default 13)
   /// @return string version of the object
   virtual std::string asString(const int prec=8, const int width=13) const;

   /// Consistent interface for all DAT records - not a config record
   virtual bool isConfig(void) const { return false; }

   /// Consistent interface for all DAT records - not a position record
   virtual bool isPosition(void) const { return false; }

   /// Consistent interface for all DAT records - is not a measurement record
   virtual bool isMeasurement(void) const { return false; }

   /// Consistent interface for all DAT records - is an angle measurement
   virtual bool isAngle(void) const { return true; }

   /// Consistent interface for all DAT records - is a linear measurement
   virtual bool isLength(void) const { return false; }

   // this ends the base class interface

   /// @return a simple label = 'To'
   std::string label(void) const { return (To); }

   /// Define the "additional sigma"
   /// @param asig double uncrtainty of measurment in meters
   void setAddSigma(double asig) throw()
   {
      addsig = asig;
      if(::fabs(addsig) > 1.e-15) hasAddsig = true;
   }

   /// Define the "centering error sigma", for To Point
   /// @param tcsig centering error in meters at To Point
   void setCenterSigmas(double tcsig) throw()
   {
      Tcentersig = tcsig;
      if(::fabs(Tcentersig) > 1.e-15) hasTcsig = true;
   }

   /// Define the corrections HtFrom, HtTo, and refraction
   /// @param htt height at To Point in meters
   /// @param htTs height uncertainty at To point in meters
   void setCorrections(double htt, double htTs) throw()
   {
      HtTo = htt;
      HtToSigma = htTs;
      if(::fabs(HtTo) > 1.e-15 || ::fabs(HtToSigma) > 1.e-15) hasHtTo = true;
   }

}; // end class DATDir

#endif   // LSA_DAT_DIRECTION_DATA_INCLUDE
