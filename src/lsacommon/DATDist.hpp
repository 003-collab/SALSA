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
/// @file DATDist.hpp  Include file for class DATDist, data for the *.dat file
///                    DIS record, distance measurement

#ifndef LSA_DAT_DIST_DATA_INCLUDE
#define LSA_DAT_DIST_DATA_INCLUDE

// disable some MSVC compiler warnings
#pragma warning(disable:4290)


#include <string>
// lsa
#include "MatrixVector.hpp"
#include "DATrecord.hpp"

/// Class DATDist encapsulates the data from a DIS record in the *.dat file;
/// a distance measurement, including the measurement sigma, and the labels "From"
/// and "To", and corrections.
/// The measurement is the distance from Point("From") to Point("To").
class DATDist : public DATrecord {
public:
   // member data ------------------------------------------------------
   /// static string giving DAT file specification for this record
   static const std::string DocString;

   // labels
   std::string From;    ///< string label for the "From" Point
   std::string To;      ///< string label for the "To" Point

   // distance and sigma
   double value;        ///< coordinate distance (To - From) in meters
   double sigma;        ///< uncrtainty on the distance in meters

   // additional sigma
   double addsig;       ///< additional sigma in linear units, unless isPPM, then PPM
   bool isPPM;          ///< if true, addsig has units parts-per-million (PPM)

   // centering errors
   double Fcentersig;   ///< centering error in meters at From Point
   double Tcentersig;   ///< centering error in meters at To Point

   // corrections
   double HtFrom;       ///< height of instrument (at From) in meters
   double HtTo;         ///< height of target (at To) in meters
   double HtFromSigma;  ///< instrument height uncertainty in meters
   double HtToSigma;    ///< instrument height uncertainty in meters

   // flags
   bool hasAddsig;      ///< true if addsig has been set
   bool hasFcsig;       ///< true if Fcentersig has been set
   bool hasTcsig;       ///< true if Tcentersig has been set
   bool hasHtFrom;      ///< true if HtFrom has been set
   bool hasHtTo;        ///< true if HtTo has been set

   // member functions -------------------------------------------------

   /// empty constructor
   DATDist(void) { }

   /// destructor
   virtual ~DATDist() { }

   /// constructor, given the From and To labels, the distance and sigma
   /// @param f string label of the "From" point
   /// @param t string label of the "To" point
   /// @param dist double distance measurement in meters
   /// @param sig sigma in meters
   DATDist(std::string f, std::string t, double dist, double sig)
      : From(f), To(t), value(dist), sigma(sig),
        addsig(0.0), Fcentersig(0.0), Tcentersig(0.0),
        hasAddsig(false), hasFcsig(false), hasTcsig(false),
        HtFrom(0.0), HtTo(0.0), HtFromSigma(0.0), HtToSigma(0.0), hasHtFrom(false), hasHtTo(false)
      { type = DATtype::DIS; }

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

   /// Define the corrections HtFrom, HtTo, and refraction
   /// @param htf height at From Point in meters
   /// @param htt height at To Point in meters
   /// @param htFs height uncertainty at From point in meters
   /// @param htTs height uncertainty at To point in meters
   /// @param ref refraction
   void setCorrections(double htf, double htt, double htFs, double htTs) throw()
   {
      HtFrom = htf;
      HtTo = htt;
      HtFromSigma = htFs;
      HtToSigma = htTs;
      if(::fabs(HtFrom) > 1.e-15 || ::fabs(HtFromSigma) > 1.e-15) hasHtFrom = true;
      if(::fabs(HtTo) > 1.e-15 || ::fabs(HtToSigma) > 1.e-15) hasHtTo = true;
   }

}; // end class DATDist

#endif   // LSA_DAT_DIST_DATA_INCLUDE
