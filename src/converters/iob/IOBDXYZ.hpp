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
/// @file IOBDXYZ.hpp  Include file for class IOBDXYZ, data for the *.IOB file
///                     record DXYZ 3-D XYZ difference

#ifndef LSA_IOB_DXYZ_INCLUDE
#define LSA_IOB_DXYZ_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "Exception.hpp"

/// Class IOBDXYZ encapsulates data for a 3-D XYZ difference and includes
/// a label and whether or not it is considered fixed.
class IOBDXYZ : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int DXYZ_widths[14];

   std::string from;          ///< unique label for from station
   std::string to;            ///< unique label for to station
   std::string dx;            ///< delta x
   std::string dy;            ///< delta y
   std::string dz;            ///< delta z
   std::string units;         ///< linear units
   std::string cov_mat;       ///< covariance matrix elements
   std::string htfrom_tag;    ///< tag for record with height of from station
   std::string htto_tag;      ///< tag for record with height of to station
   std::string cov_rec;       ///< COVM record with tag
   std::string tags;          ///< tags for relevant COVM and SIGM records

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBDXYZ() : from(std::string("Undef")),to(std::string("Undef")),dx(std::string("0.0")),
               dy(std::string("0.0")),dz(std::string("0.0")),units(std::string("m")),
               cov_mat(std::string("0.0 0.0 0.0 0.0 0.0 0.0")),
               htfrom_tag(std::string("Undef")),htto_tag(std::string("Undef")),
               cov_rec(std::string("COVM COV0 XYZ 0.0 0.0 0.0 0.0 0.0 0.0 0.0 m")),
               tags(std::string(""))
      { type = IOBtype::DXYZ; }

   /// destructor
   virtual ~IOBDXYZ() { }

   /// constructor from DXYZ, COV, ELEM and 3DD records;
   /// @param f       unique label for from station
   /// @param t       unique label for to station
   /// @param deltax  delta x
   /// @param deltay  delta y
   /// @param deltaz  delta z
   /// @param lu      linear units
   /// @param cmat    covariance matrix elements
   /// @param htf     tag for record containing height of from station
   /// @param htt     tag for record containing height of to station
   /// @param crec    COVM record with tag
   /// @param ts      tags for relevant COVM and SIGM records
   IOBDXYZ(std::string f, std::string t, std::string deltax, std::string deltay,
          std::string deltaz, std::string lu, std::string cmat, std::string htf,
          std::string htt, std::string crec, std::string ts)
      : from(f),to(t),dx(deltax),dy(deltay),dz(deltaz),units(lu),cov_mat(cmat),
        htfrom_tag(htf), htto_tag(htt),cov_rec(crec),tags(ts)
      { type = IOBtype::DXYZ; }

   /// constructor from DXYZ, COV, ELEM and 3DD records;
   /// @param f       unique label for from station
   /// @param t       unique label for to station
   /// @param deltax  delta x
   /// @param deltay  delta y
   /// @param deltaz  delta z
   /// @param lu      linear units
   /// @param cmat    covariance matrix elements
   /// @param crec    COVM record with tag
   /// @param ts      tags for relevant COVM and SIGM records
   IOBDXYZ(std::string f, std::string t, std::string deltax, std::string deltay,
          std::string deltaz, std::string lu, std::string cmat, std::string crec, std::string ts)
      : from(f),to(t),dx(deltax),dy(deltay),dz(deltaz),units(lu),cov_mat(cmat),
        htfrom_tag(std::string("")),htto_tag(std::string("")),
        cov_rec(crec),tags(ts)
      { type = IOBtype::DXYZ; }

   /// Parse a string from a single line in the IOB file.
   /// @param DXYZrecord single line containing DXYZ record from IOB file
   /// @param SIGM_label label from last 3DD record
   /// @param crec single line containing COVM record for LSA file
   /// @param cmat single line containing covariance matrix entries for LSA file
   /// @param applyVSCA bool on whether or not to apply VSCA
   /// @param VSCAtagnim int for unique VSCA tag
   /// @param HGHT_recs vector of strings of current HI/HT records
   /// @return true if successful
   bool fromString(const std::string& DXYZrecord, const std::string& SIGM_label,
              const std::string& crec, const std::string cmat, const bool applyVSCA,
              const int VSCAtagnum);

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line) { return false; }

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBXYZ

#endif   // LSA_IOB_DXYZ_INCLUDE

