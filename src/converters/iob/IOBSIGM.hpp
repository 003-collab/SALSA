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
/// @file IOBSIGM.hpp  Include file for class IOBSIGM, data for the *.IOB file
///                     record SIGM, sigma modifier record

#ifndef LSA_IOB_SIGM_INCLUDE
#define LSA_IOB_SIGM_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "Exception.hpp"

/// Class IOBXYZ encapsulates data for a 3-D coordinate XYZ position and includes
/// a label and whether or not it is considered fixed.
class IOBSIGM : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[18];

   std::string label;         ///< unique label for this point
   std::string std;           ///< standard deviation of observation
   std::string ppm;           ///< parts per million error
   std::string at_c;          ///< at station centering error
   std::string fr_c;          ///< from station centering error
   std::string to_c;          ///< to station centering error
   std::string units;         ///< linear units errors

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBSIGM() : label(std::string("Undef")),std(std::string("0.0")),ppm(std::string("0.0")),at_c(std::string("0.0")),
               fr_c(std::string("0.0")),to_c(std::string("0.0")),units(std::string("m"))
      { type = IOBtype::SIGM; }

   /// destructor
   virtual ~IOBSIGM() { }

   /// constructor from label llh coordinates;
   /// fixtype is set to 111 (fixed)
   /// @param lab    string containing the label for this record
   /// @param s      string for standard deviation
   /// @param p      string for parts-per-million
   /// @param at     string for at centering error
   /// @param from   string for from centering error
   /// @param to     string for to centering error
   /// @param lu     string linear unit for the errors
   IOBSIGM(std::string lab, std::string s, std::string p, std::string at,
          std::string from, std::string to, std::string lu)
      : label(lab),std(s),ppm(p),at_c(at),fr_c(from),to_c(to),
        units(lu)
      { type = IOBtype::SIGM; }

   IOBSIGM(IOBSIGM *source) : label(source->label),std(source->std),ppm(source->ppm),at_c(source->at_c),
       fr_c(source->fr_c),to_c(source->to_c), units(source->units)
     { type = IOBtype::SIGM; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBSIGM

#endif   // LSA_IOB_SIGM_INCLUDE

