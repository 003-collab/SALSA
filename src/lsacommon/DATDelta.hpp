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
/// @file DATDelta.hpp  Include file for class DATDelta, data for the *.dat file
///                     record DEL, 3-D XYZ coordinate difference measurement

#ifndef LSA_DAT_DELTA_DATA_INCLUDE
#define LSA_DAT_DELTA_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)

#include <string>

// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"

/// Class DATDelta encapsulates the data from a DEL record in the *.dat file;
/// a delta-XYZ measurement, including the coordinates and measurement covariance
/// matrix, and the labels "From" and "To", and corrections. The measurement is
/// defined as the coordinate difference Point("To") minus Point("From").
class DATDelta : public DATrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   // labels
   std::string From;    ///< string label for the "From" Point
   std::string To;      ///< string label for the "To" Point

   // coordinate differences
   double dx;           ///< X-coordinate difference (To - From)
   double dy;           ///< Y-coordinate difference (To - From)
   double dz;           ///< Z-coordinate difference (To - From)

   // components of the UT portion of the covariances matrix.
   double covxx;        ///< the X,X component of the covariance matrix
   double covxy;        ///< the X,Y component of the covariance matrix
   double covxz;        ///< the X,Z component of the covariance matrix
   double covyy;        ///< the Y,Y component of the covariance matrix
   double covyz;        ///< the Y,Z component of the covariance matrix
   double covzz;        ///< the Z,Z component of the covariance matrix

   // additional sigma
   double addsig;       ///< additional sigma in linear units, unless isPPM, then PPM
   bool isPPM;          ///< if true, addsig has units parts-per-million (PPM)

   // centering errors
   double Fcentersig;   ///< centering error in meters at From Point
   double Tcentersig;   ///< centering error in meters at To Point

   // corrections
   double HtFrom;       ///< height of instrument (at From) in meters
   double HtTo;         ///< height of target (at To) in meters
   double HtFromSigma;  ///< height uncertainty of instrument in meters
   double HtToSigma;    ///< height uncertainty of target in meters

   // flags
   bool hasAddsig;      ///< true if addsig has been set
   bool hasFcsig;       ///< true if Fcentersig has been set
   bool hasTcsig;       ///< true if Tcentersig has been set
   bool hasHtFrom;      ///< true if HtFrom has been set
   bool hasHtTo;        ///< true if HtTo has been set

   // member functions -------------------------------------------------

   /// empty constructor
   DATDelta(void) { }

   /// destructor
   virtual ~DATDelta() { }

   /// constructor, given the From and To labels,
   /// the X,Y,Z components of the delta, and the covariance matrix given
   /// in upper triangular form in the calling arguments, i.e. components
   /// in the order cov(X,X),cov(X,Y),cov(X,Z),cov(Y,Y),cov(Y,Z),cov(Z,Z).
   /// @param f string label of the "From" point
   /// @param t string label of the "To" point
   /// @param dxin double X-coordinate of the delta
   /// @param dyin double Y-coordinate of the delta
   /// @param dzin double Z-coordinate of the delta
   /// @param cxx double the X,X component of the covariance matrix
   /// @param cxy double the X,Y component of the covariance matrix
   /// @param cxz double the X,Z component of the covariance matrix
   /// @param cyy double the Y,Y component of the covariance matrix
   /// @param cyz double the Y,Z component of the covariance matrix
   /// @param czz double the Z,Z component of the covariance matrix
   DATDelta(std::string f, std::string t, double dxin, double dyin, double dzin,
                                           double cxx, double cxy, double cxz,
                                           double cyy, double cyz, double czz)
      : From(f), To(t), dx(dxin), dy(dyin), dz(dzin),
            covxx(cxx), covxy(cxy), covxz(cxz), covyy(cyy), covyz(cyz), covzz(czz),
            addsig(0.0), isPPM(false), Fcentersig(0.0), Tcentersig(0.0),
            hasAddsig(false), hasFcsig(false), hasTcsig(false),
            HtFrom(0.0), HtTo(0.0), HtFromSigma(0.0), HtToSigma(0.0), hasHtFrom(false), hasHtTo(false)
      { type = DATtype::DEL; }

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
   virtual std::string asString(const int prec=3, const int width=9) const;

   /// Consistent interface for all DAT records - not a config record
   virtual bool isConfig(void) const { return false; }

   /// Consistent interface for all DAT records - not a position record
   virtual bool isPosition(void) const { return false; }

   /// Consistent interface for all DAT records - is a measurement record
   virtual bool isMeasurement(void) const { return true; }

   /// Consistent interface for all DAT records - is not an angle measurement
   virtual bool isAngle(void) const { return false; }

   /// Consistent interface for all DAT records - is a linear measurement
   virtual bool isLength(void) const { return true; }

   // this ends the base class interface

   /// @return a simple label = 'From-To'
   std::string label(void) const { return (From+"-"+To); }

   /// write the object as a 3-line string of the form
   ///
   /// \verbatim
   ///     Delta labFrom-labTo  X-comp     covXX  covXY  covXZ
   ///                          Y-comp            covYY  covYZ
   ///                          Z-comp                   covZZ
   /// \endverbatim
   /// @param msg string to place at first of string
   /// @param prec integer number of digits precision (default 3)
   /// @param width integer width (default 8)
   /// @return string version of the object
   virtual std::string asString3(const std::string msg,
      const int prec=3, const int width=8) const;

   /// Define the "additional sigma", include the isPPM flag.
   /// @param asig double uncrtainty of measurment in meters, unless isppm==T
   /// @param isppm if true, addsig has units part-per-million (PPM)
   void setAddSigma(double asig, bool isppm=false) throw()
   {
      addsig = asig;
      isPPM = isppm;
      if(::fabs(addsig) > 1.e-15) hasAddsig = true;
   }

   /// Define the "centering error sigma", for From and To Points.
   /// @param fcsig centering error in meters at From Point
   /// @param tcsig centering error in meters at To Point
   void setCenterSigmas(double fcsig, double tcsig) throw()
   {
      Fcentersig = fcsig;
      Tcentersig = tcsig;
      if(::fabs(Fcentersig) > 1.e-15) hasFcsig = true;
      if(::fabs(Tcentersig) > 1.e-15) hasTcsig = true;
   }

   /// Define the corrections HtFrom and HtTo
   /// @param htf height at From Point in meters
   /// @param htt height at To Point in meters
   /// @param htFs height uncertainty at From point in meters
   /// @param htTs height uncertainty at To point in meters
   void setCorrections(double htf, double htt, double htfs, double htts) throw()
   {
      HtFrom = htf;
      HtTo = htt;
      HtFromSigma = htfs;
      HtToSigma = htts;
      if(::fabs(HtFrom) > 1.e-15 || ::fabs(HtFromSigma) > 1.e-15) hasHtFrom = true;
      if(::fabs(HtTo) > 1.e-15 || ::fabs(HtToSigma) > 1.e-15) hasHtTo = true;
   }

   /// Return the measurement covariance matrix defined just by the UT components in
   /// the constructor; see fullMeasurementCovariance() for this much + additions.
   /// @return 3x3 measurement covariance matrix
   gnsstk::Matrix<double> measurementCovariance(void);

}; // end class DATDelta

#endif   // LSA_DAT_DELTA_DATA_INCLUDE
