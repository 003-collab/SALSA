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
/// @file IOBXYZ.hpp  Include file for class IOBXYZ, data for the *.IOB file
///                     record XYZ 3-D XYZ coordinate

#ifndef LSA_IOB_XYZ_INCLUDE
#define LSA_IOB_XYZ_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "Exception.hpp"

/// Class IOBXYZ encapsulates data for a 3-D coordinate XYZ position and includes
/// a label and whether or not it is considered fixed.
class IOBXYZ : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[16];

   std::string label;         ///< unique label for this point
   std::string latfix;        ///< 0|space or not for fixing latitude
   std::string lonfix;        ///< 0|space or not for fixing longitude
   std::string htfix;         ///< 0|space or not for fixing ht
   std::string x;             ///< x-coordinate
   std::string y;             ///< y-coordinate
   std::string z;             ///< z-coordinate
   std::string units;         ///< linear units of height
   std::string cov_mat;       ///< covariance matrix elements
   std::string cov_rec;       ///< COVM record with tag
   std::string tags;          ///< tags for relevant COVM and SIGM records

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBXYZ() : label(std::string("Undef")),latfix(std::string("1")),lonfix(std::string("1")),htfix(std::string("1")),
                x(std::string("0.0")),y(std::string("0.0")),z(std::string("0.0")),units(std::string("m")),
               cov_mat(std::string("Undef")),
               cov_rec(std::string("Undef")),
               tags(std::string("Undef"))
      { type = IOBtype::XYZ; }

   /// destructor
   virtual ~IOBXYZ() { }

   /// constructor from label llh coordinates;
   /// fixtype is set to 111 (fixed)
   /// @param lab    string containing the label for this Point
   /// @param laf    string to fix latitude for this Point
   /// @param lof    string to fix longitude for this Point
   /// @param htf    string to fix ht for this Point
   /// @param xcoord string x-coordinate for this Point
   /// @param ycoord string y-coordinate for this Point
   /// @param zcoord string z-coordinate for this Point
   /// @param lu     string linear unit for this Point
   IOBXYZ(std::string lab, std::string laf, std::string lof, std::string htf,
          std::string xcoord, std::string ycoord, std::string zcoord, std::string lu)
      : label(lab),latfix(laf),lonfix(lof),htfix(htf),x(xcoord),y(ycoord),
        z(zcoord),units(lu),cov_mat(std::string("Undef")),cov_rec(std::string("Undef")),
        tags(std::string("Undef"))
      { type = IOBtype::XYZ; }

   /// constructor from label llh coordinates;
   /// fixtype is set to 111 (fixed)
   /// @param lab    string containing the label for this Point
   /// @param laf    string to fix latitude for this Point
   /// @param lof    string to fix longitude for this Point
   /// @param htf    string to fix ht for this Point
   /// @param xcoord string x-coordinate for this Point
   /// @param ycoord string y-coordinate for this Point
   /// @param zcoord string z-coordinate for this Point
   /// @param lu     string linear unit for this Point
   /// @param cmat    covariance matrix elements
   /// @param crec    COVM record with tag
   /// @param ts      tags for relevant COVM and SIGM records
   IOBXYZ(std::string lab, std::string laf, std::string lof, std::string htf,
          std::string xcoord, std::string ycoord, std::string zcoord, std::string lu,
          std::string cmat, std::string crec, std::string ts)
      : label(lab),latfix(laf),lonfix(lof),htfix(htf),x(xcoord),y(ycoord),
        z(zcoord),units(lu),cov_mat(cmat),cov_rec(crec),tags(ts)
      { type = IOBtype::XYZ; }

   /// Parse a string from a single line in the IOB file.
   /// @param XYZrecord single line containing XYZ record from IOB file
   /// @param SIGM_label label from last 3DC record
   /// @param crec single line containing COVM record for LSA file
   /// @param cmat single line containing covariance matrix entries for LSA file
   /// @param applyVSCA bool on whether or not to apply VSCA
   /// @param VSCAtagnim int for unique VSCA tag
   /// @return true if successful
   bool fromString(const std::string& XYZrecord, const std::string& crec, const std::string cmat,
                   const bool applyVSCA, const int VSCAtagnum);

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBXYZ

#endif   // LSA_IOB_XYZ_INCLUDE

