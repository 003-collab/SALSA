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
/// @file IOBHDIR.hpp  Include file for class IOBHDIR, data for the *.IOB file
///                     record HDIR - horizontal direction measurement

#ifndef LSA_IOB_HDIR_INCLUDE
#define LSA_IOB_HDIR_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include <vector>
#include <map>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "IOBSIGM.hpp"
#include "Exception.hpp"

/// Class IOBHDIR encapsulates data for a slant distance record
class IOBHDIR : public IOBrecord {
public:
//## DGRP <dir_group> <AT> [HT < atLabel | htAt cm|m|ft>] [SIGM sigma_tag] [COVM covm_tag] [REDUCED]
//## HDIR <dir_group> <TO> [*+*|-]<degrees> [min sec DMS] <sigma rad|deg|soa> [HT < toLabel | htTo cm|m|ft>]

   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[15];
   std::string from;         ///< unique label for from station
   std::string to;           ///< unique label for to station
   std::string angle_deg;    ///< angle degrees measurement
   std::string angle_min;    ///< angle minutes measurement
   std::string angle_sec;    ///< angle seconds measurement
   std::string sigma;        ///< standard deviation of measurement in seconds of arc
   std::string htto_tag;     ///< tag of HGHT record containing height of to station
   std::string DGRP_record;  ///< associated DGRP_record
   bool writeDGRP;           ///< whether or not to write a DGRP record with call to asLSAString
   bool DGRPisCommented;     ///< whether or not the DGRP should be commented out

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBHDIR() : from(std::string("Undef")),to(std::string("Undef")),
               angle_deg(std::string("0.0")),angle_min(std::string("0.0")),
               angle_sec(std::string("0.0")),sigma(std::string("0.0")),htto_tag(std::string("")),
               DGRP_record(std::string("Undef")),writeDGRP(false),DGRPisCommented(false)
      { type = IOBtype::HDIR; }

   /// destructor
   virtual ~IOBHDIR() { }

   /// constructor
   /// @param fr       string containing label for from station
   /// @param t        string containing label for to station
   /// @param d        string containing angle degree measurement
   /// @param m        string containing angle minute measurement
   /// @param s        string containing angle second measurement
   /// @param sig      string containing standard deviation of measurement in seconds of arc
   /// @param htt      string containing height tag of to station
   /// @param DGRPrec  string containing associate DGRP record
   /// @param w        bool indicated whether or not to write DGRP record
   /// @param isc      bool indicated whether or not to DGRP record is commented

   IOBHDIR(std::string fr, std::string t, std::string d, std::string m,
           std::string s, std::string sig, std::string htt,
           std::string DGRPrec, bool w, bool isc)
          : from(fr),to(t),angle_deg(d),angle_min(m),angle_sec(s),sigma(sig),
           htto_tag(htt),DGRP_record(DGRPrec),writeDGRP(w),DGRPisCommented(isc)
      { type = IOBtype::HDIR; }

   /// constructor
   /// @param fr       string containing label for from station
   /// @param t        string containing label for to station
   /// @param d        string containing angle degree measurement
   /// @param m        string containing angle minute measurement
   /// @param s        string containing angle second measurement
   /// @param sig      string containing standard deviation of measurement in seconds of arc
   /// @param DGRPrec  string containing associate DGRP record
   /// @param w        bool indicated whether or not to write DGRP record
   /// @param isc      bool indicated whether or not to DGRP record is commented
   IOBHDIR(std::string fr, std::string t, std::string d, std::string m,
           std::string s, std::string sig, std::string DGRPrec, bool w, bool isc)
          : from(fr),to(t),angle_deg(d),angle_min(m),angle_sec(s),sigma(sig),
            htto_tag(std::string("")),DGRP_record(DGRPrec),writeDGRP(w),DGRPisCommented(isc)
      { type = IOBtype::HDIR; }

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @param lastDSETrecord string of line of last DSET in IOB file
   /// @param dsetCounter int for unit DGRP tag
   /// @param wroteDGRP bool if false need to write out DGRP record in asLSAString
   /// @param lastDSETisCommented bool when converting commented DIRs don't comment out the DGRP
   /// @param applyVSCA bool on whether or not to apply VSCA
   /// @param VSCAtagnim int for unique VSCA tag
   /// @param target_HGHT_recs vector of strings of current HT records
   /// @return true if successful
   bool fromString(const std::string& line, const std::string& lastDSETrecord,
                   const int dsetCounter, const bool wroteDGRP,
                   const bool lastDSETisCommented, const bool applyVSCA,
                   const int VSCAtagnum, std::vector<std::string>& target_HGHT_recs,
                   std::map<std::string,IOBSIGM*> SIGMRecords);

   virtual bool fromString(const std::string& line) {return false;}

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBHDIR

#endif   // LSA_IOB_HDIR_INCLUDE

