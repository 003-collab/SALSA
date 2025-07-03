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
/// @file IOBDIST.hpp  Include file for class IOBDIST, data for the *.IOB file
///                     record DIST - slant distance

#ifndef LSA_IOB_DIST_INCLUDE
#define LSA_IOB_DIST_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include <vector>
#include <map>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "IOBSIGM.hpp"
#include "Exception.hpp"

/// Class IOBDIST encapsulates data for a slant distance record
class IOBDIST : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[14];

   std::string from;         ///< unique label for from station
   std::string to;           ///< unique label for to station
   std::string distance;     ///< distance measurement
   std::string units;        ///< linear units of distance
   std::string sigma;        ///< standard deviation of measurement
   std::string htfrom_tag;   ///< tag of HGHT record containing height of from station
   std::string htto_tag;     ///< tag of HGHT record containing height of to station
   std::string refract;      ///< refraction coefficient
   std::string tags;         ///< tags for relevant SIGM record (if any)

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBDIST() : from(std::string("Undef")),to(std::string("Undef")),distance(std::string("0.0")),units(std::string("m")),
                sigma(std::string("0.0")),htfrom_tag(std::string("")),htto_tag(std::string("")),
               refract(std::string("0.0")),tags(std::string(""))
      { type = IOBtype::DIST; }

   /// destructor
   virtual ~IOBDIST() { }

   /// constructor
   /// @param fr       string containing label for from station
   /// @param t        string containing label for to station
   /// @param d        string containing distance measurement
   /// @param dsig     string containing standard deviation of measurement
   /// @param du       string containing linear units of distance
   /// @param htf      string containing height tag of from station
   /// @param htt      string containing height tag of to station
   /// @param ref      string containing refraction coefficient
   /// @param tgs      string containing tags for relevant SIGM record (if any)
   IOBDIST(std::string fr, std::string t, std::string d, std::string dsig,
          std::string du, std::string htf, std::string htt, std::string ref, std::string tgs)
      : from(fr),to(t),distance(d),sigma(dsig),units(du),htfrom_tag(htf),
        htto_tag(htt),refract(ref),tags(tgs)
      { type = IOBtype::DIST; }

   /// constructor
   /// @param fr       string containing label for from station
   /// @param t        string containing label for to station
   /// @param d        string containing distance measurement
   /// @param dsig     string containing standard deviation of measurement
   /// @param du       string containing linear units of distance
   /// @param ref      string containing refraction coefficient
   /// @param tgs      string containing tags for relevant SIGM record (if any)
   IOBDIST(std::string fr, std::string t, std::string d, std::string dsig,
           std::string du, std::string ref, std::string tgs)
      : from(fr),to(t),distance(d),sigma(dsig),units(du),htfrom_tag(std::string("")),
        htto_tag(std::string("")),refract(ref),tags(tgs)
      { type = IOBtype::DIST; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @param applyVSCA bool on whether or not to apply VSCA
   /// @param VSCAtagnim int for unique VSCA tag
   /// @param instrument_HGHT_recs vector of strings of current HI records
   /// @param target_HGHT_recs vector of strings of current HT records
   /// @return true if successful
   bool fromString(const std::string& line, const bool applyVSCA, const int VSCAtagnum,
                   std::vector<std::string>& instrument_HGHT_recs, std::vector<std::string>& target_HGHT_recs,
                   std::map<std::string,IOBSIGM*> SIGMRecords);

   virtual bool fromString(const std::string& line) {return false;}

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBDIST

#endif   // LSA_IOB_DIST_INCLUDE

