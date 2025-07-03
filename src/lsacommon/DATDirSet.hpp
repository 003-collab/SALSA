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
/// @file DATDirSet.hpp  Include file for class DATDirSet, data for the *.dat file
///                      DIRSET record, direction (angle) measurement

#ifndef LSA_DAT_DIRECTION_SET_INCLUDE
#define LSA_DAT_DIRECTION_SET_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <cmath>
#include <string>
// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"

/// Class DATDirSet encapsulates the data from a DIRSET record in the *.dat file;
/// a set defined by one "From" Point and a collection of direction measurements
/// (angles) in DATDir records which share the same group.
class DATDirSet : public DATrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   // labels
   std::string From;    ///< string label for "From" Point for the whole set of DIRs
   std::string DSGroup; ///< string matching the group in the appropriate DIR records

   // centering errors
   double Fcentersig;   ///< centering error in meters at From Point

   // corrections
   double HtFrom;       ///< height of instrument (at From) in meters
   double HtFromSigma;  ///< instrument height uncertainty in meters
   bool isGeoid;        ///< if true, apply geoid corrections

   // flags
   bool hasFcsig;       ///< true if Fcentersig has been set
   bool hasHtFrom;      ///< true if HtFrom has been set

   // member functions -------------------------------------------------

   /// empty constructor
   DATDirSet(void) { }

   /// destructor
   virtual ~DATDirSet() { }

   /// constructor, given the From label and the group
   /// @param group string group matching the DIRs group
   /// @param from string label of the "From" point
   DATDirSet(std::string group, std::string from)
      : DSGroup(group), From(from), Fcentersig(0.0),
         isGeoid(false), hasFcsig(false), hasHtFrom(false)
      { type = DATtype::DIRSET; }

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

   /// @return a simple label = 'From'
   std::string label(void) const { return (From); }

   /// Define the "centering error sigma", for From Point
   /// @param fcsig centering error in meters at From Point
   void setCenterSigmas(double fcsig) throw()
   {
      Fcentersig = fcsig;
      if(::fabs(Fcentersig) > 1.e-15) hasFcsig = true;
   }

   /// Define the corrections HtFrom and target height difference
   /// @param htf height at From Point in meters
   /// @param htFs height uncertainty at From point in meters
   /// @param isgeoid if true apply geoid corrections
   void setCorrections(double htf, double htFs, bool isgeoid) throw()
   {
      HtFrom = htf;
      HtFromSigma = htFs;
      if(::fabs(HtFrom) > 1.e-15 || ::fabs(HtFromSigma) > 1.e-15) hasHtFrom = true;
      isGeoid = isgeoid;
   }

}; // end class DATDirSet

#endif   // LSA_DAT_DIRECTION_SET_INCLUDE
