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
/// @file DATPoint.hpp  Include file for class DATPoint, data for the *.dat file
///                     record POS 3-D XYZ coordinate position

#ifndef LSA_DAT_POINT_DATA_INCLUDE
#define LSA_DAT_POINT_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>
// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"

/// Class DATPoint encapsulates data for a 3-D coordinate position and includes
/// a label and whether or not it is considered fixed.
class DATPoint : public DATrecord {
public:
   enum Fixtype
   {
      // Unknown must be first, and must = 0
      Unknown = 0,         ///< unknown point fixtype
      Estimated,           ///< point is estimated
      Fixed,               ///< point is fixed
      Adjusted             ///< point is adjusted
   };
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   std::string label;         ///< unique label for this point

   // A flag to record the fixtype of the point
   Fixtype fixtype;

   double x;                  ///< X-coordinate
   double y;                  ///< Y-coordinate
   double z;                  ///< Z-coordinate
   // components of the UT portion of the covariances matrix.
   double covxx;              ///< the X,X component of the covariance matrix
   double covxy;              ///< the X,Y component of the covariance matrix
   double covxz;              ///< the X,Z component of the covariance matrix
   double covyy;              ///< the Y,Y component of the covariance matrix
   double covyz;              ///< the Y,Z component of the covariance matrix
   double covzz;              ///< the Z,Z component of the covariance matrix

   // constraint(s) - characters must be 1 or 2 of XYZNEU
   std::string constraint;    ///< 1- or 2-character coordinate(s) constraints

   // member functions -------------------------------------------------

   /// empty constructor. label is set to "Undef" if fixtype unknown,
   /// coordinates = 0,0,0 and the covariance matrix to 0.0
   DATPoint() : label(std::string("Undef")),fixtype(Fixtype::Unknown),x(0.0),y(0.0),z(0.0),
                 covxx(0.0),covxy(0.0),covxz(0.0),covyy(0.0),covyz(0.0),covzz(0.0),
                 constraint("")
      { type = DATtype::POS; }

   /// destructor
   virtual ~DATPoint() { }

   /// constructor from a label only. fixtype is set to unknown, coordinates = 0,0,0
   /// @param lab  string containing the label for this Point
   DATPoint(std::string lab) : label(lab),fixtype(Fixtype::Unknown),x(0.0),y(0.0),z(0.0),
                                covxx(0.0),covxy(0.0),covxz(0.0),
                                covyy(0.0),covyz(0.0),covzz(0.0),
                                constraint("")
      { type = DATtype::POS; }

   /// constructor from label xyz coordinates and optional covariance (default 0);
   /// fixtype is set to 1 (estimate)
   /// @param lab  string containing the label for this Point
   /// @param xin  double X-coordinate for this Point
   /// @param yin  double Y-coordinate for this Point
   /// @param zin  double Z-coordinate for this Point
   /// @param sxx double the X,X component of the covariance matrix
   /// @param sxy double the X,Y component of the covariance matrix
   /// @param sxz double the X,Z component of the covariance matrix
   /// @param syy double the Y,Y component of the covariance matrix
   /// @param syz double the Y,Z component of the covariance matrix
   /// @param szz double the Z,Z component of the covariance matrix
   DATPoint(std::string lab, double xin, double yin, double zin,
                             double sxx, double sxy, double sxz,
                             double syy, double syz, double szz)
      : label(lab),fixtype(Fixtype::Estimated),x(xin),y(yin),z(zin),
        covxx(sxx), covxy(sxy), covxz(sxz), covyy(syy), covyz(syz), covzz(szz),
        constraint("")
      { type = DATtype::POS; }

   /// set the fixtype to input
   /// @param ft  fixtype 0:unknown, 1:estimate, 2:fixed, 3:adjust
   /// @return the fixtype
   Fixtype setFixType(Fixtype ft) { fixtype = ft; return fixtype; }

   /// set the fixtype to "unknown" - may never need this routine
   /// @return the fixtype
   Fixtype setUnknown(void) { fixtype = Fixtype::Unknown; return fixtype; }

   /// set the fixtype to "estimate"
   /// @return the fixtype
   Fixtype setEstimate(void) { fixtype = Fixtype::Estimated; return fixtype; }

   /// set the fixtype to "fixed"
   /// @return the fixtype
   Fixtype setFixed(void) { fixtype = Fixtype::Fixed; return fixtype; }

   /// set the fixtype to "adjust"
   /// @return the fixtype
   Fixtype setAdjust(void) { fixtype = Fixtype::Adjusted; return fixtype; }

   /// @return true if Unknown
   bool isUnknown(void) { return (fixtype == Fixtype::Unknown); }

   /// @return true if Known
   bool isKnown(void) { return (fixtype != Fixtype::Unknown); }

   /// @return true if Estimate
   bool isEstimate(void) { return (fixtype == Fixtype::Estimated); }

   /// @return true if Fixed
   bool isFixed(void) { return (fixtype == Fixtype::Fixed); }

   /// @return true if Adjust
   bool isAdjust(void) { return (fixtype == Fixtype::Adjusted); }

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

   /// Consistent interface for all DAT records - is a position record
   virtual bool isPosition(void) const { return true; }

   /// Consistent interface for all DAT records - is not a measurement record
   virtual bool isMeasurement(void) const { return false; }

   /// Consistent interface for all DAT records - is not an angle measurement
   virtual bool isAngle(void) const { return false; }

   /// Consistent interface for all DAT records - is not a linear measurement
   virtual bool isLength(void) const { return false; }

   // this ends the base class interface

   /// Define the constraint axis (or axes)
   /// @param axes string giving the axes of constraint, e.g. "N" | "Y" | "YZ" | "NE"
   /// @throw if axes are invalid
   void setConstraintAxis(std::string axes) throw(gnsstk::Exception);

   /// Return the measurement covariance matrix defined just by the UT components in
   /// the constructor; see fullMeasurementCovariance() for this much + additions.
   /// @return 3x3 measurement covariance matrix
   gnsstk::Matrix<double> measurementCovariance(void);

}; // end class DATPoint

#endif   // LSA_DAT_POINT_DATA_INCLUDE

