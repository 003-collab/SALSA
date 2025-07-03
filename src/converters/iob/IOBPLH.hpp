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
/// @file IOBPLH.hpp  Include file for class IOBPLH, data for the *.IOB file
///                     record PLH 3-D XYZ coordinate PLHition

#ifndef LSA_IOB_PLH_INCLUDE
#define LSA_IOB_PLH_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
#include "Matrix.hpp"
#include "IOBrecord.hpp"
#include "Exception.hpp"

/// Class IOBPLH encapsulates data for a 3-D coordinate PLHition and includes
/// a label and whether or not it is considered fixed.
class IOBPLH : public IOBrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving IOB file specification for this record
   static const std::string DocString;
   static const int widths[26];

   std::string label;         ///< unique label for this point
   std::string latfix;        ///< 0|space or not for fixing latitude
   std::string lonfix;        ///< 0|space or not for fixing longitude
   std::string htfix;         ///< 0|space or not for fixing ht
   std::string latdeg;        ///< latitude degress
   std::string latmin;        ///< latitude minutes
   std::string latsec;        ///< latitude seconds
   std::string londeg;        ///< longitude degress
   std::string lonmin;        ///< longitude minutes
   std::string lonsec;        ///< longitude seconds
   std::string posnorth;      ///< n|N|space or s|S
   std::string poseast;       ///< e|E|space or w|W
   std::string height;        ///< ellipsoidal height
   std::string units;         ///< linear units of height
   std::string cov_mat;       ///< covariance matrix elements
   std::string cov_rec;       ///< COVM record with tag
   std::string tags;          ///< tags for relevant COVM and SIGM records

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef", all coordinates fixed,
   /// coordinates = 0,0,0
   IOBPLH() : label(std::string("Undef")),latfix(std::string("1")),lonfix(std::string("1")),htfix(std::string("1")),
                latdeg(std::string("0")),latmin(std::string("0")),latsec(std::string("0.0")),londeg(std::string("0")),
                lonmin(std::string("0")),lonsec(std::string("0.0")),posnorth(std::string("N")),poseast(std::string("W")),
                height(std::string("0.0")),units(std::string("m")),cov_mat(std::string("Undef")),cov_rec(std::string("Undef")),
                tags(std::string("Undef"))
      { type = IOBtype::PLH; }

   /// destructor
   virtual ~IOBPLH() { }

   /// constructor from label llh coordinates;
   /// fixtype is set to 111 (fixed)
   /// @param lab    string containing the label for this Point
   /// @param laf    string to fix latitude for this Point
   /// @param lof    string to fix longitude for this Point
   /// @param htf    string to fix ht for this Point
   /// @param lad    string latitude degree for this Point
   /// @param lam    string latitude minute for this Point
   /// @param las    string latitude second for this Point
   /// @param lod    string longitude degree for this Point
   /// @param lom    string longitude minute this Point
   /// @param los    string longitude second for this Point
   /// @param ht     string height for this Point
   /// @param posn   string on lat positive north for this Point
   /// @param pose   string on lon positive east for this Point
   /// @param htu    string height unit for this Point
   IOBPLH(std::string lab, std::string laf, std::string lof, std::string htf, std::string lad,
            std::string lam, std::string las, std::string lod, std::string lom, std::string los,
            std::string posn, std::string pose, std::string ht, std::string htu)
      : label(lab),latfix(laf),lonfix(lof),htfix(htf),latdeg(lad),latmin(lam),
        latsec(las),londeg(lod),lonmin(lom),lonsec(los),posnorth(posn),
        poseast(pose),height(ht),units(htu),cov_mat(std::string("Undef")),
        cov_rec(std::string("Undef")),tags(std::string("Undef"))
      { type = IOBtype::PLH; }

   /// constructor from label llh coordinates;
   /// fixtype is set to 111 (fixed)
   /// @param lab    string containing the label for this Point
   /// @param laf    string to fix latitude for this Point
   /// @param lof    string to fix longitude for this Point
   /// @param htf    string to fix ht for this Point
   /// @param lad    string latitude degree for this Point
   /// @param lam    string latitude minute for this Point
   /// @param las    string latitude second for this Point
   /// @param lod    string longitude degree for this Point
   /// @param lom    string longitude minute this Point
   /// @param los    string longitude second for this Point
   /// @param ht     string height for this Point
   /// @param posn   string on lat positive north for this Point
   /// @param pose   string on lon positive east for this Point
   /// @param htu    string height unit for this Point
   /// @param cmat   string covariance matrix for this point
   /// @param crec   string covariance record for this point
   /// @param tgs    string modifier record tags for this point
   IOBPLH(std::string lab, std::string laf, std::string lof, std::string htf, std::string lad,
            std::string lam, std::string las, std::string lod, std::string lom, std::string los,
            std::string posn, std::string pose, std::string ht, std::string htu,
            std::string cmat, std::string crec, std::string tgs)
      : label(lab),latfix(laf),lonfix(lof),htfix(htf),latdeg(lad),latmin(lam),
        latsec(las),londeg(lod),lonmin(lom),lonsec(los),posnorth(posn),
        poseast(pose),height(ht),units(htu),cov_mat(cmat),cov_rec(crec),tags(tgs)
      { type = IOBtype::PLH; }

   /// Parse a string from a single line in the IOB file.
   /// @param PLH record single line containing PLH record from IOB file
   /// @param crec single line containing COVM record for LSA file
   /// @param cmat single line containing covariance matrix entries for LSA file
   /// @param applyVSCA bool on whether or not to apply VSCA
   /// @param VSCAtagnim int for unique VSCA tag
   /// @return true if successful
   bool fromString(const std::string& PLHrecord, const std::string& crec,
                   const std::string cmat, const bool applyVSCA, const int VSCAtagnum);

   /// Parse a string from a single line in the IOB file.
   /// @param line single line read from IOB file
   /// @return true if successful
   virtual bool fromString(const std::string& line);

   /// Check if fields meet expected IOB spec
   /// @return true if so, false if not
   bool isValid();

   /// write the object as a 1-line string
   /// @return string, a single line for an LSA file
   virtual std::string asLSAString(void) const;

   // this ends the base class interface
}; // end class IOBPLH

#endif   // LSA_IOB_PLH_INCLUDE

